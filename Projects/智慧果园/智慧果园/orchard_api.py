"""
智慧果园 HTTP 接口客户端

依赖：pip install requests

用法示例：
    from orchard_api import OrchardClient

    client = OrchardClient("http://47.113.206.147:7070")
    data = client.get_data()
    print(data)
"""

from __future__ import annotations

from typing import Any, Dict, Optional

import requests


class OrchardClient:
    def __init__(self, base_url: str = "http://47.113.206.147:7070", timeout: float = 5.0):
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout

    def _get(self, path: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        response = requests.get(
            self.base_url + path,
            params=params,
            timeout=self.timeout,
        )
        response.raise_for_status()
        return response.json()

    def _post(self, path: str, body: Dict[str, Any]) -> Dict[str, Any]:
        response = requests.post(
            self.base_url + path,
            json=body,
            headers={"Content-Type": "application/json"},
            timeout=self.timeout,
        )
        response.raise_for_status()
        return response.json()

    def get_data(self) -> Dict[str, Any]:
        """获取实时传感器与设备状态。"""
        return self._get("/data")

    def get_history(self, hours: int = 24) -> Dict[str, Any]:
        """获取历史数据，hours 为查询小时数（1~720）。"""
        return self._get("/history", params={"hours": hours})

    def is_online(self) -> bool:
        """设备是否在线。"""
        return bool(self.get_data().get("online"))

    def control(
        self,
        mode: int = -1,
        pump: int = -1,
        fan: int = -1,
        fill: int = -1,
        servo: int = -1,
    ) -> Dict[str, Any]:
        """
        远程控制。参数为 -1 表示不修改该项。
        mode: 0=自动, 1=手动
        pump/fan/fill: 0=关, 1=开
        servo: 0~180
        """
        return self._post(
            "/control",
            {
                "mode": mode,
                "pump": pump,
                "fan": fan,
                "fill": fill,
                "servo": servo,
            },
        )

    def set_mode(self, manual: bool = True) -> Dict[str, Any]:
        """切换运行模式：manual=True 为手动，False 为自动。"""
        return self.control(mode=1 if manual else 0)

    def set_pump(self, on: bool) -> Dict[str, Any]:
        """开关水泵（自动切入手动模式）。"""
        return self.control(mode=1, pump=1 if on else 0)

    def set_fan(self, on: bool) -> Dict[str, Any]:
        """开关风扇（自动切入手动模式）。"""
        return self.control(mode=1, fan=1 if on else 0)

    def set_fill(self, on: bool) -> Dict[str, Any]:
        """开关补光灯（自动切入手动模式）。"""
        return self.control(mode=1, fill=1 if on else 0)

    def set_servo(self, angle: int) -> Dict[str, Any]:
        """设置舵机角度 0~180（自动切入手动模式）。"""
        angle = max(0, min(180, int(angle)))
        return self.control(mode=1, servo=angle)

    def set_threshold(
        self,
        temp_open: Optional[int] = None,
        temp_close: Optional[int] = None,
        light_on_x10: Optional[int] = None,
        light_off_x10: Optional[int] = None,
        soil_on_x10: Optional[int] = None,
        soil_off_x10: Optional[int] = None,
        mq2_on_x10: Optional[int] = None,
        mq2_off_x10: Optional[int] = None,
    ) -> Dict[str, Any]:
        """设置阈值。光照/土壤/烟雾等为电压×10（例如 3.0V → 30）。"""
        body: Dict[str, int] = {}
        for key, value in {
            "temp_open": temp_open,
            "temp_close": temp_close,
            "light_on_x10": light_on_x10,
            "light_off_x10": light_off_x10,
            "soil_on_x10": soil_on_x10,
            "soil_off_x10": soil_off_x10,
            "mq2_on_x10": mq2_on_x10,
            "mq2_off_x10": mq2_off_x10,
        }.items():
            if value is not None:
                body[key] = int(value)

        if not body:
            raise ValueError("至少提供一个阈值参数")
        return self._post("/threshold", body)


def main() -> None:
    client = OrchardClient()

    print("=== 实时数据 ===")
    data = client.get_data()
    print(f"在线: {data.get('online')}")
    print(f"更新时间: {data.get('updated_at')}")
    print(f"温度: {data.get('temp')} °C")
    print(f"湿度: {data.get('humi')} %")
    print(f"光照: {data.get('light')} V")
    print(f"土壤: {data.get('soil')} V")
    print(f"烟雾: {data.get('mq2')} V")
    print(f"模式: {data.get('mode')}")
    print(f"水泵: {data.get('pump')}  风扇: {data.get('fan')}  补光: {data.get('fill')}  舵机: {data.get('servo')}°")

    print("\n=== 近 1 小时历史 ===")
    history = client.get_history(hours=1)
    print(f"本时段: {history.get('count')} 条，库内共 {history.get('total_records')} 条")
    records = history.get("records") or []
    if records:
        last = records[-1]
        print(f"最新一条: {last}")


if __name__ == "__main__":
    main()
