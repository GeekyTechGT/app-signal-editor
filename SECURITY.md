# Security Policy

## Supported Versions

| Version line          | Status |
|-----------------------|--------|
| `0.1.x`               | Supported |
| Older snapshots       | Best effort only |
| Experimental branches | Not supported |

## Reporting a Vulnerability

Do not open a public issue for suspected vulnerabilities.

Report security issues through an approved private channel:

1. Contact the repository maintainer or designated internal security owner.
2. Use the approved confidential communication channel for your organization.
3. Include enough technical detail to reproduce and triage the issue safely.

Your report should include:

- affected area or executable
- affected version or branch
- impact summary
- reproduction steps
- proof-of-concept input if it is safe to share
- whether the issue affects confidentiality, integrity, availability, or supply-chain trust

## Project-Specific Security Focus

Signal Editor primarily processes local CSV data. The most relevant risks are:

- malformed CSV input causing denial of service or undefined behavior
- unsafe path handling during load and save operations
- packaging or deployment workflows shipping unintended artifacts
- accidental exposure of proprietary signal data in commits, logs, or test fixtures

## Secure Development Expectations

All contributors are expected to:

- validate untrusted file input aggressively
- preserve deterministic parsing and export behavior
- avoid logging sensitive file paths or proprietary sample data unnecessarily
- add regression tests for security-relevant parser fixes
- review dependency updates for both security and license impact

## Secrets and Sensitive Material

Never commit:

- private keys
- API tokens or passwords
- customer data not approved as test data
- confidential recordings or measurement files
- environment-specific secrets

If such material is committed accidentally, stop, notify the maintainer immediately, and rotate or revoke the exposed material.
