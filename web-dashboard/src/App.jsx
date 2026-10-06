import AuthGate from "./components/AuthGate";
import CollapsibleSection from "./components/CollapsibleSection";
import EventNotice from "./components/EventNotice";
import EventStorageNotice from "./components/EventStorageNotice";
import EventHistoryPanel from "./components/EventHistoryPanel";
import AiPanel from "./components/AiPanel";
import {labelVi} from "./services/labels";
import {
  useCallback,
  useEffect,
  useState
} from "react";

import "./App.css";


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
  getMqttHealth,
  getTelemetry,
  getWifiLocations,
  saveWifiLocation
} from "./services/api";

const AUTO_REFRESH_MS =
  3000;

function getAlertInfo(
  dashboard,
  mqttHealth,
  updateError
) {

  if (!dashboard) {

    return null;
  }

  if (mqttHealth?.status === "UNAVAILABLE") {
    return {
      className: "alert-offline",
      title: "Máy chủ mất kết nối MQTT",
      text: "Máy chủ chưa nhận được dữ liệu từ MQTT. Các chỉ số bên dưới được lưu từ lần nhận cuối; chưa xác định được trạng thái kiện hàng hiện tại."
    };
  }

  if (updateError) {
    return {
      className: "alert-offline",
      title: "Chưa cập nhật được dữ liệu",
      text: "Kết nối đến máy chủ đang gián đoạn. Các chỉ số bên dưới là dữ liệu từ lần nhận cuối."
    };
  }

  if (!dashboard.online) {

    return {
      className: "alert-offline",
      title: "Thiết bị mất kết nối",
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
        title: "Phát hiện rơi kiện hàng",
        text:
          "Phát hiện kiện hàng có dấu hiệu rơi."
      };

    case "IMPACT":

      return {
        className: "alert-danger",
        title: "Phát hiện va đập",
        text:
          "Phát hiện va đập kiện hàng."
      };

    case "FREE_FALL":

      return {
        className: "alert-danger",
        title: "Rơi tự do",
        text:
          "Thiết bị đang phát hiện trạng thái rơi tự do."
      };

    case "FLIP":

      return {
        className: "alert-warning",
        title: "Kiện hàng bị lật",
        text:
          "Kiện hàng đã bị lật."
      };

    case "TILT":

      return {
        className: "alert-warning",
        title: "Kiện hàng bị nghiêng",
        text:
          "Kiện hàng đang bị nghiêng."
      };

    case "VIBRATION":

      return {
        className: "alert-warning",
        title: "Rung lắc",
        text:
          "Phát hiện rung động bất thường."
      };

    case "NORMAL":

      return {
        className: "alert-safe",
        title: "Bình thường",
        text:
          "Trạng thái kiện hàng bình thường."
      };

    default:

      return {
        className: "alert-unknown",
        title: labelVi(state),
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

  const [deviceListError, setDeviceListError] = useState("");
  const [mqttHealth, setMqttHealth] = useState(null);

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

          setDeviceListError("");

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

          setDeviceListError(
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
            locationScansData,
            mqttHealthData
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
              ),
              getMqttHealth().catch(() => null)
            ]);

          setDashboard(
            dashboardData
          );
          setMqttHealth(mqttHealthData);

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

      const intervalId = window.setInterval(loadDevices, AUTO_REFRESH_MS);

      return () => {

        window.clearTimeout(
          timeoutId
        );

        window.clearInterval(intervalId);
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

  const mqttUnavailable = mqttHealth?.status === "UNAVAILABLE";
  const dataStale = !dashboard?.online || mqttUnavailable || Boolean(error);

  const alertInfo =
    getAlertInfo(
      dashboard,
      mqttHealth,
      error
    );

  return (
    <div className="dashboard">

      <header className="header">

        <div>

          <h1>
            Giám sát va đập kiện hàng
          </h1>

          <p>
            Theo dõi kiện hàng bằng ESP32-S3
          </p>

        </div>

        <div className="header-refresh">

          Tự cập nhật:
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
                  selectedDeviceId,
                  true
                )
            }
            disabled={
              !selectedDeviceId
            }
          >

            Làm mới

          </button>

          {
            refreshing && (

              <span className="refreshing">
                Đang cập nhật...
              </span>
            )
          }

          {
            dashboard && (

              <span
                className={
                  !dataStale
                    ? "status online"
                    : "status offline"
                }
              >

                {
                  mqttUnavailable ? "Máy chủ mất kết nối"
                    : error ? "Chưa cập nhật"
                    : dashboard.online ? "Đã kết nối" : "Mất kết nối"
                }

              </span>
            )
          }

        </div>

        {
          (error || deviceListError) && (

            <div className="error">
              {error || deviceListError}
            </div>
          )
        }

        {
          loading ? (

            <div className="loading">
              Đang tải dữ liệu...
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

              <CollapsibleSection id="notices" title="Thông báo và cảnh báo" eager
                description="Cảnh báo đã ghi nhận và tình trạng đồng bộ sự kiện"
                badge={telemetry?.pendingEvents > 0
                  ? `${telemetry.pendingEvents} chờ đồng bộ`
                  : `${eventHistory.filter(event => ["IMPACT", "DROP"].includes(event.type)).length} va đập/rơi gần đây`}
                tone={telemetry?.pendingEvents > 0 ? "warning" : "neutral"}>
                {telemetry && <EventStorageNotice
                  key={`storage:${selectedDeviceId}`}
                  deviceId={selectedDeviceId}
                  telemetry={telemetry}
                  online={dashboard.online}
                />}
                <EventNotice key={`notice:${selectedDeviceId}`} deviceId={selectedDeviceId} events={eventHistory} />
                {!(telemetry?.pendingEvents > 0) && !(telemetry?.rejectedEvents > 0)
                  && !eventHistory.some(event => ["IMPACT", "DROP"].includes(event.type))
                  && <p className="section-note">Chưa có cảnh báo được ghi nhận.</p>}
              </CollapsibleSection>

              <CollapsibleSection id="overview" title={dataStale ? "Chỉ số lần nhận gần nhất" : "Chỉ số hiện tại"} defaultOpen
                description={dataStale ? "Chưa có dữ liệu mới từ ESP32" : "Thông số mới nhất từ ESP32"}
                badge={dataStale ? "Dữ liệu cũ" : labelVi(telemetry?.state)}
                tone={dataStale ? "neutral" : telemetry?.state === "NORMAL" ? "safe" : "warning"}>
              {dataStale && <p className="data-stale-note" role="status">
                Đang hiển thị dữ liệu đã lưu
                {telemetry?.receivedAt && <> lúc <time dateTime={telemetry.receivedAt}>{new Date(telemetry.receivedAt).toLocaleString("vi-VN")}</time></>}.
                {" "}Chưa xác định được trạng thái kiện hàng hiện tại.
              </p>}
              <div className={`grid${dataStale ? " telemetry-stale" : ""}`}>

                <section className="card">

                  <h2>
                    Gia tốc tổng hợp
                  </h2>

                  <div className="value">

                    {
                      formatNumber(
                        telemetry?.gForce
                      )
                    } g

                  </div>

                  <div className="meta">

                    {dataStale ? "Trạng thái lần nhận cuối:" : "Trạng thái:"}
                    {" "}
                    {
                      labelVi(telemetry?.state)
                    }

                  </div>

                </section>

                <section className="card">

                  <h2>
                    Góc nghiêng
                  </h2>

                  <div className="value">

                    {
                      formatNumber(
                        telemetry?.angle
                      )
                    }°

                  </div>

                  <div className="meta">

                    Mức rung:
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
                    Cường độ Wi-Fi
                  </h2>

                  <div className="value">

                    {
                      telemetry?.wifiRssi ??
                      "--"
                    } dBm

                  </div>

                  <div className="meta">

                    Nhận dữ liệu cách đây:
                    {" "}
                    {
                      dashboard
                        .secondsSinceLastSeen
                    } giây

                  </div>

                </section>

                <section className="card">

                  <h2>
                    Sự kiện gần nhất
                  </h2>

                  {
                    latestEvent ? (

                      <>

                        <div className="value">

                          {
                            labelVi(latestEvent.type)
                          }

                        </div>

                        <div className="meta">

                          Mức:
                          {" "}
                          {
                            labelVi(latestEvent.level)
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
                        Chưa có sự kiện
                      </div>
                    )
                  }

                </section>

                <section className="card">

                  <h2>
                    {dataStale ? "Vị trí gần nhất" : "Vị trí hiện tại"}
                  </h2>

                  {
                    location ? (

                      <>

                        <div className="value">

                          {
                            location
                              .locationLabel ??
                            "Chưa xác định"
                          }

                        </div>

                        <div className="meta">

                          Trạng thái:
                          {" "}
                          {
                            labelVi(location.locationStatus)
                          }

                        </div>

                        <div className="meta">

                          Cường độ Wi-Fi tại mốc:
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
                        Chưa có lượt quét vị trí
                      </div>
                    )
                  }

                </section>

              </div>
              </CollapsibleSection>

              <CollapsibleSection id="sensors" title="Lịch sử cảm biến"
                description="Biểu đồ gia tốc, góc nghiêng và mức rung"
                badge={`${telemetryHistory.length} mẫu gần nhất`}>
                <TelemetryChart data={telemetryHistory} />
              </CollapsibleSection>

              <CollapsibleSection id="events" title="Lịch sử sự kiện"
                description="Tra cứu, lọc và xuất dữ liệu sự kiện">
                <EventHistoryPanel key={`history:${selectedDeviceId}`} deviceId={selectedDeviceId} />
              </CollapsibleSection>

              <CollapsibleSection id="ai" title="AI phân biệt va đập"
                description="Thu dữ liệu, gắn nhãn và quản lý mô hình">
                <AiPanel key={`ai:${selectedDeviceId}`} deviceId={selectedDeviceId} />
              </CollapsibleSection>

              <CollapsibleSection id="map" title="Bản đồ vị trí"
                description="Vị trí ước tính theo mốc Wi-Fi"
                badge={location?.locationLabel || "Chưa xác định"}>
                <p className="section-note">Định vị theo mốc Wi-Fi, không phải GPS chính xác.</p>
                <LocationMap anchors={wifiAnchors} currentLocation={location} locationHistory={locationHistory} />
              </CollapsibleSection>

              <CollapsibleSection id="locations" title="Lịch sử vị trí"
                description="Các lần quét và nhận diện vị trí gần đây"
                badge={`${locationHistory.length} lượt quét`}>
                <LocationHistoryTable scans={locationHistory} />
              </CollapsibleSection>

              <CollapsibleSection id="wifi" title="Quản lý mốc Wi-Fi"
                description="Đăng ký khu vực, tọa độ và phạm vi nhận diện"
                badge={`${wifiAnchors.length} mốc đã lưu`}>
                <WifiAnchorPanel anchors={wifiAnchors}
                  wifiAccessPoints={latestLocationScan?.wifiAccessPoints ?? []}
                  busy={anchorBusy} onSave={handleSaveAnchor} onDelete={handleDeleteAnchor} />
              </CollapsibleSection>

            </>

          ) : (

            <div className="empty">

              {
                devices.length === 0
                  ? "Chưa tìm thấy thiết bị"
                  : "Chưa có dữ liệu thiết bị"
              }

            </div>
          )
        }

      </main>

    </div>
  );
}

export default function AuthenticatedApp() {
  return <AuthGate><App /></AuthGate>;
}
