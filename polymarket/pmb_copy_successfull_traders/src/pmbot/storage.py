"""Simple JSON state storage for the bot."""
from __future__ import annotations

import json
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path
from typing import Dict


@dataclass
class BotState:
    last_seen_activity: Dict[str, str] = field(default_factory=dict)
    daily_position_count: int = 0
    daily_notional_usd: float = 0.0
    open_positions_count: int = 0
    last_leaderboard_refresh: float = 0.0
    leaderboard_traders: list[str] = field(default_factory=list)
    state_date: str = field(default_factory=lambda: date.today().isoformat())

    def reset_daily_if_needed(self) -> None:
        today = date.today().isoformat()
        if self.state_date != today:
            self.state_date = today
            self.daily_position_count = 0
            self.daily_notional_usd = 0.0


class StateStore:
    def __init__(self, path: Path) -> None:
        self.path = path

    def load(self) -> BotState:
        if not self.path.exists():
            return BotState()
        with self.path.open("r", encoding="utf-8") as handle:
            raw = json.load(handle)
        state = BotState(
            last_seen_activity=raw.get("last_seen_activity", {}),
            daily_position_count=int(raw.get("daily_position_count", 0)),
            daily_notional_usd=float(raw.get("daily_notional_usd", 0.0)),
            open_positions_count=int(raw.get("open_positions_count", 0)),
            last_leaderboard_refresh=float(raw.get("last_leaderboard_refresh", 0.0)),
            leaderboard_traders=list(raw.get("leaderboard_traders", [])),
            state_date=raw.get("state_date", date.today().isoformat()),
        )
        state.reset_daily_if_needed()
        return state

    def save(self, state: BotState) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("w", encoding="utf-8") as handle:
            json.dump(
                {
                    "last_seen_activity": state.last_seen_activity,
                    "daily_position_count": state.daily_position_count,
                    "daily_notional_usd": state.daily_notional_usd,
                    "open_positions_count": state.open_positions_count,
                    "last_leaderboard_refresh": state.last_leaderboard_refresh,
                    "leaderboard_traders": state.leaderboard_traders,
                    "state_date": state.state_date,
                },
                handle,
                indent=2,
            )
