"""Terminal UI for live market monitoring."""

from __future__ import annotations

import curses
from collections import deque
from dataclasses import dataclass, replace
from datetime import datetime, timedelta
from typing import Any


@dataclass(frozen=True)
class TuiStatus:
    now: datetime
    price: float
    baseline_price: float | None
    price_change_from_start: float | None
    signal_direction: str
    confidence: float
    slope: float
    volatility: float
    horizon_probs: dict[str, dict[str, float]]
    points: int
    window_start: datetime
    window_end: datetime | None
    market_slug: str | None
    position_direction: str | None
    entry_share_price: float | None
    entry_horizon_stats: dict[str, dict[str, float]]
    trader_mode: str
    trading_enabled: bool
    paper_only: bool
    orderbook: dict[str, dict[str, float]]


class TuiApp:
    def __init__(self, stdscr: Any, paper_only: bool, trading_enabled: bool) -> None:
        self._stdscr = stdscr
        self._logs: deque[str] = deque(maxlen=12)
        self._last_bids: dict[str, float | None] = {"UP": None, "DOWN": None}
        self._last_trade: str = "n/a"
        self._status: TuiStatus | None = None
        self._paper_only = paper_only
        self._trading_enabled = trading_enabled
        self._force_exit = False
        self._last_price: float | None = None
        self._confidence_history: deque[float] = deque(maxlen=30)
        self._refresh_requested = False
        self._last_refresh_at: datetime | None = None
        curses.curs_set(0)
        self._init_colors()
        stdscr.nodelay(True)
        stdscr.timeout(200)

    def _init_colors(self) -> None:
        if not curses.has_colors():
            return
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_GREEN)  # paper badge
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_RED)  # live badge
        curses.init_pair(3, curses.COLOR_CYAN, -1)  # header
        curses.init_pair(4, curses.COLOR_YELLOW, -1)  # hold
        curses.init_pair(5, curses.COLOR_GREEN, -1)  # up
        curses.init_pair(6, curses.COLOR_RED, -1)  # down

    def log(self, message: str) -> None:
        timestamp = datetime.utcnow().strftime("%H:%M:%S")
        self._logs.append(f"{timestamp} {message}")

    def record_trade(
        self, action: str, direction: str | None, reason: str, share_price: float | None = None
    ) -> None:
        if direction:
            if share_price is None:
                self._last_trade = f"{action} {direction} ({reason})"
            else:
                self._last_trade = f"{action} {direction} @{share_price:.4f} ({reason})"
        else:
            self._last_trade = f"{action} ({reason})"

    def update(self, status: TuiStatus) -> bool:
        self._status = status
        self._render()
        return self._handle_input()

    def pump(self) -> bool:
        return self._handle_input()

    def _handle_input(self) -> bool:
        key = self._stdscr.getch()
        if key in (ord("q"), ord("Q")):
            return False
        if key in (ord("p"), ord("P")):
            self._paper_only = not self._paper_only
            mode = "paper-only" if self._paper_only else "live"
            self.log(f"Execution mode toggled: {mode}")
            self._refresh_status()
        if key in (ord("t"), ord("T")):
            self._trading_enabled = not self._trading_enabled
            state = "enabled" if self._trading_enabled else "disabled"
            self.log(f"Trading {state}")
            self._refresh_status()
        if key in (ord("x"), ord("X")):
            self._force_exit = True
            self.log("Manual liquidation requested")
        if key in (ord("r"), ord("R")):
            self._refresh_requested = True
        return True

    def _render(self) -> None:
        if not self._status:
            return
        status = self._status
        self._stdscr.erase()
        height, width = self._stdscr.getmaxyx()

        def add(line: int, text: str) -> None:
            if line >= height:
                return
            clipped = text[: max(0, width - 1)]
            self._stdscr.addstr(line, 0, clipped)

        header = (
            f"PMBot TUI | {status.now.isoformat(timespec='seconds')}"
            " (q quit, p paper/live, t trade, x exit, r refresh)"
        )
        self._add_color(0, 0, header, 3, bold=True)
        market_label = status.market_slug or "rolling-window"
        exec_mode = "paper-only" if status.paper_only else "live"
        prefix = f"Mode: {status.trader_mode} | Trading: {status.trading_enabled} | Exec: "
        add(1, prefix)
        badge_col = len(prefix)
        badge_text = f" {exec_mode} "
        self._add_badge(1, badge_col, exec_mode)
        suffix = f" | Market: {market_label}"
        self._add_text_at(1, badge_col + len(badge_text), suffix)

        line = 3
        self._add_color(line, 0, "Market", 3, bold=True)
        line += 1
        window_line = f"Window: {status.window_start.isoformat(timespec='seconds')}"
        if status.window_end:
            window_line += f" -> {status.window_end.isoformat(timespec='seconds')}"
        add(line, window_line)
        line += 1
        baseline = "n/a" if status.baseline_price is None else f"{status.baseline_price:.2f}"
        delta_baseline = "n/a"
        if status.price_change_from_start is not None:
            delta_baseline = f"{status.price_change_from_start:+.2f}"
        delta_tick = "n/a"
        delta_tick_pct = "n/a"
        if self._last_price is not None:
            delta_tick_value = status.price - self._last_price
            delta_tick = f"{delta_tick_value:+.2f}"
            if self._last_price != 0:
                delta_tick_pct = f"{(delta_tick_value / self._last_price) * 100:+.3f}%"
        add(
            line,
            "Price: {price:.2f} | Baseline: {baseline} | Δ baseline: {db} | Δ tick: {dt} ({dtp})".format(
                price=status.price,
                baseline=baseline,
                db=delta_baseline,
                dt=delta_tick,
                dtp=delta_tick_pct,
            ),
        )
        self._last_price = status.price
        line += 2

        self._add_color(line, 0, "Signal", 3, bold=True)
        line += 1
        signal_line = (
            "Signal: {direction} | Conf: {conf:.3f} | Slope: {slope:.6f} | Vol: {vol:.4f} | Points: {pts}"
        ).format(
            direction=status.signal_direction,
            conf=status.confidence,
            slope=status.slope,
            vol=status.volatility,
            pts=status.points,
        )
        add(line, signal_line)
        self._highlight_signal(line, signal_line, status.signal_direction)
        line += 1
        self._render_horizons(line, status.horizon_probs, label="Baseline odds")
        line += 2

        self._add_color(line, 0, "Confidence Trend", 3, bold=True)
        line += 1
        self._render_confidence_chart(line, status.confidence)
        line += 2

        self._add_color(line, 0, "Orderbook", 3, bold=True)
        line += 1
        for row in self._format_orderbook_lines(status.orderbook):
            add(line, row)
            line += 1
        line += 1

        self._add_color(line, 0, "Position", 3, bold=True)
        line += 1
        position_line = self._format_position_line(status.position_direction, status.entry_share_price)
        add(line, position_line)
        line += 1
        self._render_entry_stats(line, status.entry_horizon_stats, status.position_direction)
        line += 2

        add(line, f"Last trade: {self._last_trade}")
        line += 2
        self._add_color(line - 1, 0, "Logs", 3, bold=True)
        for idx, entry in enumerate(reversed(self._logs), start=0):
            add(line + idx, entry)

        self._stdscr.refresh()

    def _refresh_status(self) -> None:
        if not self._status:
            return
        self._status = replace(
            self._status,
            paper_only=self._paper_only,
            trading_enabled=self._trading_enabled,
        )
        self._render()

    def _add_badge(self, line: int, col: int, label: str) -> None:
        pair = 1 if label == "paper-only" else 2
        try:
            if curses.has_colors():
                attr = curses.color_pair(pair) | curses.A_BOLD
                self._stdscr.addstr(line, col, f" {label} ", attr)
            else:
                self._stdscr.addstr(line, col, f" {label} ")
        except curses.error:
            return

    def _add_text_at(self, line: int, col: int, text: str) -> None:
        try:
            self._stdscr.addstr(line, col, text)
        except curses.error:
            return

    def _add_color(self, line: int, col: int, text: str, pair: int, bold: bool = False) -> None:
        try:
            if curses.has_colors():
                attr = curses.color_pair(pair)
                if bold:
                    attr |= curses.A_BOLD
                self._stdscr.addstr(line, col, text, attr)
            else:
                self._stdscr.addstr(line, col, text)
        except curses.error:
            return

    def _highlight_signal(self, line: int, text: str, direction: str) -> None:
        if not curses.has_colors():
            return
        key = f"Signal: {direction}"
        idx = text.find(key)
        if idx < 0:
            return
        pair = 4
        if direction == "UP":
            pair = 5
        elif direction == "DOWN":
            pair = 6
        self._add_color(line, idx, key, pair, bold=True)

    def _render_horizons(
        self, line: int, probs: dict[str, dict[str, float]], label: str
    ) -> None:
        if not probs:
            return
        labels = ["10s", "1m", "3m", "5m", "10m"]
        parts: list[str] = []
        for label in labels:
            entry = probs.get(label)
            if not entry:
                continue
            parts.append(f"{label} U:{entry['up'] * 100:5.1f}%")
        if not parts:
            return
        text = f"{label}: " + " | ".join(parts)
        self._add_text_at(line, 0, text)
        if curses.has_colors():
            offset = len(label) + 2
            for idx, label in enumerate(labels):
                entry = probs.get(label)
                if not entry:
                    continue
                up_text = f"{label} U:{entry['up'] * 100:5.1f}%"
                if idx == 0:
                    segment_start = 0
                    for prev in labels[:idx]:
                        if probs.get(prev):
                            segment_start += len(f"{prev} U:{probs[prev]['up'] * 100:5.1f}% | ")
                    start = offset + segment_start + len(label) + 3
                else:
                    segment_start = 0
                    for prev in labels[:idx]:
                        if probs.get(prev):
                            segment_start += len(f"{prev} U:{probs[prev]['up'] * 100:5.1f}% | ")
                    start = offset + segment_start + len(label) + 3
                percent_text = f"{entry['up'] * 100:5.1f}%"
                self._add_color(line, start, percent_text, 5, bold=True)

    def _format_orderbook_lines(self, orderbook: dict[str, dict[str, float]]) -> list[str]:
        lines: list[str] = []
        for label in ("UP", "DOWN"):
            data = orderbook.get(label, {})
            bid = data.get("bid")
            ask = data.get("ask")
            delta = ""
            if bid is not None:
                prev = self._last_bids.get(label)
                self._last_bids[label] = bid
                if prev is not None:
                    delta = f" ({bid - prev:+.4f})"
            bid_text = "n/a" if bid is None else f"{bid:.4f}{delta}"
            ask_text = "n/a" if ask is None else f"{ask:.4f}"
            mid_text = "n/a"
            if bid is not None and ask is not None:
                mid_text = f"{(bid + ask) / 2:.4f}"
            lines.append(f"{label} bid/ask: {bid_text} / {ask_text}")
            lines.append(f"{label} mid: {mid_text}")
        return lines

    def _format_position_line(self, direction: str | None, entry_share_price: float | None) -> str:
        if direction is None:
            return "Position: none"
        entry = "n/a" if entry_share_price is None else f"{entry_share_price:.4f}"
        return f"Position: {direction} | Entry share: {entry}"

    def _render_entry_stats(
        self,
        line: int,
        stats: dict[str, dict[str, float]],
        direction: str | None,
    ) -> None:
        if not stats or direction is None:
            return
        labels = ["10s", "1m", "3m", "5m", "10m"]
        parts: list[str] = []
        for label in labels:
            entry = stats.get(label)
            if not entry:
                continue
            parts.append(f"{label} W:{entry['win'] * 100:5.1f}%")
        if not parts:
            return
        text = "Entry odds: " + " | ".join(parts)
        self._add_text_at(line, 0, text)
        if curses.has_colors():
            offset = len("Entry odds: ")
            for idx, label in enumerate(labels):
                entry = stats.get(label)
                if not entry:
                    continue
                segment_start = 0
                for prev in labels[:idx]:
                    if stats.get(prev):
                        segment_start += len(f"{prev} W:{stats[prev]['win'] * 100:5.1f}% | ")
                start = offset + segment_start + len(label) + 3
                percent_text = f"{entry['win'] * 100:5.1f}%"
                color = 5 if direction == "UP" else 6
                self._add_color(line, start, percent_text, color, bold=True)

    def _render_confidence_chart(self, line: int, current: float) -> None:
        self._confidence_history.append(current)
        if not self._confidence_history:
            return
        values = list(self._confidence_history)
        max_val = max(values) if max(values) > 0 else 1.0
        levels = " .:-=+*#%@"
        chart = "".join(levels[min(int((v / max_val) * (len(levels) - 1)), len(levels) - 1)] for v in values)
        self._add_text_at(line, 0, f"{chart}  ({current:.2f})")

    @property
    def paper_only(self) -> bool:
        return self._paper_only

    @property
    def trading_enabled(self) -> bool:
        return self._trading_enabled

    def consume_force_exit(self) -> bool:
        if not self._force_exit:
            return False
        self._force_exit = False
        return True

    def consume_refresh_request(self, cooldown_sec: float) -> bool:
        if not self._refresh_requested:
            return False
        now = datetime.utcnow()
        if self._last_refresh_at and now - self._last_refresh_at < timedelta(seconds=cooldown_sec):
            return False
        self._refresh_requested = False
        self._last_refresh_at = now
        return True
