const api = require("../../utils/api");
const util = require("../../utils/util");
const { REFRESH_MS } = require("../../config");

Page({
  data: {
    tab: 0,
    loading: true,
    online: false,
    updatedAt: "",
    alarmText: "当前无告警",
    alarmWarn: false,
    temp: "--",
    humi: "--",
    light: "--",
    soil: "--",
    mq2: "--",
    mode: "auto",
    modeText: "自动",
    pump: 0,
    fan: 0,
    fill: 0,
    servo: 0,
    servoDraft: 0,
    servoEditing: false,
    servoPending: null,
    controlMsg: "",
    thresholdMsg: "",
    thTempOpen: "30",
    thTempClose: "20",
    thLightOn: "3.0",
    thLightOff: "2.0",
    thSoilOn: "2.0",
    thSoilOff: "1.5",
    thMq2On: "2.0",
    thMq2Off: "1.5",
    historyCount: 0,
    historyPreview: [],
    sensors: []
  },

  timer: null,

  onLoad() {
    this.refresh();
    this.timer = setInterval(() => this.refresh(), REFRESH_MS);
  },

  onUnload() {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
  },

  onPullDownRefresh() {
    this.refresh().finally(() => wx.stopPullDownRefresh());
  },

  switchTab(e) {
    const tab = Number(e.currentTarget.dataset.tab);
    if (tab === 1) {
      this.setData({
        servoDraft: this.data.servo,
        servoEditing: false,
        servoPending: null
      });
    } else if (this.data.tab === 1) {
      this.setData({ servoEditing: false, servoPending: null });
    }
    this.setData({ tab });
    if (tab === 2) {
      this.loadHistory();
    }
  },

  refresh() {
    return api
      .fetchData()
      .then((data) => {
        this.applyData(data);
        if (this.data.tab === 0) {
          this.loadHistory();
        }
      })
      .catch(() => {
        this.setData({
          loading: false,
          online: false,
          controlMsg: "无法连接服务器，请检查网络或 config.js 中的地址"
        });
      });
  },

  applyData(data) {
    const alarm = util.parseAlarm(data.alarm || 0);
    const isAuto = data.mode === "auto" || data.mode === 0 || data.mode === "0";

    const tempRaw = data.temp;
    const humiRaw = data.humi;
    const lightRaw = data.light;
    const soilRaw = data.soil;
    const mq2Raw = data.mq2;

    const sensors = [
      {
        key: "temp",
        icon: "🌡️",
        label: "温度",
        theme: "theme-temp",
        num: tempRaw != null ? Number(tempRaw).toFixed(1) : "--",
        unit: "°C",
        pct: util.clampPct(tempRaw, 50),
        hint: "适宜 18~28°C"
      },
      {
        key: "humi",
        icon: "💧",
        label: "湿度",
        theme: "theme-humi",
        num: humiRaw != null ? String(Math.round(Number(humiRaw))) : "--",
        unit: "%",
        pct: util.clampPct(humiRaw, 100),
        hint: "适宜 40~70%"
      },
      {
        key: "light",
        icon: "☀️",
        label: "光照",
        theme: "theme-light",
        num: lightRaw != null ? Number(lightRaw).toFixed(1) : "--",
        unit: "V",
        pct: util.clampPct(lightRaw, 3.3),
        hint: "电压越高越亮"
      },
      {
        key: "soil",
        icon: "🌱",
        label: "土壤",
        theme: "theme-soil",
        num: soilRaw != null ? Number(soilRaw).toFixed(1) : "--",
        unit: "V",
        pct: util.clampPct(soilRaw, 3.3),
        hint: "湿度传感器电压"
      },
      {
        key: "mq2",
        icon: "💨",
        label: "气体 MQ2",
        theme: "theme-mq2",
        num: mq2Raw != null ? Number(mq2Raw).toFixed(1) : "--",
        unit: "V",
        pct: util.clampPct(mq2Raw, 3.3),
        hint: "浓度监测",
        wide: true
      }
    ];

    const serverServo =
      data.servo !== null && data.servo !== undefined ? Number(data.servo) : 0;
    const servoVal = Number.isNaN(serverServo) ? 0 : serverServo;

    const patch = {
      loading: false,
      online: !!data.online,
      updatedAt: util.formatTime(data.updated_at),
      alarmText: alarm.text,
      alarmWarn: alarm.warn,
      sensors,
      temp: util.formatValue(data.temp, "°C", 1),
      humi: util.formatValue(data.humi, "%", 0),
      light: util.formatValue(data.light, " V", 1),
      soil: util.formatValue(data.soil, " V", 1),
      mq2: util.formatValue(data.mq2, " V", 1),
      mode: isAuto ? "auto" : "manual",
      modeText: isAuto ? "自动" : "手动",
      pump: Number(data.pump) || 0,
      fan: Number(data.fan) || 0,
      fill: Number(data.fill) || 0,
      thTempOpen: String(data.temp_open != null ? data.temp_open : 30),
      thTempClose: String(data.temp_close != null ? data.temp_close : 20),
      thLightOn: util.x10ToVolt(data.light_on_x10) || "3.0",
      thLightOff: util.x10ToVolt(data.light_off_x10) || "2.0",
      thSoilOn: util.x10ToVolt(data.soil_on_x10) || "2.0",
      thSoilOff: util.x10ToVolt(data.soil_off_x10) || "1.5",
      thMq2On: util.x10ToVolt(data.mq2_on_x10) || "2.0",
      thMq2Off: util.x10ToVolt(data.mq2_off_x10) || "1.5"
    };

    if (this.data.servoEditing) {
      if (
        this.data.servoPending != null &&
        servoVal === this.data.servoPending
      ) {
        patch.servo = servoVal;
        patch.servoDraft = servoVal;
        patch.servoEditing = false;
        patch.servoPending = null;
      }
    } else {
      patch.servo = servoVal;
      patch.servoDraft = servoVal;
    }

    this.setData(patch);
  },

  loadHistory() {
    api
      .fetchHistory(6)
      .then((res) => {
        const records = (res.records || []).slice(-8).reverse();
        const preview = records.map((r, i) => ({
          time: (r.ts || "").slice(11, 19),
          temp: r.temp != null ? r.temp : "--",
          soil: r.soil != null ? r.soil : "--",
          last: i === 0
        }));
        this.setData({
          historyCount: res.total_records || 0,
          historyPreview: preview
        });
      })
      .catch(() => {});
  },

  setMode(e) {
    if (!this.data.online) {
      return;
    }
    const mode = e.currentTarget.dataset.mode;
    const modeVal = mode === "auto" ? 0 : 1;
    this.doControl({ mode: modeVal });
  },

  togglePump(e) {
    this.doControl({ mode: 1, pump: e.detail.value ? 1 : 0 });
  },

  toggleFan(e) {
    this.doControl({ mode: 1, fan: e.detail.value ? 1 : 0 });
  },

  toggleFill(e) {
    this.doControl({ mode: 1, fill: e.detail.value ? 1 : 0 });
  },

  onServoTouchStart() {
    this.setData({ servoEditing: true });
  },

  onServoChanging(e) {
    this.setData({
      servoDraft: Number(e.detail.value),
      servoEditing: true
    });
  },

  onServoChange(e) {
    this.setData({
      servoDraft: Number(e.detail.value),
      servoEditing: true
    });
  },

  onServoCommit() {
    const angle = Number(this.data.servoDraft);
    if (Number.isNaN(angle)) {
      return;
    }
    this.setData({ servoPending: angle, servoEditing: true });
    this.doControl({ mode: 1, servo: angle });
  },

  doControl(body) {
    this.setData({ controlMsg: "发送中..." });
    api
      .sendControl(body)
      .then((res) => {
        const patch = {
          controlMsg: res.message || (res.ok ? "已发送" : "失败")
        };
        if (res.ok && body.servo !== undefined && body.servo >= 0) {
          patch.servo = body.servo;
          patch.servoDraft = body.servo;
          patch.servoPending = body.servo;
        }
        this.setData(patch);
        setTimeout(() => this.refresh(), 600);
      })
      .catch(() => {
        this.setData({ controlMsg: "控制指令发送失败" });
      });
  },

  onThInput(e) {
    const field = e.currentTarget.dataset.field;
    const patch = {};
    patch[field] = e.detail.value;
    this.setData(patch);
  },

  submitThreshold() {
    const d = this.data;
    const body = {
      temp_open: Number(d.thTempOpen),
      temp_close: Number(d.thTempClose),
      light_on_x10: util.voltToX10(d.thLightOn),
      light_off_x10: util.voltToX10(d.thLightOff),
      soil_on_x10: util.voltToX10(d.thSoilOn),
      soil_off_x10: util.voltToX10(d.thSoilOff),
      mq2_on_x10: util.voltToX10(d.thMq2On),
      mq2_off_x10: util.voltToX10(d.thMq2Off)
    };

    this.setData({ thresholdMsg: "保存中..." });
    api
      .sendThreshold(body)
      .then((res) => {
        this.setData({ thresholdMsg: res.message || (res.ok ? "已保存" : "失败") });
        setTimeout(() => this.refresh(), 400);
      })
      .catch(() => {
        this.setData({ thresholdMsg: "阈值保存失败" });
      });
  }
});
