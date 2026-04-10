"""Core copy-trading logic."""
from __future__ import annotations

import logging
import time
from dataclasses import dataclass
from typing import Iterable, List

from pmbot.config import BotConfig
from pmbot.models import CopyOrder, TraderActivity
from pmbot.leaderboard import select_traders
from pmbot.polymarket_client import PolymarketClient
from pmbot.profile_resolver import resolve_trader_id
from pmbot.storage import BotState, StateStore

LOGGER = logging.getLogger(__name__)


@dataclass
class CopyResult:
    copied: int
    skipped: int


def build_copy_order(activity: TraderActivity, copy_bet_usd: float) -> CopyOrder:
    return CopyOrder(
        market_id=activity.market_id,
        token_id=activity.token_id,
        outcome=activity.outcome,
        side=activity.side,
        size=copy_bet_usd,
        price=activity.price,
    )


def enforce_risk_limits(state: BotState, config: BotConfig, order: CopyOrder) -> bool:
    if state.open_positions_count >= config.risk_limits.max_open_positions:
        LOGGER.info("Risk limit hit: max open positions")
        return False
    if state.daily_position_count >= config.risk_limits.max_daily_positions:
        LOGGER.info("Risk limit hit: max daily positions")
        return False
    if state.daily_notional_usd + order.size > config.risk_limits.max_daily_notional_usd:
        LOGGER.info("Risk limit hit: max daily notional")
        return False
    return True


class CopyTraderBot:
    def __init__(self, config: BotConfig, client: PolymarketClient, store: StateStore) -> None:
        self.config = config
        self.client = client
        self.store = store

    def run_once(self) -> CopyResult:
        state = self.store.load()
        state.reset_daily_if_needed()
        if self.config.leaderboard_enabled:
            self._refresh_leaderboard(state)
        copied = 0
        skipped = 0

        trader_source = state.leaderboard_traders if self.config.leaderboard_enabled else self.config.traders
        for trader_entry in trader_source:
            trader_id = resolve_trader_id(trader_entry)
            if not trader_id:
                LOGGER.warning("Skipping trader entry '%s' (unable to resolve address)", trader_entry)
                continue
            last_seen = state.last_seen_activity.get(trader_id)
            activities = self.client.fetch_recent_activity(trader_id, last_seen)
            if not activities:
                continue

            ordered = sorted(activities, key=lambda item: item.created_at)
            for activity in ordered:
                state.last_seen_activity[trader_id] = activity.activity_id
                order = build_copy_order(activity, self.config.copy_bet_usd)
                if not enforce_risk_limits(state, self.config, order):
                    skipped += 1
                    continue

                order_id = self.client.place_order(order)
                LOGGER.info(
                    "Copied trade: trader=%s market=%s outcome=%s size=%.2f order_id=%s",
                    trader_id,
                    order.market_id,
                    order.outcome,
                    order.size,
                    order_id,
                )
                copied += 1
                state.daily_position_count += 1
                state.daily_notional_usd += order.size
                state.open_positions_count += 1

        self.store.save(state)
        return CopyResult(copied=copied, skipped=skipped)

    def poll_forever(self) -> None:
        LOGGER.info("Starting copy bot with %d traders", len(self.config.traders))
        while True:
            result = self.run_once()
            LOGGER.info("Cycle complete: copied=%d skipped=%d", result.copied, result.skipped)
            time.sleep(self.config.poll_interval_seconds)

    def _refresh_leaderboard(self, state: BotState) -> None:
        now = time.time()
        if now - state.last_leaderboard_refresh < self.config.leaderboard_refresh_seconds:
            return
        state.leaderboard_traders = select_traders(
            min_pnl=self.config.leaderboard_min_pnl,
            min_vol=self.config.leaderboard_min_vol,
            max_traders=self.config.leaderboard_max_traders,
        )
        state.last_leaderboard_refresh = now
        LOGGER.info("Leaderboard refreshed: %d traders selected", len(state.leaderboard_traders))
