"""Derive Polymarket CLOB API credentials using py-clob-client.

This script uses a private key and optional funder address to derive
API credentials. It does not place trades.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Dict

from py_clob_client.client import ClobClient


def _load_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8").strip()


def _load_json(path: Path) -> Dict[str, Any]:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}


def _resolve_secret(env_key: str, path: Path, json_keys: tuple[str, ...]) -> str:
    value = os.environ.get(env_key, "").strip()
    if value:
        return value

    raw_json = _load_json(path)
    for key in json_keys:
        candidate = raw_json.get(key)
        if isinstance(candidate, str) and candidate.strip():
            return candidate.strip()

    return _load_text(path)


def _serialize_credentials(creds: Any) -> Dict[str, Any]:
    if isinstance(creds, dict):
        return creds
    if hasattr(creds, "model_dump"):
        return creds.model_dump()
    if all(hasattr(creds, attr) for attr in ("api_key", "api_secret", "api_passphrase")):
        return {
            "api_key": getattr(creds, "api_key"),
            "api_secret": getattr(creds, "api_secret"),
            "api_passphrase": getattr(creds, "api_passphrase"),
        }
    if hasattr(creds, "__dict__"):
        return dict(creds.__dict__)
    return {"value": str(creds)}


def main() -> int:
    host = os.environ.get("CLOB_API_URL", "https://clob.polymarket.com")
    signature_type = int(os.environ.get("CLOB_SIGNATURE_TYPE", "1"))

    key = _resolve_secret(
        "CLOB_PRIVATE_KEY",
        Path("credentials/.magiclink_credentials"),
        ("private_key", "key"),
    )
    funder = _resolve_secret(
        "CLOB_FUNDER",
        Path("credentials/.funder_address"),
        ("funder", "address", "funder_address"),
    )

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

    api_credentials = _serialize_credentials(client.derive_api_key())

    output_path = Path("credentials/.clob_credentials")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(api_credentials, indent=2), encoding="utf-8")
    print(f"Saved credentials to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
