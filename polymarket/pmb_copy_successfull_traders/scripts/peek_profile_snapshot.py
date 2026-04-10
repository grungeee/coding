"""Fetch profile snapshot data from the Polymarket profile page."""
from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from typing import Any, Dict, List

import requests


@dataclass(frozen=True)
class ProfileSnapshot:
    username: str
    proxy_address: str | None
    base_address: str | None
    primary_address: str | None
    stats: Dict[str, Any]
    volume: Dict[str, Any]


def load_profile_html(username: str) -> str:
    url = f"https://polymarket.com/@{username}"
    resp = requests.get(url, timeout=10)
    resp.raise_for_status()
    return resp.text


def extract_next_data(html: str) -> Dict[str, Any]:
    match = re.search(r"<script[^>]*id=\"__NEXT_DATA__\"[^>]*>(.*?)</script>", html, re.S)
    if not match:
        raise RuntimeError("__NEXT_DATA__ not found in profile HTML")
    return json.loads(match.group(1))


def find_query_data(queries: List[Dict[str, Any]], key_prefix: str) -> Dict[str, Any]:
    for item in queries:
        key = item.get("queryKey")
        if isinstance(key, list) and key and key[0] == key_prefix:
            return item.get("state", {}).get("data") or {}
    return {}


def fetch_snapshot(username: str) -> ProfileSnapshot:
    html = load_profile_html(username)
    data = extract_next_data(html)
    page_props = data.get("props", {}).get("pageProps", {})
    queries = page_props.get("dehydratedState", {}).get("queries", [])

    stats = find_query_data(queries, "/api/profile/stats")
    volume = find_query_data(queries, "/api/profile/volume")

    return ProfileSnapshot(
        username=username,
        proxy_address=page_props.get("proxyAddress"),
        base_address=page_props.get("baseAddress"),
        primary_address=page_props.get("primaryAddress"),
        stats=stats,
        volume=volume,
    )


def main() -> int:
    username = sys.argv[1].lstrip("@").strip() if len(sys.argv) > 1 else "gopfan2"
    snapshot = fetch_snapshot(username)
    print(f"Profile: @{snapshot.username}")
    print(f"  proxy_address: {snapshot.proxy_address}")
    print(f"  base_address: {snapshot.base_address}")
    print(f"  primary_address: {snapshot.primary_address}")
    if snapshot.stats:
        print(f"  stats: {snapshot.stats}")
    if snapshot.volume:
        print(f"  volume: {snapshot.volume}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
