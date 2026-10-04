import {
  useEffect,
  useState
} from "react";

import "./App.css";

import {
  getDevices,
  getDeviceDashboard
} from "./services/api";

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
    loading,
    setLoading
  ] =
    useState(true);

  const [
    error,
    setError
  ] =
    useState("");

  async function loadDevices() {

    try {

      const data =
        await getDevices();

      setDevices(
        data
      );

      if (
        data.length > 0 &&
        !selectedDeviceId
      ) {

        setSelectedDeviceId(
          data[0].deviceId
        );
      }

    }
    catch (err) {

      setError(
        err.message
      );
    }
  }

  async function loadDashboard(
    deviceId
  ) {

    if (!deviceId) {

      return;
    }

    try {

      setLoading(
        true
      );

      setError(
        ""
      );

      const data =
        await getDeviceDashboard(
          deviceId
        );

      setDashboard(
        data
      );

    }
    catch (err) {

      setError(
        err.message
      );

      setDashboard(
        null
      );

    }
    finally {

      setLoading(
        false
      );
    }
  }

  useEffect(
    () => {

      loadDevices();

    },
    []
  );

  useEffect(
    () => {

      if (selectedDeviceId) {

        loadDashboard(
          selectedDeviceId
        );
      }

    },
    [selectedDeviceId]
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

  const event =
    dashboard?.latestEvent;

  const location =
    dashboard?.latestLocation;

  return (
    <div className="dashboard">

      <header className="header">

        <h1>
          Smart Package Monitoring
        </h1>

        <p>
          ESP32-S3 package monitoring dashboard
        </p>

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
                loadDashboard(
                  selectedDeviceId
                )
            }
          >
            Refresh
          </button>

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
                  Wi-Fi
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
                  event ? (

                    <>
                      <div className="value">
                        {event.type}
                      </div>

                      <div className="meta">
                        Level:
                        {" "}
                        {event.level}
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

                        RSSI:
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

          ) : (

            <div className="empty">
              No device data
            </div>
          )
        }

      </main>

    </div>
  );
}

export default App;
