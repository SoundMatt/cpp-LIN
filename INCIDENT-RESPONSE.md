# Incident Response Policy — cpp-LIN

**Document ID:** IR-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standard:** ISO/SAE 21434:2021 §7.4, IEC 62443-4-1 SM-2, IEC/TR 62443-2-3  
**Contact:** matt@jellybaby.com

---

## 1. Scope

This document defines the incident response process for cpp-LIN, covering
vulnerability discovery, classification, notification, remediation, and public
disclosure for all versions of the library.

---

## 2. Incident Classification

| Severity | Description | Examples |
|----------|-------------|---------|
| **Critical** | Memory safety bug enabling arbitrary code execution; E2E bypass | Buffer overflow in frame parsing; CRC check skip |
| **High** | Data corruption in safety-critical path; denial-of-service via API | Integer overflow in checksum; unbounded allocation |
| **Medium** | Non-safety-critical logic error; incorrect but non-exploitable behaviour | Wrong PID parity for specific ID; schedule order drift |
| **Low** | Documentation error; minor API misalignment with RELAY spec | Typo in error message; wrong version string |

---

## 3. Reporting a Vulnerability

**Private reporting (preferred):**  
Email **matt@jellybaby.com** with subject `[SECURITY] cpp-LIN <brief description>`.

Include:
- Affected version(s)
- Reproduction steps or proof-of-concept
- Potential impact assessment
- Suggested severity (Critical / High / Medium / Low)

Do **not** open a public GitHub issue for security vulnerabilities. Public
issues should be reserved for non-security bugs and feature requests.

**GitHub Security Advisories:**  
Alternatively, use the [GitHub Security Advisory](https://github.com/SoundMatt/cpp-LIN/security/advisories/new)
draft mechanism for private coordinated disclosure.

---

## 4. Response SLA

| Severity | Acknowledgment | Initial Assessment | Patch Target | Public Disclosure |
|----------|----------------|-------------------|-------------|-------------------|
| Critical | 24 hours | 48 hours | 7 days | 14 days after patch |
| High     | 48 hours | 5 days   | 30 days | 30 days after patch |
| Medium   | 5 days   | 14 days  | 90 days | 90 days after patch |
| Low      | 14 days  | 30 days  | Next release | At release |

All timelines are calendar days from initial validated report. If a reporter
requests a later disclosure date, we will accommodate up to 180 days for
Critical/High vulnerabilities.

---

## 5. Response Process

### Phase 1: Receipt and Acknowledgment
1. Acknowledge receipt within the SLA above.
2. Assign an internal tracking ID (format: `CVE-<YEAR>-XXXXXXX` if applicable, otherwise `IR-<YEAR>-NNN`).
3. Confirm or challenge the reported severity.

### Phase 2: Assessment
1. Reproduce the issue on the latest `main` branch.
2. Assess ASIL impact: does this affect a safety-critical path (E2E, checksum, frame validation)?
3. Check whether a mitigation exists at the integrating system level (SEOOC.md assumptions).
4. Draft a CVSS 3.1 score for Critical/High findings.

### Phase 3: Remediation
1. Develop a fix on a private branch (`security/IR-<YEAR>-NNN`).
2. Write a regression test that triggers the vulnerability (must be in the public test suite).
3. Run full CI pipeline (build matrix, sanitizers, coverage, RELAY conformance, cpfusa ASIL-B).
4. Issue a patch release (semantic version bump, SECURITY.md advisory link added to CHANGELOG).

### Phase 4: Disclosure
1. Publish a GitHub Security Advisory with CVE ID (if MITRE assigned).
2. Tag the patch release and update README badge.
3. Notify the reporter before public disclosure so they may publish their own writeup.
4. Add the incident to the internal FMEA traceability (fmea.json) and TARA update cycle.

---

## 6. Severity Escalation Rules

A finding is **automatically escalated to Critical** if it affects:
- `lin::safety::Protector::protect()` or `lin::safety::Receiver::unwrap()` (E2E bypass)
- `lin::protect_id()` or `lin::verify_pid()` (PID integrity)
- `lin::validate_frame()` (frame boundary checking)
- `lin::virt::Bus` concurrency primitives (data race → undefined behaviour)

---

## 7. ASIL Impact Assessment

For safety-critical (ASIL-B) vulnerabilities, the following additional steps apply:

1. Update `HARA.md` if the vulnerability introduces a new hazardous event.
2. Update `SAFETY_PLAN.md` Confirmation Measures table with a reference to the incident.
3. Issue an **Interim Safety Notice (ISN)** to known integrators (via GitHub release notes tagged `[SAFETY]`).
4. Increment the `.fusa-reqs.json` requirement that was bypassed (add `status: "under-review"` field).

---

## 8. Contact and Escalation Chain

| Role | Contact | Availability |
|------|---------|-------------|
| Primary Security Contact | matt@jellybaby.com | Business hours UTC+0 |
| Backup / Emergency | GitHub Security Advisory form | 24/7 async |

---

## 9. Policy References

- **ISO/SAE 21434:2021** §7.4 — Vulnerability management and disclosure
- **IEC 62443-4-1:2018** SM-2 — Security management (vulnerability response)
- **IEC/TR 62443-2-3:2015** — Patch management for IACS
- **RELAY Spec v1.10** §22 — Security considerations
- `SECURITY.md` — Public-facing vulnerability disclosure policy
- `SEOOC.md` — Safety Element out of Context assumptions (integrator scope)
