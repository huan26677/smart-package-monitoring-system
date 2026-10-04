import {
  useCallback,
  useEffect,
  useState
} from "react";

import "./App.css";

import EventTable
  from "./components/EventTable";

import LocationMap
  from "./components/LocationMap";

import LocationHistoryTable
  from "./components/LocationHistoryTable";

import TelemetryChart
  from "./components/TelemetryChart";

import WifiAnchorPanel
  from "./components/WifiAnchorPanel";

import {
  deleteWifiLocation,
  getDevices,
  getDeviceDashboard,
  getEvents,
  getLocationScans,
  getTelemetry,
  getWifiLocations,
  saveWifiLocation
} from "./services/api";

const AUTO_REFRESH_MS =
  3000;

function getAlertInfo(
  dashboard
) {

  if (!dashboard) {

    return null;
  }

  if (!dashboard.online) {

    return {
      className: "alert-offline",
      title: "DEVICE OFFLINE",
      text:
        "Không nhận được dữ liệu mới từ thiết bị."
    };
  }

  const state =
    dashboard
      .latestTelemetry
      ?.state ??
    "UNKNOWN";

  switch (state) {

    case "DROP":

      return {
        className: "alert-danger",
        title: "DROP DETECTED",
        text:
          "Phát hiện kiện hàng có dấu hiệu rơi."
      };

    case "IMPACT":

      return {
        className: "alert-danger",
        title: "IMPACT DETECTED",
        text:
          "Phát hiện va đập kiện hàng."
      };

    case "FREE_FALL":

      return {
        className: "alert-danger",
        title: "FREE FALL",
        text:
          "Thiết bị đang phát hiện trạng thái rơi tự do."
      };

    case "FLIP":

      return {
        className: "alert-warning",
        title: "PACKAGE FLIPPED",
        text:
          "Kiện hàng đã bị lật."
      };

    case "TILT":

      return {
        className: "alert-warning",
        title: "PACKAGE TILTED",
        text:
          "Kiện hàng đang bị nghiêng."
      };

    case "VIBRATION":

      return {
        className: "alert-warning",
        title: "VIBRATION",
        text:
          "Phát hiện rung động bất thường."
      };

    case "NORMAL":

      return {
        className: "alert-safe",
        title: "NORMAL",
        text:
          "Trạng thái kiện hàng bình thường."
      };

    default:

      return {
        className: "alert-unknown",
        title: state,
        text:
          "Chưa xác định trạng thái kiện hàng."
      };
  }
}

function App() {

  const [
    devices,
    setDevices
  ] =
    useState([]);

  const [
    selectedDeviceId,
    setSelectedDeviceId
  ] =
    useState("");

  const [
    dashboard,
    setDashboard
  ] =
    useState(null);

  const [
    telemetryHistory,
    setTelemetryHistory
  ] =
    useState([]);

  const [
    eventHistory,
    setEventHistory
  ] =
    useState([]);

  const [
    wifiAnchors,
    setWifiAnchors
  ] =
    useState([]);

  const [
    latestLocationScan,
    setLatestLocationScan
  ] =
    useState(null);

  const [
    locationHistory,
    setLocationHistory
  ] =
    useState([]);

  const [
    anchorBusy,
    setAnchorBusy
  ] =
    useState(false);

  const [
    loading,
    setLoading
  ] =
    useState(true);

  const [
    refreshing,
    setRefreshing
  ] =
    useState(false);

  const [
    error,
    setError
  ] =
    useState("");

  const loadDevices =
    useCallback(
      async () => {

        try {

          const [
            deviceData,
            anchorData
          ] =
            await Promise.all([

              getDevices(),

              getWifiLocations()
            ]);

          setDevices(
            deviceData
          );

          setWifiAnchors(
            anchorData
          );

          if (
            deviceData.length > 0
          ) {

            setSelectedDeviceId(
              (currentDeviceId) =>
                currentDeviceId ||
                deviceData[0].deviceId
            );
          }

        }
        catch (err) {

          setError(
            err.message
          );

        }
        finally {

          setLoading(
            false
          );
        }
      },
      []
    );

  const loadDeviceData =
    useCallback(
      async (
        deviceId,
        background = false
      ) => {

        if (!deviceId) {

          return;
        }

        if (background) {

          setRefreshing(
            true
          );

        }
        else {

          setLoading(
            true
          );
        }

        try {

          const [
            dashboardData,
            telemetryData,
            eventsData,
            locationScansData
          ] =
            await Promise.all([

              getDeviceDashboard(
                deviceId
              ),

              getTelemetry(
                deviceId,
                60
              ),

              getEvents(
                deviceId,
                20
              ),

              getLocationScans(
                deviceId,
                50
              )
            ]);

          setDashboard(
            dashboardData
          );

          setTelemetryHistory(
            telemetryData
          );

          setEventHistory(
            eventsData
          );

          setLatestLocationScan(

            locationScansData[0] ??
            null
          );

          setLocationHistory(
            locationScansData
          );

          setError(
            ""
          );

        }
        catch (err) {

          setError(
            err.message
          );

        }
        finally {

          if (background) {

            setRefreshing(
              false
            );

          }
          else {

            setLoading(
              false
            );
          }
        }
      },
      []
    );

  const reloadAnchors =
    useCallback(
      async () => {

        const data =
          await getWifiLocations();

        setWifiAnchors(
          data
        );
      },
      []
    );

  const handleSaveAnchor =
    useCallback(
      async (
        anchor
      ) => {

        setAnchorBusy(
          true
        );

        try {

          await saveWifiLocation(
            anchor
          );

          await reloadAnchors();

          setError(
            ""
          );

        }
        catch (err) {

          setError(
            err.message
          );

          throw err;

        }
        finally {

          setAnchorBusy(
            false
          );
        }
      },
      [reloadAnchors]
    );

  const handleDeleteAnchor =
    useCallback(
      async (
        bssid
      ) => {

        setAnchorBusy(
          true
        );

        try {

          await deleteWifiLocation(
            bssid
          );

          await reloadAnchors();

          setError(
            ""
          );

        }
        catch (err) {

          setError(
            err.message
          );

          throw err;

        }
        finally {

          setAnchorBusy(
            false
          );
        }
      },
      [reloadAnchors]
    );

  useEffect(
    () => {

      const timeoutId =
        window.setTimeout(
          () => {

            loadDevices();

          },
          0
        );

      return () => {

        window.clearTimeout(
          timeoutId
        );
      };

    },
    [loadDevices]
  );

  useEffect(
    () => {

      if (!selectedDeviceId) {

        return;
      }

      const initialLoadTimeoutId =
        window.setTimeout(
          () => {

            loadDeviceData(
              selectedDeviceId
            );

          },
          0
        );

      const intervalId =
        window.setInterval(
          () => {

            loadDeviceData(
              selectedDeviceId,
              true
            );

          },
          AUTO_REFRESH_MS
        );

      return () => {

        window.clearTimeout(
          initialLoadTimeoutId
        );

        window.clearInterval(
          intervalId
        );
      };

    },
    [
      selectedDeviceId,
      loadDeviceData
    ]
  );

  function formatNumber(
    value,
    digits = 2
  ) {

    if (
      value === null ||
      value === undefined
    ) {

      return "--";
    }

    return Number(
      value
    ).toFixed(
      digits
    );
  }

  const telemetry =
    dashboard?.latestTelemetry;

  const latestEvent =
    dashboard?.latestEvent;

  const location =
    dashboard?.latestLocation;

  const alertInfo =
    getAlertInfo(
      dashboard
    );

  return (
    <div className="dashboard">

      <header className="header">

        <div>

          <h1>
            Smart Package Monitoring
          </h1>

          <p>
            ESP32-S3 package monitoring dashboard
          </p>

        </div>

        <div className="header-refresh">

          Auto refresh:
          {" "}
          {AUTO_REFRESH_MS / 1000}s

        </div>

      </header>

      <main className="content">

        <div className="toolbar">

          <select
            value={selectedDeviceId}
            onChange={
              (event) =>
                setSelectedDeviceId(
                  event.target.value
                )
            }
          >

            {
              devices.map(
                (device) => (

                  <option
                    key={device.deviceId}
                    value={device.deviceId}
                  >

                    {device.deviceId}

                  </option>

                )
              )
            }

          </select>

          <button
            onClick={
              () =>
                loadDeviceData(
                  selectedDeviceId
                )
            }
            disabled={
              !selectedDeviceId
            }
          >

            Refresh

          </button>

          {
            refreshing && (

              <span className="refreshing">
                Updating...
              </span>
            )
          }

          {
            dashboard && (

              <span
                className={
                  dashboard.online
                    ? "status online"
                    : "status offline"
                }
              >

                {
                  dashboard.online
                    ? "ONLINE"
                    : "OFFLINE"
                }

              </span>
            )
          }

        </div>

        {
          error && (

            <div className="error">
              {error}
            </div>
          )
        }

        {
          loading ? (

            <div className="loading">
              Loading dashboard...
            </div>

          ) : dashboard ? (

            <>

              {
                alertInfo && (

                  <section
                    className={
                      `alert-banner ${alertInfo.className}`
                    }
                  >

                    <div className="alert-title">
                      {alertInfo.title}
                    </div>

                    <div className="alert-text">
                      {alertInfo.text}
                    </div>

                  </section>
                )
              }

              <div className="grid">

                <section className="card">

                  <h2>
                    Total G
                  </h2>

                  <div className="value">

                    {
                      formatNumber(
                        telemetry?.gForce
                      )
                    } g

                  </div>

                  <div className="meta">

                    State:
                    {" "}
                    {
                      telemetry?.state ??
                      "--"
                    }

                  </div>

                </section>

                <section className="card">

                  <h2>
                    Angle
                  </h2>

                  <div className="value">

                    {
                      formatNumber(
                        telemetry?.angle
                      )
                    }°

                  </div>

                  <div className="meta">

                    Vibration:
                    {" "}
                    {
                      formatNumber(
                        telemetry?.vibration,
                        3
                      )
                    }

                  </div>

                </section>

                <section className="card">

                  <h2>
                    Wi-Fi RSSI
                  </h2>

                  <div className="value">

                    {
                      telemetry?.wifiRssi ??
                      "--"
                    } dBm

                  </div>

                  <div className="meta">

                    Last seen:
                    {" "}
                    {
                      dashboard
                        .secondsSinceLastSeen
                    }s ago

                  </div>

                </section>

                <section className="card">

                  <h2>
                    Latest Event
                  </h2>

                  {
                    latestEvent ? (

                      <>

                        <div className="value">

                          {
                            latestEvent.type
                          }

                        </div>

                        <div className="meta">

                          Level:
                          {" "}
                          {
                            latestEvent.level
                          }

                        </div>

                        <div className="meta">

                          G:
                          {" "}
                          {
                            formatNumber(
                              latestEvent.gForce
                            )
                          }

                        </div>

                      </>

                    ) : (

                      <div className="empty">
                        No event
                      </div>
                    )
                  }

                </section>

                <section className="card">

                  <h2>
                    Current Location
                  </h2>

                  {
                    location ? (

                      <>

                        <div className="value">

                          {
                            location
                              .locationLabel ??
                            "Unknown"
                          }

                        </div>

                        <div className="meta">

                          Status:
                          {" "}
                          {
                            location
                              .locationStatus ??
                            "--"
                          }

                        </div>

                        <div className="meta">

                          Anchor RSSI:
                          {" "}
                          {
                            location
                              .matchedRssi ??
                            "--"
                          } dBm

                        </div>

                      </>

                    ) : (

                      <div className="empty">
                        No location scan
                      </div>
                    )
                  }

                </section>

              </div>

              <section className="card chart-card">

                <div className="section-heading">

                  <div>

                    <h2>
                      Telemetry History
                    </h2>

                    <div className="meta">
                      Latest 60 samples
                    </div>

                  </div>

                </div>

                <TelemetryChart
                  data={
                    telemetryHistory
                  }
                />

              </section>

              <section className="card event-card">

                <div className="section-heading">

                  <div>

                    <h2>
                      Package Event History
                    </h2>

                    <div className="meta">
                      Latest 20 events
                    </div>

                  </div>

                </div>

                <EventTable
                  events={
                    eventHistory
                  }
                />

              </section>

              <section className="card location-card">

                <div className="section-heading">

                  <div>

                    <h2>
                      Package Location Map
                    </h2>

                    <div className="meta">

                      Wi-Fi Anchor location,
                      không phải GPS chính xác.

                    </div>

                  </div>

                </div>

                <LocationMap

                  anchors={
                    wifiAnchors
                  }

                  currentLocation={
                    location
                  }

                  locationHistory={
                    locationHistory
                  }

                />

              </section>

              <section className="card location-history-card">

                <div className="section-heading">

                  <div>

                    <h2>
                      Location History
                    </h2>

                    <div className="meta">

                      50 Wi-Fi location scans
                      gần nhất.

                    </div>

                  </div>

                </div>

                <LocationHistoryTable

                  scans={
                    locationHistory
                  }

                />

              </section>

              <section className="card anchor-card">

                <div className="section-heading">

                  <div>

                    <h2>
                      Wi-Fi Anchor Management
                    </h2>

                    <div className="meta">

                      Đăng ký BSSID với
                      tên khu vực và tọa độ.

                    </div>

                  </div>

                </div>

                <WifiAnchorPanel

                  anchors={
                    wifiAnchors
                  }

                  wifiAccessPoints={
                    latestLocationScan
                      ?.wifiAccessPoints ??
                    []
                  }

                  busy={
                    anchorBusy
                  }

                  onSave={
                    handleSaveAnchor
                  }

                  onDelete={
                    handleDeleteAnchor
                  }

                />

              </section>

            </>

          ) : (

            <div className="empty">

              {
                devices.length === 0
                  ? "No device found"
                  : "No device data"
              }

            </div>
          )
        }

      </main>

    </div>
  );
}

export default App;
