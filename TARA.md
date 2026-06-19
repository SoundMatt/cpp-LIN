# Threat Analysis and Risk Assessment (TARA) — cpp-LIN

**Document ID:** TARA-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standards:** ISO/SAE 21434:2021, IEC 62443-4-1:2018 SL-2, IEC 62443-4-2:2019  
**Target Security Level:** SL-2 (resistance against intentional violation with moderate resources)

---

## 1. Scope and Context

This TARA covers the cpp-LIN library as a SEooC (Safety Element out of Context).
The library processes LIN bus frames in software. Physical bus access, transceiver
hardware, and network topology are **integrator responsibilities** (see `SEOOC.md`).

The primary security objective is to prevent an attacker with access to the LIN bus
or the host software environment from:
1. Injecting corrupt frames that reach safety-critical actuators
2. Replaying legitimate frames out of temporal context
3. Exhausting resources and causing Denial-of-Service on the LIN schedule
4. Bypassing E2E protection to cause undetected data corruption

---

## 2. Asset Identification

| Asset ID | Asset | Confidentiality | Integrity | Availability |
|----------|-------|----------------|-----------|-------------|
| A-01 | LIN frame payload (safety-critical) | Low | **High** | **High** |
| A-02 | Frame ID / PID (routing) | Low | **High** | Medium |
| A-03 | E2E sequence counter | Low | **High** | Medium |
| A-04 | E2E CRC / header bytes | Low | **High** | Medium |
| A-05 | LDF configuration data | Low | **High** | Medium |
| A-06 | Master schedule table | Low | **High** | **High** |
| A-07 | Bus subscriber registry | Low | Medium | Medium |
| A-08 | Heap and channel buffers | Low | Medium | **High** |

---

## 3. Threat Scenarios

### THREAT-01 — LIN Bus Frame Injection

| Field | Value |
|-------|-------|
| **ID** | THREAT-01 |
| **Assets** | A-01, A-02 |
| **STRIDE** | Spoofing, Tampering |
| **Attack vector** | Physical or logical LIN bus access |
| **Threat** | Attacker injects a crafted LIN frame with a valid-looking PID and corrupted payload, bypassing `validate_frame()` if the integrating system does not call it. |
| **CVSS 3.1** | AV:P/AC:H/PR:N/UI:N/S:U/C:N/I:H/A:L = **5.3 (Medium)** |
| **Risk rating** | High (safety impact on ASIL-B path) |
| **Controls** | SC-01, SC-02, SC-05 |
| **Residual risk** | Low — E2E CRC detects any single-bit corruption; PID parity detects ID tampering |

### THREAT-02 — Frame Replay Attack

| Field | Value |
|-------|-------|
| **ID** | THREAT-02 |
| **Assets** | A-01, A-03 |
| **STRIDE** | Tampering, Elevation of Privilege |
| **Attack vector** | Physical LIN bus tap + replay device |
| **Threat** | Attacker records valid E2E-protected frames and replays them later, causing stale sensor data to appear fresh. |
| **CVSS 3.1** | AV:P/AC:H/PR:N/UI:N/S:U/C:N/I:H/A:N = **4.2 (Medium)** |
| **Risk rating** | High (E2E sequence counter is the primary defence) |
| **Controls** | SC-03 |
| **Residual risk** | Low — `safety::Receiver::unwrap()` rejects any non-sequential counter as `ErrSequenceGap` |

### THREAT-03 — Resource Exhaustion / Denial of Service

| Field | Value |
|-------|-------|
| **ID** | THREAT-03 |
| **Assets** | A-06, A-08 |
| **STRIDE** | Denial of Service |
| **Attack vector** | Malformed LDF file, or rapid-fire publish calls from a compromised host process |
| **Threat** | Attacker supplies a large LDF file or calls `Bus::subscribe()` many times to exhaust heap memory, starving the master schedule runner. |
| **CVSS 3.1** | AV:L/AC:L/PR:L/UI:N/S:U/C:N/I:N/A:H = **5.5 (Medium)** |
| **Risk rating** | Medium |
| **Controls** | SC-04, SC-06 |
| **Residual risk** | Medium — integrating system must enforce heap budgets (DSR-01) |

### THREAT-04 — E2E Header Bypass (Corrupted DataID/SourceID)

| Field | Value |
|-------|-------|
| **ID** | THREAT-04 |
| **Assets** | A-04 |
| **STRIDE** | Tampering |
| **Attack vector** | LIN bus bit-flip or software bug in protector |
| **Threat** | DataID or SourceID field is corrupted, but CRC happens to match (birthday attack) — receiver accepts a frame from the wrong source. |
| **CVSS 3.1** | AV:P/AC:H/PR:N/UI:N/S:U/C:N/I:H/A:N = **4.2 (Medium)** |
| **Risk rating** | Medium (CRC-16 has 2^-16 false-accept probability) |
| **Controls** | SC-05 |
| **Residual risk** | Low — CRC-16 false-accept rate is ≤1.5×10^-5 per frame; acceptable for ASIL-B |

### THREAT-05 — LDF File Tampering

| Field | Value |
|-------|-------|
| **ID** | THREAT-05 |
| **Assets** | A-05, A-06 |
| **STRIDE** | Tampering, Spoofing |
| **Attack vector** | Compromised build system or insecure boot |
| **Threat** | Attacker modifies the LDF file to change frame IDs, signal offsets, or schedule timing, causing the master to poll wrong IDs or decode signals incorrectly. |
| **CVSS 3.1** | AV:L/AC:H/PR:H/UI:N/S:U/C:N/I:H/A:M = **4.4 (Medium)** |
| **Risk rating** | Medium |
| **Controls** | SC-07 |
| **Residual risk** | Low — integrating system must verify LDF integrity via secure boot (ASM-10, DSR-05) |

### THREAT-06 — Timing Attack on Schedule Runner

| Field | Value |
|-------|-------|
| **ID** | THREAT-06 |
| **Assets** | A-06 |
| **STRIDE** | Denial of Service |
| **Attack vector** | OS-level priority manipulation on host process |
| **Threat** | Attacker starves the LIN schedule runner thread by scheduling high-priority processes, causing slot timing jitter or total schedule stall. |
| **CVSS 3.1** | AV:L/AC:H/PR:H/UI:N/S:U/C:N/I:N/A:H = **4.1 (Medium)** |
| **Risk rating** | Low (requires OS-level privilege; hardware watchdog mitigates) |
| **Controls** | SC-06 |
| **Residual risk** | Low — integrating system implements watchdog (DSR-06) |

### THREAT-07 — Data Race in Concurrent Bus Access

| Field | Value |
|-------|-------|
| **ID** | THREAT-07 |
| **Assets** | A-01, A-07 |
| **STRIDE** | Tampering |
| **Attack vector** | Multi-threaded host application |
| **Threat** | Concurrent calls to `Bus::publish()`, `subscribe()`, and `send_header()` without proper internal locking cause undefined behaviour or data corruption. |
| **CVSS 3.1** | AV:L/AC:H/PR:L/UI:N/S:U/C:N/I:H/A:H = **6.3 (Medium)** |
| **Risk rating** | Low (mitigated by implementation) |
| **Controls** | SC-08 |
| **Residual risk** | Low — `virt::Bus` uses `shared_mutex`; verified by ThreadSanitizer in CI |

---

## 4. Security Controls

| Control ID | Control | Standard Reference | Implementation |
|-----------|---------|-------------------|----------------|
| SC-01 | Frame ID boundary check | IEC 62443-4-2 CR 3.4 | `validate_frame()` throws on ID > 0x3F |
| SC-02 | PID parity verification | ISO 26262-6 REQ-LIN-006/007 | `verify_pid()` checks P0/P1 per LIN 2.x §2.3.1 |
| SC-03 | E2E sequence counter | IEC 62443-4-2 CR 3.2, ISO 26262-7 E2E | `safety::Receiver::unwrap()` enforces counter monotonicity |
| SC-04 | Bounded channel depth | IEC 62443-4-2 CR 7.1 | `Chan<T>` drop-oldest / drop-newest backpressure (REQ-RELAY-015) |
| SC-05 | CRC-16/CCITT-FALSE E2E | IEC 62443-4-2 CR 3.4 | `safety::Protector::protect()` computes CRC over header+payload |
| SC-06 | Watchdog integration | IEC 62443-4-2 CR 7.2 | DSR-06: integrating system provides watchdog |
| SC-07 | LDF integrity at load | IEC 62443-4-2 CR 3.3 | ASM-10, DSR-05: secure boot hash verification |
| SC-08 | Thread-safe bus | IEC 62443-4-2 CR 2.1 | `shared_mutex` in `virt::Bus`; TSan CI gate |

---

## 5. Attack Paths and Mitigations (Attack Tree Summary)

```
Goal: Corrupt ASIL-B actuator output
├── THREAT-01: Inject frame with valid PID
│   ├── SC-02 (PID parity): blocks forged PID
│   └── SC-05 (E2E CRC): blocks corrupted payload
├── THREAT-02: Replay valid frame
│   └── SC-03 (sequence counter): detects gap
├── THREAT-03: DoS via heap exhaustion
│   └── SC-04 (bounded channels): drops frames, does not crash
├── THREAT-04: E2E birthday attack
│   └── SC-05 (CRC-16 2^-16): acceptable residual risk
├── THREAT-05: LDF file tampering
│   └── SC-07 (secure boot): integrator responsibility
├── THREAT-06: Schedule starvation
│   └── SC-06 (watchdog): integrator responsibility
└── THREAT-07: Data race
    └── SC-08 (shared_mutex + TSan): eliminated in implementation
```

---

## 6. Risk Acceptance

| Threat | Residual Risk | Accepted By | Date |
|--------|--------------|-------------|------|
| THREAT-01 | Low | Matt Jones | 2026-06-19 |
| THREAT-02 | Low | Matt Jones | 2026-06-19 |
| THREAT-03 | Medium — integrator must enforce heap budget (DSR-01) | Matt Jones | 2026-06-19 |
| THREAT-04 | Low — CRC-16 false-accept within ASIL-B tolerance | Matt Jones | 2026-06-19 |
| THREAT-05 | Low — integrator must provide secure boot (DSR-05) | Matt Jones | 2026-06-19 |
| THREAT-06 | Low — integrator must provide watchdog (DSR-06) | Matt Jones | 2026-06-19 |
| THREAT-07 | Low — eliminated in implementation | Matt Jones | 2026-06-19 |

---

## 7. Penetration Testing Scope (v1.0.0 target)

| Test | Method |
|------|--------|
| Frame injection via virtual bus | Fuzzing with AFL++ on `validate_frame()` |
| Replay detection | Property-based test: random counter gaps must always be rejected |
| Heap exhaustion | Valgrind massif profiling with 1000 concurrent subscribers |
| Race condition | ThreadSanitizer on all bus operations (CI gate) |
| LDF parser fuzz | AFL++ on `ldf::parse()` with random inputs |

---

## 8. References

- ISO/SAE 21434:2021 §9 — Threat analysis and risk assessment
- IEC 62443-4-1:2018 SR-2, SR-3 — Security requirements
- IEC 62443-4-2:2019 CR 3.2, 3.3, 3.4, 7.1, 7.2 — Component requirements
- `HARA.md` — Hazard Analysis (safety view)
- `SEOOC.md` — Assumptions and derived safety requirements
- `INCIDENT-RESPONSE.md` — Vulnerability response process
- `tara.json` — Machine-readable TARA (cpfusa format)
