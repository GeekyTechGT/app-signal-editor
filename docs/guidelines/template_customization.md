# Repository Finalization Notes

This repository is no longer treated as a generic scaffold. The primary customization work has already been completed for the `Signal Editor` product.

## Current Canonical Project Identity

- Product name: `Signal Editor`
- CMake project: `SignalEditor`
- Namespace prefix: `myprj`
- Primary module: `signal_editor`

## What This Means for Contributors

- Do not reintroduce scaffold placeholders such as `MyProject` or `my_module` into active project docs.
- Keep new documentation aligned with the real module names and targets already used in the repository.
- If additional modules are introduced later, document them explicitly instead of copying template wording.

## Cleanup Expectation

When touching legacy files, prefer removing stale scaffold language rather than preserving it for historical reasons.
