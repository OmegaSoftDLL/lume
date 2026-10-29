# Security Policy

Lume is pre-release software; do not use it in production or to process untrusted input in a
security-sensitive environment.

## Supported versions

Security fixes are applied to the latest release and the `main` branch. Older releases are not
maintained. Before reporting a problem, reproduce it against the latest release or current `main`.

| Version | Status |
| --- | --- |
| Latest release | Supported |
| `main` | Supported |
| Older releases | Unsupported |

## Reporting a vulnerability

Report suspected vulnerabilities privately through
[GitHub Security Advisories](https://github.com/OmegaSoftDLL/lume/security/advisories/new).
Do not open a public issue, pull request, or discussion containing an exploitable reproduction.

Include:

- the affected release or commit, operating system, and architecture;
- the security impact and any affected trust boundary;
- a minimal, self-contained reproduction with exact commands;
- relevant diagnostics or logs, with credentials and personal data removed; and
- a suggested mitigation, if available.

Maintainers will review the report, assess its scope and severity, and coordinate a fix and
disclosure with the reporter. Do not disclose the issue publicly before a coordinated resolution.

## Scope notes

- The `lume0` seed interpreter and the `lume0 asm` ELF writer execute and emit native code.
  Treat any `.lume`/`.src`/`.s` input you did not write as untrusted.
- P33 software-protection profiles are implemented, but obfuscation and tamper resistance do
  not prevent reverse engineering by a determined attacker. Do not treat them as a security
  boundary or as a substitute for protecting credentials and other secrets.
