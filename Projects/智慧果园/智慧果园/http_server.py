import json
import socket
import sqlite3
import threading
from datetime import datetime, timedelta
from pathlib import Path

from flask import Flask, jsonify, render_template, request

app = Flask(__name__)


@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    return response


@app.route("/control", methods=["OPTIONS"])
@app.route("/threshold", methods=["OPTIONS"])
def cors_preflight():
    return "", 204


TCP_HOST = "0.0.0.0"
TCP_PORT = 7777
HTTP_PORT = 7070
DB_PATH = Path(__file__).with_name("orchard_history.db")
HISTORY_RETAIN_DAYS = 30
HISTORY_SENSOR_KEYS = ("temp", "humi", "light", "soil", "mq2")

DEFAULT_THRESHOLDS = {
    "temp_open": 30,
    "temp_close": 20,
    "light_on_x10": 30,
    "light_off_x10": 20,
    "soil_on_x10": 20,
    "soil_off_x10": 15,
    "mq2_on_x10": 20,
    "mq2_off_x10": 15,
}

data_store = {
    "temp": None,
    "humi": None,
    "light": None,
    "soil": None,
    "mq2": None,
    "mode": "auto",
    "pump": 0,
    "fan": 0,
    "fill": 0,
    "servo": 0,
    "alarm": 0,
    "online": False,
    "updated_at": None,
    **DEFAULT_THRESHOLDS,
}

radar_store = {
    "device": "radar",
    "online": False,
    "updated_at": None,
    "breath": None,
    "heart": None,
    "presence": None,
    "body_status": None,
    "move": None,
    "awake_min": None,
    "light_min": None,
    "deep_min": None,
    "sleep_score": None,
    "sleep_grade": None,
    "sleep_quality": None,
    "bed": None,
    "dist": None,
    "x": None,
    "y": None,
    "z": None,
    "breath_status": None,
    "sleep_abn": None,
    "struggle": None,
    "noperson": None,
    "avg_breath": None,
    "avg_heart": None,
    "turns": None,
    "temp": None,
    "heart_wave": None,
    "breath_wave": None,
    "heart_wave_avg": None,
    "breath_wave_avg": None,
    "sleep_duration": None,
    "sleep_total_min": None,
    "leave_bed_min": None,
    "leave_bed_count": None,
    "big_move_ratio": None,
    "small_move_ratio": None,
    "apnea_count": None,
    "awake_ratio": None,
    "light_ratio": None,
    "deep_ratio": None,
}

data_lock = threading.Lock()
db_lock = threading.Lock()
# orchard / radar can both connect on TCP 7777
orchard_client = {"sock": None, "addr": None}
radar_client = {"sock": None, "addr": None}
client_meta = {}  # sock -> "orchard" | "radar" | "unknown"
active_lock = threading.Lock()

THRESHOLD_KEYS = list(DEFAULT_THRESHOLDS.keys())
RADAR_KEYS = [
    "breath",
    "heart",
    "presence",
    "body_status",
    "move",
    "awake_min",
    "light_min",
    "deep_min",
    "sleep_score",
    "sleep_grade",
    "sleep_quality",
    "bed",
    "dist",
    "x",
    "y",
    "z",
    "breath_status",
    "sleep_abn",
    "struggle",
    "noperson",
    "avg_breath",
    "avg_heart",
    "turns",
    "temp",
    "heart_wave",
    "breath_wave",
    "heart_wave_avg",
    "breath_wave_avg",
    "sleep_duration",
    "sleep_total_min",
    "leave_bed_min",
    "leave_bed_count",
    "big_move_ratio",
    "small_move_ratio",
    "apnea_count",
    "awake_ratio",
    "light_ratio",
    "deep_ratio",
]


def init_db():
    with db_lock:
        conn = sqlite3.connect(DB_PATH, check_same_thread=False)
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS sensor_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ts TEXT NOT NULL,
                temp REAL,
                humi REAL,
                light REAL,
                soil REAL,
                mq2 REAL,
                mode TEXT,
                pump INTEGER,
                fan INTEGER,
                fill INTEGER,
                servo INTEGER,
                alarm INTEGER
            )
            """
        )
        conn.execute(
            "CREATE INDEX IF NOT EXISTS idx_sensor_history_ts ON sensor_history(ts)"
        )
        conn.commit()
        conn.close()
    print(f"[DB] ready: {DB_PATH}")


def _parse_float(value):
    if value is None:
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def save_sensor_history(payload):
    if not any(key in payload for key in HISTORY_SENSOR_KEYS):
        return

    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cutoff = (datetime.now() - timedelta(days=HISTORY_RETAIN_DAYS)).strftime(
        "%Y-%m-%d %H:%M:%S"
    )

    with db_lock:
        conn = sqlite3.connect(DB_PATH, check_same_thread=False)
        conn.execute(
            """
            INSERT INTO sensor_history (
                ts, temp, humi, light, soil, mq2,
                mode, pump, fan, fill, servo, alarm
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                ts,
                _parse_float(payload.get("temp")),
                _parse_float(payload.get("humi")),
                _parse_float(payload.get("light")),
                _parse_float(payload.get("soil")),
                _parse_float(payload.get("mq2")),
                payload.get("mode"),
                payload.get("pump"),
                payload.get("fan"),
                payload.get("fill"),
                payload.get("servo"),
                payload.get("alarm"),
            ),
        )
        conn.execute("DELETE FROM sensor_history WHERE ts < ?", (cutoff,))
        conn.commit()
        conn.close()


def query_history(hours):
    since = (datetime.now() - timedelta(hours=hours)).strftime("%Y-%m-%d %H:%M:%S")

    with db_lock:
        conn = sqlite3.connect(DB_PATH, check_same_thread=False)
        conn.row_factory = sqlite3.Row
        rows = conn.execute(
            """
            SELECT ts, temp, humi, light, soil, mq2
            FROM sensor_history
            WHERE ts >= ?
            ORDER BY ts ASC
            """,
            (since,),
        ).fetchall()
        count = conn.execute("SELECT COUNT(*) AS c FROM sensor_history").fetchone()[0]
        conn.close()

    return [dict(row) for row in rows], count


def update_from_payload(payload):
    global data_store

    with data_lock:
        for key in (
            "temp",
            "humi",
            "light",
            "soil",
            "mq2",
            "mode",
            "pump",
            "fan",
            "fill",
            "servo",
            "alarm",
        ):
            if key in payload:
                data_store[key] = payload[key]

        for key in THRESHOLD_KEYS:
            if key in payload:
                data_store[key] = int(payload[key])

        data_store["online"] = True
        data_store["updated_at"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    save_sensor_history(payload)


def update_radar_payload(payload):
    global radar_store

    with data_lock:
        for key in RADAR_KEYS:
            if key in payload:
                radar_store[key] = payload[key]
        radar_store["online"] = True
        radar_store["updated_at"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def classify_payload(payload):
    device = str(payload.get("device", "")).lower()
    if device == "radar":
        return "radar"
    if device in ("orchard", "smart-orchard"):
        return "orchard"
    if "breath" in payload or "heart" in payload or "sleep_score" in payload:
        return "radar"
    if any(k in payload for k in ("temp", "humi", "light", "soil", "mq2", "pump", "servo")):
        return "orchard"
    return "unknown"


def bind_client(kind, sock, addr):
    """
    记录当前设备连接。同一类型允许多路上报（模拟器 + 真机可并存），
    不再强制 close 旧连接，避免互相踢线导致 WinError 10053。
    控制指令仍发给最近一次绑定的 orchard 连接。
    """
    with active_lock:
        client_meta[sock] = kind
        if kind == "orchard":
            orchard_client["sock"] = sock
            orchard_client["addr"] = addr
        elif kind == "radar":
            radar_client["sock"] = sock
            radar_client["addr"] = addr


def send_to_device(command):
    with active_lock:
        sock = orchard_client["sock"]
        if sock is None:
            return False, "果园设备未连接"
        try:
            line = json.dumps(command, ensure_ascii=False) + "\n"
            sock.sendall(line.encode("utf-8"))
            print(f"[TCP] -> orchard: {line.strip()}")
            return True, "指令已发送"
        except OSError as exc:
            return False, str(exc)


def handle_client(client_socket, addr):
    buf = ""
    kind = "unknown"
    print(f"[TCP] connected: {addr}")

    with active_lock:
        client_meta[client_socket] = "unknown"

    try:
        while True:
            chunk = client_socket.recv(4096)
            if not chunk:
                break
            buf += chunk.decode("utf-8", errors="replace")
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                text = line.strip().strip("\r")
                if not text:
                    continue
                print(f"[TCP] {addr} -> {text}")
                try:
                    payload = json.loads(text)
                except json.JSONDecodeError:
                    print(f"[TCP] invalid json: {text}")
                    continue

                if payload.get("cmd") is not None:
                    continue

                detected = classify_payload(payload)
                if detected != "unknown":
                    kind = detected
                    bind_client(kind, client_socket, addr)

                if kind == "radar":
                    update_radar_payload(payload)
                else:
                    # default to orchard for backward compatibility
                    if kind == "unknown":
                        kind = "orchard"
                        bind_client(kind, client_socket, addr)
                    update_from_payload(payload)
    except OSError as exc:
        print(f"[TCP] client error: {exc}")
    finally:
        with active_lock:
            client_meta.pop(client_socket, None)
            if orchard_client["sock"] is client_socket:
                # 若还有其它果园连接，切到其上
                other = next(
                    (s for s, k in client_meta.items() if k == "orchard"),
                    None,
                )
                orchard_client["sock"] = other
                orchard_client["addr"] = other.getpeername() if other else None
                if other is None:
                    with data_lock:
                        data_store["online"] = False
            if radar_client["sock"] is client_socket:
                other = next(
                    (s for s, k in client_meta.items() if k == "radar"),
                    None,
                )
                radar_client["sock"] = other
                try:
                    radar_client["addr"] = other.getpeername() if other else None
                except OSError:
                    radar_client["addr"] = None
                if other is None:
                    with data_lock:
                        radar_store["online"] = False
        try:
            client_socket.close()
        except OSError:
            pass
        print(f"[TCP] disconnected: {addr} kind={kind}")


def start_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((TCP_HOST, TCP_PORT))
    server.listen(5)
    print(f"[TCP] listening on {TCP_HOST}:{TCP_PORT} (orchard + radar)")

    while True:
        client_socket, addr = server.accept()
        thread = threading.Thread(
            target=handle_client, args=(client_socket, addr), daemon=True
        )
        thread.start()


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/data", methods=["GET"])
def get_data():
    with data_lock:
        return jsonify(data_store)


@app.route("/radar", methods=["GET"])
@app.route("/radar/data", methods=["GET"])
def get_radar_data():
    with data_lock:
        return jsonify(radar_store)


@app.route("/devices", methods=["GET"])
def get_devices():
    with active_lock:
        orchard_online = orchard_client["sock"] is not None
        radar_online = radar_client["sock"] is not None
        orchard_addr = str(orchard_client["addr"]) if orchard_client["addr"] else None
        radar_addr = str(radar_client["addr"]) if radar_client["addr"] else None
    with data_lock:
        return jsonify(
            {
                "orchard": {
                    "online": data_store.get("online") and orchard_online,
                    "addr": orchard_addr,
                    "updated_at": data_store.get("updated_at"),
                },
                "radar": {
                    "online": radar_store.get("online") and radar_online,
                    "addr": radar_addr,
                    "updated_at": radar_store.get("updated_at"),
                },
            }
        )


@app.route("/history", methods=["GET"])
def get_history():
    hours = request.args.get("hours", default=24, type=int)
    if hours is None or hours < 1:
        hours = 1
    if hours > HISTORY_RETAIN_DAYS * 24:
        hours = HISTORY_RETAIN_DAYS * 24

    records, total = query_history(hours)
    return jsonify(
        {
            "hours": hours,
            "total_records": total,
            "count": len(records),
            "records": records,
        }
    )


@app.route("/control", methods=["POST"])
def control():
    body = request.get_json(silent=True) or {}
    command = {
        "cmd": "control",
        "mode": int(body.get("mode", -1)),
        "pump": int(body.get("pump", -1)),
        "fan": int(body.get("fan", -1)),
        "fill": int(body.get("fill", -1)),
        "servo": int(body.get("servo", -1)),
    }
    ok, message = send_to_device(command)
    if ok:
        with data_lock:
            if command["mode"] >= 0:
                data_store["mode"] = "manual" if command["mode"] == 1 else "auto"
            if command["pump"] >= 0:
                data_store["pump"] = command["pump"]
            if command["fan"] >= 0:
                data_store["fan"] = command["fan"]
            if command["fill"] >= 0:
                data_store["fill"] = command["fill"]
            if command["servo"] >= 0:
                data_store["servo"] = command["servo"]
    return jsonify({"ok": ok, "message": message})


@app.route("/threshold", methods=["POST"])
def threshold():
    body = request.get_json(silent=True) or {}
    command = {"cmd": "threshold"}
    for key in THRESHOLD_KEYS:
        if key in body:
            command[key] = int(body[key])

    if len(command) == 1:
        return jsonify({"ok": False, "message": "未提供阈值参数"})

    ok, message = send_to_device(command)
    if ok:
        with data_lock:
            for key in THRESHOLD_KEYS:
                if key in command:
                    data_store[key] = command[key]
    return jsonify({"ok": ok, "message": message})


if __name__ == "__main__":
    init_db()
    tcp_thread = threading.Thread(target=start_tcp_server, daemon=True)
    tcp_thread.start()
    print(f"[HTTP] dashboard: http://0.0.0.0:{HTTP_PORT}")
    print(f"[DB] history retain {HISTORY_RETAIN_DAYS} days")
    app.run(debug=False, host="0.0.0.0", port=HTTP_PORT)
