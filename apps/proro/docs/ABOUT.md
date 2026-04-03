# proro — Overview

TODO: Describe what this module does and the problem it solves.

## Features

- TODO: list key features

## Usage

### CLI

```bash
proro --id <id> --name <name> [--config <path>]
proro --help
```

### GUI

Launch `proro-gui` for the graphical interface.

## Configuration

The module reads a JSON config file with the following structure:

```json
{
    "data_dir": "./data"
}
```

## Architecture

This module follows hexagonal architecture:

- `src/proro/core/` — domain entities and use cases (no I/O)
- `src/proro/ports/` — abstract repository interfaces
- `src/proro/adapters/` — filesystem, JSON, CLI, and GUI adapters
- `src/proro/api/` — public entry point for applications
