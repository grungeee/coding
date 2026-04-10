# PMB Coding Assistant Project Setup Guide

This guide adapts `universal_coding_assistant_setup_v005.md` for initializing Polymarket
trading-bot projects. Use it as the default blueprint when starting a new repo.

## Repository Layout Patterns
- **Core anchors**: keep `README.md`, `TASKS.md`, and `docs/` at the top level.
- **Prompt logs**: add `docs/vibe/` only when prompt work or agent ideation is in scope.
- **Application code**: use `src/` for runtime code and `tests/` for pytest suites.
- **Automation**: place scripts in `scripts/` and heavier workflows in `automation/`.
- **Optional extras**: add `assets/`, `examples/`, or `infra/` only when they contain
  real artifacts; document deviations in `docs/setup.md`.

## Environment Bootstrap Checklist
1. **Runtime**: standardize on Python 3.11 (or document a different version in
   `.tool-versions` or `docs/setup.md`).
2. **Local setup**:
   ```bash
   python -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   ```
3. **System dependencies**: document any RPC providers, Chainlink feeds, or
   OS-level tools in `docs/setup.md`.
4. **Optional tooling**: if you add Docker, dev containers, or VS Code tasks,
   mark them as opt-in and keep them aligned with the primary setup flow.

## Tooling & Automation
- Prefer canonical entrypoints like `scripts/bootstrap.sh`, `scripts/lint.sh`, or
  `make` targets once they exist; document the exact commands here.
- Keep configuration in versioned files (`pyproject.toml`, `ruff.toml`, etc.)
  instead of CLI flags so automation stays reproducible.

## Development Workflow
- **Branching**: use `feat/<slug>` or `chore/<slug>` unless the project defines
  another standard.
- **Commits**: follow Conventional Commits (`feat:`, `fix:`, `chore:`).
- **PRs**: keep them small, summarize changes, and note validation commands.

## Testing & Quality Gates
- Use `pytest` by default; store tests in `tests/` with `test_*.py` naming.
- Share fixtures in `tests/fixtures/` when data setup is reusable.
- Record coverage targets or intentional gaps in `docs/setup.md`.

## Polymarket Trading Bot Defaults
- **Secrets**: store API keys in `.env` and add `.env` to `.gitignore`.
- **Environment variables** (document in `.env.example`):
  - `CLOB_API_KEY`, `CLOB_API_SECRET`, `CLOB_API_PASSPHRASE`, `CLOB_FUNDER`
  - `CLOB_PRIVATE_KEY` (required if auto-generating keys)
  - `CLOB_AUTO_CREATE_KEY=true` (optional)
  - `CHAINLINK_PRICE_URL`, `CHAINLINK_PRICE_JSON_PATH`
  - `CHAINLINK_AUTH_MODE`, `CHAINLINK_API_KEY`, `CHAINLINK_API_SECRET`
  - `FEED_MODE` (`http` or `onchain`)
  - `ETH_RPC_URL`, `CHAINLINK_FEED_ADDRESS`
- **Market alignment**: document how you map `MARKET_SLUG` and any windowing
  parameters (for example `START_DELTA_THRESHOLD`).
- **Local run**: add a canonical command to `docs/setup.md` once the app entry
  point is finalized (for example `PYTHONPATH=src python -m pmbot.main`).

## Documentation & Knowledge Capture
- Keep setup notes, ADRs, and architecture references in `docs/`.
- Track active prompts or experiments in `docs/vibe/active/` and link them from
  `TASKS.md` when relevant.
- Update this guide if you introduce new folders, tooling, or workflow changes.

## Release & Readiness Checklist
- Run lint/format commands and `pytest` before release.
- Regenerate snapshots or fixtures if the data model changes.
- Verify external references (RPC endpoints, feeds, market ids) are correct.

