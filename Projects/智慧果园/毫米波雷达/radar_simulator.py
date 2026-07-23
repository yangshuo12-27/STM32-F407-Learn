"""
毫米波雷达设备模拟器

不接 STM32 / R60ABD1，也能向云端 TCP 7777 上报雷达 JSON，
方便后端同学随时联调。

用法：
    cd 毫米波雷达
    python radar_simulator.py

    # 指定服务器
    python radar_simulator.py --host 47.113.206.147 --port 7777

    # 上报间隔（秒）
    python radar_simulator.py --interval 2

依赖：仅标准库
停止：Ctrl+C
"""

from __future__ import annotations

import argparse
import json
import math
import random
import socket
import time
from datetime import datetime
from typing import Dict, Optional


DEFAULT_HOST = "47.113.206.147"
DEFAULT_PORT = 7777


def now_str() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def make_radar_payload(t: float) -> Dict:
    presence = 1 if math.sin(t / 20.0) > -0.7 else 0
    heart = int(70 + 8 * math.sin(t / 8.0) + random.uniform(-2, 2)) if presence else 0
    breath = int(14 + 3 * math.sin(t / 10.0) + random.uniform(-1, 1)) if presence else 0
    base = 128
    heart_wave = [max(0, min(255, int(base + 40 * math.sin(t / 2.0 + i) + random.uniform(-5, 5)))) for i in range(5)]
    breath_wave = [max(0, min(255, int(base + 25 * math.sin(t / 4.0 + i * 0.5) + random.uniform(-5, 5)))) for i in range(5)]
    return {
        "device": "radar",
        "breath": max(0, breath),
        "heart": max(0, heart),
        "presence": presence,
        "body_status": 1 if presence and random.random() > 0.7 else 0,
        "move": random.randint(0, 40) if presence else 0,
        "awake_min": 20,
        "light_min": 120,
        "deep_min": 90,
        "sleep_score": 78,
        "sleep_grade": 1,
        "sleep_quality": 1 if presence else 3,
        "bed": 1 if presence else 0,
        "dist": random.randint(40, 120) if presence else 0,
        "x": random.randint(-30, 30),
        "y": random.randint(50, 150),
        "z": random.randint(-20, 20),
        "breath_status": 1 if presence else 4,
        "sleep_abn": 3,
        "struggle": 0,
        "noperson": 1,
        "avg_breath": max(0, breath),
        "avg_heart": max(0, heart),
        "turns": random.randint(0, 5),
        "temp": round(36.3 + random.uniform(-0.2, 0.4), 1),
        "heart_wave": heart_wave,
        "breath_wave": breath_wave,
        "heart_wave_avg": int(sum(heart_wave) / 5),
        "breath_wave_avg": int(sum(breath_wave) / 5),
        "sleep_duration": 230,
        "sleep_total_min": 420,
        "leave_bed_min": 15,
        "leave_bed_count": random.randint(0, 3),
        "big_move_ratio": random.randint(5, 25),
        "small_move_ratio": random.randint(10, 40),
        "apnea_count": random.randint(0, 2),
        "awake_ratio": 10,
        "light_ratio": 55,
        "deep_ratio": 35,
    }


def _enable_keepalive(sock: socket.socket) -> None:
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    # Windows: TCP_KEEPIDLE=3, TCP_KEEPINTVL=17, TCP_KEEPCNT=16 (ms on some builds; best-effort)
    if hasattr(socket, "TCP_KEEPIDLE"):
        try:
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 30)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 10)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)
        except OSError:
            pass


def _safe_close(sock: Optional[socket.socket]) -> None:
    if sock is None:
        return
    try:
        sock.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass
    try:
        sock.close()
    except OSError:
        pass


def run_simulator(host: str, port: int, interval: float) -> None:
    t0 = time.time()
    print(f"雷达模拟器 -> {host}:{port}，间隔 {interval}s，Ctrl+C 停止")
    print("提示：若频繁断线，请联系QQ:2732775288")
    print("      及时沟通。")

    while True:
        sock: Optional[socket.socket] = None
        try:
            sock = socket.create_connection((host, port), timeout=10)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            _enable_keepalive(sock)
            # 发送超时：对端踢线时尽快发现，而不是卡在 send
            sock.settimeout(15)
            print(f"[{now_str()}] connected")

            while True:
                payload = make_radar_payload(time.time() - t0)
                line = json.dumps(payload, ensure_ascii=False) + "\n"
                sock.sendall(line.encode("utf-8"))
                print(f"[{now_str()}] -> {line.strip()}")
                time.sleep(interval)

        except KeyboardInterrupt:
            print("\n已停止")
            _safe_close(sock)
            return
        except (OSError, TimeoutError) as exc:
            print(f"[{now_str()}] 连接中断: {exc}，2 秒后重连")
            _safe_close(sock)
            time.sleep(2)


def main() -> None:
    parser = argparse.ArgumentParser(description="毫米波雷达 TCP 模拟器")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--interval", type=float, default=2.0)
    args = parser.parse_args()
    run_simulator(args.host, args.port, args.interval)


if __name__ == "__main__":
    main()
