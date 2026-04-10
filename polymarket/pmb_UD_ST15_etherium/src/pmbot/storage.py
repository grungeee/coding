"""SQLite-backed storage for price history."""

from __future__ import annotations

import sqlite3
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Iterable


@dataclass
class PricePoint:
    timestamp: datetime
    price: float


class PriceStore:
    def __init__(self, path: str) -> None:
        self._path = path
        self._init_db()

    def _init_db(self) -> None:
        with sqlite3.connect(self._path) as conn:
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS prices (
                    timestamp TEXT NOT NULL,
                    price REAL NOT NULL
                )
                """
            )
            conn.execute(
                "CREATE INDEX IF NOT EXISTS idx_prices_ts ON prices(timestamp)"
            )
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS baselines (
                    window_start TEXT PRIMARY KEY,
                    baseline REAL NOT NULL,
                    created_at TEXT NOT NULL
                )
                """
            )

    def add_price(self, timestamp: datetime, price: float) -> None:
        iso_ts = timestamp.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            conn.execute(
                "INSERT INTO prices (timestamp, price) VALUES (?, ?)",
                (iso_ts, price),
            )

    def get_prices_since(self, since: datetime) -> list[PricePoint]:
        since_iso = since.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            rows = conn.execute(
                "SELECT timestamp, price FROM prices WHERE timestamp >= ? ORDER BY timestamp",
                (since_iso,),
            ).fetchall()
        points: list[PricePoint] = []
        for timestamp_str, price in rows:
            points.append(
                PricePoint(
                    timestamp=datetime.fromisoformat(timestamp_str).astimezone(timezone.utc),
                    price=float(price),
                )
            )
        return points

    def get_prices_between(self, start: datetime, end: datetime) -> list[PricePoint]:
        start_iso = start.astimezone(timezone.utc).isoformat()
        end_iso = end.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            rows = conn.execute(
                """
                SELECT timestamp, price FROM prices
                WHERE timestamp >= ? AND timestamp <= ?
                ORDER BY timestamp
                """,
                (start_iso, end_iso),
            ).fetchall()
        points: list[PricePoint] = []
        for timestamp_str, price in rows:
            points.append(
                PricePoint(
                    timestamp=datetime.fromisoformat(timestamp_str).astimezone(timezone.utc),
                    price=float(price),
                )
            )
        return points

    def get_first_price_after(self, after: datetime) -> PricePoint | None:
        after_iso = after.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            row = conn.execute(
                "SELECT timestamp, price FROM prices WHERE timestamp >= ? ORDER BY timestamp LIMIT 1",
                (after_iso,),
            ).fetchone()
        if not row:
            return None
        timestamp_str, price = row
        return PricePoint(
            timestamp=datetime.fromisoformat(timestamp_str).astimezone(timezone.utc),
            price=float(price),
        )

    def prune_before(self, before: datetime) -> None:
        before_iso = before.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            conn.execute("DELETE FROM prices WHERE timestamp < ?", (before_iso,))

    def get_window_baseline(self, window_start: datetime) -> float | None:
        start_iso = window_start.astimezone(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            row = conn.execute(
                "SELECT baseline FROM baselines WHERE window_start = ?",
                (start_iso,),
            ).fetchone()
        if not row:
            return None
        return float(row[0])

    def set_window_baseline(self, window_start: datetime, baseline: float) -> None:
        start_iso = window_start.astimezone(timezone.utc).isoformat()
        created_at = datetime.now(timezone.utc).isoformat()
        with sqlite3.connect(self._path) as conn:
            conn.execute(
                """
                INSERT OR IGNORE INTO baselines (window_start, baseline, created_at)
                VALUES (?, ?, ?)
                """,
                (start_iso, float(baseline), created_at),
            )
