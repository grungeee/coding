"""Bot entrypoint."""

from __future__ import annotations

import argparse
import curses
import logging
import time
from datetime import datetime, timedelta, timezone

from dotenv import load_dotenv

from .config import load_config
from dataclasses import asdict

from .engine import DecisionEngine, TradePolicy
from .market_data import MarketDataError, fetch_market_by_slug
from .price_feed import ChainlinkHTTPPriceFeed, ChainlinkOnChainPriceFeed, PriceFeedError
from .storage import PriceStore
from .strategy import compute_signal
from .trader import PaperTrader, PolymarketTrader, TradeError
from .tui import TuiApp, TuiStatus


def _build_feed(config):
    if config.feed_mode == "onchain":
        return ChainlinkOnChainPriceFeed(
            rpc_url=config.eth_rpc_url,
            feed_address=config.chainlink_feed_address,
            decimals=None if config.chainlink_feed_decimals < 0 else config.chainlink_feed_decimals,
        )
    return ChainlinkHTTPPriceFeed(
        url=config.chainlink_price_url,
        json_path=config.chainlink_price_json_path,
        auth_mode=config.chainlink_auth_mode,
        api_key=config.chainlink_api_key,
        api_secret=config.chainlink_api_secret,
        key_header=config.chainlink_api_key_header,
        signature_header=config.chainlink_api_signature_header,
        timestamp_header=config.chainlink_api_timestamp_header,
    )


def _lookup_share_price(
    orderbook: dict[str, dict[str, float]],
    direction: str | None,
    side: str,
) -> float | None:
    if not direction:
        return None
    data = orderbook.get(direction, {})
    key = "ask" if side == "BUY" else "bid"
    value = data.get(key)
    if value is None:
        return None
    return float(value)


def _entry_horizon_stats(signal, position) -> dict[str, dict[str, float]]:
    if not position.direction or position.entry_share_price is None:
        return {}
    if not signal.horizon_probs:
        return {}
    stats: dict[str, dict[str, float]] = {}
    for label, probs in signal.horizon_probs.items():
        win = probs["up"] if position.direction == "UP" else probs["down"]
        stats[label] = {"win": win, "edge": win - position.entry_share_price}
    return stats


def _get_orderbook_snapshot(live_trader, default_trader, ui) -> dict[str, dict[str, float]]:
    if not ui:
        return {}
    try:
        if live_trader:
            return live_trader.get_orderbook_snapshot()
        return default_trader.get_orderbook_snapshot()
    except TradeError as exc:
        ui.log(f"Orderbook error: {exc}")
        return {}


def _render_ui(
    ui: TuiApp,
    signal,
    price: float,
    baseline_price: float | None,
    window_start: datetime,
    window_end: datetime | None,
    market_window,
    engine: DecisionEngine,
    orderbook: dict[str, dict[str, float]],
    config,
) -> bool:
    entry_stats = _entry_horizon_stats(signal, engine.position)
    status = TuiStatus(
        now=datetime.now(timezone.utc),
        price=price,
        baseline_price=baseline_price,
        price_change_from_start=(price - baseline_price if baseline_price is not None else None),
        signal_direction=signal.direction,
        confidence=signal.confidence,
        slope=signal.slope,
        volatility=signal.volatility,
        horizon_probs=signal.horizon_probs,
        points=signal.points,
        window_start=window_start,
        window_end=window_end,
        market_slug=market_window.slug if market_window else None,
        position_direction=engine.position.direction,
        entry_share_price=engine.position.entry_share_price,
        entry_horizon_stats=entry_stats,
        trader_mode=config.trader_mode,
        trading_enabled=ui.trading_enabled,
        paper_only=ui.paper_only,
        orderbook=orderbook,
    )
    return ui.update(status)


def _resolve_outcome_ids(outcomes: list[str], token_ids: list[str]) -> tuple[str | None, str | None]:
    up_id = None
    down_id = None
    for outcome, token_id in zip(outcomes, token_ids):
        label = outcome.strip().lower()
        if label in {"up", "yes"}:
            up_id = token_id
        elif label in {"down", "no"}:
            down_id = token_id
    return up_id, down_id


def _persist_env_updates(updates: dict[str, str]) -> None:
    env_path = ".env"
    try:
        with open(env_path, "r", encoding="utf-8") as handle:
            lines = handle.read().splitlines()
    except FileNotFoundError:
        lines = []
    keys_written: set[str] = set()
    for idx, line in enumerate(lines):
        if not line or line.lstrip().startswith("#") or "=" not in line:
            continue
        key = line.split("=", 1)[0].strip()
        if key in updates:
            lines[idx] = f"{key}={updates[key]}"
            keys_written.add(key)
    for key, value in updates.items():
        if key not in keys_written:
            lines.append(f"{key}={value}")
    with open(env_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def _current_window_slug(now: datetime) -> str:
    ts = int(now.timestamp())
    window_start = ts - (ts % 900)
    return f"eth-updown-15m-{window_start}"


def _refresh_market_from_slug(config, slug: str, ui: TuiApp | None):
    market_window = fetch_market_by_slug(slug)
    config_data = asdict(config)
    config_data["market_id"] = market_window.condition_id
    up_id, down_id = _resolve_outcome_ids(market_window.outcomes, market_window.clob_token_ids)
    if up_id:
        config_data["outcome_up_id"] = up_id
    if down_id:
        config_data["outcome_down_id"] = down_id
    config_data["market_slug"] = slug
    config = config.__class__(**config_data)
    updates = {"MARKET_ID": market_window.condition_id, "MARKET_SLUG": slug}
    if up_id:
        updates["OUTCOME_UP_ID"] = up_id
    if down_id:
        updates["OUTCOME_DOWN_ID"] = down_id
    _persist_env_updates(updates)
    logging.info("Market IDs updated from slug.")
    if ui:
        ui.log("Market IDs updated from slug")
    return config, market_window


def build_trader(config):
    if config.trader_mode == "paper":
        return PaperTrader()
    if config.trader_mode == "polymarket":
        return PolymarketTrader(config)
    raise ValueError(f"Unknown TRADER_MODE '{config.trader_mode}'")


def run() -> None:
    load_dotenv()
    config = load_config()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    _run_loop(config, ui=None, plain=False)


def _run_loop(config, ui: TuiApp | None, plain: bool) -> None:
    feed = _build_feed(config)
    store = PriceStore(config.sqlite_path)
    paper_trader = PaperTrader()
    live_trader = PolymarketTrader(config) if config.trader_mode == "polymarket" else None
    default_trader = live_trader or paper_trader
    policy = TradePolicy(
        enter_confidence=config.enter_confidence,
        hold_confidence=config.hold_confidence,
        trailing_stop_mult=config.trailing_stop_mult,
        max_hold_minutes=config.max_hold_minutes,
        cooldown_minutes=config.cooldown_minutes,
        stake_usd=config.stake_usd,
        min_shares=config.min_shares,
    )
    engine = DecisionEngine(policy)

    logging.info("Starting bot with %s mode", config.trader_mode)
    if ui:
        ui.log(f"Starting bot with {config.trader_mode} mode")

    market_window = None
    if config.market_slug:
        try:
            config, market_window = _refresh_market_from_slug(config, config.market_slug, ui)
            logging.info(
                "Market window: %s start=%s end=%s",
                market_window.slug,
                market_window.start_time.isoformat(),
                market_window.end_time.isoformat(),
            )
            if ui:
                ui.log(
                    f"Market window start={market_window.start_time.isoformat()} end={market_window.end_time.isoformat()}"
                )
        except MarketDataError as exc:
            logging.error("Market metadata error: %s", exc)
            if ui:
                ui.log(f"Market metadata error: {exc}")

    next_fetch_at = 0.0
    while True:
        try:
            now_ts = time.time()
            manual_refresh = False
            if ui and ui.consume_refresh_request(config.manual_refresh_cooldown_sec):
                manual_refresh = True
            if not manual_refresh and now_ts < next_fetch_at:
                sleep_for = max(0.0, next_fetch_at - now_ts)
                if ui:
                    if not _sleep_with_ui(sleep_for, ui):
                        return
                else:
                    time.sleep(sleep_for)
                continue
            timestamp, price = feed.fetch_price()
            next_fetch_at = time.time() + config.poll_interval_sec
            store.add_price(timestamp, price)
            now = datetime.now(timezone.utc)
            if ui and manual_refresh:
                ui.log("Manual refresh")
            orderbook_snapshot = _get_orderbook_snapshot(live_trader, default_trader, ui)
            if market_window:
                if now < market_window.start_time:
                    logging.info("Market not started yet; waiting for %s", market_window.start_time.isoformat())
                    if plain:
                        print(
                            f"{now.isoformat()} status=waiting window_start={market_window.start_time.isoformat()} price={price:.2f}"
                        )
                    if ui:
                        ui.log(f"Market not started yet; waiting for {market_window.start_time.isoformat()}")
                        waiting_signal = compute_signal(
                            prices=[],
                            window_minutes=config.window_minutes,
                            min_points=config.min_points,
                            slope_threshold=config.slope_threshold,
                            baseline_price=None,
                            start_delta_threshold=config.start_delta_threshold,
                        )
                        if not _render_ui(
                            ui,
                            waiting_signal,
                            price,
                            None,
                            market_window.start_time,
                            market_window.end_time,
                            market_window,
                            engine,
                            orderbook_snapshot,
                            config,
                        ):
                            return
                    time.sleep(config.poll_interval_sec)
                    continue
                if now >= market_window.end_time:
                    logging.info("Market window ended at %s; forcing exit if needed", market_window.end_time.isoformat())
                    if plain:
                        print(
                            f"{now.isoformat()} status=ended window_end={market_window.end_time.isoformat()} price={price:.2f}"
                        )
                    if ui:
                        ui.log(f"Market window ended at {market_window.end_time.isoformat()}")
                    force_action = engine.force_exit(now, "market-window-ended")
                    if force_action:
                        active_trader = paper_trader if (ui and ui.paper_only) else default_trader
                        active_trader.execute(force_action)
                        logging.info(
                            "Trade action=%s direction=%s reason=%s stake=%.2f",
                            force_action.action,
                            force_action.direction,
                            force_action.reason,
                            force_action.stake_usd,
                        )
                        if ui:
                            ui.record_trade(force_action.action, force_action.direction, force_action.reason)
                    if ui:
                        ended_signal = compute_signal(
                            prices=[],
                            window_minutes=config.window_minutes,
                            min_points=config.min_points,
                            slope_threshold=config.slope_threshold,
                            baseline_price=None,
                            start_delta_threshold=config.start_delta_threshold,
                        )
                        if not _render_ui(
                            ui,
                            ended_signal,
                            price,
                            None,
                            market_window.start_time,
                            market_window.end_time,
                            market_window,
                            engine,
                            orderbook_snapshot,
                            config,
                        ):
                            return
                    next_slug = _current_window_slug(datetime.now(timezone.utc))
                    if next_slug != config.market_slug:
                        try:
                            config, market_window = _refresh_market_from_slug(config, next_slug, ui)
                            logging.info(
                                "Switched to market window: %s start=%s end=%s",
                                market_window.slug,
                                market_window.start_time.isoformat(),
                                market_window.end_time.isoformat(),
                            )
                            if ui:
                                ui.log(
                                    f"Switched window start={market_window.start_time.isoformat()} "
                                    f"end={market_window.end_time.isoformat()}"
                                )
                        except MarketDataError as exc:
                            logging.error("Market metadata error: %s", exc)
                            if ui:
                                ui.log(f"Market metadata error: {exc}")
                    time.sleep(config.poll_interval_sec)
                    continue
                window_start = market_window.start_time
            else:
                window_start = now - timedelta(minutes=config.window_minutes)
            points = store.get_prices_between(window_start, now)
            store.prune_before(window_start - timedelta(minutes=5))

            if market_window:
                baseline_price = store.get_window_baseline(window_start)
                if baseline_price is None and now >= window_start:
                    baseline_price = price
                    store.set_window_baseline(window_start, baseline_price)
                    logging.info("Baseline locked at window start: %.2f", baseline_price)
                    if ui:
                        ui.log(f"Baseline locked at window start: {baseline_price:.2f}")
            else:
                baseline_point = store.get_first_price_after(window_start)
                baseline_price = baseline_point.price if baseline_point else None
                if baseline_price is None and now >= window_start:
                    baseline_price = price
                    logging.warning("Baseline price missing; using current price as fallback")
                    if ui:
                        ui.log("Baseline missing; using current price")

            signal = compute_signal(
                prices=points,
                window_minutes=config.window_minutes,
                min_points=config.min_points,
                slope_threshold=config.slope_threshold,
                baseline_price=baseline_price,
                start_delta_threshold=config.start_delta_threshold,
            )
            logging.info(
                "Price %.2f | points=%d slope=%.6f confidence=%.3f signal=%s",
                price,
                signal.points,
                signal.slope,
                signal.confidence,
                signal.direction,
            )
            if plain:
                baseline_text = "n/a" if baseline_price is None else f"{baseline_price:.2f}"
                delta_text = "n/a"
                if baseline_price is not None:
                    delta_text = f"{price - baseline_price:+.2f}"
                print(
                    f"{now.isoformat()} price={price:.2f} baseline={baseline_text} "
                    f"delta={delta_text} signal={signal.direction} conf={signal.confidence:.3f} points={signal.points}"
                )
            if ui:
                if not _render_ui(
                    ui,
                    signal,
                    price,
                    baseline_price,
                    window_start,
                    market_window.end_time if market_window else None,
                    market_window,
                    engine,
                    orderbook_snapshot,
                    config,
                ):
                    return

            if ui and ui.consume_force_exit():
                force_action = engine.force_exit(now, "manual-liquidation")
                if force_action:
                    active_trader = paper_trader if ui.paper_only else default_trader
                    active_trader.execute(force_action)
                    if ui:
                        ui.record_trade(force_action.action, force_action.direction, force_action.reason)
            trading_enabled = ui.trading_enabled if ui else config.trading_enabled
            if trading_enabled:
                action = engine.evaluate(price, signal)
                active_trader = paper_trader if (ui and ui.paper_only) else default_trader
                if action.action == "ENTER":
                    if active_trader is default_trader and live_trader is not None:
                        allowed, reason = live_trader.preflight_entry(action, signal)
                        if not allowed:
                            engine.rollback_entry(now, f"preflight-{reason}")
                            logging.info("Entry blocked by preflight: %s", reason)
                            if ui:
                                ui.log(f"Entry blocked: {reason}")
                            time.sleep(config.poll_interval_sec)
                            continue
                entry_share_price = None
                if action.action == "ENTER":
                    if not orderbook_snapshot and live_trader:
                        orderbook_snapshot = live_trader.get_orderbook_snapshot()
                    entry_share_price = _lookup_share_price(orderbook_snapshot, action.direction, "BUY")
                elif action.action == "EXIT":
                    if not orderbook_snapshot and live_trader:
                        orderbook_snapshot = live_trader.get_orderbook_snapshot()
                    entry_share_price = _lookup_share_price(orderbook_snapshot, action.direction, "SELL")

                if action.action == "ENTER" and active_trader is paper_trader and entry_share_price is None:
                    engine.rollback_entry(now, "missing-orderbook")
                    logging.info("Paper entry blocked: missing orderbook price")
                    if ui:
                        ui.log("Paper entry blocked: missing orderbook price")
                    time.sleep(config.poll_interval_sec)
                    continue

                active_trader.execute(action)
                if action.action == "ENTER":
                    engine.set_entry_share_price(entry_share_price)
                if action.action != "HOLD":
                    if entry_share_price is None:
                        logging.info(
                            "Trade action=%s direction=%s reason=%s stake=%.2f",
                            action.action,
                            action.direction,
                            action.reason,
                            action.stake_usd,
                        )
                    else:
                        logging.info(
                            "Trade action=%s direction=%s reason=%s stake=%.2f share=%.4f",
                            action.action,
                            action.direction,
                            action.reason,
                            action.stake_usd,
                            entry_share_price,
                        )
                    if ui:
                        ui.record_trade(action.action, action.direction, action.reason, entry_share_price)
            else:
                logging.info("Trading disabled; set TRADING_ENABLED=true to trade")
                if ui:
                    ui.log("Trading disabled; set TRADING_ENABLED=true to trade")
        except (PriceFeedError, TradeError) as exc:
            logging.error("Run error: %s", exc)
            if ui:
                ui.log(f"Run error: {exc}")
        except Exception:
            logging.exception("Unexpected error")
            if ui:
                ui.log("Unexpected error; check logs")

        if ui:
            if not _sleep_with_ui(config.poll_interval_sec, ui):
                return
        else:
            time.sleep(config.poll_interval_sec)


def _sleep_with_ui(seconds: float, ui: TuiApp) -> bool:
    end_time = time.time() + seconds
    while time.time() < end_time:
        if not ui.pump():
            return False
        time.sleep(0.2)
    return True


def main() -> None:
    load_dotenv()
    parser = argparse.ArgumentParser(description="ETH 15m trend Polymarket bot")
    parser.add_argument("--once", action="store_true", help="Fetch one price tick and exit")
    parser.add_argument("--tui", action="store_true", help="Run with a curses TUI")
    parser.add_argument("--plain", action="store_true", help="Print plain logs each tick")
    args = parser.parse_args()

    if args.once:
        config = load_config()
        feed = _build_feed(config)
        timestamp, price = feed.fetch_price()
        print(f"{timestamp.isoformat()} {price}")
        return

    config = load_config()
    if args.tui:
        logging.basicConfig(
            filename="bot.log",
            level=logging.INFO,
            format="%(asctime)s %(levelname)s %(message)s",
        )
        initial_paper_only = True
        curses.wrapper(
            lambda stdscr: _run_loop(
                config, ui=TuiApp(stdscr, initial_paper_only, config.trading_enabled), plain=False
            )
        )
    else:
        logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
        _run_loop(config, ui=None, plain=args.plain)


if __name__ == "__main__":
    main()
