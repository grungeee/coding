# Universal Coding Assistant Project Setup Guide

<!-- CHANGELOG: v005 (2024-05-19) – validated the versioned naming workflow and documented the rename procedure; no content updates beyond metadata adjustments. Ignore these comments when bootstrapping a new project; review them only when updating the guide. -->

This living guide captures a reusable blueprint for standing up new coding-assistant projects while remaining adaptable to different languages, frameworks, and deployment targets. Treat each section as a menu of conventions to assemble the right workflow for the project at hand.

## Repository Layout Patterns
- **Core anchors**: keep top-level files (`README.md`, `TASKS.md`, `Makefile` or equivalent automation entrypoint) and a `docs/` space for briefs, ADRs, and setup notes. Pair them with a `docs/vibe/` log when prompt work or agent ideation is part of the scope.
- **Domain modules**: create stack-aligned directories only when they house committed code. Examples include `src/` or `lib/` for application logic, `web/` or `ui/` for front-end surfaces, `agents/` for prompt bundles, and `automation/` for scripted workflows.
- **Quality & testing**: co-locate test suites beside the code they validate (`tests/`, `spec/`, `e2e/`, etc.) and structure fixtures under a dedicated subfolder (for example `tests/fixtures/` or `automation/fixtures/`).
- **Operational extras**: introduce `infra/`, `config/`, `examples/`, or `assets/` only when the project needs them. For shared media, nest assets under `assets/media/` and use relative links.
- Document any deviations from these patterns in `docs/setup.md` so future projects can judge whether the variation is universal-worthy.

## Environment Bootstrap Checklist
1. Declare the primary runtime(s) (for example Node 20, Python 3.11, Go 1.22). Capture multi-runtime needs in `.tool-versions`, `.nvmrc`, or a dedicated setup doc.
2. Provide language-specific bootstraps that follow the same shape:
   ```bash
   # Example (replace with the stack in use)
   python -m venv .venv && source .venv/bin/activate
   pip install -r requirements.txt
   # or
   npm install && npm run prepare
   ```
   Keep these recipes in `scripts/bootstrap.*` or `make bootstrap` so they stay executable.
3. List any system dependencies (browsers, databases, playwright drivers) with platform notes in `docs/setup.md`.
4. Record optional accelerators (Docker Compose, dev containers, VS Code tasks) but flag them as opt-in when they are not required.

## Tooling & Automation
- Choose formatters, linters, and static analyzers that match the stack (for example `prettier`, `eslint`, `stylelint`, `ruff`, `mypy`, `golangci-lint`). Note the authoritative command names in the guide and in automation scripts.
- Define canonical scripts (`make lint`, `npm run lint`, `just test`, etc.) so contributors and CI call the same entrypoints. When tools change, update both the command and this document.
- Capture reusable automation (snapshot capture, prompt generation, deployment) under `automation/` or `scripts/`, and document inputs/outputs at the top of each script.
- Favor configuration files (`.htmlhintrc`, `.eslintrc`, `pyproject.toml`) over inline flags so new projects can adopt or extend them easily.

## Development Workflow
- Branch naming stays flexible: document the preferred pattern (`feat/<slug>`, `chore/<slug>`, etc.) per project and reference it here.
- Commits should follow Conventional Commits unless the project charter defines a different style; call out exceptions explicitly.
- Encourage small, reviewable pull requests with clear validation notes (commands run, screenshots, recordings) tailored to the surface that changed.
- For prompt and agent work, track active briefs in `docs/vibe/active/`, link them from `TASKS.md`, and archive once complete.

## Testing & Quality Gates
- Establish baseline expectations (unit, integration, end-to-end) and map them to tooling by stack (`pytest`, `vitest`, `playwright`, `go test`).
- Require both happy-path and failure-mode coverage where feasible. Share data builders and fixtures to reduce duplication.
- Set coverage targets appropriate to the project phase; document intentional gaps or untestable flows with references to ADRs or issues.
- Keep CI pipelines aligned with local commands: the same `make` target or package script should run locally and in automation.

## Documentation & Knowledge Capture
- Maintain ADRs, setup notes, and architecture references under `docs/`. Highlight which pieces are universal defaults versus project-specific overrides.
- Capture reusable prompts, experiment logs, and alternative ideas under `docs/vibe/` so future assistants can reuse or refine them.
- Keep `TASKS.md` in sync with active workstreams and link to the supporting docs, scripts, or forms.
- When introducing new tooling or conventions, update both the relevant doc and this setup guide to reflect the broadened “universal” baseline.

## Release & Readiness Checklist
Before requesting review or publishing artifacts, confirm the project’s agreed validation gates. Typical checkpoints include:
- Run the designated lint and format commands.
- Execute the primary test suites (unit, integration, E2E) and capture coverage if required.
- Regenerate snapshots, fixtures, or documentation affected by the change.
- Verify release notes, changelog entries, and external references remain accurate.

## Continuous Improvement
- Treat this guide as a changelog-backed artifact. After each project, record lessons learned, new directory patterns, or tooling adjustments so subsequent teams inherit the improvements.
- Append update dates and context in a short section at the bottom (for example `## Revision History`) when material changes ship.
- Encourage contributors to propose refinements through pull requests referencing the project that motivated the update. Reviewers should check that examples remain stack-agnostic or clearly marked as stack-specific.
- When modifying the guide, increment the filename suffix (for example `universal_coding_assistant_setup_v006.md`), update the changelog comment near the top, and log the change in the revision history below. Skip these comments when initiating a fresh project, but consult them before making further edits.

By evolving these instructions as new projects uncover fresh requirements, the setup stays broadly applicable while still giving each coding-assistant effort the freedom to adopt the right tools and structure.

## Revision History
- **v005** (2024-05-19): Renamed the guide to follow the versioned filename convention and confirmed the changelog workflow.
- **v004** (2024-05-19): Generalized the setup guidance across stacks, added versioning expectations, and formalized continuous improvement practices.
