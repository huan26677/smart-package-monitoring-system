import {
  useCallback,
  useEffect,
  useState
} from "react";

import "./App.css";

import TelemetryChart
  from "./components/TelemetryChart";

import {
  getDevices,
  getDeviceDashboard,
  getTelemetry
} from "./services/api";

const AUTO_REFRESH_MS =
  3000;

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

          const data =
            await getDevices();

          setDevices(
            data
          );

          if (
            data.length > 0
          ) {

            setSelectedDeviceId(
              (currentDeviceId) =>
                currentDeviceId ||
                data[0].deviceId
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
            telemetryData
          ] =
            await Promise.all([

              getDeviceDashboard(
                deviceId
              ),

              getTelemetry(
                deviceId,
                60
              )
            ]);

          setDashboard(
            dashboardData
          );

          setTelemetryHistory(
            telemetryData
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
