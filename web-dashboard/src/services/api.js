const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL ||
  "http://localhost:8080";

async function request(path) {

  const response =
    await fetch(
      `${API_BASE_URL}${path}`
    );

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

  return response.json();
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
