# Repository Guidelines

## Project Structure & Module Organization
Follow the layout patterns in `universal_coding_assistant_setup_v005.md` and keep top-level anchors in place. For this Python project, preferred defaults are:
- `README.md`, `TASKS.md`, and `docs/` (add `docs/vibe/` if prompt logs are needed).
- `src/` for application code and `tests/` for test suites.
- Optional folders such as `assets/`, `examples/`, `automation/`, or `infra/` only when they contain real assets or scripts.

Document any deviations in `docs/setup.md` once it exists.

## Build, Test, and Development Commands
No scripts are defined yet. When you add tooling, use canonical, repeatable commands and document them here. Suggested Python defaults:
- `python -m venv .venv && source .venv/bin/activate` to create/activate a local venv.
- `pip install -r requirements.txt` to install dependencies.
- `pytest` to run tests.

Prefer `make` or `scripts/bootstrap.*` when you need repeatable setup.

## Coding Style & Naming Conventions
Until tools are defined, stick to standard Python conventions:
- 4-space indentation, `snake_case` for functions/modules, `PascalCase` for classes.
- Keep modules small and focused; align file names with their primary responsibility.

If you add formatters/linters (e.g., `ruff`, `black`, `mypy`), list the exact commands and config files here.

## Testing Guidelines
Use `pytest` unless the project specifies otherwise. Recommended conventions:
- Test files named `test_*.py` under `tests/`.
- Use fixtures in `tests/fixtures/` when data setup is shared.
- Document any coverage targets and the command used to enforce them.

## Commit & Pull Request Guidelines
Follow the workflow in `universal_coding_assistant_setup_v005.md`:
- Use Conventional Commits unless the project charter states otherwise.
- Keep PRs small and include a summary plus validation notes (commands run).
- Link issues or specs when applicable; add screenshots for UI changes.

## Security & Configuration Tips
Store secrets in `.env` files and add them to `.gitignore`. Provide `.env.example` with required keys, and document any system dependencies in `docs/setup.md`.

## Agent-Specific Instructions
This repository uses `AGENTS.md` as the contributor guide. Update this file whenever you add new directories, tooling, or workflow changes.
