# Semantic Versioning

This project follows [Semantic Versioning 2.0.0](https://semver.org/).

## Version Format

```
MAJOR.MINOR.PATCH
```

| Component | When to increment                                                                 |
|-----------|-----------------------------------------------------------------------------------|
| `MAJOR`   | Incompatible API changes that break existing callers                              |
| `MINOR`   | New backward-compatible functionality                                             |
| `PATCH`   | Backward-compatible bug fixes                                                     |

## Pre-release Labels

| Label      | Meaning                               | Example         |
|------------|---------------------------------------|-----------------|
| `alpha`    | Early development, unstable           | `1.1.0-alpha.1` |
| `beta`     | Feature-complete, may have bugs       | `1.1.0-beta.2`  |
| `rc`       | Release candidate, final testing      | `1.1.0-rc.1`    |

## Version Files

The authoritative version is stored in two places:

1. `VERSION` — plain text, used for scripting
2. `CMakeLists.txt` — `project(MyProject VERSION x.y.z)`, used for build-time header generation

Keep both files in sync. The CMake `configure_file()` call generates `include/myprj/version.h` from the CMake project version.

## Changelog Policy

Every version increment must include a `CHANGELOG.md` entry describing:
- What changed
- Why it changed
- Whether the change is breaking

## Module Versions

Individual modules have independent version variables in `CMakeLists.txt`:

```cmake
set(MYPRJ_MY_MODULE_VERSION "1.0.0")
```

Module versions follow the same MAJOR.MINOR.PATCH convention and are independent from the project version.
