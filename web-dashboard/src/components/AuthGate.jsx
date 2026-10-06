import { useEffect, useState } from "react";
import { getSession, signIn, signOut } from "../services/api";

export default function AuthGate({children}) {
  const [session, setSession] = useState(null);
  const [username, setUsername] = useState("admin");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [busy, setBusy] = useState(false);
  useEffect(() => {
    let active = true;
    getSession().then(data => {if (active) setSession(data);})
      .catch(() => {if (active) {setError("Không kết nối được máy chủ."); setSession({authenticated:false});}});
    const expired = () => setSession({authenticated:false});
    window.addEventListener("session-expired", expired);
    return () => { active = false; window.removeEventListener("session-expired", expired); };
  }, []);
  async function login(event) {
    event.preventDefault(); setBusy(true); setError("");
    try { await signIn(username, password); setPassword(""); setSession(await getSession()); }
    catch { setError("Đăng nhập thất bại. Kiểm tra tài khoản, mật khẩu và kết nối."); }
    finally { setBusy(false); }
  }
  async function logout() {
    setBusy(true); setError("");
    try { await signOut(); setSession({authenticated:false}); }
    catch { setError("Không đăng xuất được. Vui lòng thử lại."); }
    finally { setBusy(false); }
  }
  if (session === null) return <div className="loading">Đang kiểm tra phiên đăng nhập…</div>;
  if (!session.authenticated) return <main className="login-container">
    <form className="card login-form" onSubmit={login}>
      <h1>Giám sát va đập kiện hàng</h1><p>Đăng nhập để xem dữ liệu và quản lý hệ thống.</p>
      <label>Tài khoản<input autoComplete="username" value={username} onChange={e => setUsername(e.target.value)} required /></label>
      <label>Mật khẩu<input type="password" autoComplete="current-password" value={password} onChange={e => setPassword(e.target.value)} required /></label>
      {error && <div className="error" role="alert">{error}</div>}
      <button type="submit" className="primary-button" disabled={busy}>{busy ? "Đang đăng nhập…" : "Đăng nhập"}</button>
    </form>
  </main>;
  return <><div className="session-bar"><span>{session.username}</span><button onClick={logout} disabled={busy}>Đăng xuất</button>{error && <span role="alert">{error}</span>}</div>{children}</>;
}
