# Safety Element out of Context (SEooC) — cpp-LIN

**Document ID:** SEOOC-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standard:** ISO 26262:2018 Part 10 §9  
**ASIL:** ASIL-B

---

## 1. Purpose

This document defines the assumptions made during the development of cpp-LIN
as a Safety Element out of Context (SEooC). The integrating item must validate
that these assumptions hold in the target operational environment.

---

## 2. Assumed Operational Context

### ASM-01: C++ runtime environment
The integrating system provides a standards-conformant C++17 runtime
(or later) without UB suppression flags or non-conformant ABI patches.

### ASM-02: POSIX thread support
The integrating system provides a POSIX-compatible threading environment
(`std::thread`, `std::mutex`, `std::condition_variable`). On bare-metal
targets, a threading abstraction layer must be provided.

### ASM-03: No use of `terminate()` suppression
The integrating system must not override `std::terminate()` to swallow
exceptions. cpp-LIN relies on exceptions propagating correctly.

### ASM-04: Heap availability
The integrating system provides a heap with sufficient capacity to allocate
subscriber channels (`Chan<Frame>`) with the configured capacity. For ASIL-B
integrations, a memory monitor or MPU protection zone should be used to
detect heap exhaustion.

### ASM-05: Single LIN master per bus
cpp-LIN's `master::Node` assumes it is the sole LIN master on the bus.
Multi-master configurations are not supported and violate LIN spec §4.1.

### ASM-06: Correct clock resolution
`std::this_thread::sleep_for` in `master::Node::run()` must have at least
1 ms resolution. Systems with lower-resolution clocks may exhibit schedule
jitter exceeding the LIN spec inter-frame timeout.

### ASM-07: External hardware CRC for ASIL-C/D
cpp-LIN's E2E protection uses software CRC-16/CCITT-FALSE. For ASIL-C or
ASIL-D system-level targets, an additional hardware CRC layer must be
applied by the integrating system.

### ASM-08: No concurrent destruction
`IBus::close()` must be called before any shared_ptr to the bus is
destroyed while subscriber threads are still running. The integrating
system is responsible for correct shutdown sequencing.

### ASM-09: LIN network wiring integrity
cpp-LIN does not detect open-circuit or short-circuit conditions on the
physical LIN bus wire. The integrating system must provide hardware-level
diagnostics for wiring faults.

### ASM-10: LDF file integrity
`lin::ldf::parse()` validates LDF syntax but does not cryptographically
verify the LDF file's authenticity. The integrating system is responsible
for verifying that the LDF file has not been tampered with (e.g., via a
secure boot manifest).

---

## 3. Derived Safety Requirements for Integrators

| ID | Requirement | Rationale |
|----|-------------|-----------|
| DSR-01 | The integrating system shall monitor heap usage and trigger a safe state on heap exhaustion | ASM-04 |
| DSR-02 | The integrating system shall enforce single-master topology at the hardware level | ASM-05 |
| DSR-03 | The integrating system shall apply hardware CRC for any ASIL-C or ASIL-D data path using cpp-LIN | ASM-07 |
| DSR-04 | The integrating system shall call `IBus::close()` before destroying the bus object while subscriber threads are active | ASM-08 |
| DSR-05 | The integrating system shall verify LDF file integrity via a secure boot mechanism | ASM-10 |
| DSR-06 | The integrating system shall include a watchdog timer to detect schedule runner starvation | ASM-06 |

---

## 4. Out-of-Scope Hazards

The following hazards are outside the scope of cpp-LIN and must be handled
by the integrating system:

- Physical LIN bus electromagnetic interference (EMI)
- LIN transceiver hardware faults
- Power supply instability affecting LIN signalling
- Operating system (RTOS) scheduling faults
- Flash memory corruption affecting the cpp-LIN binary image

---

## 5. Integration Checklist

Before integrating cpp-LIN into an ASIL-B system, the integrating team
must confirm:

- [ ] All assumptions (ASM-01 through ASM-10) are validated for the target
- [ ] All derived safety requirements (DSR-01 through DSR-06) are implemented
- [ ] cpp-LIN version and build configuration are recorded in the SBOM
- [ ] The cpp-FuSa qualification evidence (CI artifacts) is attached to the
      system safety case
- [ ] RELAY conform and interop tests pass on the target toolchain

---

## 6. References

- ISO 26262:2018 Part 10 §9 — Safety element out of context
- `HARA.md` — Hazard Analysis and Risk Assessment
- `SAFETY_PLAN.md` — Development safety plan
- `.fusa-reqs.json` — Machine-readable requirements
