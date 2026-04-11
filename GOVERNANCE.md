# Governance

## Purpose

This document defines how the Signal Editor repository is governed, how technical decisions are made, and what standards are used when evolving the codebase, documentation, and release process.

The repository is operated as a production-oriented engineering asset rather than a casual code sample. Governance therefore exists to protect three things:

- product clarity
- architectural integrity
- change accountability

## Governance Principles

All material repository decisions should optimize for the following principles:

### 1. Product usefulness over ornamental complexity

Signal Editor exists to solve concrete engineering editing problems. Features should be accepted because they improve real editing workflows, data fidelity, or maintainability, not because they are visually novel or technically fashionable.

### 2. Stable architecture over short-term convenience

The repository intentionally uses a hexagonal architecture. Contributors are expected to preserve the dependency direction between domain, use cases, ports, and adapters even when faster but less disciplined alternatives exist.

### 3. Documentation as part of the product

Documentation is not treated as a secondary artifact. Requirements, architecture, governance, contribution rules, and format contracts must stay aligned with the actual implementation.

### 4. Explicit change accountability

User-visible behavior, persistence semantics, and workflow changes must be visible in version history and supporting documentation. Silent behavior drift is not acceptable.

## Repository Roles

The exact people filling these roles may vary by organization, but the responsibilities should remain stable.

### Maintainer

The maintainer is responsible for:

- accepting or rejecting changes
- protecting the architectural direction of the repository
- ensuring releases are coherent and documented
- coordinating security-sensitive fixes
- deciding when breaking changes are acceptable

### Contributor

A contributor is responsible for:

- understanding the existing architecture before editing it
- keeping changes scoped and technically coherent
- adding or updating tests where behavior changes
- updating documentation when meaningfully affected
- avoiding repository churn unrelated to the stated task

### Reviewer

A reviewer is responsible for:

- evaluating correctness, regressions, and design impact
- checking whether the change respects repository boundaries
- verifying that tests and docs match the implementation
- identifying hidden workflow or compatibility risks

## Decision-Making Model

### Default rule

Changes should be decided by maintainers based on technical fit with the documented product and architecture.

### Decision inputs

A material decision should consider:

- user workflow value
- architectural consistency
- implementation complexity
- testability
- documentation impact
- backward compatibility for file formats and expected user behavior

### When a decision must be documented explicitly

Document a design decision in architecture and/or product docs when the change affects:

- supported file formats
- workspace behavior
- interpolation semantics
- enumerated signal behavior
- repository structure or dependency rules
- versioning or release expectations

## Change Control Expectations

The following changes require corresponding documentation updates before they should be considered complete:

- new import/export formats
- changes to CSV metadata behavior
- changes to enumerated signal semantics
- changes to workspace navigation or editing flows
- changes to public module boundaries
- changes to build/test support policy

At minimum, the relevant updates should include some combination of:

- `README.md`
- `CHANGELOG.md`
- `docs/product/*`
- `docs/specs/srs.md`
- `docs/architecture/*`
- contributor or security guidance when applicable

## Release Governance

### Versioning policy

Signal Editor follows semantic versioning.

- use `MAJOR` for breaking changes in public behavior, format semantics, or supported workflows
- use `MINOR` for backward-compatible capability additions
- use `PATCH` for compatible fixes and polishing

### Release readiness criteria

A release candidate should satisfy all applicable conditions:

- implementation is complete
- core behavior is tested
- user-visible changes are documented
- architectural changes are documented
- changelog entries are updated
- known limitations are stated honestly

## Documentation Governance

All active documentation must describe the real Signal Editor project.

Specifically:

- generic scaffold wording should not be reintroduced
- examples should match the actual format contract used by the repository
- UI descriptions should reflect the implemented workspace model
- governance, contribution, and security files should stay actionable rather than ceremonial

## Configuration and Deployment Artifacts

Files in `deploy/config/` and other release-related areas should be treated as controlled project assets. They must not be deleted, replaced, or repurposed casually as part of unrelated feature work. If they need revision, the rationale should be explicit and the impact on packaging or delivery should be documented.

## Behavior for Unrelated Changes

Contributors should not bundle unrelated deletions, formatting churn, or repository cleanup into a feature change unless that cleanup is directly required to complete the task safely. Governance favors focused commits with clear intent.

## Escalation Path

When uncertainty exists about whether a change is appropriate, the escalation path should be:

1. confirm the current documented product intent
2. inspect the architecture and requirements documents
3. prefer the smallest design that satisfies the use case
4. document the decision if it changes repository expectations
5. defer to maintainer judgment if ambiguity remains
