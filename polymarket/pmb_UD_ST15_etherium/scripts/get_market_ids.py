"""Fetch Polymarket market IDs and outcome token IDs by slug."""

from __future__ import annotations

import argparse
import json
import sys
import urllib.parse
import urllib.request


def fetch_slug(slug: str) -> dict:
    query = urllib.parse.urlencode({"slug": slug})
    url = f"https://gamma-api.polymarket.com/markets?{query}"
    req = urllib.request.Request(url, headers={"User-Agent": "pmbot/0.1"})
    with urllib.request.urlopen(req, timeout=20) as resp:
        payload = json.loads(resp.read())
    if not payload:
        raise RuntimeError(f"No market found for slug '{slug}'")
    return payload[0]


def main() -> None:
    parser = argparse.ArgumentParser(description="Get Polymarket IDs by slug")
    parser.add_argument("slug", help="Market slug, e.g. eth-updown-15m-1766543400")
    parser.add_argument("--env", action="store_true", help="Print env var format")
    args = parser.parse_args()

    market = fetch_slug(args.slug)
    condition_id = market.get("conditionId")
    outcomes = market.get("outcomes") or []
    clob_token_ids = market.get("clobTokenIds") or []

    if isinstance(outcomes, str):
        outcomes = json.loads(outcomes)
    if isinstance(clob_token_ids, str):
        clob_token_ids = json.loads(clob_token_ids)
    if not isinstance(outcomes, list) or not isinstance(clob_token_ids, list):
        raise RuntimeError("Unexpected outcomes/clobTokenIds format in response")
    if not condition_id or len(outcomes) != len(clob_token_ids):
        raise RuntimeError(
            f"Missing conditionId or clobTokenIds in response "
            f"(conditionId={condition_id!r}, outcomes={len(outcomes)}, clobTokenIds={len(clob_token_ids)})"
        )

    outcome_map = dict(zip(outcomes, clob_token_ids))

    if args.env:
        up_id = outcome_map.get("Up") or outcome_map.get("up")
        down_id = outcome_map.get("Down") or outcome_map.get("down")
        print(f"MARKET_ID={condition_id}")
        if up_id:
            print(f"OUTCOME_UP_ID={up_id}")
        if down_id:
            print(f"OUTCOME_DOWN_ID={down_id}")
        return

    print(json.dumps(
        {
            "slug": market.get("slug"),
            "conditionId": condition_id,
            "outcomes": outcomes,
            "clobTokenIds": clob_token_ids,
        },
        indent=2,
    ))


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
