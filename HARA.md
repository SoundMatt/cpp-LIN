# Hazard Analysis and Risk Assessment (HARA) — cpp-LIN

**Document ID:** HARA-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standard:** ISO 26262:2018 Part 3  
**ASIL Target:** ASIL-B

---

## 1. Scope

This HARA covers cpp-LIN as a Safety Element out of Context (SEooC) for
integration into automotive ECUs communicating over LIN bus networks.

---

## 2. Item Definition

**Item:** cpp-LIN software component  
**Function:** Transmit and receive LIN bus frames; parse LDF files; run master
schedule; publish slave responses; provide E2E protection; adapt to RELAY
routing.

---

## 3. Hazardous Events

### H-01: Corrupt frame data transmitted on LIN bus

| Field | Value |
|-------|-------|
| Hazard | Corrupted payload transmitted to slave ECU |
| Cause | Checksum computed incorrectly; buffer overrun |
| Operational situation | Normal driving with LIN-controlled actuator active |
| Severity | S2 (moderate injury possible) |
| Exposure | E4 (high probability in normal operation) |
| Controllability | C2 (normally controllable) |
| ASIL | ASIL-B |
| Safety Goal | SG-01, SG-03 |

### H-02: Invalid frame ID accepted and processed

| Field | Value |
|-------|-------|
| Hazard | Frame with ID > 0x3F processed, causing undefined behaviour in slave |
| Cause | Missing or incorrect ID range check |
| Operational situation | Any operating condition |
| Severity | S1 (light injury) |
| Exposure | E3 (moderate) |
| Controllability | C2 (normally controllable) |
| ASIL | ASIL-A (→ handled at ASIL-B for margin) |
| Safety Goal | SG-02 |

### H-03: Diagnostic frame processed with Enhanced checksum

| Field | Value |
|-------|-------|
| Hazard | Diagnostic request/response frame (0x3C/0x3D) processed with wrong checksum type, corrupting diagnostic data |
| Cause | Missing enforcement of Classic-only checksum for diagnostic IDs |
| Operational situation | Diagnostic session (workshop or OBD) |
| Severity | S2 |
| Exposure | E2 (low probability, only in diagnostic sessions) |
| Controllability | C2 |
| ASIL | ASIL-A (→ handled at ASIL-B for margin) |
| Safety Goal | SG-06 |

### H-04: E2E protection failure not detected

| Field | Value |
|-------|-------|
| Hazard | Bit-flipped or replayed safety-critical LIN message accepted by E2E Receiver |
| Cause | CRC check bypassed; sequence counter overflow not handled |
| Operational situation | Any safety-critical LIN message in motion |
| Severity | S3 (severe injury possible) |
| Exposure | E4 |
| Controllability | C1 (difficult to control) |
| ASIL | ASIL-B |
| Safety Goal | SG-05 |

### H-05: Unbounded memory growth causing resource exhaustion

| Field | Value |
|-------|-------|
| Hazard | Subscriber channel fill-up causes `send_drop_oldest` to silently discard frames, or unbounded queue growth crashes the ECU |
| Cause | Back-pressure policy not enforced; subscriber too slow |
| Operational situation | High-frequency schedule with slow consumer |
| Severity | S2 |
| Exposure | E3 |
| Controllability | C2 |
| ASIL | ASIL-A (→ handled at ASIL-B for margin) |
| Safety Goal | SG-04 |

---

## 4. ASIL Decomposition

cpp-LIN operates as a single-channel ASIL-B element. No ASIL decomposition
is applied at the library level. The integrating system must provide
complementary measures (e.g., watchdog, hardware CRC) to achieve ASIL-C or
ASIL-D at system level if required.

---

## 5. Safety Goals Summary

| ID | Safety Goal | ASIL | Status |
|----|-------------|------|--------|
| SG-01 | No data corruption during TX/RX | ASIL-B | Implemented (checksum, validation) |
| SG-02 | Reject frames with ID > 0x3F | ASIL-B | Implemented (validate_frame) |
| SG-03 | Detect incorrect checksums | ASIL-B | Implemented (calc_checksum, verify_pid) |
| SG-04 | No unbounded memory growth | ASIL-B | Implemented (Chan capacity, drop policy) |
| SG-05 | Detect E2E protection errors | ASIL-B | Implemented (E2E CRC-16, seq check) |
| SG-06 | Diagnostic frames use Classic checksum | ASIL-B | Implemented (validate_frame) |

---

## 6. Open Items

| ID | Description | Target |
|----|-------------|--------|
| OI-01 | Formal proof of checksum algorithm correctness | v1.0.0 |
| OI-02 | MC/DC coverage analysis | v1.0.0 |
| OI-03 | MISRA C++ 2023 compliance scan | v1.0.0 |
| OI-04 | Hardware-in-the-loop (HiL) test campaign | v1.0.0 |

---

## 7. References

- ISO 26262:2018 Part 3 — Concept phase
- LIN Specification Package Rev 2.2A — Section 2 (Frame structure)
- RELAY Specification v1.10 — §15 (Error handling)
- `SEOOC.md` — Assumptions on use
- `SAFETY_PLAN.md` — Development process
