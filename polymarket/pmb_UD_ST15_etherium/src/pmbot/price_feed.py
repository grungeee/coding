"""Price feed adapters."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime, timezone
import hmac
import hashlib
from typing import Any

import requests
from urllib.parse import urlparse


class PriceFeedError(RuntimeError):
    pass


class PriceFeed:
    def fetch_price(self) -> tuple[datetime, float]:
        raise NotImplementedError


@dataclass(frozen=True)
class ChainlinkHTTPPriceFeed(PriceFeed):
    url: str
    json_path: str
    timeout_sec: int = 10
    auth_mode: str = "none"
    api_key: str = ""
    api_secret: str = ""
    key_header: str = "X-API-KEY"
    signature_header: str = "X-API-SIGNATURE"
    timestamp_header: str = "X-API-TIMESTAMP"

    def fetch_price(self) -> tuple[datetime, float]:
        if not self.url:
            raise PriceFeedError("CHAINLINK_PRICE_URL is not set")
        headers = _build_auth_headers(
            url=self.url,
            method="GET",
            auth_mode=self.auth_mode,
            api_key=self.api_key,
            api_secret=self.api_secret,
            key_header=self.key_header,
            signature_header=self.signature_header,
            timestamp_header=self.timestamp_header,
        )
        response = requests.get(self.url, timeout=self.timeout_sec, headers=headers)
        response.raise_for_status()
        payload = response.json()
        price = _extract_json_path(payload, self.json_path)
        try:
            price_value = float(price)
        except (TypeError, ValueError) as exc:
            raise PriceFeedError(f"Invalid price value: {price!r}") from exc
        return datetime.now(timezone.utc), price_value


@dataclass
class ChainlinkOnChainPriceFeed(PriceFeed):
    rpc_url: str
    feed_address: str
    timeout_sec: int = 10
    decimals: int | None = None
    method: str = "latestRoundData"

    def fetch_price(self) -> tuple[datetime, float]:
        if not self.rpc_url:
            raise PriceFeedError("ETH_RPC_URL is not set")
        if not self.feed_address:
            raise PriceFeedError("CHAINLINK_FEED_ADDRESS is not set")
        if self.decimals is None:
            self.decimals = int(self._eth_call(DECIMALS_ID)[-1])

        if self.method == "latestAnswer":
            raw = self._eth_call(LATEST_ANSWER_ID)[-1]
            price_value = _scale_price(raw, self.decimals)
        else:
            raw = self._eth_call(LATEST_ROUND_DATA_ID)[1]
            price_value = _scale_price(raw, self.decimals)

        return datetime.now(timezone.utc), price_value

    def _eth_call(self, data: str) -> list[int]:
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "eth_call",
            "params": [
                {
                    "to": self.feed_address,
                    "data": data,
                },
                "latest",
            ],
        }
        response = requests.post(self.rpc_url, json=payload, timeout=self.timeout_sec)
        response.raise_for_status()
        result = response.json().get("result")
        if not result or not isinstance(result, str):
            raise PriceFeedError(f"Invalid RPC response: {response.text[:200]}")
        return _decode_words(result)


def _extract_json_path(payload: Any, path: str) -> Any:
    current = payload
    for segment in path.split("."):
        if not isinstance(current, dict) or segment not in current:
            raise PriceFeedError(
                f"JSON path '{path}' not found in response: {json.dumps(payload)[:500]}"
            )
        current = current[segment]
    return current


def _build_auth_headers(
    *,
    url: str,
    method: str,
    auth_mode: str,
    api_key: str,
    api_secret: str,
    key_header: str,
    signature_header: str,
    timestamp_header: str,
) -> dict[str, str]:
    if auth_mode.lower() != "hmac":
        return {}
    if not api_key or not api_secret:
        raise PriceFeedError("CHAINLINK_API_KEY/CHAINLINK_API_SECRET required for HMAC auth")
    timestamp = str(int(datetime.now(timezone.utc).timestamp()))
    parsed = urlparse(url)
    path = parsed.path or "/"
    if parsed.query:
        path = f"{path}?{parsed.query}"
    payload = f"{timestamp}{method.upper()}{path}"
    signature = hmac.new(
        api_secret.encode("utf-8"),
        payload.encode("utf-8"),
        hashlib.sha256,
    ).hexdigest()
    return {
        key_header: api_key,
        signature_header: signature,
        timestamp_header: timestamp,
    }


DECIMALS_ID = "0x313ce567"
LATEST_ANSWER_ID = "0x50d25bcd"
LATEST_ROUND_DATA_ID = "0xfeaf968c"


def _decode_words(hex_value: str) -> list[int]:
    clean = hex_value[2:] if hex_value.startswith("0x") else hex_value
    if len(clean) % 64 != 0:
        raise PriceFeedError(f"Unexpected ABI payload length: {len(clean)}")
    words = []
    for i in range(0, len(clean), 64):
        word = int(clean[i : i + 64], 16)
        words.append(_decode_int256(word))
    return words


def _decode_int256(value: int) -> int:
    if value >= 2**255:
        return value - 2**256
    return value


def _scale_price(value: int, decimals: int) -> float:
    if decimals <= 0:
        return float(value)
    return float(value) / (10**decimals)
