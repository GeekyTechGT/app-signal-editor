# Security Policy

## Supported Versions

| Version line           | Status          |
|------------------------|-----------------|
| `0.1.x` (current)      | Supported        |
| Older snapshots        | Best effort only |
| Experimental branches  | Not supported    |

## How to Report a Vulnerability

Do not create a public issue for a suspected vulnerability.

Report security issues through an approved private channel:

1. Contact the repository maintainer or the designated internal security owner.
2. Use your organization's approved confidential communication channel.
3. Include enough detail to reproduce and triage the issue safely.

Your report should include:

- affected module or executable
- affected version or branch
- impact summary
- reproduction steps
- proof-of-concept input or configuration, if safe to share
- whether the issue exposes confidentiality, integrity, availability, or compliance risk

## Triage Priorities

High priority:

- arbitrary file overwrite or unsafe path handling
- improper parsing of untrusted inputs
- cryptographic misuse
- insecure key handling or storage
- packaging or deploy flows that leak private artifacts

Medium priority:

- denial-of-service through malformed inputs
- accidental disclosure through logs or generated artifacts

## Secure Development Expectations

All contributors are expected to:

- validate untrusted inputs aggressively
- preserve deterministic behavior in data-processing pipelines
- avoid logging secrets, keys, or sensitive identifiers
- keep cryptographic operations isolated to reviewed modules and adapters
- update third-party dependency notices when dependency behavior changes
- add regression tests for security-relevant fixes

## Dependency and Supply-Chain Considerations

This repository uses third-party components. When updating any dependency:

- record the version change
- assess security and license implications
- update `compliance/` material if redistribution implications change

## Secrets and Sensitive Material

Never commit:

- private keys
- API tokens or passwords
- customer data not approved as test data
- internal certificates
- environment-specific confidential configuration

If such material is accidentally committed: stop, notify the maintainer immediately, and rotate or revoke the exposed material.
