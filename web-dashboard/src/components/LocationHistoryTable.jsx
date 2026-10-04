function formatTime(
  value
) {

  if (!value) {

    return "--";
  }

  return new Date(
    value
  ).toLocaleString(
    "vi-VN"
  );
}

function formatCoordinate(
  value
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
    6
  );
}

function getStatusClass(
  status
) {

  switch (status) {

    case "ANCHOR_MATCHED":

      return "location-matched";

    case "NO_ANCHOR":

      return "location-unknown";

    case "PENDING":

      return "location-pending";

    default:

      return "";
  }
}

function LocationHistoryTable({
  scans
}) {

  if (
    !scans ||
    scans.length === 0
  ) {

    return (
      <div className="empty">
        Chưa có lịch sử vị trí.
      </div>
    );
  }

  return (
    <div className="table-wrapper">

      <table className="event-table">

        <thead>

          <tr>

            <th>
              Time
            </th>

            <th>
              Status
            </th>

            <th>
              Location
            </th>

            <th>
              BSSID
            </th>

            <th>
              RSSI
            </th>

            <th>
              Latitude
            </th>

            <th>
              Longitude
            </th>

          </tr>

        </thead>

        <tbody>

          {
            scans.map(
              (scan) => (

                <tr
                  key={
                    scan.id
                  }
                >

                  <td>
                    {
                      formatTime(
                        scan.receivedAt
                      )
                    }
                  </td>

                  <td>

                    <span
                      className={
                        `location-status ${getStatusClass(
                          scan.locationStatus
                        )}`
                      }
                    >

                      {
                        scan.locationStatus ??
                        "--"
                      }

                    </span>

                  </td>

                  <td>
                    {
                      scan.locationLabel ??
                      "--"
                    }
                  </td>

                  <td>
                    {
                      scan.matchedBssid ??
                      "--"
                    }
                  </td>

                  <td>
                    {
                      scan.matchedRssi ??
                      "--"
                    }
                    {
                      scan.matchedRssi !== null &&
                      scan.matchedRssi !== undefined
                        ? " dBm"
                        : ""
                    }
                  </td>

                  <td>
                    {
                      formatCoordinate(
                        scan.latitude
                      )
                    }
                  </td>

                  <td>
                    {
                      formatCoordinate(
                        scan.longitude
                      )
                    }
                  </td>

                </tr>
              )
            )
          }

        </tbody>

      </table>

    </div>
  );
}

export default LocationHistoryTable;
