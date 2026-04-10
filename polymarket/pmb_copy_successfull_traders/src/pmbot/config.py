"""Configuration loading for the copy-trading bot."""
from __future__ import annotations

import json
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List


DEFAULT_CONFIG_PATH = Path("config/bot_config.json")


class ConfigError(RuntimeError):
    """Raised when configuration is invalid or missing."""


@dataclass(frozen=True)
class RiskLimits:
    max_open_positions: int
    max_daily_positions: int
    max_daily_notional_usd: float


@dataclass(frozen=True)
class BotConfig:
    traders: List[str]
    copy_bet_usd: float
    poll_interval_seconds: int
    risk_limits: RiskLimits
    state_path: Path
    leaderboard_enabled: bool
    leaderboard_min_pnl: float
    leaderboard_min_vol: float
    leaderboard_max_traders: int
    leaderboard_refresh_seconds: int


def _require(config: Dict[str, Any], key: str) -> Any:
    if key not in config:
        raise ConfigError(f"Missing required config key: {key}")
    return config[key]


def load_config(path: Path | None = None) -> BotConfig:
    if path is None:
        env_path = os.environ.get("BOT_CONFIG_PATH")
        config_path = Path(env_path) if env_path else DEFAULT_CONFIG_PATH
    else:
        config_path = path
    if not config_path.exists():
        raise ConfigError(f"Config file not found: {config_path}")

    with config_path.open("r", encoding="utf-8") as handle:
        raw = json.load(handle)

    traders = list(_require(raw, "traders"))
    if not traders:
        raise ConfigError("Config 'traders' must include at least one profile id")

    risk_raw = _require(raw, "risk_limits")
    risk_limits = RiskLimits(
        max_open_positions=int(_require(risk_raw, "max_open_positions")),
        max_daily_positions=int(_require(risk_raw, "max_daily_positions")),
        max_daily_notional_usd=float(_require(risk_raw, "max_daily_notional_usd")),
    )

    return BotConfig(
        traders=traders,
        copy_bet_usd=float(_require(raw, "copy_bet_usd")),
        poll_interval_seconds=int(_require(raw, "poll_interval_seconds")),
        risk_limits=risk_limits,
        state_path=Path(raw.get("state_path", "data/state.json")),
        leaderboard_enabled=bool(raw.get("leaderboard", {}).get("enabled", False)),
        leaderboard_min_pnl=float(raw.get("leaderboard", {}).get("min_pnl", 0.0)),
        leaderboard_min_vol=float(raw.get("leaderboard", {}).get("min_vol", 0.0)),
        leaderboard_max_traders=int(raw.get("leaderboard", {}).get("max_traders", 25)),
        leaderboard_refresh_seconds=int(raw.get("leaderboard", {}).get("refresh_seconds", 3600)),
    )
