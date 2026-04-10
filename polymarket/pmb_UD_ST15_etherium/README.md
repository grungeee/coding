# Polymarket ETH 15m Trend Bot

This project is a Python trading bot that predicts 15-minute ETH/USD direction using Chainlink data streams, then trades the corresponding Polymarket market. The bot is intentionally conservative by default and requires explicit configuration to enable live trading.

## Project Setup Guide
See `pmb_coding_assistant_setup.md` for the reusable Polymarket trading-bot setup template.

## Quick Start
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

export CHAINLINK_PRICE_URL="<chainlink-price-endpoint>"
export CHAINLINK_PRICE_JSON_PATH="price"
PYTHONPATH=src python -m pmbot.main
```

## Live TUI Monitor
Run the bot with a curses TUI that shows signal, logs, and orderbook snapshots:
```bash
PYTHONPATH=src python -m pmbot.main --tui
```
Press `q` to exit, `p` to toggle paper/live, `t` to enable/disable trading, `x` to force exit, `r` to refresh (rate-limited). Logs are written to `bot.log` while the TUI is active.

Plain log mode (prints one line per tick):
```bash
PYTHONPATH=src python -m pmbot.main --plain
```

Update the market slug to the current 15-minute window:
```bash
python3 scripts/update_market_slug.py
```

## On-Chain Price Feed (Alternative)
If you prefer on-chain Chainlink reads, configure an Ethereum RPC endpoint and feed address:
```bash
export FEED_MODE="onchain"
export ETH_RPC_URL="<rpc-endpoint>"
export CHAINLINK_FEED_ADDRESS="<chainlink-aggregator-address>"
PYTHONPATH=src python -m pmbot.main
```

## Polymarket MCP (Optional)
An MCP server is available under `automation/polymarket-mcp/`. It exposes market data tools
but does not place orders. To run it locally, see `automation/polymarket-mcp/README.md`.

## Configuration
Environment variables control the runtime behavior:
- `FEED_MODE` (`http` or `onchain`): Price source mode.
- `CHAINLINK_PRICE_URL` (required for `http`): HTTP endpoint returning Chainlink ETH/USD data.
- `CHAINLINK_PRICE_JSON_PATH` (default `price`): Dot path to the price value in the JSON (e.g. `result.price`).
- `CHAINLINK_AUTH_MODE` (`none` or `hmac`): Enable HMAC auth for Data Streams.
- `CHAINLINK_API_KEY`, `CHAINLINK_API_SECRET`: Credentials for HMAC auth.
- `CHAINLINK_API_TIMESTAMP_HEADER` (default `X-API-TIMESTAMP`): Timestamp header name.
- `CHAINLINK_API_KEY_HEADER` (default `X-API-KEY`): API key header name.
- `CHAINLINK_API_SIGNATURE_HEADER` (default `X-API-SIGNATURE`): Signature header name.
- `CHAINLINK_FEED_ADDRESS` (required for `onchain`): Chainlink feed contract address.
- `ETH_RPC_URL` (required for `onchain`): Ethereum JSON-RPC endpoint.
- `CHAINLINK_FEED_DECIMALS` (optional): Override feed decimals if needed.
- `WINDOW_MINUTES` (default `15`): Trend window size.
- `POLL_INTERVAL_SEC` (default `10`): Sampling interval.
- `MIN_POINTS` (default `10`): Minimum samples before emitting a signal.
- `SLOPE_THRESHOLD` (default `0.0`): Minimum slope to consider a directional signal.
- `SQLITE_PATH` (default `bot_state.sqlite3`): Local storage for price history.
- `TRADER_MODE` (`paper` or `polymarket`): Execution mode.
- `TRADING_ENABLED` (default `false`): Enable live execution.
- `MARKET_ID`, `OUTCOME_UP_ID`, `OUTCOME_DOWN_ID`: Polymarket identifiers for live trading (`MARKET_ID` is the `conditionId`).
- `CLOB_HOST` (default `https://clob.polymarket.com`): CLOB API host.
- `CLOB_API_KEY`, `CLOB_API_SECRET`, `CLOB_API_PASSPHRASE`: CLOB API credentials.
- `CLOB_FUNDER`: Wallet address used as funder.
- `CLOB_PRIVATE_KEY`: Private key for order signing (required for trading).
- `BUILDER_API_KEY`, `BUILDER_API_SECRET`, `BUILDER_API_PASSPHRASE`: Optional builder signing creds.
- `CLOB_AUTO_CREATE_KEY` (default `false`): Call `create_api_key()` if API creds are missing.
- `ENTER_CONFIDENCE` (default `1.2`): Minimum confidence to enter.
- `HOLD_CONFIDENCE` (default `0.6`): Minimum confidence to keep holding.
- `MAX_SPREAD_PCT` (default `0.08`): Max bid/ask spread percentage allowed for entry.
- `MIN_EDGE` (default `0.03`): Minimum edge over implied price required for entry.
- `CONFIDENCE_SCALE` (default `2.0`): Smooths confidence into an edge estimate.
- `TRAILING_STOP_MULT` (default `1.5`): Volatility multiple for trailing stops.
- `MAX_HOLD_MINUTES` (default `20`): Exit after this duration.
- `COOLDOWN_MINUTES` (default `2`): Delay before re-entry after exit.
- `STAKE_USD` (default `1.0`): Nominal stake for each entry.
- `MIN_SHARES` (default `1`): Minimum shares when placing orders.
- `MARKET_SLUG`: Polymarket market slug to align to the exact 15-minute window.
- `START_DELTA_THRESHOLD` (default `0.0`): Minimum move from window start to signal direction.

## Notes
- The market resolves using Chainlink ETH/USD data streams. Ensure your `CHAINLINK_PRICE_URL` reflects that source.
- Live trading requires Polymarket API credentials, a signing private key, and outcome token IDs.
- Store credentials in a local `.env` file; do not commit secrets to the repo.
- This project uses Polymarket’s `py-clob-client` from GitHub (`requirements.txt`) as the CLOB client library.

## Getting Market IDs
Use the Gamma API to fetch the market and token IDs for a slug:
```bash
curl -s "https://gamma-api.polymarket.com/markets?slug=eth-updown-15m-1766543400" | jq .
```
Look for:
- `conditionId` (use as `MARKET_ID`)
- `clobTokenIds` aligned with `outcomes` (use as `OUTCOME_UP_ID` / `OUTCOME_DOWN_ID`)

Or run the helper script:
```bash
python3 scripts/get_market_ids.py eth-updown-15m-1766543400 --env
```

To verify a Chainlink feed address on Polygon:
```bash
python3 scripts/check_chainlink_feed.py https://polygon-rpc.com 0xF9680D99D6C9589e2a93a78A04A279e509205945
```

## Status
Core loop, storage, and strategy logic are implemented. Polymarket execution still needs API wiring.
