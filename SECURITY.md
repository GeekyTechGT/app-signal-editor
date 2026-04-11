# Security Policy

## Supported Versions

| Version line | Status |
|--------------|--------|
| `0.1.x` | Supported |
| Older snapshots | Best effort only |
| Experimental branches | Not supported |

## Reporting a Vulnerability

Do not report suspected vulnerabilities through a public issue.

Use an approved private reporting path:

1. Contact the repository maintainer or designated internal security owner.
2. Use the approved confidential communication channel for your organization.
3. Provide enough detail to reproduce and triage the issue safely.

A useful report should include:

- affected component or executable
- affected branch or version
- impact summary
- reproduction steps
- representative input file or payload if it is safe to share
- whether confidentiality, integrity, availability, or supply-chain trust is affected

## Security Scope for Signal Editor

Signal Editor is a local desktop engineering tool. Its most relevant security risks are not traditional internet-facing web threats, but local data handling and parser correctness issues.

The most important repository-specific risk areas are:

- malformed or adversarial input files causing crashes or undefined behavior
- parser inconsistencies across CSV, TSV/TXT, JSON, and SpreadsheetML XML
- unsafe path handling during load/save operations
- accidental disclosure of proprietary waveform data in fixtures, logs, or commits
- packaging or deployment steps distributing unintended or stale artifacts

## Secure Development Expectations

All contributors are expected to:

- treat imported files as untrusted input
- validate parser assumptions explicitly and fail clearly
- add regression tests for security-relevant parsing fixes
- avoid logging sensitive paths or sample values unless necessary for diagnosis
- review new dependencies for both security and license impact
- preserve deterministic behavior in persistence flows to reduce ambiguous parser outcomes

## Sensitive Material

Never commit:

- private keys
- API tokens, passwords, or secrets
- confidential customer measurement files
- proprietary traces that are not explicitly approved as repository fixtures
- internal-only credentials for packaging, deployment, or test infrastructure

If sensitive material is committed accidentally:

1. stop normal work immediately
2. notify the maintainer
3. rotate or revoke the affected secret where applicable
4. remove the material using an approved remediation workflow
5. assess whether any downstream distribution or fork already contains it

## Security Review Triggers

Treat the following as changes that deserve explicit security attention:

- new import or export formats
- new parser logic for tabular metadata or enumerated state handling
- changes to filesystem path resolution
- changes to deployment configuration or packaging descriptors
- dependency additions or major dependency upgrades

## Disclosure and Remediation Expectations

The repository should prefer responsible private handling first, followed by clear remediation notes once a fix is available. If a vulnerability results in a behavior change, the relevant documentation and changelog entries should be updated so downstream users understand the mitigation and any compatibility impact.
