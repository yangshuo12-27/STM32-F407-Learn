const { SERVER_URL } = require("../config");

function request(path, options) {
  const opts = options || {};
  const method = opts.method || "GET";
  const data = opts.data;

  return new Promise((resolve, reject) => {
    wx.request({
      url: SERVER_URL + path,
      method,
      data,
      header: {
        "Content-Type": "application/json"
      },
      success(res) {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(res.data);
          return;
        }
        reject(new Error("HTTP " + res.statusCode));
      },
      fail(err) {
        reject(err);
      }
    });
  });
}

function fetchData() {
  return request("/data");
}

function fetchHistory(hours) {
  return request("/history?hours=" + (hours || 24));
}

function sendControl(body) {
  return request("/control", {
    method: "POST",
    data: {
      mode: body.mode !== undefined ? body.mode : -1,
      pump: body.pump !== undefined ? body.pump : -1,
      fan: body.fan !== undefined ? body.fan : -1,
      fill: body.fill !== undefined ? body.fill : -1,
      servo: body.servo !== undefined ? body.servo : -1
    }
  });
}

function sendThreshold(body) {
  return request("/threshold", {
    method: "POST",
    data: body
  });
}

module.exports = {
  fetchData,
  fetchHistory,
  sendControl,
  sendThreshold
};
