# Setup Notes

Follow the layout and workflow guidance in `universal_coding_assistant_setup_v005.md`. This project uses Python.
Use `pmb_coding_assistant_setup.md` as the Polymarket trading-bot setup template for new repos.

## Bootstrap
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Local Run
```bash
export CHAINLINK_PRICE_URL="<chainlink-price-endpoint>"
export CHAINLINK_PRICE_JSON_PATH="result.price"
export CHAINLINK_AUTH_MODE="hmac"
export CHAINLINK_API_KEY="<api-key>"
export CHAINLINK_API_SECRET="<api-secret>"
export FEED_MODE="http"
PYTHONPATH=src python -m pmbot.main
```

## Credentials
Use a local `.env` file for secrets (do not commit it). Supported keys include:
`CLOB_API_KEY`, `CLOB_API_SECRET`, `CLOB_API_PASSPHRASE`, `CLOB_FUNDER`, `CLOB_PRIVATE_KEY`.
Optional builder keys: `BUILDER_API_KEY`, `BUILDER_API_SECRET`, `BUILDER_API_PASSPHRASE`.
If you prefer to auto-generate API keys, set `CLOB_AUTO_CREATE_KEY=true` (requires `CLOB_PRIVATE_KEY`).

## Market Window Alignment
Set `MARKET_SLUG` to align the signal to the official 15-minute window.
Use `START_DELTA_THRESHOLD` to avoid trades on small moves from the window start.

## On-Chain Feed
```bash
export FEED_MODE="onchain"
export ETH_RPC_URL="<rpc-endpoint>"
export CHAINLINK_FEED_ADDRESS="<chainlink-aggregator-address>"
PYTHONPATH=src python -m pmbot.main
```

## MCP Server (Optional)
If you want MCP tooling for market data, run the server in `automation/polymarket-mcp/` and
provide its env vars (`KEY`, `FUNDER`) per its README.

Add any system dependencies or extended setup steps here as the project evolves.
