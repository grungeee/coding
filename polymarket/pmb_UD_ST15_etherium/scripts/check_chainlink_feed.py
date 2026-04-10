"""Verify a Chainlink price feed over RPC."""

from __future__ import annotations

import argparse
import json
import urllib.request


DECIMALS_ID = "0x313ce567"
LATEST_ROUND_DATA_ID = "0xfeaf968c"


def eth_call(rpc_url: str, to: str, data: str) -> dict:
    payload = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": "eth_call",
        "params": [{"to": to, "data": data}, "latest"],
    }
    req = urllib.request.Request(
        rpc_url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req, timeout=20) as resp:
        return json.loads(resp.read())


def decode_words(hex_value: str) -> list[int]:
    clean = hex_value[2:] if hex_value.startswith("0x") else hex_value
    return [int(clean[i : i + 64], 16) for i in range(0, len(clean), 64)]


def main() -> None:
    parser = argparse.ArgumentParser(description="Check Chainlink feed")
    parser.add_argument("rpc", help="RPC URL (e.g., https://polygon-rpc.com)")
    parser.add_argument("address", help="Feed contract address")
    args = parser.parse_args()

    dec_resp = eth_call(args.rpc, args.address, DECIMALS_ID)
    decimals = decode_words(dec_resp["result"])[-1]

    data_resp = eth_call(args.rpc, args.address, LATEST_ROUND_DATA_ID)
    words = decode_words(data_resp["result"])
    answer = words[1]
    price = answer / (10**decimals)

    print(f"decimals={decimals} price={price}")


if __name__ == "__main__":
    main()
