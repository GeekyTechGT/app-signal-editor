# Semantic Versioning

Signal Editor follows [Semantic Versioning 2.0.0](https://semver.org/).

## Version Format

```text
MAJOR.MINOR.PATCH
```

| Component | Increment when |
|-----------|----------------|
| `MAJOR` | introducing breaking API, format, or workflow changes |
| `MINOR` | adding backward-compatible functionality |
| `PATCH` | fixing bugs without changing expected compatibility |

## Authoritative Version Sources

Keep these aligned:

1. `VERSION`
2. `CMakeLists.txt` project version
3. `CHANGELOG.md` release entries

The repository also defines a module version through `MYPRJ_SIGNAL_EDITOR_VERSION` in `CMakeLists.txt`.

## When a Version Bump Is Expected

Increase the version when changes affect at least one of these areas:

- public module APIs
- CSV persistence semantics
- user-visible GUI behavior
- supported build/test workflows
- packaging or deployment outputs

## Changelog Policy

Every release should describe:

- what changed
- why it matters
- whether compatibility was affected
- whether user workflow or data format expectations changed
