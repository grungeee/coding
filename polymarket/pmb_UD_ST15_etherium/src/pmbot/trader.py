"""Trading execution layer."""

from __future__ import annotations

from dataclasses import dataclass
import logging

from .config import Config
from .engine import TradeAction
from .strategy import Signal


class TradeError(RuntimeError):
    pass


class Trader:
    def execute(self, action: TradeAction) -> None:
        raise NotImplementedError

    def preflight_entry(self, action: TradeAction, signal: Signal) -> tuple[bool, str]:
        return True, "ok"

    def get_orderbook_snapshot(self) -> dict[str, dict[str, float]]:
        return {}


@dataclass
class PaperTrade:
    action: str
    direction: str | None
    reason: str


class PolymarketTrader(Trader):
    def __init__(self, config: Config) -> None:
        self._config = config
        self._client = None
        self._position_size: float | None = None
        self._logger = logging.getLogger(self.__class__.__name__)

    def _ensure_client(self) -> None:
        if self._client is not None:
            return
        missing = [
            name
            for name, value in {
                "CLOB_FUNDER": self._config.clob_funder,
                "CLOB_PRIVATE_KEY": self._config.clob_private_key,
            }.items()
            if not value
        ]
        if missing:
            raise TradeError(f"Missing CLOB credentials: {', '.join(missing)}")
        try:
            from py_clob_client.client import ClobClient
            from py_clob_client.clob_types import ApiCreds
            from py_clob_client.constants import POLYGON
            from py_builder_signing_sdk.config import BuilderApiKeyCreds, BuilderConfig
        except ImportError as exc:
            raise TradeError("py-clob-client is required for live trading") from exc

        creds = None
        if self._config.clob_api_key and self._config.clob_api_secret and self._config.clob_api_passphrase:
            creds = ApiCreds(
                api_key=self._config.clob_api_key,
                api_secret=self._config.clob_api_secret,
                api_passphrase=self._config.clob_api_passphrase,
            )
        builder_config = None
        if self._config.builder_api_key and self._config.builder_api_secret:
            builder_config = BuilderConfig(
                local_builder_creds=BuilderApiKeyCreds(
                    key=self._config.builder_api_key,
                    secret=self._config.builder_api_secret,
                    passphrase=self._config.builder_api_passphrase,
                )
            )
        client = ClobClient(
            self._config.clob_host,
            key=self._config.clob_private_key,
            chain_id=POLYGON,
            funder=self._config.clob_funder,
            creds=creds,
            builder_config=builder_config,
        )
        if creds is None and self._config.clob_auto_create_key:
            new_creds = client.create_api_key()
            client.creds = new_creds
            self._logger.warning(
                "Created new CLOB API key via create_api_key(); persist it in .env."
            )
        elif creds is None:
            raise TradeError(
                "CLOB API credentials missing. Set CLOB_API_KEY/CLOB_API_SECRET/"
                "CLOB_API_PASSPHRASE or enable CLOB_AUTO_CREATE_KEY=true."
            )
        self._client = client

    def execute(self, action: TradeAction) -> None:
        if action.action == "HOLD":
            return
        self._ensure_client()
        self._ensure_market_open()
        token_id = self._resolve_token_id(action)
        if not token_id:
            raise TradeError("Missing outcome token IDs for trading")
        self._place_market_order(action, token_id)

    def preflight_entry(self, action: TradeAction, signal: Signal) -> tuple[bool, str]:
        if action.action != "ENTER":
            return True, "ok"
        self._ensure_client()
        self._ensure_market_open()
        token_id = self._resolve_token_id(action)
        if not token_id:
            return False, "missing-token-id"
        return self._odds_check(token_id, signal)

    def get_orderbook_snapshot(self) -> dict[str, dict[str, float]]:
        self._ensure_client()
        snapshot: dict[str, dict[str, float]] = {}
        for label, token_id in {"UP": self._config.outcome_up_id, "DOWN": self._config.outcome_down_id}.items():
            if not token_id:
                continue
            ask = self._client.get_price(token_id, side="BUY")
            bid = self._client.get_price(token_id, side="SELL")
            data: dict[str, float] = {}
            if ask is not None:
                data["ask"] = float(ask)
            if bid is not None:
                data["bid"] = float(bid)
            snapshot[label] = data
        return snapshot

    def _resolve_token_id(self, action: TradeAction) -> str | None:
        if action.direction == "UP":
            return self._config.outcome_up_id
        if action.direction == "DOWN":
            return self._config.outcome_down_id
        return None

    def _place_market_order(self, action: TradeAction, token_id: str) -> None:
        from py_clob_client.clob_types import MarketOrderArgs, OrderType
        from py_clob_client.order_builder.constants import BUY, SELL

        if action.action == "ENTER":
            amount_usd = action.stake_usd
            price = self._client.get_price(token_id, side="BUY")
            if price:
                self._position_size = max(self._config.min_shares, amount_usd / float(price))
            order_args = MarketOrderArgs(token_id=token_id, amount=amount_usd, side=BUY)
            signed = self._client.create_market_order(order_args)
            self._client.post_order(signed, orderType=OrderType.FOK)
            return

        if action.action == "EXIT":
            size = self._position_size or self._config.min_shares
            order_args = MarketOrderArgs(token_id=token_id, amount=size, side=SELL)
            signed = self._client.create_market_order(order_args)
            self._client.post_order(signed, orderType=OrderType.FOK)
            self._position_size = None

    def _ensure_market_open(self) -> None:
        if not self._config.market_id:
            return
        market = self._client.get_market(self._config.market_id)
        if not isinstance(market, dict):
            return
        if market.get("closed") or not market.get("active"):
            raise TradeError("Market is closed or inactive; skipping trade")

    def _odds_check(self, token_id: str, signal: Signal) -> tuple[bool, str]:
        ask = self._client.get_price(token_id, side="BUY")
        bid = self._client.get_price(token_id, side="SELL")
        if ask is None or bid is None:
            return False, "missing-orderbook"
        ask_price = float(ask)
        bid_price = float(bid)
        if ask_price <= 0 or bid_price <= 0:
            return False, "invalid-prices"
        mid = (ask_price + bid_price) / 2
        if mid <= 0:
            return False, "invalid-mid"
        spread_pct = (ask_price - bid_price) / mid
        if spread_pct > self._config.max_spread_pct:
            return False, "spread-too-wide"

        confidence_ratio = signal.confidence / (signal.confidence + self._config.confidence_scale)
        model_prob = 0.5 + 0.5 * confidence_ratio
        implied_prob = ask_price
        edge = model_prob - implied_prob
        if edge < self._config.min_edge:
            return False, "edge-too-small"
        return True, "ok"


class PaperTrader(Trader):
    def __init__(self) -> None:
        self.trades: list[PaperTrade] = []

    def execute(self, action: TradeAction) -> None:
        if action.action == "HOLD":
            return
        self.trades.append(PaperTrade(action.action, action.direction, action.reason))

    def preflight_entry(self, action: TradeAction, signal: Signal) -> tuple[bool, str]:
        return True, "ok"
