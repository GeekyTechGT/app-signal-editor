# Repository Customization and Identity

## Purpose

This repository originated from a structured project template, but it is now the active home of the Signal Editor product. This document explains the identity rules contributors should preserve so the repository does not drift back toward scaffold-style ambiguity.

## Canonical Project Identity

- Product name: `Signal Editor`
- CMake project: `SignalEditor`
- Namespace root: `signal_editor`
- Primary module: `signal_editor`
- GUI target family: `signal_editor_*`

## Customization Rules

Contributors should preserve the repository’s real identity in all active project artifacts.

That means:

- do not reintroduce placeholder names such as `SignalEditor`, `Signal Editor`, `my_module`, or generic scaffold prose
- do not describe the repository as a reusable template unless you are explicitly documenting historical context
- do not leave architecture docs at a generic level when the implementation is already specific
- do not leave agent or contributor instructions describing a scaffold instead of Signal Editor

## What Should Be Updated When Identity Changes

If the repository’s active product identity changes in the future, the following artifacts must be updated together:

- `README.md`
- `GOVERNANCE.md`
- `CONTRIBUTING.md`
- `SECURITY.md`
- `CHANGELOG.md`
- `CLAUDE.md`
- `docs/product/*`
- `docs/specs/srs.md`
- `docs/architecture/*`
- naming and versioning guidance

## Cleanup Rule

When touching legacy files, prefer removing stale scaffold language rather than preserving it for sentimental or historical reasons. The active repository should optimize for clarity to current contributors, not for template nostalgia.
