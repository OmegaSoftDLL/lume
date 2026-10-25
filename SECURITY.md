# Security Policy

Lume is pre-release bootstrap software; do not use it in production.

## Reporting a vulnerability

Please report security issues privately via GitHub Security Advisories
("Report a vulnerability" on the repository's Security tab) rather than a public issue.
Include a minimal reproduction and the affected commit.

## Scope notes

- The `lume0` seed interpreter and the `lume0 asm` ELF writer execute and emit native code.
  Treat any `.lume`/`.src`/`.s` input you did not write as untrusted.
- Anti-reverse-engineering (REA) protection is a designed goal and is **not yet implemented**;
  do not rely on it.
