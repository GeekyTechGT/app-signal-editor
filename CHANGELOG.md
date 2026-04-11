# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and the project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Workspace tab navigation that separates plot and table workflows into dedicated views
- Shared interpolation control positioned at workspace level so it remains available in both editing contexts
- Multi-format enumerated signal fixtures in `tests/01.data/` for CSV, inline CSV, JSON, and SpreadsheetML XML
- Explicit repository governance documentation through `GOVERNANCE.md`
- Expanded repository guidance aligned with the real Signal Editor product rather than scaffold placeholders
- Repository review scaffolding through `CODEOWNERS` and a pull request template
- A tag-driven release packaging workflow that archives delivery artifacts for controlled distribution

### Changed
- Refined the workspace UX to reduce header density, improve space usage, and replace the plot/table splitter with tab-based navigation
- Updated documentation across product, architecture, governance, contribution, security, and guideline areas to describe the actual current implementation in detail
- Tightened the tab presentation and transition behavior for a more polished and less intrusive workspace switch
- Moved interpolation selection out of the table-specific panel into a shared workspace control area
- Strengthened GitHub Actions CI with Windows validation, Linux Clang test execution, and GCC coverage artifact generation
- Simplified shared common-types compilation by removing an unnecessary translation unit

### Fixed
- Resolved test warnings related to `nodiscard` usage in unit tests
- Corrected Qt build regressions introduced during the workspace navigation refactor
- Removed transient tab-transition bleed-through caused by transparent tab-page rendering
- Restored linear interpolation when enum mappings are explicitly removed from a signal

### Removed
- Stale scaffold and template-centric repository guidance from active documentation
- Table-local interpolation control in favor of a shared workspace-level control

## [0.1.0] - 2026-04-10

### Added
- Initial Signal Editor implementation with CSV import/export, interactive plot editing, and unit test coverage
