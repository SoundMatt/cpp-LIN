# Safety Plan — cpp-LIN

**Document ID:** SP-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**ASIL Target:** ASIL-B (ISO 26262:2018 Part 6 — Software)  
**SIL Target:** SIL-2 (IEC 61508:2010)

---

## 1. Scope

This Safety Plan governs the development of **cpp-LIN**, a C++ software
component implementing the LIN (Local Interconnect Network) bus protocol,
conformant to the RELAY specification v1.11.

cpp-LIN is classified as a **Safety Element out of Context (SEooC)** per
ISO 26262-10 §9. It is intended for integration into automotive ECUs and
other safety-related embedded systems at ASIL-B.

---

## 2. Safety Goals

| ID | Safety Goal | ASIL |
|----|-------------|------|
| SG-01 | cpp-LIN shall not corrupt LIN frame data during transmission or reception | ASIL-B |
| SG-02 | cpp-LIN shall detect and reject frames with invalid IDs (> 0x3F) | ASIL-B |
| SG-03 | cpp-LIN shall detect and reject frames with incorrect checksums | ASIL-B |
| SG-04 | cpp-LIN shall not cause unbounded memory growth under sustained load | ASIL-B |
| SG-05 | cpp-LIN shall detect E2E protection errors (CRC mismatch, sequence gaps) | ASIL-B |
| SG-06 | cpp-LIN shall enforce that diagnostic frames use Classic checksum only | ASIL-B |

---

## 3. Functional Safety Requirements

All functional safety requirements are tracked in `.fusa-reqs.json` and
`requirements/requirements.json`. Requirements use the identifiers:

- `REQ-LIN-NNN` — Core LIN protocol requirements
- `REQ-VIRT-NNN` — Virtual bus requirements
- `REQ-E2E-NNN` — End-to-End safety requirements
- `REQ-LDF-NNN` — LDF parser requirements
- `REQ-MASTER-NNN` — Master node requirements
- `REQ-SLAVE-NNN` — Slave node requirements
- `REQ-RELAY-NNN` — RELAY adapter requirements
- `REQ-CLI-NNN` — CLI requirements
- `REQ-SEOOC-NNN` — SEooC-level requirements

Each source file contains `// fusa:req REQ-XXX-NNN` annotations to trace
implementation to requirements. Each test file contains `// fusa:test
REQ-XXX-NNN` annotations to trace tests to requirements.

---

## 4. Development Process

### 4.1 Version Control

- Git with DCO sign-off on all commits.
- SLSA L3: two-party review required (CODEOWNERS enforces this).
- All safety-critical files (`.fusa-reqs.json`, `requirements/`, `HARA.md`,
  `SEOOC.md`) require explicit owner approval.

### 4.2 Static Analysis

- `clang-tidy` with `bugprone-*` and `clang-analyzer-*` checks.
- Enabled in CI as a blocking gate.

### 4.3 Dynamic Analysis

- AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan) enabled in CI.
- All tests pass under sanitizers with `halt_on_error=1`.

### 4.4 Coverage

- Line coverage enforced at ≥ 70% (LCOV in CI).
- MC/DC coverage analysis is a v1.0.0 goal.

### 4.5 Testing

- Unit tests with Catch2.
- RELAY conformance gate: `relay conform --strict`.
- RELAY interop gate: `relay interop --protocol LIN`.
- All tests must pass on Linux (GCC 12, Clang 14), macOS (Clang), and
  Windows (MSVC 2022).

### 4.6 FuSa Toolchain

- `cpfusa check` — annotation completeness.
- `cpfusa trace` — requirements traceability.
- `cpfusa qualify --asil ASIL-B` — ASIL-B qualification gate.
- `cpfusa tara` — TARA generation.
- `cpfusa fmea` — FMEA generation.
- `cpfusa safety-case` — GSN safety case.

---

## 5. Known Limitations (SEooC Assumptions)

See [SEOOC.md](SEOOC.md) for the full list of assumptions about the
operational context that must be validated by the integrating system.

---

## 6. Hazard Analysis and Risk Assessment

See [HARA.md](HARA.md).

---

## 7. Confirmation Measures

| Measure | Responsible | Frequency |
|---------|-------------|-----------|
| Code review (CODEOWNERS) | @SoundMatt | Every PR |
| CI pipeline (all gates) | CI/CD | Every commit |
| cpfusa qualify | CI/CD | Every commit |
| RELAY conform --strict | CI/CD | Every commit |
| ASan + UBSan test run | CI/CD | Every commit |
| Coverage gate (≥70%) | CI/CD | Every commit |
| Manual safety review | @SoundMatt | Every release |

---

## 8. References

- ISO 26262:2018 — Road vehicles — Functional safety
- IEC 61508:2010 — Functional safety of E/E/PE safety-related systems
- ISO/SAE 21434:2021 — Road vehicles — Cybersecurity engineering
- LIN Specification Package, Revision 2.2A (LIN Consortium)
- RELAY Specification v1.11 (SoundMatt)
- `.fusa-reqs.json` — Machine-readable requirements
- `HARA.md` — Hazard Analysis and Risk Assessment
- `SEOOC.md` — Safety Element out of Context document
