"""HTTP client interface to Polymarket CLOB APIs.

This module intentionally keeps endpoints abstract until configured. It provides
clear extension points where the real Polymarket endpoints can be wired in.
"""
from __future__ import annotations

import json
import os
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional

from py_clob_client.client import ClobClient
from py_clob_client.clob_types import ApiCreds, MarketOrderArgs, OrderArgs, TradeParams

from pmbot.models import CopyOrder, TraderActivity


class ClientError(RuntimeError):
    """Raised when the client is misconfigured or a request fails."""


@dataclass
class ClientConfig:
    base_url: str
    api_key: str | None
    api_secret: str | None
    api_passphrase: str | None
    private_key: str | None
    funder: str | None
    signature_type: int


class PolymarketClient:
    def fetch_recent_activity(self, trader_id: str, since_id: str | None) -> List[TraderActivity]:
        raise NotImplementedError

    def place_order(self, order: CopyOrder) -> str:
        raise NotImplementedError


class MockPolymarketClient(PolymarketClient):
    """No-op client used during local development without live credentials."""

    def fetch_recent_activity(self, trader_id: str, since_id: str | None) -> List[TraderActivity]:
        _ = (trader_id, since_id)
        return []

    def place_order(self, order: CopyOrder) -> str:
        _ = order
        return "mock-order-id"


class PolymarketHTTPClient(PolymarketClient):
    """HTTP client for live Polymarket CLOB interactions."""

    def __init__(self, config: ClientConfig) -> None:
        if not config.base_url:
            raise ClientError("CLOB_API_URL is required for live trading")
        if not config.private_key:
            raise ClientError("CLOB_PRIVATE_KEY is required for live trading")
        self.config = config
        self.client = self._build_client()

    def fetch_recent_activity(self, trader_id: str, since_id: str | None) -> List[TraderActivity]:
        params = TradeParams(maker_address=trader_id)
        trades = self.client.get_trades(params=params)
        activities: List[TraderActivity] = []
        for trade in trades:
            activity_id = str(trade.get("id") or trade.get("trade_id") or "")
            if since_id and activity_id == since_id:
                continue
            created_at = _parse_trade_time(trade)
            activities.append(
                TraderActivity(
                    trader_id=trader_id,
                    activity_id=activity_id,
                    market_id=str(trade.get("market") or trade.get("market_id") or ""),
                    token_id=str(trade.get("asset_id") or trade.get("token_id") or ""),
                    outcome=str(trade.get("outcome") or trade.get("event_outcome") or ""),
                    side=str(trade.get("side") or trade.get("maker_side") or ""),
                    price=float(trade.get("price") or 0),
                    size=float(trade.get("size") or trade.get("amount") or 0),
                    created_at=created_at,
                )
            )
        return activities

    def place_order(self, order: CopyOrder) -> str:
        if not order.token_id:
            raise ClientError("token_id is required to place an order")
        if order.price is None:
            market_args = MarketOrderArgs(token_id=order.token_id, amount=order.size, side=order.side)
            placed = self.client.create_market_order(market_args)
        else:
            order_args = OrderArgs(
                token_id=order.token_id,
                price=order.price,
                size=order.size,
                side=order.side,
            )
            placed = self.client.create_and_post_order(order_args)
        return str(placed.get("order_id") or placed.get("id") or "unknown-order-id")

    def _build_client(self) -> ClobClient:
        if self.config.signature_type in (1, 2) and not self.config.funder:
            raise ClientError("CLOB_FUNDER is required for signature_type 1 or 2")
        if self.config.signature_type == 1:
            client = ClobClient(
                self.config.base_url,
                key=self.config.private_key,
                chain_id=137,
                signature_type=1,
                funder=self.config.funder,
            )
        elif self.config.signature_type == 2:
            client = ClobClient(
                self.config.base_url,
                key=self.config.private_key,
                chain_id=137,
                signature_type=2,
                funder=self.config.funder,
            )
        else:
            client = ClobClient(
                self.config.base_url,
                key=self.config.private_key,
                chain_id=137,
            )
        creds = _load_api_creds()
        if creds:
            client.set_api_creds(creds)
        return client


def load_client() -> PolymarketClient:
    if os.environ.get("ENABLE_LIVE_TRADING", "false").lower() != "true":
        return MockPolymarketClient()

    config = ClientConfig(
        base_url=os.environ.get("CLOB_API_URL", ""),
        api_key=os.environ.get("CLOB_API_KEY"),
        api_secret=os.environ.get("CLOB_API_SECRET"),
        api_passphrase=os.environ.get("CLOB_API_PASSPHRASE"),
        private_key=_load_text_secret("CLOB_PRIVATE_KEY", Path("credentials/.magiclink_credentials")),
        funder=_load_text_secret("CLOB_FUNDER", Path("credentials/.funder_address")),
        signature_type=int(os.environ.get("CLOB_SIGNATURE_TYPE", "1")),
    )
    return PolymarketHTTPClient(config)


def _parse_trade_time(trade: Dict[str, Any]) -> datetime:
    timestamp = trade.get("timestamp") or trade.get("created_at")
    if isinstance(timestamp, (int, float)):
        return datetime.fromtimestamp(float(timestamp), tz=timezone.utc)
    if isinstance(timestamp, str) and timestamp:
        try:
            return datetime.fromisoformat(timestamp.replace("Z", "+00:00"))
        except ValueError:
            return datetime.now(tz=timezone.utc)
    return datetime.now(tz=timezone.utc)


def _load_api_creds() -> Optional[ApiCreds]:
    creds = _load_credentials_file()
    if creds:
        return ApiCreds(
            api_key=creds.get("api_key", ""),
            api_secret=creds.get("api_secret", ""),
            api_passphrase=creds.get("api_passphrase", ""),
        )
    env_api_key = os.environ.get("CLOB_API_KEY")
    env_api_secret = os.environ.get("CLOB_API_SECRET")
    env_api_passphrase = os.environ.get("CLOB_API_PASSPHRASE")
    if env_api_key and env_api_secret and env_api_passphrase:
        return ApiCreds(
            api_key=env_api_key,
            api_secret=env_api_secret,
            api_passphrase=env_api_passphrase,
        )
    return None


def _load_credentials_file() -> Dict[str, Any]:
    path = Path("credentials/.clob_credentials")
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}


def _load_text_secret(env_key: str, path: Path) -> Optional[str]:
    value = os.environ.get(env_key, "").strip()
    if value:
        return value
    if not path.exists():
        return None
    content = path.read_text(encoding="utf-8").strip()
    return content or None
