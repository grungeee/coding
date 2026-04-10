"""Configuration helpers for the trading bot."""

from __future__ import annotations

import os
from dataclasses import dataclass


@dataclass(frozen=True)
class Config:
    poll_interval_sec: int
    window_minutes: int
    min_points: int
    slope_threshold: float
    feed_mode: str
    chainlink_price_url: str
    chainlink_price_json_path: str
    chainlink_feed_address: str
    eth_rpc_url: str
    chainlink_feed_decimals: int
    chainlink_auth_mode: str
    chainlink_api_key: str
    chainlink_api_secret: str
    chainlink_api_timestamp_header: str
    chainlink_api_key_header: str
    chainlink_api_signature_header: str
    trading_enabled: bool
    trader_mode: str
    market_id: str
    outcome_up_id: str
    outcome_down_id: str
    sqlite_path: str
    clob_host: str
    clob_api_key: str
    clob_api_secret: str
    clob_api_passphrase: str
    clob_funder: str
    clob_private_key: str
    builder_api_key: str
    builder_api_secret: str
    builder_api_passphrase: str
    clob_auto_create_key: bool
    market_slug: str
    start_delta_threshold: float
    enter_confidence: float
    hold_confidence: float
    trailing_stop_mult: float
    max_hold_minutes: int
    cooldown_minutes: int
    stake_usd: float
    min_shares: int
    max_spread_pct: float
    min_edge: float
    confidence_scale: float
    manual_refresh_cooldown_sec: float


def _get_bool(value: str | None, default: bool) -> bool:
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on"}


def load_config() -> Config:
    return Config(
        poll_interval_sec=int(os.getenv("POLL_INTERVAL_SEC", "10")),
        window_minutes=int(os.getenv("WINDOW_MINUTES", "15")),
        min_points=int(os.getenv("MIN_POINTS", "10")),
        slope_threshold=float(os.getenv("SLOPE_THRESHOLD", "0.0")),
        feed_mode=os.getenv("FEED_MODE", "http"),
        chainlink_price_url=os.getenv("CHAINLINK_PRICE_URL", ""),
        chainlink_price_json_path=os.getenv("CHAINLINK_PRICE_JSON_PATH", "price"),
        chainlink_feed_address=os.getenv("CHAINLINK_FEED_ADDRESS", ""),
        eth_rpc_url=os.getenv("ETH_RPC_URL", ""),
        chainlink_feed_decimals=int(os.getenv("CHAINLINK_FEED_DECIMALS", "-1")),
        chainlink_auth_mode=os.getenv("CHAINLINK_AUTH_MODE", "none"),
        chainlink_api_key=os.getenv("CHAINLINK_API_KEY", ""),
        chainlink_api_secret=os.getenv("CHAINLINK_API_SECRET", ""),
        chainlink_api_timestamp_header=os.getenv("CHAINLINK_API_TIMESTAMP_HEADER", "X-API-TIMESTAMP"),
        chainlink_api_key_header=os.getenv("CHAINLINK_API_KEY_HEADER", "X-API-KEY"),
        chainlink_api_signature_header=os.getenv("CHAINLINK_API_SIGNATURE_HEADER", "X-API-SIGNATURE"),
        trading_enabled=_get_bool(os.getenv("TRADING_ENABLED"), False),
        trader_mode=os.getenv("TRADER_MODE", "paper"),
        market_id=os.getenv("MARKET_ID", ""),
        outcome_up_id=os.getenv("OUTCOME_UP_ID", ""),
        outcome_down_id=os.getenv("OUTCOME_DOWN_ID", ""),
        sqlite_path=os.getenv("SQLITE_PATH", "bot_state.sqlite3"),
        clob_host=os.getenv("CLOB_HOST", "https://clob.polymarket.com"),
        clob_api_key=os.getenv("CLOB_API_KEY", ""),
        clob_api_secret=os.getenv("CLOB_API_SECRET", ""),
        clob_api_passphrase=os.getenv("CLOB_API_PASSPHRASE", ""),
        clob_funder=os.getenv("CLOB_FUNDER", ""),
        clob_private_key=os.getenv("CLOB_PRIVATE_KEY", ""),
        builder_api_key=os.getenv("BUILDER_API_KEY", ""),
        builder_api_secret=os.getenv("BUILDER_API_SECRET", ""),
        builder_api_passphrase=os.getenv("BUILDER_API_PASSPHRASE", ""),
        clob_auto_create_key=_get_bool(os.getenv("CLOB_AUTO_CREATE_KEY"), False),
        market_slug=os.getenv("MARKET_SLUG", ""),
        start_delta_threshold=float(os.getenv("START_DELTA_THRESHOLD", "0.0")),
        enter_confidence=float(os.getenv("ENTER_CONFIDENCE", "1.2")),
        hold_confidence=float(os.getenv("HOLD_CONFIDENCE", "0.6")),
        trailing_stop_mult=float(os.getenv("TRAILING_STOP_MULT", "1.5")),
        max_hold_minutes=int(os.getenv("MAX_HOLD_MINUTES", "20")),
        cooldown_minutes=int(os.getenv("COOLDOWN_MINUTES", "2")),
        stake_usd=float(os.getenv("STAKE_USD", "1.0")),
        min_shares=int(os.getenv("MIN_SHARES", "1")),
        max_spread_pct=float(os.getenv("MAX_SPREAD_PCT", "0.08")),
        min_edge=float(os.getenv("MIN_EDGE", "0.03")),
        confidence_scale=float(os.getenv("CONFIDENCE_SCALE", "2.0")),
        manual_refresh_cooldown_sec=float(os.getenv("MANUAL_REFRESH_COOLDOWN_SEC", "0.8")),
    )
