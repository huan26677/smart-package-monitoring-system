import {labelVi} from "../services/labels";
import { formatOccurredAt, formatReceivedAt } from "../services/eventFormat";

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
        Chưa có sự kiện kiện hàng
      </div>
    );
  }

  return (
    <div className="table-wrapper">

      <table className="event-table">

        <thead>

          <tr>

            <th>
              Giờ xảy ra
            </th>

            <th>Giờ nhận</th><th>Thời lượng</th><th>Thang đo</th>
            <th>
              Loại sự kiện
            </th>

            <th>
              Mức
            </th>

            <th>
              G
            </th>

            <th>
              Góc nghiêng
            </th>

            <th>
              Mức rung
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
                      formatOccurredAt(
                        event
                      )
                    }
                  </td>

                  <td>{formatReceivedAt(event)}</td>
                  <td>{event.durationMs == null ? "--" : `${event.durationMs} ms`}</td>
                  <td>{event.saturated == null ? "--" : event.saturated ? "Chạm giới hạn đo" : "Trong thang đo"}</td>
                  <td>

                    <strong>
                      {labelVi(event.type)}
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
                        labelVi(event.level)
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
