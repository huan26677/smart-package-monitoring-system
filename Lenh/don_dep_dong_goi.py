#!/usr/bin/env python3
"""Dọn thành phần có thể tạo lại; mặc định sao lưu dữ liệu trước khi dọn."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTAINERS = {name: f"smart-package-{name}" for name in ("postgres", "emqx", "backend", "web", "cloudflared")}
GENERATED = (
    "web-dashboard/node_modules", "web-dashboard/dist", "smart_package_backend/target",
    "firmware-esp32/build", "firmware-esp32/managed_components",
    "ai-training/.venv.local", "ai-training/.cache.local", "ai-training/__pycache__",
    "Lenh/__pycache__", ".pytest_cache", "ai-training/.pytest_cache",
)
SECRETS: list[str] = []


class DemoError(Exception):
    """Lỗi vận hành được trình bày bằng tiếng Việt."""


class VietnameseHelp(argparse.HelpFormatter):
    def add_usage(self, usage, actions, groups, prefix=None):
        return super().add_usage(usage, actions, groups, prefix or "Cách chạy: ")


def argument_parser(description: str) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description, formatter_class=VietnameseHelp, add_help=False)
    parser._optionals.title = "Tùy chọn"
    parser.add_argument("-h", "--help", action="help", help="Hiển thị hướng dẫn và thoát.")
    return parser


def say(message: str) -> None:
    print(message, flush=True)


def redact(message: str) -> str:
    for secret in sorted(SECRETS, key=len, reverse=True):
        if secret:
            message = message.replace(secret, "[ĐÃ ẨN]")
    return message


def configure_console() -> None:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")


def validate_root(root: Path) -> None:
    if not (root / "docker-compose.yml").is_file() or not (root / "firmware-esp32/main/main.c").is_file():
        raise DemoError("Đặt cả thư mục Lenh trong dự án, cạnh docker-compose.yml và firmware-esp32.")
    for relative in (".env", "demo-private"):
        path = root / relative
        if path.exists() or path.is_symlink():
            safe_target(root, path)


def run(root: Path, args: list[str], timeout: int = 60, progress: str | None = None) -> str:
    if progress:
        say(f"[ĐANG] {progress}")
    try:
        options = {"creationflags": subprocess.CREATE_NO_WINDOW} if os.name == "nt" else {}
        process = subprocess.Popen(args, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **options)
    except OSError as error:
        raise DemoError(f"Không chạy được {args[0]}: {error}") from error
    started = time.monotonic()
    last_notice = started
    try:
        while True:
            try:
                output, _ = process.communicate(timeout=min(5, max(0.1, timeout - (time.monotonic() - started))))
                break
            except subprocess.TimeoutExpired:
                now = time.monotonic()
                if now - started >= timeout:
                    process.kill()
                    process.communicate()
                    raise DemoError(f"{progress or args[0]} quá thời gian chờ {timeout} giây.")
                if progress and now - last_notice >= 30:
                    say(f"[ĐANG] {progress}: đã chờ {int(now - started)} giây…")
                    last_notice = now
    except KeyboardInterrupt:
        process.terminate()
        process.communicate()
        raise
    decoded = output.decode("utf-8", errors="replace")
    text = decoded.removesuffix("\n") if "printenv" in args else decoded.strip()
    if process.returncode:
        # Không in đầu ra config vì có thể chứa toàn bộ thông tin truy cập.
        detail = "" if "config" in args else redact(text[-2200:])
        raise DemoError(f"{progress or 'Lệnh ' + args[0]} thất bại (mã {process.returncode}). {detail}")
    return text


def compose(root: Path, *args: str, timeout: int = 60, progress: str | None = None) -> str:
    return run(root, ["docker", "compose", *args], timeout, progress)


def settings(root: Path, public: bool = True) -> dict:
    if not (root / ".env").is_file():
        raise DemoError("Thiếu .env. Mang .env cùng bộ sao lưu từ máy cũ; xem docs/DEMO_MIGRATION.md.")
    config = json.loads(compose(root, "config", "--format", "json"))
    services = config["services"]
    backend = effective_environment(services["backend"]["environment"])
    postgres = effective_environment(services["postgres"]["environment"])
    result = {
        "db_user": postgres.get("POSTGRES_USER", ""), "db_name": postgres.get("POSTGRES_DB", ""),
        "db_password": postgres.get("POSTGRES_PASSWORD", ""),
        "mqtt_user": backend.get("MQTT_USERNAME", ""), "mqtt_password": backend.get("MQTT_PASSWORD", ""),
        "admin_user": backend.get("DASHBOARD_ADMIN_USERNAME", ""),
        "admin_password": backend.get("DASHBOARD_ADMIN_PASSWORD", ""),
        "tunnel_token": effective_environment(services["cloudflared"]["environment"]).get("TUNNEL_TOKEN", ""),
    }
    SECRETS.extend(str(result[key]) for key in ("db_password", "mqtt_password", "admin_password", "tunnel_token") if result[key])
    required = ["db_user", "db_name", "db_password", "mqtt_user", "mqtt_password", "admin_user", "admin_password"]
    if public:
        required.append("tunnel_token")
    placeholders = {"change_me", "replace_with_a_strong_password", "replace_with_a_strong_mqtt_password"}
    for key in required:
        if not result[key] or result[key] in placeholders:
            raise DemoError(f"Chưa điền giá trị thật cho {key} trong .env.")
    if len(result["admin_password"]) < 16:
        raise DemoError("Mật khẩu dashboard trong .env phải có ít nhất 16 ký tự.")
    return result


def effective_environment(environment: dict) -> dict:
    # Compose config thoát dấu $ thành $$ khi xuất cấu hình để có thể đọc lại.
    return {key: value.replace("$$", "$") if isinstance(value, str) else value for key, value in environment.items()}


def dotenv_literal(value: str) -> str:
    # JSON double quotes giữ Unicode và thoát dấu nháy/backslash/xuống dòng;
    # $$ giữ dấu $ literal qua phép nội suy .env của Compose.
    return json.dumps(str(value).replace("$", "$$"), ensure_ascii=False)


def docker_ready(root: Path) -> None:
    if not shutil.which("docker"):
        raise DemoError("Chưa có Docker trong PATH. Cài Docker Desktop, bật Linux containers rồi mở lại terminal.")
    run(root, ["docker", "info", "--format", "{{.ServerVersion}}"], 15)
    compose(root, "version", "--short")


def container_state(root: Path, name: str) -> dict | None:
    try:
        rows = json.loads(run(root, ["docker", "inspect", "--format", "{{json .State}}", CONTAINERS[name]], 15))
        return rows
    except DemoError:
        return None


def latest_backup(root: Path, explicit: str | None = None) -> Path | None:
    if explicit:
        path = Path(explicit).expanduser()
        if not path.is_absolute():
            path = root / path
        candidates = [path]
    else:
        folder = root / "demo-private"
        candidates = sorted((p for p in folder.iterdir() if p.is_dir()), key=lambda p: p.name, reverse=True) if folder.exists() else []
    for path in candidates:
        try:
            manifest = path / "manifest.local.json"
            if manifest.exists() and not json.loads(manifest.read_text(encoding="utf-8")).get("complete"):
                continue
            if (path / ".env").is_file() and (path / "smart-package-demo.dump").is_file() and len(list(path.glob("emqx-export-*.tar.gz"))) == 1:
                with (path / "smart-package-demo.dump").open("rb") as source:
                    if source.read(5) == b"PGDMP":
                        return path.resolve()
        except (OSError, ValueError):
            continue
    if explicit:
        raise DemoError("Bộ sao lưu phải có .env, smart-package-demo.dump và đúng một emqx-export-*.tar.gz.")
    return None


def snapshot(root: Path, config: dict) -> Path:
    for name in ("postgres", "emqx"):
        state = container_state(root, name)
        if not state or not state.get("Running"):
            raise DemoError(f"Cần {name} đang chạy để tạo bản sao lưu mới. Chạy script chay_demo.py trước.")
    status = run(root, ["docker", "exec", CONTAINERS["emqx"], "emqx", "ctl", "status"])
    if "is started" not in status:
        raise DemoError("EMQX chưa sẵn sàng xuất dữ liệu. Chờ healthy rồi thử lại.")
    for env_key, value in (("POSTGRES_USER", config["db_user"]), ("POSTGRES_DB", config["db_name"]), ("POSTGRES_PASSWORD", config["db_password"])):
        actual = run(root, ["docker", "exec", CONTAINERS["postgres"], "printenv", env_key])
        if actual != value:
            raise DemoError(f"{env_key} trong .env không khớp container đang chạy; chưa dọn dữ liệu build.")
    backend = container_state(root, "backend")
    if backend and backend.get("Running"):
        for env_key, key in (("MQTT_USERNAME", "mqtt_user"), ("MQTT_PASSWORD", "mqtt_password"), ("DASHBOARD_ADMIN_USERNAME", "admin_user"), ("DASHBOARD_ADMIN_PASSWORD", "admin_password")):
            if run(root, ["docker", "exec", CONTAINERS["backend"], "printenv", env_key]) != config[key]:
                raise DemoError(f"{env_key} trong .env chưa khớp backend. Áp dụng cấu hình bằng docker compose up -d backend trước khi sao lưu.")
    private = root / "demo-private"
    if private.exists():
        safe_target(root, private)
    folder = root / "demo-private" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    folder.mkdir(parents=True)
    try:
        # Viết các biến đã được Compose phân giải để giữ cả giá trị override từ terminal.
        env_keys = {"POSTGRES_DB": "db_name", "POSTGRES_USER": "db_user", "POSTGRES_PASSWORD": "db_password", "MQTT_USERNAME": "mqtt_user", "MQTT_PASSWORD": "mqtt_password", "DASHBOARD_ADMIN_USERNAME": "admin_user", "DASHBOARD_ADMIN_PASSWORD": "admin_password", "CLOUDFLARE_TUNNEL_TOKEN": "tunnel_token"}
        (folder / ".env").write_text("\n".join(key + "=" + dotenv_literal(config[name]) for key, name in env_keys.items()) + "\n", encoding="utf-8")
        remote_dump = f"/tmp/smart-package-{folder.name}.dump"
        run(root, ["docker", "exec", CONTAINERS["postgres"], "pg_dump", "--username", config["db_user"], "--dbname", config["db_name"], "--format=custom", "--file", remote_dump], 180, "Sao lưu PostgreSQL")
        run(root, ["docker", "cp", f"{CONTAINERS['postgres']}:{remote_dump}", str(folder / "smart-package-demo.dump")])
        exported = run(root, ["docker", "exec", CONTAINERS["emqx"], "emqx", "ctl", "data", "export"], 90, "Sao lưu cấu hình và tài khoản EMQX")
        matches = re.findall(r"data/backup/(emqx-export-[\d.\-]+\.tar\.gz)", exported)
        if not matches or "successfully exported" not in exported:
            raise DemoError("EMQX chưa xác nhận xuất thành công; chưa dọn các thư mục build.")
        archive = matches[-1]
        run(root, ["docker", "cp", f"{CONTAINERS['emqx']}:/opt/emqx/data/backup/{archive}", str(folder / archive)])
        if (folder / "smart-package-demo.dump").stat().st_size < 5 or (folder / archive).stat().st_size < 5:
            raise DemoError("Bản sao lưu rỗng; chưa dọn các thư mục build.")
        write_json(folder / "manifest.local.json", {"complete": True, "createdAt": datetime.now().astimezone().isoformat(), "emqxVersion": "6.3.1", "note": "Dữ liệu tại thời điểm xuất; không chứa các sự kiện ESP32 chưa gửi."})
        (root / "demo-private/latest-migration-backup.local").write_text(str(folder.relative_to(root)), encoding="utf-8")
        return folder
    except Exception:
        # Không xóa bản sao lưu dở; ghi rõ để lần chạy demo không chọn nhầm.
        write_json(folder / "manifest.local.json", {"complete": False})
        raise


def write_json(path: Path, value: dict) -> None:
    safe_target(ROOT, path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(redact(json.dumps(value, ensure_ascii=False, indent=2)), encoding="utf-8")


def is_link(path: Path) -> bool:
    info = path.lstat()
    return path.is_symlink() or bool(getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400))


def safe_target(root: Path, path: Path) -> None:
    resolved_root = root.resolve()
    try:
        relative = path.absolute().relative_to(root.absolute())
        resolved = path.resolve()
    except (ValueError, OSError) as error:
        raise DemoError(f"Từ chối dọn đường dẫn ngoài dự án: {path}") from error
    if resolved == resolved_root or not resolved.is_relative_to(resolved_root):
        raise DemoError(f"Từ chối dọn đường dẫn ngoài dự án: {path}")
    current = root
    for part in relative.parts:
        current = current / part
        if current.exists() and is_link(current):
            raise DemoError(f"Từ chối đi theo symlink/junction: {current}")


def folder_size(path: Path) -> int:
    total = 0
    for current, dirs, files in os.walk(path, followlinks=False):
        dirs[:] = [name for name in dirs if not is_link(Path(current) / name)]
        for name in files:
            try:
                total += (Path(current) / name).lstat().st_size
            except OSError:
                pass
    return total


def remove_generated(root: Path, path: Path) -> None:
    safe_target(root, path)
    # rmtree từ Python 3.10 không duyệt vào junction trên Windows.
    def retry(function, failed_path, _error):
        target = Path(failed_path)
        safe_target(root, target)
        target.chmod(stat.S_IWRITE | stat.S_IREAD)
        function(failed_path)
    shutil.rmtree(path, onerror=retry)


def clean(root: Path, dry_run: bool, backup_enabled: bool) -> dict:
    validate_root(root)
    targets = []
    for relative in GENERATED:
        path = root / relative
        if path.exists():
            safe_target(root, path)
            size = folder_size(path)
            if relative == "firmware-esp32/build" and (path / "backups").exists():
                safe_target(root, path / "backups")
                size -= folder_size(path / "backups")
            targets.append((path, size))
            say(f"[DỌN] {relative}: {size / 1024**2:.1f} MB")
    report = {"dryRun": dry_run, "estimatedBytes": sum(size for _, size in targets), "deleted": [], "errors": []}
    say("[GIỮ] Mã nguồn, .git, .env, sdkconfig, tài liệu/báo cáo, dữ liệu AI và demo-private.")
    if dry_run:
        say(f"[XEM TRƯỚC] Có thể dọn khoảng {report['estimatedBytes'] / 1024**2:.1f} MB; chưa xóa gì.")
        return report
    if backup_enabled:
        docker_ready(root)
        report["backup"] = str(snapshot(root, settings(root, public=False)).relative_to(root))
        say(f"[OK] Đã tạo bộ sao lưu: {report['backup']}")
    else:
        say("[LƯU Ý] Bỏ qua tạo bản sao lưu mới theo --khong-sao-luu; các bản sao lưu cũ vẫn được giữ.")
    preserved = root / "firmware-esp32/build/backups"
    if preserved.exists():
        safe_target(root, preserved)
        destination = root / "demo-private/firmware-backups" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        safe_target(root, destination)
        # Không đi theo link trong bản sao lưu khi sao chép.
        shutil.copytree(preserved, destination, symlinks=True)
        report["preservedFirmwareBackups"] = str(destination.relative_to(root))
        say(f"[GIỮ] Bản sao lưu trong build được chép sang {report['preservedFirmwareBackups']}")
    for path, _ in targets:
        try:
            remove_generated(root, path)
            report["deleted"].append(str(path.relative_to(root)))
        except (OSError, DemoError) as error:
            report["errors"].append({"path": str(path.relative_to(root)), "message": str(error)})
            say(f"[LỖI] Chưa dọn được {path.relative_to(root)}: {error}")
    return report


def main() -> int:
    configure_console()
    parser = argument_parser(__doc__)
    parser.add_argument("--xem-truoc", action="store_true", help="Liệt kê dung lượng, không sao lưu hoặc xóa.")
    parser.add_argument("--khong-sao-luu", action="store_true", help="Chỉ dọn tệp có thể tạo lại, không xuất bản sao lưu mới.")
    args = parser.parse_args()
    try:
        result = clean(ROOT, args.xem_truoc, not args.khong_sao_luu)
        if not args.xem_truoc:
            write_json(ROOT / "demo-private/lenh" / f"don-dep-{datetime.now():%Y%m%d-%H%M%S}.local.json", result)
            say("[DỌN CHƯA HẾT] Một số thư mục đang bận hoặc không cho xóa; xem báo cáo." if result["errors"] else "[XONG] Đóng gói thư mục dự án đã dọn, mang cả .env và demo-private sang máy mới.")
            say("[BƯỚC TIẾP] Trên máy mới: python Lenh/chay_demo.py")
        return 1 if result["errors"] else 0
    except (DemoError, OSError, ValueError) as error:
        say(f"[LỖI] {redact(str(error))}")
        return 1
    except KeyboardInterrupt:
        say("\n[ĐÃ DỪNG] Các thư mục chưa xử lý vẫn được giữ.")
        return 130


if __name__ == "__main__":
    sys.exit(main())
