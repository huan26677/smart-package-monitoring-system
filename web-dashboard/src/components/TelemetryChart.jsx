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
        No telemetry history
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
        title="Total G"
        data={chartData}
        dataKey="g"
        unit="g"
      />

      <MetricChart
        title="Angle"
        data={chartData}
        dataKey="angle"
        unit="°"
      />

      <MetricChart
        title="Vibration"
        data={chartData}
        dataKey="vibration"
        unit=""
      />

    </div>
  );
}

export default TelemetryChart;
