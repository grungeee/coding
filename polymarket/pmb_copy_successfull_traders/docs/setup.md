# Setup

## Local Environment
- Python 3.11
- Virtual environment and dependencies:
  `python -m venv .venv && source .venv/bin/activate && pip install -r requirements.txt`

## Configuration
1. Copy `.env.example` to `.env` and fill in credentials when ready.
2. Update `config/bot_config.json` with the trader profile IDs to follow.
3. Adjust risk limits and polling interval to match your scale.
4. Derive CLOB API credentials (if using Magic or proxy-based login):
   `CLOB_PRIVATE_KEY=... CLOB_FUNDER=... CLOB_SIGNATURE_TYPE=1 python scripts/derive_clob_credentials.py`
5. Optional: store the funder address in `credentials/.funder_address` if you do not want it in env vars.
6. Optional: enable leaderboard auto-selection in `config/bot_config.json` to follow top traders by PnL/volume.

## Run
`PYTHONPATH=src python -m pmbot.main`

## Notes
- `ENABLE_LIVE_TRADING=false` keeps the bot in no-op mode.
- Wire the live API endpoints in `src/pmbot/polymarket_client.py` before enabling trading.
- Validate connectivity with `python3.11 scripts/check_clob_connectivity.py`.
