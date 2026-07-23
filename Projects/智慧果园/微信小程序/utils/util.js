const ALARM_MAP = {
  0x01: "气体浓度异常",
  0x02: "土壤过干",
  0x04: "光照不足",
  0x08: "温度过高",
  0x10: "温度过低"
};

function parseAlarm(alarm) {
  if (!alarm) {
    return { text: "当前无告警", warn: false };
  }
  const items = [];
  Object.keys(ALARM_MAP).forEach((bit) => {
    if (alarm & Number(bit)) {
      items.push(ALARM_MAP[bit]);
    }
  });
  if (items.length === 0) {
    return { text: "当前无告警", warn: false };
  }
  return { text: items.join(" · "), warn: true };
}

function x10ToVolt(value) {
  if (value === null || value === undefined || value === "") {
    return "";
  }
  return (Number(value) / 10).toFixed(1);
}

function voltToX10(volt) {
  const n = Number(volt);
  if (Number.isNaN(n)) {
    return 0;
  }
  return Math.round(n * 10);
}

function formatValue(value, unit, digits) {
  if (value === null || value === undefined) {
    return "--";
  }
  const n = Number(value);
  if (Number.isNaN(n)) {
    return "--";
  }
  if (digits !== undefined) {
    return n.toFixed(digits) + unit;
  }
  return String(n) + unit;
}

function formatTime(iso) {
  if (!iso) {
    return "暂无数据";
  }
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) {
    return iso;
  }
  const pad = (n) => (n < 10 ? "0" + n : "" + n);
  return (
    pad(d.getMonth() + 1) +
    "-" +
    pad(d.getDate()) +
    " " +
    pad(d.getHours()) +
    ":" +
    pad(d.getMinutes()) +
    ":" +
    pad(d.getSeconds())
  );
}

function clampPct(n, max) {
  if (n === null || n === undefined || Number.isNaN(Number(n))) {
    return 0;
  }
  const v = Math.max(0, Math.min(100, (Number(n) / max) * 100));
  return Math.round(v);
}

function splitValue(text) {
  if (!text || text === "--") {
    return { num: "--", unit: "" };
  }
  const m = String(text).match(/^([\d.]+)(.*)$/);
  if (!m) {
    return { num: text, unit: "" };
  }
  return { num: m[1], unit: m[2] };
}

module.exports = {
  parseAlarm,
  x10ToVolt,
  voltToX10,
  formatValue,
  formatTime,
  clampPct,
  splitValue
};
