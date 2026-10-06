import {
  useState
} from "react";

const EMPTY_FORM = {

  bssid: "",

  name: "",

  latitude: "",

  longitude: "",

  radiusMeters: "30"
};

function WifiAnchorPanel({

  anchors,

  wifiAccessPoints,

  onSave,

  onDelete,

  busy

}) {

  const [
    form,
    setForm
  ] =
    useState(
      EMPTY_FORM
    );

  const [
    message,
    setMessage
  ] =
    useState("");

  function updateField(
    field,
    value
  ) {

    setForm(
      (current) => ({

        ...current,

        [field]:
          value
      })
    );
  }

  function useBrowserLocation() {

    if (
      !navigator.geolocation
    ) {

      setMessage(
        "Trình duyệt không hỗ trợ xác định vị trí."
      );

      return;
    }

    setMessage(
      "Đang lấy vị trí trình duyệt..."
    );

    navigator.geolocation
      .getCurrentPosition(

        (position) => {

          setForm(
            (current) => ({

              ...current,

              latitude:
                position
                  .coords
                  .latitude
                  .toFixed(
                    6
                  ),

              longitude:
                position
                  .coords
                  .longitude
                  .toFixed(
                    6
                  )
            })
          );

          setMessage(
            `Đã lấy vị trí, độ chính xác khoảng ±${Math.round(
              position.coords.accuracy
            )} m`
          );
        },

        () => {

          setMessage(
            "Không lấy được vị trí trình duyệt."
          );
        },

        {
          enableHighAccuracy: true,
          timeout: 10000,
          maximumAge: 0
        }
      );
  }

  async function handleSubmit(
    event
  ) {

    event.preventDefault();

    if (
      !form.bssid.trim() ||
      !form.name.trim() ||
      !form.latitude.trim() ||
      !form.longitude.trim()
    ) {

      setMessage(
        "Địa chỉ BSSID, tên, vĩ độ và kinh độ là bắt buộc."
      );

      return;
    }

    const payload = {

      bssid:
        form.bssid
          .trim()
          .toUpperCase(),

      name:
        form.name
          .trim(),

      latitude:
        Number(
          form.latitude
        ),

      longitude:
        Number(
          form.longitude
        ),

      radiusMeters:
        form.radiusMeters.trim()
          ? Number(
              form.radiusMeters
            )
          : null
    };

    if (
      !Number.isFinite(
        payload.latitude
      ) ||
      !Number.isFinite(
        payload.longitude
      )
    ) {

      setMessage(
        "Vĩ độ hoặc kinh độ không hợp lệ."
      );

      return;
    }

    try {

      await onSave(
        payload
      );

      setMessage(
        "Đã lưu mốc Wi-Fi."
      );

      setForm(
        EMPTY_FORM
      );

    }
    catch {

      setMessage(
        "Không thể lưu mốc Wi-Fi."
      );
    }
  }

  async function handleDelete(
    bssid
  ) {

    const confirmed =
      window.confirm(
        `Xóa mốc ${bssid}?`
      );

    if (!confirmed) {

      return;
    }

    try {

      await onDelete(
        bssid
      );

      setMessage(
        "Đã xóa mốc Wi-Fi."
      );

    }
    catch {

      setMessage(
        "Không thể xóa mốc Wi-Fi."
      );
    }
  }

  return (
    <>

      <form
        className="anchor-form"
        onSubmit={
          handleSubmit
        }
      >

        <label className="form-field">

          <span>
            BSSID
          </span>

          <input

            list="detected-bssids"

            value={
              form.bssid
            }

            onChange={
              (event) =>
                updateField(
                  "bssid",
                  event.target.value
                )
            }

            placeholder={
              "AA:BB:CC:DD:EE:FF"
            }

          />

          <datalist
            id="detected-bssids"
          >

            {
              (
                wifiAccessPoints ||
                []
              ).map(
                (ap) => (

                  <option

                    key={
                      ap.bssid
                    }

                    value={
                      ap.bssid
                    }

                    label={
                      `RSSI ${ap.rssi} dBm`
                    }

                  />

                )
              )
            }

          </datalist>

        </label>

        <label className="form-field">

          <span>
            Tên vị trí
          </span>

          <input

            value={
              form.name
            }

            onChange={
              (event) =>
                updateField(
                  "name",
                  event.target.value
                )
            }

            placeholder="Kho A"

          />

        </label>

        <label className="form-field">

          <span>
            Vĩ độ
          </span>

          <input

            type="number"

            step="any"

            value={
              form.latitude
            }

            onChange={
              (event) =>
                updateField(
                  "latitude",
                  event.target.value
                )
            }

          />

        </label>

        <label className="form-field">

          <span>
            Kinh độ
          </span>

          <input

            type="number"

            step="any"

            value={
              form.longitude
            }

            onChange={
              (event) =>
                updateField(
                  "longitude",
                  event.target.value
                )
            }

          />

        </label>

        <label className="form-field">

          <span>
            Bán kính (m)
          </span>

          <input

            type="number"

            min="1"

            value={
              form.radiusMeters
            }

            onChange={
              (event) =>
                updateField(
                  "radiusMeters",
                  event.target.value
                )
            }

          />

        </label>

        <div className="anchor-actions">

          <button
            type="button"
            onClick={
              useBrowserLocation
            }
            disabled={
              busy
            }
          >
            Lấy vị trí trình duyệt
          </button>

          <button
            type="submit"
            className="primary-button"
            disabled={
              busy
            }
          >

            {
              busy
                ? "Đang lưu..."
                : "Lưu / cập nhật mốc"
            }

          </button>

        </div>

      </form>

      {
        message && (

          <div className="anchor-message">
            {message}
          </div>
        )
      }

      <div className="meta anchor-note">

        Có thể chọn BSSID từ Wi-Fi
        mà ESP32 vừa quét.
        Gửi lại cùng BSSID sẽ cập nhật mốc cũ.

      </div>

      <div className="table-wrapper anchor-table-wrapper">

        <table className="event-table">

          <thead>

            <tr>

              <th>
                Tên
              </th>

              <th>
                BSSID
              </th>

              <th>
                Vĩ độ
              </th>

              <th>
                Kinh độ
              </th>

              <th>
                Bán kính
              </th>

              <th>
                Thao tác
              </th>

            </tr>

          </thead>

          <tbody>

            {
              (
                anchors ||
                []
              ).map(
                (anchor) => (

                  <tr
                    key={
                      anchor.bssid
                    }
                  >

                    <td>
                      <strong>
                        {anchor.name}
                      </strong>
                    </td>

                    <td>
                      {anchor.bssid}
                    </td>

                    <td>
                      {
                        anchor.latitude
                      }
                    </td>

                    <td>
                      {
                        anchor.longitude
                      }
                    </td>

                    <td>
                      {
                        anchor.radiusMeters
                      } m
                    </td>

                    <td>

                      <button
                        type="button"
                        className="danger-button"
                        disabled={
                          busy
                        }
                        onClick={
                          () =>
                            handleDelete(
                              anchor.bssid
                            )
                        }
                      >
                        Xóa
                      </button>

                    </td>

                  </tr>
                )
              )
            }

          </tbody>

        </table>

      </div>

    </>
  );
}

export default WifiAnchorPanel;
