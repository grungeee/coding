"""Fetch top traders from Polymarket leaderboard."""
from __future__ import annotations

import logging
from dataclasses import dataclass
from typing import List

import requests

LOGGER = logging.getLogger(__name__)

LEADERBOARD_URL = "https://data-api.polymarket.com/v1/leaderboard"


@dataclass(frozen=True)
class LeaderboardEntry:
    username: str
    proxy_wallet: str
    pnl: float
    vol: float


def fetch_leaderboard() -> List[LeaderboardEntry]:
    resp = requests.get(LEADERBOARD_URL, timeout=20)
    resp.raise_for_status()
    data = resp.json()
    entries: List[LeaderboardEntry] = []
    for item in data:
        try:
            entries.append(
                LeaderboardEntry(
                    username=str(item.get("userName") or ""),
                    proxy_wallet=str(item.get("proxyWallet") or ""),
                    pnl=float(item.get("pnl") or 0.0),
                    vol=float(item.get("vol") or 0.0),
                )
            )
        except (TypeError, ValueError):
            LOGGER.warning("Skipping malformed leaderboard item: %s", item)
    return entries


def select_traders(
    min_pnl: float,
    min_vol: float,
    max_traders: int,
) -> List[str]:
    entries = fetch_leaderboard()
    filtered = [
        entry
        for entry in entries
        if entry.proxy_wallet and entry.pnl >= min_pnl and entry.vol >= min_vol
    ]
    filtered.sort(key=lambda entry: entry.pnl, reverse=True)
    return [entry.proxy_wallet for entry in filtered[:max_traders]]
