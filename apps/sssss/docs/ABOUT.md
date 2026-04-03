# sssss — Overview

TODO: Describe what this module does and the problem it solves.

## Features

- TODO: list key features

## Usage

### CLI

```bash
sssss --id <id> --name <name> [--config <path>]
sssss --help
```

### GUI

Launch `sssss-gui` for the graphical interface.

## Configuration

The module reads a JSON config file with the following structure:

```json
{
    "data_dir": "./data"
}
```

## Architecture

This module follows hexagonal architecture:

- `src/sssss/core/` — domain entities and use cases (no I/O)
- `src/sssss/ports/` — abstract repository interfaces
- `src/sssss/adapters/` — filesystem, JSON, CLI, and GUI adapters
- `src/sssss/api/` — public entry point for applications
