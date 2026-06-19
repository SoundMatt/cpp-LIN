# Security Policy — cpp-LIN

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | Yes |
| < 0.1   | No  |

## Reporting a Vulnerability

**Do not open public GitHub issues for security vulnerabilities.**

Please report security issues by email to **matt@jellybaby.com** with subject
`[SECURITY] cpp-LIN <brief description>`, or via the
[GitHub Security Advisory](https://github.com/SoundMatt/cpp-LIN/security/advisories/new)
private reporting mechanism.

### What to Include

- Affected version(s) and platform
- Reproduction steps or proof-of-concept
- Potential impact (is a safety-critical path affected?)
- Your preferred disclosure timeline

## Response Timeline

| Severity | Acknowledgment | Patch |
|----------|----------------|-------|
| Critical (ASIL-B path affected) | 24 h | 7 days |
| High | 48 h | 30 days |
| Medium | 5 days | 90 days |
| Low | 14 days | Next release |

We follow **coordinated disclosure**: patches are published before or simultaneously
with public disclosure. The maximum embargo period is 180 days for Complex/Critical issues.

## Scope

In scope:
- Memory safety bugs (buffer overflow, use-after-free) in any cpp-LIN API
- E2E safety bypass: any input that causes `lin::safety::Receiver::unwrap()` to
  accept a corrupted or replayed payload without throwing `E2EError`
- Data races in `lin::virt::Bus` concurrent operations
- Integer overflow in `protect_id()`, `verify_pid()`, `calc_checksum()`
- Denial-of-service via unbounded resource allocation in frame processing

Out of scope:
- Physical LIN bus attacks (transceiver, wiring) — see `SEOOC.md` ASM-09
- LDF file tampering — integrator responsibility (see `SEOOC.md` ASM-10)
- Issues in the RELAY CLI tool that do not affect the library
- Issues in test-only code

## Safety Impact Disclosure

If a vulnerability affects an ASIL-B annotated function (marked with `fusa:req`),
we will issue an **Interim Safety Notice (ISN)** alongside the patch release.
ISNs are tagged `[SAFETY]` in release notes.

## Full Policy

See [`INCIDENT-RESPONSE.md`](INCIDENT-RESPONSE.md) for the complete incident
response process including ASIL impact assessment, FMEA update procedure, and
escalation contacts.

## Standards References

- ISO/SAE 21434:2021 §7.4 — Vulnerability management
- IEC 62443-4-1:2018 SM-2 — Security management
- RELAY Spec v1.11 §22 — Security considerations
