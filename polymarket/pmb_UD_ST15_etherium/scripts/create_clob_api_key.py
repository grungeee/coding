#!/usr/bin/env python3
"""Create CLOB API key and persist it to .env without printing secrets."""

from __future__ import annotations

import os
from pathlib import Path

from dotenv import load_dotenv


def _load_env_file(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    if not path.exists():
        return data
    for line in path.read_text().splitlines():
        if not line or line.lstrip().startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def _write_env_file(path: Path, data: dict[str, str]) -> None:
    lines: list[str] = []
    if path.exists():
        lines = path.read_text().splitlines()
    keys_written: set[str] = set()
    for idx, line in enumerate(lines):
        if not line or line.lstrip().startswith("#") or "=" not in line:
            continue
        key = line.split("=", 1)[0].strip()
        if key in data:
            lines[idx] = f"{key}={data[key]}"
            keys_written.add(key)
    for key, value in data.items():
        if key not in keys_written:
            lines.append(f"{key}={value}")
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    load_dotenv()
    env_path = Path(".env")
    env_data = _load_env_file(env_path)

    private_key = os.getenv("CLOB_PRIVATE_KEY") or env_data.get("CLOB_PRIVATE_KEY")
    funder = os.getenv("CLOB_FUNDER") or env_data.get("CLOB_FUNDER")
    host = os.getenv("CLOB_HOST") or env_data.get("CLOB_HOST") or "https://clob.polymarket.com"

    if not private_key or not funder:
        raise SystemExit("Missing CLOB_PRIVATE_KEY or CLOB_FUNDER in .env")

    from py_clob_client.client import ClobClient
    from py_clob_client.constants import POLYGON

    client = ClobClient(
        host,
        key=private_key,
        chain_id=POLYGON,
        funder=funder,
        creds=None,
    )
    creds = client.create_api_key()

    updates = {
        "CLOB_API_KEY": creds.api_key,
        "CLOB_API_SECRET": creds.api_secret,
        "CLOB_API_PASSPHRASE": creds.api_passphrase,
        "CLOB_HOST": host,
    }
    _write_env_file(env_path, updates)
    print("Updated .env with CLOB API credentials.")


if __name__ == "__main__":
    main()
