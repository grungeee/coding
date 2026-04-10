"""Resolve Polymarket profile handles to wallet addresses."""
from __future__ import annotations

import logging
import re
from dataclasses import dataclass
from typing import Optional

import requests

LOGGER = logging.getLogger(__name__)

ADDRESS_RE = re.compile(r"0x[a-fA-F0-9]{40}")


@dataclass(frozen=True)
class ProfileAddress:
    username: str
    proxy_address: str
    base_address: str | None
    primary_address: str | None


def resolve_profile_address(handle: str) -> Optional[ProfileAddress]:
    username = handle.lstrip("@").strip()
    if not username:
        return None

    url = f"https://polymarket.com/@{username}"
    try:
        resp = requests.get(url, timeout=10)
        resp.raise_for_status()
    except requests.RequestException as exc:
        LOGGER.warning("Profile fetch failed for %s: %s", username, exc)
        return None

    text = resp.text
    proxy_match = re.search(r"\"proxyAddress\":\"(0x[a-fA-F0-9]{40})\"", text)
    base_match = re.search(r"\"baseAddress\":\"(0x[a-fA-F0-9]{40})\"", text)
    primary_match = re.search(r"\"primaryAddress\":\"(0x[a-fA-F0-9]{40})\"", text)

    proxy_address = proxy_match.group(1) if proxy_match else None
    if not proxy_address:
        # Fall back to any address found in the HTML if proxy address is missing.
        candidates = ADDRESS_RE.findall(text)
        proxy_address = candidates[0] if candidates else None

    if not proxy_address:
        LOGGER.warning("No address found for %s", username)
        return None

    return ProfileAddress(
        username=username,
        proxy_address=proxy_address,
        base_address=base_match.group(1) if base_match else None,
        primary_address=primary_match.group(1) if primary_match else None,
    )


def resolve_trader_id(entry: str) -> Optional[str]:
    value = entry.strip()
    if not value:
        return None
    if value.startswith("0x") and len(value) == 42:
        return value
    profile = resolve_profile_address(value)
    if profile is None:
        return None
    return profile.proxy_address
