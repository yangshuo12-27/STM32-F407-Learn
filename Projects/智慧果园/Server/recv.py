#!/usr/bin/env python3
import socket
import sys
from datetime import datetime

HOST = "0.0.0.0"
PORT = 7777
IDLE_TIMEOUT = 60


def log(msg):
    print(f"[{datetime.now()}] {msg}", flush=True)


def handle_client(conn, addr):
    buf = b""
    log(f"connected: {addr}")
    conn.settimeout(IDLE_TIMEOUT)
    try:
        while True:
            try:
                data = conn.recv(4096)
            except socket.timeout:
                log(f"idle timeout, closing: {addr}")
                break
            if not data:
                break
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode("utf-8", errors="replace").strip()
                if text:
                    log(f"{addr} -> {text}")
    except OSError:
        pass
    finally:
        try:
            conn.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            conn.close()
        except OSError:
            pass
        log(f"disconnected: {addr}")


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(1)
    log(f"listening on {HOST}:{PORT}")

    while True:
        conn, addr = server.accept()
        handle_client(conn, addr)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
