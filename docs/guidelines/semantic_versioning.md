# Semantic Versioning

Signal Editor follows [Semantic Versioning 2.0.0](https://semver.org/).

## Version Format

```text
MAJOR.MINOR.PATCH
```

## Version Meaning

| Component | Increment when |
|-----------|----------------|
| `MAJOR` | introducing breaking changes to public behavior, persistence semantics, supported workflows, or integration expectations |
| `MINOR` | adding backward-compatible functionality or materially expanding product capability |
| `PATCH` | fixing defects, polishing UX, or improving documentation without breaking expected compatibility |

## What Counts as Breaking in This Repository

For Signal Editor, breaking change assessment is not limited to C++ API signatures.

A change may justify a major version bump if it breaks expectations around:

- supported import/export format semantics
- interpolation behavior
- enumerated signal encoding and decoding
- user workflows that scripts or teams rely on operationally
- build or packaging expectations for supported environments
- settings persistence scope or cross-version compatibility guarantees

## Authoritative Version Sources

Keep these aligned whenever a release is cut:

1. `VERSION`
2. top-level `CMakeLists.txt` project version
3. `CHANGELOG.md`

## Release Documentation Policy

A release should communicate:

- what changed
- why it matters
- whether compatibility changed
- whether user workflow or file-format expectations changed
- whether any constraints or migration notes apply
- whether the settings namespace changed and whether preferences are intentionally isolated by version

## Typical Version Decisions

Examples:

- adding JSON or SpreadsheetML support: usually `MINOR`
- fixing a parsing bug without changing the supported contract: usually `PATCH`
- improving import diagnostics without changing accepted file semantics: usually `PATCH`
- adding dense-signal LOD rendering while preserving signal data semantics: usually `MINOR`
- changing the CSV metadata contract in a backward-incompatible way: likely `MAJOR`
- reworking workspace navigation without breaking file semantics: usually `MINOR` or `PATCH`, depending on scope and release policy
