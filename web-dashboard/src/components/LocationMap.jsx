import {
  useEffect
} from "react";

import {
  CircleMarker,
  MapContainer,
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

function LocationMap({
  anchors,
  currentLocation
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
    )
  ];

  if (
    points.length === 0
  ) {

    return (
      <div className="empty">

        Chưa có tọa độ Anchor
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

      <MapAutoFit
        points={
          points
        }
      />

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

                Radius:
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
                "Unknown"
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
