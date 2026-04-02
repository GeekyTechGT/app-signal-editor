# my_module — Overview

TODO: Describe what this module does and the problem it solves.

## Features

- TODO: list key features

## Usage

### CLI

```bash
my_module --id <id> --name <name> [--config <path>]
my_module --help
```

### GUI

Launch `my_module-gui` for the graphical interface.

## Configuration

The module reads a JSON config file with the following structure:

```json
{
    "data_dir": "./data"
}
```

## Architecture

This module follows hexagonal architecture:

- `src/my_module/core/` — domain entities and use cases (no I/O)
- `src/my_module/ports/` — abstract repository interfaces
- `src/my_module/adapters/` — filesystem, JSON, CLI, and GUI adapters
- `src/my_module/api/` — public entry point for applications
