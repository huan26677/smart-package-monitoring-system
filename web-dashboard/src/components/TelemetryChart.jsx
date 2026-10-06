import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis
} from "recharts";

function MetricChart({
  title,
  data,
  dataKey,
  unit
}) {

  return (
    <div className="metric-chart">

      <div className="metric-chart-title">
        {title}
      </div>

      <ResponsiveContainer
        width="100%"
        height={240}
      >

        <LineChart
          data={data}
          margin={{
            top: 10,
            right: 15,
            left: 0,
            bottom: 5
          }}
        >

          <CartesianGrid
            strokeDasharray="3 3"
          />

          <XAxis
            dataKey="time"
            minTickGap={30}
            tick={{
              fontSize: 11
            }}
          />

          <YAxis
            width={55}
            tick={{
              fontSize: 11
            }}
          />

          <Tooltip
            formatter={
              (value) => [
                `${value} ${unit}`,
                title
              ]
            }
          />

          <Line
            type="monotone"
            dataKey={dataKey}
            name={title}
            dot={false}
            isAnimationActive={false}
          />

        </LineChart>

      </ResponsiveContainer>

    </div>
  );
}

function TelemetryChart({
  data
}) {

  if (
    !data ||
    data.length === 0
  ) {

    return (
      <div className="empty">
        Chưa có lịch sử cảm biến
      </div>
    );
  }

  const chartData =
    [...data]
      .reverse()
      .map(
        (item, index) => ({

          time:
            item.receivedAt
              ? new Date(
                  item.receivedAt
                ).toLocaleTimeString(
                  "vi-VN",
                  {
                    hour: "2-digit",
                    minute: "2-digit",
                    second: "2-digit"
                  }
                )
              : `${index + 1}`,

          g:
            item.gForce,

          angle:
            item.angle,

          vibration:
            item.vibration
        })
      );

  return (
    <div className="telemetry-charts">

      <MetricChart
        title="Gia tốc tổng hợp"
        data={chartData}
        dataKey="g"
        unit="g"
      />

      <MetricChart
        title="Góc nghiêng"
        data={chartData}
        dataKey="angle"
        unit="°"
      />

      <MetricChart
        title="Mức rung"
        data={chartData}
        dataKey="vibration"
        unit=""
      />

    </div>
  );
}

export default TelemetryChart;
