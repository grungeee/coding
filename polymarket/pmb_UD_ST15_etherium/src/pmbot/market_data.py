"""Market metadata lookup via Polymarket Gamma API."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Any
import urllib.parse
import urllib.request


class MarketDataError(RuntimeError):
    pass


@dataclass(frozen=True)
class MarketWindow:
    slug: str
    condition_id: str
    start_time: datetime
    end_time: datetime
    outcomes: list[str]
    clob_token_ids: list[str]


def fetch_market_by_slug(slug: str) -> MarketWindow:
    query = urllib.parse.urlencode({"slug": slug})
    url = f"https://gamma-api.polymarket.com/markets?{query}"
    req = urllib.request.Request(url, headers={"User-Agent": "pmbot/0.1"})
    with urllib.request.urlopen(req, timeout=20) as resp:
        payload = json.loads(resp.read())

    if not payload:
        raise MarketDataError(f"No market found for slug '{slug}'")

    market = payload[0]
    condition_id = market.get("conditionId")
    start_iso = market.get("startDate") or market.get("startDateIso")
    end_iso = market.get("endDate") or market.get("endDateIso")
    outcomes = _decode_list(market.get("outcomes"))
    clob_token_ids = _decode_list(market.get("clobTokenIds"))

    if not condition_id or not start_iso or not end_iso:
        raise MarketDataError("Missing conditionId/startDate/endDate in market response")
    if len(outcomes) != len(clob_token_ids):
        raise MarketDataError("outcomes/clobTokenIds length mismatch")

    start_time = _parse_iso(start_iso)
    end_time = _parse_iso(end_iso)

    return MarketWindow(
        slug=market.get("slug", slug),
        condition_id=condition_id,
        start_time=start_time,
        end_time=end_time,
        outcomes=outcomes,
        clob_token_ids=clob_token_ids,
    )


def _decode_list(value: Any) -> list[str]:
    if isinstance(value, list):
        return [str(item) for item in value]
    if isinstance(value, str):
        return [str(item) for item in json.loads(value)]
    return []


def _parse_iso(value: str) -> datetime:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)
