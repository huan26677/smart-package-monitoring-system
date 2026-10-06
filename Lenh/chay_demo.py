#!/usr/bin/env python3
"""Khởi động, khôi phục máy mới và kiểm tra hệ thống/ESP32 trước demo."""

from __future__ import annotations

import base64
import hashlib
import http.cookiejar
import json
import math
import os
import shutil
import socket
import ssl
import struct
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
import webbrowser
from datetime import datetime, timezone
from pathlib import Path

from don_dep_dong_goi import (
    ROOT, CONTAINERS, DemoError, compose, configure_console, container_state,
    argument_parser, docker_ready, latest_backup, redact, run, say, settings, validate_root, write_json,
)


class AccessRequired(DemoError):
    pass


class Api:
    def __init__(self, base: str):
        self.base = base.rstrip("/")
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
        self.opener.addheaders = [("User-Agent", "SmartPackageDemoCheck/1.0"), ("Accept", "application/json")]

    def request(self, path: str, data: bytes | None = None, headers: dict | None = None, allow_error: bool = False):
        try:
            response = self.opener.open(urllib.request.Request(self.base + path, data=data, headers=headers or {}), timeout=12)
        except urllib.error.HTTPError as error:
            response = error
        except (OSError, urllib.error.URLError) as error:
            raise DemoError(f"Không truy cập được {self.base}{path}: {error}") from error
        with response:
            content = response.read(2_000_000)
            if b"Cloudflare Access" in content or "/cdn-cgi/access/login" in response.geturl():
                raise AccessRequired("Tên miền web yêu cầu Cloudflare Access. Đăng nhập trên trình duyệt rồi kiểm tra dashboard public; script không tự đăng nhập OTP.")
            if response.status >= 400 and not allow_error:
                raise DemoError(f"{self.base}{path}: HTTP {response.status}. Kiểm tra đăng nhập/API hoặc Cloudflare Access.")
            try:
                body = json.loads(content)
            except (ValueError, UnicodeError) as error:
                raise DemoError(f"{self.base}{path} không trả JSON của ứng dụng. Kiểm tra tên miền/Cloudflare Access.") from error
            return response.status, body

    def get(self, path: str):
        return self.request(path)[1]

    def login(self, config: dict) -> None:
        csrf = self.get("/api/auth/csrf")
        _, response = self.request("/api/auth/login", urllib.parse.urlencode({"username": config["admin_user"], "password": config["admin_password"]}).encode(), {"Content-Type": "application/x-www-form-urlencoded", csrf["headerName"]: csrf["token"]})
        if not response.get("authenticated") or not self.get("/api/auth/session").get("authenticated"):
            raise DemoError("Đăng nhập dashboard không thành công. Kiểm tra tài khoản trong .env.")


def encode_length(length: int) -> bytes:
    result = bytearray()
    while True:
        digit = length % 128
        length //= 128
        result.append(digit | (128 if length else 0))
        if not length:
            return bytes(result)


def mqtt_string(value: str) -> bytes:
    encoded = value.encode("utf-8")
    return struct.pack("!H", len(encoded)) + encoded


class MqttProbe:
    """MQTT 3.1.1 qua TCP hoặc WSS, chỉ đăng ký nhận dữ liệu thật."""

    def __init__(self, uri: str, username: str, password: str):
        address = urllib.parse.urlsplit(uri)
        self.ws = address.scheme == "wss"
        if address.scheme not in ("tcp", "wss") or not address.hostname or address.username:
            raise DemoError("Địa chỉ kiểm tra MQTT phải là tcp://host:port hoặc wss://host/mqtt, không chứa mật khẩu.")
        self.socket = socket.create_connection((address.hostname, address.port or (443 if self.ws else 1883)), timeout=12)
        self.raw_buffer = bytearray()
        self.mqtt_buffer = bytearray()
        try:
            if self.ws:
                self.socket = ssl.create_default_context().wrap_socket(self.socket, server_hostname=address.hostname)
                nonce = base64.b64encode(os.urandom(16)).decode()
                host = address.hostname + (f":{address.port}" if address.port else "")
                target = address.path or "/mqtt"
                if address.query:
                    target += "?" + address.query
                request = f"GET {target} HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: {nonce}\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: mqtt\r\n\r\n"
                self.socket.sendall(request.encode("ascii"))
                header = bytearray()
                while b"\r\n\r\n" not in header:
                    part = self.socket.recv(4096)
                    if not part or len(header) > 16384:
                        raise DemoError("Hostname MQTT không hoàn tất nâng cấp WebSocket.")
                    header.extend(part)
                headers, remainder = bytes(header).split(b"\r\n\r\n", 1)
                expected = base64.b64encode(hashlib.sha1((nonce + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()).decode()
                lines = headers.decode("latin1").split("\r\n")
                fields = {key.lower().strip(): value.strip() for key, value in (line.split(":", 1) for line in lines[1:] if ":" in line)}
                if len(lines[0].split()) < 2 or lines[0].split()[1] != "101" or fields.get("sec-websocket-accept") != expected:
                    raise DemoError("MQTT public không mở được WebSocket. Kiểm tra tuyến http://emqx:8083 và Cloudflare Access.")
                self.raw_buffer.extend(remainder)
            payload = mqtt_string("MQTT") + bytes([4, 0xC2, 0, 30]) + mqtt_string("demo-check-" + uuid.uuid4().hex[:16]) + mqtt_string(username) + mqtt_string(password)
            self.send(bytes([0x10]) + encode_length(len(payload)) + payload)
            kind, body = self.receive()
            if kind != 0x20 or len(body) != 2 or body[1] != 0:
                raise DemoError("Broker từ chối tài khoản MQTT backend; kiểm tra .env và bản khôi phục EMQX.")
        except Exception:
            self.close()
            raise

    def close(self) -> None:
        self.socket.close()

    def exact(self, count: int) -> bytes:
        while len(self.raw_buffer) < count:
            data = self.socket.recv(max(4096, count - len(self.raw_buffer)))
            if not data:
                raise DemoError("MQTT đóng kết nối trong lúc kiểm tra.")
            self.raw_buffer.extend(data)
        data = bytes(self.raw_buffer[:count])
        del self.raw_buffer[:count]
        return data

    def send(self, packet: bytes, opcode: int = 2) -> None:
        if not self.ws:
            self.socket.sendall(packet)
            return
        mask = os.urandom(4)
        length = len(packet)
        header = bytes([0x80 | opcode, 0x80 | length]) if length < 126 else bytes([0x80 | opcode, 0x80 | 126]) + struct.pack("!H", length)
        self.socket.sendall(header + mask + bytes(value ^ mask[index % 4] for index, value in enumerate(packet)))

    def ws_data(self) -> bytes:
        result = bytearray()
        while True:
            first, second = self.exact(2)
            opcode, length = first & 15, second & 127
            if length == 126:
                length = struct.unpack("!H", self.exact(2))[0]
            elif length == 127:
                length = struct.unpack("!Q", self.exact(8))[0]
            if length > 1_000_000:
                raise DemoError("Khung WebSocket vượt giới hạn kiểm tra.")
            mask = self.exact(4) if second & 128 else None
            payload = self.exact(length)
            if mask:
                payload = bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
            if opcode == 8:
                raise DemoError("MQTT public đóng WebSocket.")
            if opcode == 9:
                self.send(payload, opcode=10)
                continue
            if opcode == 10:
                continue
            if opcode not in (0, 2):
                raise DemoError("Hostname MQTT trả khung không phải dữ liệu MQTT.")
            result.extend(payload)
            if len(result) > 1_000_000:
                raise DemoError("Thông điệp MQTT vượt giới hạn kiểm tra.")
            if first & 128:
                return bytes(result)

    def receive(self):
        def ensure(count):
            while len(self.mqtt_buffer) < count:
                self.mqtt_buffer.extend(self.ws_data() if self.ws else self.exact(count - len(self.mqtt_buffer)))
        ensure(2)
        remaining, multiplier, index = 0, 1, 1
        while True:
            ensure(index + 1)
            digit = self.mqtt_buffer[index]
            remaining += (digit & 127) * multiplier
            index += 1
            if not digit & 128:
                break
            if index > 4:
                raise DemoError("Độ dài gói MQTT không hợp lệ.")
            multiplier *= 128
        if remaining > 1_000_000:
            raise DemoError("Gói MQTT quá lớn.")
        ensure(index + remaining)
        kind = self.mqtt_buffer[0]
        body = bytes(self.mqtt_buffer[index:index + remaining])
        del self.mqtt_buffer[:index + remaining]
        return kind, body

    def telemetry(self, device: str) -> dict:
        topic = f"smart-package/{device}/telemetry"
        payload = b"\x00\x01" + mqtt_string(topic) + b"\x00"
        self.send(b"\x82" + encode_length(len(payload)) + payload)
        subscribed = False
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            kind, body = self.receive()
            if kind >> 4 == 9:
                if len(body) < 3 or body[:2] != b"\x00\x01" or body[2] == 0x80:
                    raise DemoError("Tài khoản MQTT không có quyền nhận telemetry.")
                subscribed = True
            elif kind >> 4 == 3 and not kind & 1 and subscribed:
                length = struct.unpack("!H", body[:2])[0]
                offset = 2 + length + (2 if (kind >> 1) & 3 else 0)
                received_topic = body[2:2 + length].decode()
                data = json.loads(body[offset:])
                if received_topic == topic and data.get("deviceId") == device:
                    return data
        raise DemoError("MQTT public đã kết nối nhưng chưa nhận telemetry mới của ESP32.")


def wait_services(root: Path, names: tuple[str, ...], timeout: int = 180) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        missing = []
        for name in names:
            state = container_state(root, name)
            if not state or not state.get("Running") or state.get("Health", {}).get("Status", "healthy") != "healthy":
                missing.append(name)
        if not missing:
            return
        time.sleep(3)
    raise DemoError(f"Dịch vụ chưa sẵn sàng: {', '.join(missing)}. Xem docker compose logs --tail 80 {' '.join(missing)}.")


def ensure_docker(root: Path, check_only: bool) -> None:
    try:
        docker_ready(root)
        return
    except DemoError:
        if check_only or os.name != "nt":
            raise
    executable = Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "Docker/Docker/Docker Desktop.exe"
    if not executable.exists():
        raise DemoError("Cài Docker Desktop, bật Linux containers và mở lại terminal.")
    say("[ĐANG] Mở Docker Desktop và đợi Docker Engine…")
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    subprocess.Popen([str(executable)], startupinfo=startup, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        try:
            docker_ready(root)
            return
        except DemoError:
            time.sleep(4)
    raise DemoError("Docker Engine chưa chạy sau 120 giây. Kiểm tra WSL2/ảo hóa và Linux containers.")


def prepare_data(root: Path, config: dict, backup: Path | None) -> None:
    status = run(root, ["docker", "exec", CONTAINERS["emqx"], "emqx", "ctl", "status"])
    if "is started" not in status:
        raise DemoError("EMQX chưa sẵn sàng nhận lệnh khôi phục. Đợi node khởi động rồi chạy lại.")
    query = "SELECT count(*) FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE';"
    count = int(run(root, ["docker", "exec", CONTAINERS["postgres"], "psql", "-U", config["db_user"], "-d", config["db_name"], "-Atc", query]))
    expression = "case catch mnesia:table_info(emqx_authn_mnesia, size) of N when is_integer(N) -> N; _ -> 0 end."
    users = int(run(root, ["docker", "exec", CONTAINERS["emqx"], "emqx", "eval", expression]).rstrip("."))
    if count and users:
        say("[OK] Database và tài khoản MQTT đã tồn tại; giữ dữ liệu hiện tại.")
        return
    if backup is None:
        raise DemoError("Máy mới chưa có dữ liệu/tài khoản MQTT và không tìm thấy bộ sao lưu. Mang demo-private từ máy cũ hoặc làm mục cài mới trong docs/DEMO_MIGRATION.md.")
    manifest = backup / "manifest.local.json"
    if manifest.exists() and not json.loads(manifest.read_text(encoding="utf-8")).get("complete"):
        raise DemoError("Bộ sao lưu chưa hoàn tất. Chọn bộ đầy đủ bằng --ban-sao-luu.")
    say(f"[ĐANG] Khôi phục phần còn trống từ {backup.name}; giữ các phần đã có dữ liệu.")
    if count == 0:
        run(root, ["docker", "cp", str(backup / "smart-package-demo.dump"), f"{CONTAINERS['postgres']}:/tmp/demo-restore.dump"])
        run(root, ["docker", "exec", CONTAINERS["postgres"], "pg_restore", "-U", config["db_user"], "-d", config["db_name"], "--no-owner", "--no-acl", "--single-transaction", "--exit-on-error", "/tmp/demo-restore.dump"], 180, "Khôi phục PostgreSQL")
    if users == 0:
        archive = next(backup.glob("emqx-export-*.tar.gz"))
        run(root, ["docker", "cp", str(archive), f"{CONTAINERS['emqx']}:/tmp/{archive.name}"])
        imported = run(root, ["docker", "exec", CONTAINERS["emqx"], "emqx", "ctl", "data", "import", f"/tmp/{archive.name}"], 120, "Khôi phục cấu hình và tài khoản EMQX")
        if "imported successfully" not in imported:
            raise DemoError("EMQX chưa xác nhận khôi phục thành công; chưa khởi động backend.")


def age(value: str | None) -> float:
    if not value:
        return float("inf")
    return (datetime.now(timezone.utc) - datetime.fromisoformat(value.replace("Z", "+00:00"))).total_seconds()


def valid_telemetry(dashboard: dict) -> bool:
    data = dashboard.get("latestTelemetry") or {}
    if not dashboard.get("online") or not -5 <= age(data.get("receivedAt")) <= 15:
        return False
    limits = {"gForce": (0, 30), "angle": (0, 181), "vibration": (0, 40), "wifiRssi": (-127, 0)}
    for key, (low, high) in limits.items():
        value = data.get(key)
        if not isinstance(value, (int, float)) or not math.isfinite(value) or not low <= value <= high:
            raise DemoError(f"Chỉ số {key} từ ESP32 không hợp lệ. Kiểm tra cảm biến/I2C, nguồn và dây nối.")
    return True


def check_device(api: Api, device: str, timeout: int, diagnostics: dict | None = None) -> tuple[dict, dict]:
    path = "/api/devices/" + urllib.parse.quote(device, safe="")
    say(f"[ĐANG] Đợi {device} gửi telemetry và đoạn cảm biến mới (tối đa {timeout} giây)…")
    deadline = time.monotonic() + timeout
    first_id = None
    first_rejected = None
    last_error = "Chưa nhận được thiết bị."
    last_notice = time.monotonic()
    while time.monotonic() < deadline:
        try:
            dashboard = api.get(path + "/dashboard")
            telemetry = dashboard.get("latestTelemetry") or {}
            if diagnostics is not None:
                diagnostics.update({"online": dashboard.get("online"), "telemetry": telemetry})
            if valid_telemetry(dashboard):
                telemetry = dashboard["latestTelemetry"]
                if first_id is None:
                    first_id = telemetry["id"]
                    first_rejected = telemetry.get("rejectedEvents") or 0
                if (telemetry.get("rejectedEvents") or 0) > first_rejected:
                    raise DemoError("ESP32 đang từ chối lưu sự kiện mới, cần kiểm tra bộ nhớ/nhịp ghi sự kiện.")
                ai = api.get(path + "/ai")
                live = ai.get("latest") or {}
                if diagnostics is not None:
                    diagnostics["aiLatest"] = live
                if telemetry["id"] != first_id and telemetry.get("pendingEvents") == 0 and -5 <= age(live.get("receivedAt")) <= 15 and live.get("timingValid") and not live.get("saturated"):
                    return dashboard, ai
                reasons = []
                if telemetry["id"] == first_id:
                    reasons.append("chưa có bản đo tiếp theo")
                if telemetry.get("pendingEvents") != 0:
                    pending = telemetry.get("pendingEvents")
                    reasons.append(f"còn {pending} sự kiện chưa được xác nhận lưu" if pending is not None else "thiếu số sự kiện chờ đồng bộ")
                if not -5 <= age(live.get("receivedAt")) <= 15:
                    reasons.append("chưa có đoạn AI mới")
                elif not live.get("timingValid"):
                    reasons.append("nhịp đo AI chưa hợp lệ")
                if live.get("saturated"):
                    reasons.append("cảm biến đang vượt dải đo")
                last_error = "Đã nhận dữ liệu ESP32 nhưng " + "; ".join(reasons) + "."
            else:
                last_error = "ESP32 mất kết nối hoặc chỉ có dữ liệu cũ."
        except DemoError as error:
            last_error = str(error)
        if diagnostics is not None:
            diagnostics["reason"] = last_error
        if time.monotonic() - last_notice >= 15:
            say(f"[ĐANG] {last_error}")
            last_notice = time.monotonic()
        time.sleep(2)
    raise DemoError(f"ESP32 chưa sẵn sàng: {last_error} Cấp nguồn, kiểm tra Wi-Fi 2,4 GHz và WSS. Giữ BOOT 3 giây rồi thả, vào SMART_PACKAGE_SETUP/smart1234 → http://192.168.4.1 để đổi mạng. Nếu chuyển máy chủ, dừng cloudflared trên máy cũ. Xem cả nguồn/cảm biến nếu Wi-Fi đã kết nối.")


def main() -> int:
    configure_console()
    parser = argument_parser(__doc__)
    parser.add_argument("--chi-kiem-tra", action="store_true", help="Không khởi động/build/khôi phục; chỉ kiểm tra dịch vụ đang chạy.")
    parser.add_argument("--khong-public", action="store_true", help="Demo local, không bật tunnel hoặc kiểm tra hostname public.")
    parser.add_argument("--khong-mo-trinh-duyet", action="store_true", help="Không tự mở trang dashboard.")
    parser.add_argument("--ban-sao-luu", help="Thư mục sao lưu cụ thể; mặc định chọn bộ đầy đủ mới nhất trong demo-private.")
    parser.add_argument("--thiet-bi", default="esp32-001", help="ID ESP32 cần kiểm tra.")
    parser.add_argument("--doi-esp32", type=int, default=90, help="Thời gian chờ dữ liệu ESP32, mặc định 90 giây.")
    args = parser.parse_args()
    report = {"startedAt": datetime.now().astimezone().isoformat(), "ready": False, "localReady": False, "checks": [], "warnings": [], "manualChecks": []}
    report_path = ROOT / "demo-private/lenh" / f"demo-{datetime.now():%Y%m%d-%H%M%S}.local.json"
    try:
        validate_root(ROOT)
        if args.doi_esp32 < 5 or args.doi_esp32 > 600:
            raise DemoError("--doi-esp32 phải từ 5 đến 600 giây.")
        if not args.thiet_bi or any(character not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for character in args.thiet_bi):
            raise DemoError("ID thiết bị chỉ dùng chữ, số, dấu gạch ngang và gạch dưới.")
        ensure_docker(ROOT, args.chi_kiem_tra)
        backup = latest_backup(ROOT, args.ban_sao_luu)
        if not (ROOT / ".env").exists() and backup and not args.chi_kiem_tra:
            shutil.copy2(backup / ".env", ROOT / ".env")
            say("[OK] Đã lấy .env từ bộ sao lưu; không hiển thị thông tin truy cập.")
        config = settings(ROOT, public=not args.khong_public)
        say("[OK] Docker, Compose và các biến cấu hình bắt buộc.")
        if not args.chi_kiem_tra:
            compose(ROOT, "up", "-d", "--build", "postgres", "emqx", timeout=900, progress="Khởi động PostgreSQL và EMQX")
            wait_services(ROOT, ("postgres", "emqx"))
            prepare_data(ROOT, config, backup)
        else:
            wait_services(ROOT, ("postgres", "emqx", "backend", "web"), timeout=15)
        probe = MqttProbe("tcp://127.0.0.1:1883", config["mqtt_user"], config["mqtt_password"])
        probe.close()
        say("[OK] Tài khoản MQTT backend được broker chấp nhận.")
        if not args.chi_kiem_tra:
            compose(ROOT, "up", "-d", "--build", "backend", "web", timeout=1800, progress="Build và khởi động backend/dashboard")
            wait_services(ROOT, ("postgres", "emqx", "backend", "web"))
            if not args.khong_public:
                compose(ROOT, "up", "-d", "cloudflared", timeout=180, progress="Bật Cloudflare Tunnel trên máy demo")
        local = Api("http://localhost:8088")
        for path in ("/api/health/database", "/api/health/mqtt"):
            if local.get(path).get("status") != "OK":
                raise DemoError(f"{path} chưa OK.")
        status, _ = local.request("/api/devices", allow_error=True)
        if status != 401:
            raise DemoError("API dữ liệu không yêu cầu đăng nhập như cấu hình dự án.")
        local.login(config)
        say("[OK] Database/MQTT, API bắt buộc đăng nhập và tài khoản dashboard.")
        report["deviceChecks"] = {}
        dashboard, ai = check_device(local, args.thiet_bi, args.doi_esp32, report["deviceChecks"])
        telemetry = dashboard["latestTelemetry"]
        report["localReady"] = True
        report.update({"deviceId": args.thiet_bi, "telemetry": telemetry, "aiLatest": ai.get("latest"), "modelAvailable": bool(ai.get("model", {}).get("available"))})
        report["checks"].extend(["Docker/Compose", "PostgreSQL", "MQTT xác thực", "Đăng nhập/API", "Telemetry mới thay đổi", "pendingEvents = 0", "Đoạn AI mới, nhịp hợp lệ"])
        say(f"[OK] {args.thiet_bi} có dữ liệu mới; gia tốc {telemetry['gForce']:.2f} g, Wi-Fi {telemetry['wifiRssi']} dBm, chờ đồng bộ 0.")
        if telemetry.get("rejectedEvents"):
            report["warnings"].append(f"{telemetry['rejectedEvents']} sự kiện từng không lưu được; đây là bộ đếm lịch sử.")
        if not report["modelAvailable"]:
            report["warnings"].append("AI chưa có mô hình: demo quy tắc cảnh báo và chức năng thu/gắn nhãn dữ liệu.")
        if not (ai.get("latest") or {}).get("timestamp"):
            report["warnings"].append("ESP32 chưa đồng bộ giờ NTP. Dashboard vẫn có thời điểm nhận dữ liệu của máy chủ; thời điểm xảy ra trên thiết bị có thể chưa xác định.")
        if telemetry["wifiRssi"] < -75:
            report["warnings"].append("Wi-Fi yếu; đặt điểm phát sóng gần bộ mạch hơn khi demo.")
        if not args.khong_public:
            wait_services(ROOT, ("cloudflared",), timeout=30)
            public = Api("https://monitor.huan2k5.id.vn")
            try:
                if public.get("/api/health/mqtt").get("status") != "OK" or public.get("/api/health/database").get("status") != "OK":
                    raise DemoError("Tên miền public chưa kết nối đúng backend đang hoạt động.")
                public.login(config)
                device_path = "/api/devices/" + urllib.parse.quote(args.thiet_bi, safe="") + "/dashboard"
                public_dashboard = public.get(device_path)
                current_local = local.get(device_path)
                if not valid_telemetry(public_dashboard) or abs(public_dashboard["latestTelemetry"]["id"] - current_local["latestTelemetry"]["id"]) > 10:
                    raise DemoError("Dashboard public không nhận cùng dữ liệu mới với local. Kiểm tra tunnel máy cũ còn chạy hay không.")
                report["checks"].append("Dashboard HTTPS public")
            except AccessRequired as error:
                report["manualChecks"].append(str(error))
                say(f"[CẦN TRÌNH DUYỆT] {error}")
            wss = MqttProbe("wss://mqtt.huan2k5.id.vn/mqtt", config["mqtt_user"], config["mqtt_password"])
            try:
                wss.telemetry(args.thiet_bi)
            finally:
                wss.close()
            report["checks"].append("MQTT WSS public nhận telemetry thật")
            say("[OK] MQTT WSS public nhận dữ liệu ESP32 thật.")
        report["localReady"] = True
        report["ready"] = not report["manualChecks"]
        for warning in report["warnings"]:
            say(f"[LƯU Ý] {warning}")
        result = "[CẦN ĐĂNG NHẬP ACCESS] Local và MQTT public đã đạt; hoàn tất đăng nhập và kiểm tra web public trên trình duyệt." if report["manualChecks"] else "[SẴN SÀNG DEMO LOCAL]" if args.khong_public else "[SẴN SÀNG DEMO]"
        say(result)
        say("Thử nghiêng/rung/gõ nhẹ để quan sát LCD và còi; script không tự xác nhận các thao tác vật lý này.")
        dashboard_url = "http://localhost:8088" if args.khong_public else "https://monitor.huan2k5.id.vn"
        say(f"[WEB] {dashboard_url}")
        if not args.khong_mo_trinh_duyet:
            webbrowser.open(dashboard_url)
        return 2 if report["manualChecks"] else 0
    except (DemoError, OSError, ValueError, KeyError) as error:
        report["error"] = redact(str(error))
        say(f"[CHƯA SẴN SÀNG] {report['error']}")
        say("[GỢI Ý] Xem docker compose ps và docker compose logs --tail 80. Không xóa volume hoặc nạp lại ESP32 để xử lý lỗi kết nối.")
        return 1
    except KeyboardInterrupt:
        report["error"] = "Người dùng dừng kiểm tra; các dịch vụ đang chạy được giữ."
        say("\n[ĐÃ DỪNG] Các dịch vụ và dữ liệu đang chạy được giữ.")
        return 130
    finally:
        report["finishedAt"] = datetime.now().astimezone().isoformat()
        try:
            write_json(report_path, report)
            say(f"[BÁO CÁO] {report_path.relative_to(ROOT)}")
        except OSError as error:
            say(f"[LƯU Ý] Không lưu được báo cáo: {error}")


if __name__ == "__main__":
    sys.exit(main())
