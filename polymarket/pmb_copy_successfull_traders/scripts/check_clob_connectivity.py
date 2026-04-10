"""Check connectivity to Polymarket CLOB using generated credentials."""
from __future__ import annotations

import json
import os
from pathlib import Path

from py_clob_client.client import ClobClient
from py_clob_client.clob_types import ApiCreds


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


def load_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8").strip()


def main() -> int:
    host = os.environ.get("CLOB_API_URL", "https://clob.polymarket.com")
    signature_type = int(os.environ.get("CLOB_SIGNATURE_TYPE", "1"))
    key = os.environ.get("CLOB_PRIVATE_KEY", "") or load_text(Path("credentials/.magiclink_credentials"))
    if not key:
        raise SystemExit("CLOB_PRIVATE_KEY is required")

    funder = os.environ.get("CLOB_FUNDER", "") or load_text(Path("credentials/.funder_address"))
    if signature_type in (1, 2) and not funder:
        raise SystemExit("CLOB_FUNDER is required for signature_type 1 or 2")

    if signature_type == 1:
        client = ClobClient(host, key=key, chain_id=137, signature_type=1, funder=funder)
    elif signature_type == 2:
        client = ClobClient(host, key=key, chain_id=137, signature_type=2, funder=funder)
    else:
        client = ClobClient(host, key=key, chain_id=137)

    client.set_api_creds(load_creds())
    ok = client.get_ok()
    server_time = client.get_server_time()
    print(f"CLOB OK: {ok}")
    print(f"Server time: {server_time}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
