function getLevelClass(
  level
) {

  if (!level) {

    return "";
  }

  return (
    "level-" +
    level.toLowerCase()
  );
}

function formatEventTime(
  event
) {

  if (event.receivedAt) {

    return new Date(
      event.receivedAt
    ).toLocaleString(
      "vi-VN"
    );
  }

  if (event.timeText) {

    return event.timeText;
  }

  return "--";
}

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

function EventTable({
  events
}) {

  if (
    !events ||
    events.length === 0
  ) {

    return (
      <div className="empty">
        No package event
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
              Type
            </th>

            <th>
              Level
            </th>

            <th>
              G
            </th>

            <th>
              Angle
            </th>

            <th>
              Vibration
            </th>

          </tr>

        </thead>

        <tbody>

          {
            events.map(
              (event) => (

                <tr
                  key={event.id}
                >

                  <td>
                    {
                      formatEventTime(
                        event
                      )
                    }
                  </td>

                  <td>

                    <strong>
                      {event.type}
                    </strong>

                  </td>

                  <td>

                    <span
                      className={
                        `event-level ${getLevelClass(
                          event.level
                        )}`
                      }
                    >

                      {
                        event.level ||
                        "--"
                      }

                    </span>

                  </td>

                  <td>
                    {
                      formatNumber(
                        event.gForce
                      )
                    }
                  </td>

                  <td>
                    {
                      formatNumber(
                        event.angle
                      )
                    }°
                  </td>

                  <td>
                    {
                      formatNumber(
                        event.vibration,
                        3
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

export default EventTable;
