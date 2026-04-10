"""Decision engine for entering/exiting positions."""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timedelta

from .strategy import Signal


@dataclass
class TradePolicy:
    enter_confidence: float
    hold_confidence: float
    trailing_stop_mult: float
    max_hold_minutes: int
    cooldown_minutes: int
    stake_usd: float
    min_shares: int


@dataclass
class TradeAction:
    action: str  # "ENTER", "EXIT", "HOLD"
    direction: str | None
    reason: str
    stake_usd: float
    min_shares: int


@dataclass
class PositionState:
    direction: str | None = None
    entry_price: float | None = None
    entry_share_price: float | None = None
    entry_time: datetime | None = None
    peak_price: float | None = None
    trough_price: float | None = None
    last_exit_time: datetime | None = None


class DecisionEngine:
    def __init__(self, policy: TradePolicy) -> None:
        self._policy = policy
        self._position = PositionState()

    @property
    def position(self) -> PositionState:
        return self._position

    def evaluate(self, price: float, signal: Signal) -> TradeAction:
        now = signal.as_of
        if self._position.direction is None:
            return self._evaluate_entry(price, signal, now)
        return self._evaluate_exit(price, signal, now)

    def rollback_entry(self, now: datetime, reason: str) -> TradeAction | None:
        if self._position.direction is None:
            return None
        self._position = PositionState(direction=None, last_exit_time=now)
        return TradeAction("HOLD", None, reason, 0.0, 0)

    def force_exit(self, now: datetime, reason: str) -> TradeAction | None:
        if self._position.direction is None:
            return None
        direction = self._position.direction
        self._position = PositionState(direction=None, last_exit_time=now)
        return TradeAction("EXIT", direction, reason, 0.0, 0)

    def _evaluate_entry(self, price: float, signal: Signal, now: datetime) -> TradeAction:
        if signal.direction == "HOLD":
            return TradeAction("HOLD", None, "no-signal", 0.0, 0)

        if signal.confidence < self._policy.enter_confidence:
            return TradeAction("HOLD", None, "low-confidence", 0.0, 0)

        if self._position.last_exit_time:
            cooldown = timedelta(minutes=self._policy.cooldown_minutes)
            if now - self._position.last_exit_time < cooldown:
                return TradeAction("HOLD", None, "cooldown", 0.0, 0)

        self._position = PositionState(
            direction=signal.direction,
            entry_price=price,
            entry_share_price=None,
            entry_time=now,
            peak_price=price,
            trough_price=price,
            last_exit_time=self._position.last_exit_time,
        )
        return TradeAction(
            "ENTER",
            signal.direction,
            "enter-signal",
            self._policy.stake_usd,
            self._policy.min_shares,
        )

    def _evaluate_exit(self, price: float, signal: Signal, now: datetime) -> TradeAction:
        position = self._position
        if position.direction == "UP":
            position.peak_price = max(position.peak_price or price, price)
        elif position.direction == "DOWN":
            position.trough_price = min(position.trough_price or price, price)

        reason = self._exit_reason(price, signal, now)
        if reason:
            self._position = PositionState(direction=None, last_exit_time=now)
            return TradeAction("EXIT", position.direction, reason, 0.0, 0)

        return TradeAction("HOLD", position.direction, "in-position", 0.0, 0)

    def _exit_reason(self, price: float, signal: Signal, now: datetime) -> str | None:
        position = self._position
        if signal.direction == "HOLD" and signal.confidence < self._policy.hold_confidence:
            return "confidence-decay"

        if signal.direction in {"UP", "DOWN"} and signal.direction != position.direction:
            if signal.confidence >= self._policy.hold_confidence:
                return "reverse-signal"

        if position.entry_time:
            max_hold = timedelta(minutes=self._policy.max_hold_minutes)
            if now - position.entry_time >= max_hold:
                return "max-hold"

        if signal.volatility > 0:
            if position.direction == "UP" and position.peak_price is not None:
                stop_price = position.peak_price - (signal.volatility * self._policy.trailing_stop_mult)
                if price <= stop_price:
                    return "trailing-stop"
            if position.direction == "DOWN" and position.trough_price is not None:
                stop_price = position.trough_price + (signal.volatility * self._policy.trailing_stop_mult)
                if price >= stop_price:
                    return "trailing-stop"

        return None

    def set_entry_share_price(self, price: float | None) -> None:
        if self._position.direction is None:
            return
        if price is None:
            return
        self._position.entry_share_price = price
