const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL ||
  "";

async function request(
  path,
  options = {}
) {

  if (options.method && !["GET", "HEAD"].includes(options.method)) {
    const csrf = await getCsrf();
    options.headers = { ...options.headers, [csrf.headerName]: csrf.token };
  }

  const response =
    await fetch(
      `${API_BASE_URL}${path}`,
      options
    );

  if (response.status === 401) window.dispatchEvent(new Event("session-expired"));
  if (!response.ok) {

    let message =
      `HTTP ${response.status}`;

    try {

      const error =
        await response.json();

      if (error.message) {

        message =
          error.message;
      }

    }
    catch {

      // Response khong phai JSON.
    }

    throw new Error(
      message
    );
  }

  if (
    response.status === 204
  ) {

    return null;
  }

  const body = await response.text();
  return body ? JSON.parse(body) : null;
}

export function getDevices() {

  return request(
    "/api/devices"
  );
}

export function getDeviceDashboard(
  deviceId
) {

  return request(
    `/api/devices/${encodeURIComponent(deviceId)}/dashboard`
  );
}

export function getTelemetry(
  deviceId,
  limit = 60
) {

  return request(
    `/api/devices/${encodeURIComponent(deviceId)}/telemetry?limit=${limit}`
  );
}

export function getEvents(
  deviceId,
  limit = 20
) {

  return request(
    `/api/devices/${encodeURIComponent(deviceId)}/events?limit=${limit}`
  );
}

export function getWifiLocations() {

  return request(
    "/api/wifi-locations"
  );
}

export function saveWifiLocation(
  anchor
) {

  return request(
    "/api/wifi-locations",
    {
      method: "POST",

      headers: {
        "Content-Type":
          "application/json"
      },

      body:
        JSON.stringify(
          anchor
        )
    }
  );
}

export function deleteWifiLocation(
  bssid
) {

  return request(
    `/api/wifi-locations?bssid=${encodeURIComponent(bssid)}`,
    {
      method: "DELETE"
    }
  );
}

export function getLocationScans(
  deviceId,
  limit = 1
) {

  return request(
    `/api/devices/${encodeURIComponent(deviceId)}/location-scans?limit=${limit}`
  );
}

export async function getCsrf() {
  const response = await fetch(`${API_BASE_URL}/api/auth/csrf`);
  if (!response.ok) throw new Error("Không lấy được phiên bảo mật.");
  return response.json();
}
export function getSession() { return request("/api/auth/session"); }
export async function signIn(username, password) {
  return request("/api/auth/login", {method: "POST", body: new URLSearchParams({username, password})});
}
export function signOut() { return request("/api/auth/logout", {method: "POST"}); }
export function eventQuery(filters = {}) {
  return new URLSearchParams(Object.entries(filters).filter(([, value]) => value !== "" && value != null)).toString();
}
export function getEventHistory(deviceId, filters) {
  return request(`/api/devices/${encodeURIComponent(deviceId)}/event-history?${eventQuery(filters)}`);
}
export function getEventSummary(deviceId, filters) {
  return request(`/api/devices/${encodeURIComponent(deviceId)}/event-summary?${eventQuery(filters)}`);
}
export async function exportEvents(deviceId, filters) {
  const response = await fetch(`${API_BASE_URL}/api/devices/${encodeURIComponent(deviceId)}/events.csv?${eventQuery(filters)}`);
  if (response.status === 401) window.dispatchEvent(new Event("session-expired"));
  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    throw new Error(error.message || "Không xuất được CSV.");
  }
  const url = URL.createObjectURL(await response.blob());
  const link = document.createElement("a");
  link.href = url; link.download = `events-${deviceId}.csv`; link.click();
  URL.revokeObjectURL(url);
}

const aiPath = deviceId => `/api/devices/${encodeURIComponent(deviceId)}/ai`;
export const getAiStatus = deviceId => request(aiPath(deviceId));
export const startAiCapture = (deviceId, data) => request(`${aiPath(deviceId)}/captures`,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(data)});
export const stopAiCapture = (deviceId,id) => request(`${aiPath(deviceId)}/captures/${encodeURIComponent(id)}/stop`,{method:"POST"});
export const getAiWindows = (deviceId,id,page=0) => request(`${aiPath(deviceId)}/captures/${encodeURIComponent(id)}/windows?page=${page}`);
export const getAiWindow = (deviceId,id) => request(`${aiPath(deviceId)}/windows/${id}`);
export const labelAiWindow = (deviceId,id,label) => request(`${aiPath(deviceId)}/windows/${id}`,{method:"PATCH",headers:{"Content-Type":"application/json"},body:JSON.stringify({label})});
export const deleteAiCapture = (deviceId,id) => request(`${aiPath(deviceId)}/captures/${encodeURIComponent(id)}`,{method:"DELETE"});
export const removeAiModel = deviceId => request(`${aiPath(deviceId)}/model`,{method:"DELETE"});
export function importAiModel(deviceId,file) {
  const body=new FormData();body.append("file",file);
  return request(`${aiPath(deviceId)}/model`,{method:"POST",body});
}
export async function exportAiDataset(deviceId) {
  const response=await fetch(`${API_BASE_URL}${aiPath(deviceId)}/dataset.json`);
  if(response.status===401) window.dispatchEvent(new Event("session-expired"));
  if(!response.ok) {const error=await response.json().catch(()=>({}));throw new Error(error.message||"Không xuất được dữ liệu AI.");}
  const url=URL.createObjectURL(await response.blob());const link=document.createElement("a");
  link.href=url;link.download=`du-lieu-ai-${deviceId}.json`;link.click();URL.revokeObjectURL(url);
}
