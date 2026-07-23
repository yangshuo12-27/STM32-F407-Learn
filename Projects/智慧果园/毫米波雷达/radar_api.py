"""
毫米波雷达 —— 后端 HTTP 接口客户端

拉取雷达实时数据（模拟真机硬件）。

依赖：
    pip install requests

用法：
    from radar_api import RadarAPI

    api = RadarAPI("http://47.113.206.147:7070")
    data = api.get_data()
    print(data)
"""

from __future__ import annotations

from typing import Any, Dict, Optional

import requests


class RadarAPI:
    """毫米波雷达云端接口。"""

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

    def get_data(self) -> Dict[str, Any]:
        """获取雷达实时数据（含全部字段）。GET /radar"""
        return self._get("/radar")

    def is_online(self) -> bool:
        """雷达是否在线。"""
        return bool(self.get_data().get("online"))

    def get_status(self) -> Dict[str, Any]:
        """返回全量状态字段（与 get_data 业务字段一致，去掉无关元数据以外的全保留）。"""
        d = self.get_data()
        keys = (
            "online",
            "updated_at",
            "device",
            "breath",
            "heart",
            "presence",
            "body_status",
            "move",
            "dist",
            "x",
            "y",
            "z",
            "breath_status",
            "bed",
            "sleep_quality",
            "sleep_score",
            "sleep_grade",
            "sleep_abn",
            "struggle",
            "noperson",
            "awake_min",
            "light_min",
            "deep_min",
            "sleep_duration",
            "sleep_total_min",
            "avg_breath",
            "avg_heart",
            "turns",
            "temp",
            "heart_wave",
            "breath_wave",
            "heart_wave_avg",
            "breath_wave_avg",
            "leave_bed_min",
            "leave_bed_count",
            "big_move_ratio",
            "small_move_ratio",
            "apnea_count",
            "awake_ratio",
            "light_ratio",
            "deep_ratio",
        )
        return {k: d.get(k) for k in keys}


def main() -> None:
    api = RadarAPI()
    print("=== 雷达实时数据 ===")
    data = api.get_data()
    print(f"在线: {data.get('online')}")
    print(f"更新: {data.get('updated_at')}")
    print(f"心率: {data.get('heart')}  呼吸: {data.get('breath')}")
    print(f"有人: {data.get('presence')}  距离: {data.get('dist')} cm")
    print(f"体温: {data.get('temp')}  评分: {data.get('sleep_score')}")
    print(f"波形心/呼均值: {data.get('heart_wave_avg')} / {data.get('breath_wave_avg')}")
    print(f"总睡长: {data.get('sleep_total_min')} min  离床次数: {data.get('leave_bed_count')}")
    print(f"完整 JSON: {data}")


if __name__ == "__main__":
    main()
