import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis
} from "recharts";

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

          index:
            index + 1,

          g:
            item.gForce,

          angle:
            item.angle,

          vibration:
            item.vibration
        })
      );

  return (
    <div className="chart-wrapper">

      <ResponsiveContainer
        width="100%"
        height={320}
      >

        <LineChart
          data={chartData}
          margin={{
            top: 10,
            right: 20,
            left: 0,
            bottom: 10
          }}
        >

          <CartesianGrid
            strokeDasharray="3 3"
          />

          <XAxis
            dataKey="index"
            tick={{
              fontSize: 12
            }}
          />

          <YAxis
            tick={{
              fontSize: 12
            }}
          />

          <Tooltip />

          <Legend />

          <Line
            type="monotone"
            dataKey="g"
            name="Total G"
            dot={false}
            isAnimationActive={false}
          />

          <Line
            type="monotone"
            dataKey="angle"
            name="Angle"
            dot={false}
            isAnimationActive={false}
          />

          <Line
            type="monotone"
            dataKey="vibration"
            name="Vibration"
            dot={false}
            isAnimationActive={false}
          />

        </LineChart>

      </ResponsiveContainer>

    </div>
  );
}

export default TelemetryChart;
