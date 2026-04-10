"""Fetch and print recent trades for configured traders."""
from __future__ import annotations

import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List

from py_clob_client.client import ClobClient
from py_clob_client.clob_types import ApiCreds, TradeParams

from pmbot.config import load_config
from pmbot.profile_resolver import resolve_trader_id


def load_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8").strip()


def load_creds() -> ApiCreds:
    path = Path("credentials/.clob_credentials")
    if not path.exists():
        raise SystemExit("credentials/.clob_credentials not found")
    raw = json.loads(path.read_text(encoding="utf-8"))
    return ApiCreds(
        api_key=raw.get("api_key", ""),
        api_secret=raw.get("api_secret", ""),
        api_passphrase=raw.get("api_passphrase", ""),
    )


def parse_time(value: Any) -> str:
    if isinstance(value, (int, float)):
        return datetime.fromtimestamp(float(value), tz=timezone.utc).isoformat()
    if isinstance(value, str) and value:
        try:
            return datetime.fromisoformat(value.replace("Z", "+00:00")).isoformat()
        except ValueError:
            return value
    return "unknown"


def main() -> int:
    config = load_config()
    host = os.environ.get("CLOB_API_URL", "https://clob.polymarket.com")
    signature_type = int(os.environ.get("CLOB_SIGNATURE_TYPE", "1"))
    key = os.environ.get("CLOB_PRIVATE_KEY", "") or load_text(Path("credentials/.magiclink_credentials"))
    funder = os.environ.get("CLOB_FUNDER", "") or load_text(Path("credentials/.funder_address"))

    if not key:
        raise SystemExit("CLOB_PRIVATE_KEY is required")
    if signature_type in (1, 2) and not funder:
        raise SystemExit("CLOB_FUNDER is required for signature_type 1 or 2")

    if signature_type == 1:
        client = ClobClient(host, key=key, chain_id=137, signature_type=1, funder=funder)
    elif signature_type == 2:
        client = ClobClient(host, key=key, chain_id=137, signature_type=2, funder=funder)
    else:
        client = ClobClient(host, key=key, chain_id=137)

    client.set_api_creds(load_creds())

    trader_ids = []
    for entry in config.traders:
        resolved = resolve_trader_id(entry)
        if resolved:
            trader_ids.append(resolved)

    if trader_ids:
        for trader_id in trader_ids:
            print(f"Trader: {trader_id}")
            params = TradeParams(maker_address=trader_id)
            trades = client.get_trades(params=params)
            if not trades:
                print("  No trades returned")
                continue
            for trade in trades[:5]:
                print(
                    "  id={id} market={market} token={token} side={side} price={price} size={size} time={time}".format(
                        id=trade.get("id") or trade.get("trade_id"),
                        market=trade.get("market") or trade.get("market_id"),
                        token=trade.get("asset_id") or trade.get("token_id"),
                        side=trade.get("side") or trade.get("maker_side"),
                        price=trade.get("price"),
                        size=trade.get("size") or trade.get("amount"),
                        time=parse_time(trade.get("timestamp") or trade.get("created_at")),
                    )
                )
    else:
        print("No trader addresses configured; showing sample markets instead.")
        markets_response = client.get_simplified_markets()
        if isinstance(markets_response, dict):
            markets = markets_response.get("data", [])
        else:
            markets = markets_response
        markets_list = list(markets)
        if not markets_list:
            print("  No markets returned")
            print(f"  Raw response type: {type(markets_response)}")
            return 0
        print("  Sample market payload:", markets_list[0])
        for market in markets_list[:5]:
            if not isinstance(market, dict):
                print(f"  market={market}")
                continue
            tokens = market.get("tokens") or []
            outcomes = [token.get("outcome") for token in tokens if isinstance(token, dict)]
            print(
                "  condition_id={cid} active={active} closed={closed} outcomes={outcomes}".format(
                    cid=market.get("condition_id") or market.get("id") or market.get("market_id"),
                    active=market.get("active"),
                    closed=market.get("closed"),
                    outcomes=outcomes,
                )
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
