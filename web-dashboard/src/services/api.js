const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL ||
  "http://localhost:8080";

async function request(
  path,
  options = {}
) {

  const response =
    await fetch(
      `${API_BASE_URL}${path}`,
      options
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

  if (
    response.status === 204
  ) {

    return null;
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
