import {
  useEffect
} from "react";

import {
  CircleMarker,
  MapContainer,
  Polyline,
  Popup,
  TileLayer,
  useMap
} from "react-leaflet";

import "leaflet/dist/leaflet.css";

function hasCoordinates(
  item
) {

  return (
    item &&
    item.latitude !== null &&
    item.latitude !== undefined &&
    item.latitude !== "" &&
    item.longitude !== null &&
    item.longitude !== undefined &&
    item.longitude !== "" &&
    Number.isFinite(
      Number(
        item.latitude
      )
    ) &&
    Number.isFinite(
      Number(
        item.longitude
      )
    )
  );
}

function buildHistoryPoints(
  history
) {

  const scans =
    [...(history || [])]
      .reverse()
      .filter(
        (scan) =>
          scan.locationStatus ===
            "ANCHOR_MATCHED" &&
          hasCoordinates(
            scan
          )
      );

  const result =
    [];

  let lastKey =
    null;

  for (
    const scan of scans
  ) {

    const key =
      `${scan.latitude},${scan.longitude}`;

    if (
      key === lastKey
    ) {

      continue;
    }

    result.push([

      Number(
        scan.latitude
      ),

      Number(
        scan.longitude
      )
    ]);

    lastKey =
      key;
  }

  return result;
}

function MapAutoFit({
  points
}) {

  const map =
    useMap();

  useEffect(
    () => {

      if (
        points.length === 1
      ) {

        map.setView(
          points[0],
          16
        );

        return;
      }

      if (
        points.length > 1
      ) {

        map.fitBounds(
          points,
          {
            padding: [
              30,
              30
            ],

            maxZoom: 17
          }
        );
      }

    },
    [
      map,
      points
    ]
  );

  return null;
}

function MapResize() {
  const map = useMap();
  useEffect(() => {
    const container = map.getContainer();
    let frame;
    const observer = new ResizeObserver(() => {
      if (container.clientWidth > 0 && container.clientHeight > 0) {
        cancelAnimationFrame(frame);
        frame = requestAnimationFrame(() => map.invalidateSize({ pan: false }));
      }
    });
    observer.observe(container);
    return () => { observer.disconnect(); cancelAnimationFrame(frame); };
  }, [map]);
  return null;
}

function LocationMap({
  anchors,
  currentLocation,
  locationHistory
}) {

  const validAnchors =
    (anchors || [])
      .filter(
        hasCoordinates
      );

  const currentValid =
    hasCoordinates(
      currentLocation
    );

  const historyPoints =
    buildHistoryPoints(
      locationHistory
    );

  const points = [

    ...validAnchors.map(
      (anchor) => [

        Number(
          anchor.latitude
        ),

        Number(
          anchor.longitude
        )
      ]
    ),

    ...(
      currentValid
        ? [[

            Number(
              currentLocation.latitude
            ),

            Number(
              currentLocation.longitude
            )
          ]]
        : []
    ),

    ...historyPoints
  ];

  if (
    points.length === 0
  ) {

    return (
      <div className="empty">

        Chưa có tọa độ mốc Wi-Fi
        để hiển thị bản đồ.

      </div>
    );
  }

  return (
    <MapContainer

      center={
        points[0]
      }

      zoom={15}

      className="map-container"

      scrollWheelZoom={true}

    >

      <TileLayer

        attribution={
          '&copy; OpenStreetMap contributors'
        }

        url={
          "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        }

      />

      <MapResize />

      <MapAutoFit
        points={
          points
        }
      />

      {
        historyPoints.length >= 2 && (

          <Polyline

            positions={
              historyPoints
            }

            pathOptions={{
              color: "#2563eb",
              weight: 4,
              opacity: 0.75
            }}

          />
        )
      }

      {
        validAnchors.map(
          (anchor) => (

            <CircleMarker

              key={
                anchor.bssid
              }

              center={[

                Number(
                  anchor.latitude
                ),

                Number(
                  anchor.longitude
                )
              ]}

              radius={8}

            >

              <Popup>

                <strong>
                  {anchor.name}
                </strong>

                <br />

                BSSID:
                {" "}
                {anchor.bssid}

                <br />

                Bán kính:
                {" "}
                {anchor.radiusMeters} m

              </Popup>

            </CircleMarker>
          )
        )
      }

      {
        currentValid && (

          <CircleMarker

            center={[

              Number(
                currentLocation.latitude
              ),

              Number(
                currentLocation.longitude
              )
            ]}

            radius={12}

            pathOptions={{
              color: "#c62828",
              fillColor: "#ef5350",
              fillOpacity: 0.8
            }}

          >

            <Popup>

              <strong>
                Kiện hàng hiện tại
              </strong>

              <br />

              {
                currentLocation
                  .locationLabel ??
                "Chưa xác định"
              }

              <br />

              RSSI:
              {" "}
              {
                currentLocation
                  .matchedRssi ??
                "--"
              } dBm

            </Popup>

          </CircleMarker>
        )
      }

    </MapContainer>
  );
}

export default LocationMap;
