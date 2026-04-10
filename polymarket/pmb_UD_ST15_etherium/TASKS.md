# Tasks

## Active
- Confirm Chainlink ETH/USD data stream endpoint and JSON shape.
- Validate live Polymarket order placement with real credentials.
- Add strategy tuning (thresholds, volatility filters).

## Done
- Add market-odds checks (spread/price alignment) before entering trades.
- Subtask: Define the odds/edge rule (max spread %, min edge vs implied probability).
- Add market-window-aligned entry/exit refinement (trade within the official 15-minute window).

## Backlog
- Add unit tests for signal generation and storage.
- Add metrics/telemetry export.
