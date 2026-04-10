#!/usr/bin/env python3
"""Update MARKET_SLUG to the current 15-minute ETH market window."""

from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path


def _window_start_epoch(now: datetime) -> int:
    ts = int(now.timestamp())
    return ts - (ts % 900)


def _load_env(path: Path) -> list[str]:
    if not path.exists():
        return []
    return path.read_text().splitlines()


def _write_env(path: Path, lines: list[str]) -> None:
    path.write_text("\n".join(lines) + "\n")


def main() -> None:
    env_path = Path(".env")
    lines = _load_env(env_path)
    slug_value = f"eth-updown-15m-{_window_start_epoch(datetime.now(timezone.utc))}"
    key = "MARKET_SLUG"
    updated = False
    for i, line in enumerate(lines):
        if line.startswith(f"{key}="):
            lines[i] = f"{key}={slug_value}"
            updated = True
            break
    if not updated:
        lines.append(f"{key}={slug_value}")
    _write_env(env_path, lines)
    print(f"MARKET_SLUG={slug_value}")


if __name__ == "__main__":
    main()
