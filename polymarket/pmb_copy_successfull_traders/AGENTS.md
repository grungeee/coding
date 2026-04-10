# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds runtime code (Python modules and entry points).
- `tests/` contains pytest suites, named `test_*.py`; shared fixtures can live in
  `tests/fixtures/`.
- `scripts/` and `automation/` are for repeatable workflows and heavier tasks.
- `docs/` keeps setup notes, ADRs, and reference material; `docs/vibe/` is only
  used when prompt work or agent ideation is tracked.
- Top-level anchors are expected: `README.md`, `TASKS.md`, and `docs/`.

## Build, Test, and Development Commands
Use the standard Python 3.11 workflow unless the repo documents a different version.

- Create a virtualenv and install dependencies:
  `python -m venv .venv && source .venv/bin/activate && pip install -r requirements.txt`
- Run tests (when present): `pytest`
- Run the app entry point (once defined): e.g. `PYTHONPATH=src python -m pmbot.main`

If `scripts/` or `Makefile` commands exist, prefer those for consistency.

## Coding Style & Naming Conventions
- Indentation: 4 spaces (Python default).
- Filenames: `snake_case.py` for modules, `test_*.py` for tests.
- Config files should live in versioned files (`pyproject.toml`, `ruff.toml`, etc.)
  rather than CLI flags.

## Testing Guidelines
- Framework: `pytest`.
- Name tests as `test_<behavior>.py` and functions as `test_<scenario>()`.
- Keep reusable data in `tests/fixtures/` when it reduces duplication.

## Commit & Pull Request Guidelines
- Commits follow Conventional Commits: `feat:`, `fix:`, `chore:`, etc.
- Branches use `feat/<slug>` or `chore/<slug>` unless a different standard is noted.
- PRs should be small, include a concise summary, and list validation commands run
  (e.g. `pytest`). Link related issues when available.

## Security & Configuration Tips
- Store secrets in `.env` and keep it out of version control.
- Document required environment variables in `.env.example` (e.g. CLOB/Chainlink
  settings and RPC URLs).
