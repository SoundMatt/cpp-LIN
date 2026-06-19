# Safety Manual — cpp-LIN

**Document ID:** SM-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standard:** ISO 26262:2018 Part 6 §7 (user documentation), Part 10 §9 (SEooC)  
**ASIL:** ASIL-B

---

## 1. Purpose and Scope

This Safety Manual provides guidance for system integrators incorporating
cpp-LIN into an automotive ECU or IACS component. It documents:

- Assumptions that the integrating system must validate (from `SEOOC.md`)
- API usage patterns that preserve ASIL-B guarantees
- Anti-patterns that void the safety argument
- Integration checklist

This manual supplements `SEOOC.md` (assumptions) and `HARA.md` (hazard analysis)
and must be read alongside those documents.

---

## 2. Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                  Integrating System                  │
│                                                     │
│  ┌──────────────┐    ┌──────────────────────────┐  │
│  │ Application  │    │  Safety Monitor (watchdog│  │
│  │   Layer      │    │  heap guard, MPU)        │  │
│  └──────┬───────┘    └──────────────────────────┘  │
│         │                                            │
│  ┌──────▼───────────────────────────────────────┐  │
│  │                  cpp-LIN                      │  │
│  │  ┌───────────┐  ┌─────────┐  ┌───────────┐  │  │
│  │  │ lin::      │  │lin::    │  │lin::      │  │  │
│  │  │ validate   │  │safety:: │  │master::   │  │  │
│  │  │ protect_id │  │E2E Prot │  │slave::    │  │  │
│  │  │ checksum   │  │E2E Recv │  │ldf::      │  │  │
│  │  └─────┬─────┘  └────┬────┘  └─────┬─────┘  │  │
│  │        └─────────────▼─────────────┘         │  │
│  │               lin::virt::Bus / IBus           │  │
│  └──────────────────────┬────────────────────────┘  │
│                         │                            │
│  ┌──────────────────────▼────────────────────────┐  │
│  │       LIN Physical Layer (integrator-owned)   │  │
│  │  (transceiver, break detection, bit timing)   │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

cpp-LIN operates **above the physical layer**. The integrating system is responsible
for all hardware I/O. cpp-LIN provides software-level frame validation, E2E
protection, scheduling, and protocol adaptation.

---

## 3. Assumptions Validation Checklist

Before deploying cpp-LIN in an ASIL-B system, validate **every** assumption:

| ASM-ID | Assumption | How to Validate |
|--------|------------|-----------------|
| ASM-01 | C++17 runtime is standards-conformant | Compile with `-std=c++17 -Wall -Wextra -Wpedantic`; run test suite |
| ASM-02 | POSIX threading available | Verify `std::thread`, `std::mutex`, `std::condition_variable` work on target |
| ASM-03 | `std::terminate()` not suppressed | Confirm no custom `terminate_handler` swallows exceptions |
| ASM-04 | Sufficient heap capacity | Profile peak allocations; configure MPU/heap guard |
| ASM-05 | Single LIN master on bus | Verify hardware configuration; no multi-master wiring |
| ASM-06 | Monotonic clock ≥ 1 ms resolution | Measure `std::this_thread::sleep_for(1ms)` actual latency |
| ASM-07 | Hardware CRC for ASIL-C/D | Implement hardware CRC layer if system ASIL exceeds ASIL-B |
| ASM-08 | Correct shutdown sequencing | Review destructor order; call `IBus::close()` before bus destruction |
| ASM-09 | Physical bus integrity monitored | Implement open/short circuit detection in hardware layer |
| ASM-10 | LDF file integrity verified | Integrate LDF loading into secure boot manifest checking |

---

## 4. Safe API Usage Patterns

### 4.1 Frame Validation (SG-01, SG-02)

**ALWAYS** validate frames received from external hardware before using them:

```cpp
lin::Frame hw_frame = read_from_hardware();
try {
    lin::validate_frame(hw_frame);        // throws ErrInvalidFrame if invalid
} catch (const lin::ErrInvalidFrame& e) {
    // handle or log; do NOT process the frame
    trigger_safe_state();
    return;
}
process_frame(hw_frame);
```

**NEVER** bypass `validate_frame()` for frames originating outside cpp-LIN.

### 4.2 E2E Protection for Safety-Critical Payloads (SG-05)

Use `lin::safety::Protector` and `Receiver` for all ASIL-B data paths:

```cpp
// Sender side (e.g., sensor ECU)
lin::safety::Config cfg{.data_id = 0x0042, .source_id = 0x0001};
lin::safety::Protector protector{cfg};

auto raw_payload   = read_sensor_value();
auto safe_payload  = protector.protect(raw_payload);
bus->publish(FRAME_ID_SENSOR, safe_payload);

// Receiver side (e.g., actuator ECU)
lin::safety::Receiver receiver{cfg};
try {
    auto verified = receiver.unwrap(frame.data);
    actuate(verified);
} catch (const lin::safety::E2EError& e) {
    // mandatory safe state on E2E failure
    trigger_safe_state();
    log_safety_event(e.kind(), e.counter());
}
```

**NEVER** ignore `E2EError` exceptions — they indicate data corruption or replay.

### 4.3 Error Handling for ErrNoResponse (SG-01)

A `lin::ErrNoResponse` from `send_header()` or `master::Node::run()` means a
slave did not respond. This is **not a fatal error** for cpp-LIN, but the
integrating system must decide the safe-state action:

```cpp
node.on_error([](std::error_code err) {
    if (err == lin::make_error_code(lin::Errc::no_response)) {
        // slave absent — integrating system decides safe action
        increment_absent_slave_counter();
        if (absent_count > MAX_ABSENT_FRAMES) {
            trigger_degraded_mode();
        }
    }
});
```

**NEVER** silently discard `ErrNoResponse` without a counter or timeout guard.

### 4.4 Schedule Table Management (SG-01)

```cpp
lin::master::Node master_node(bus);
auto err = master_node.set_schedule({
    {0x10, 10},   // ID 0x10, 10 ms slot
    {0x20, 10},
    {0x3C, 5},    // diagnostic ID
});
if (err) {
    // schedule rejected — programming error
    log_fault(err);
    return;
}

std::atomic<bool> stop{false};
std::thread run_thread([&]{
    (void)master_node.run(stop);
});
// ... on shutdown:
stop.store(true);
run_thread.join();
bus->close();
```

**ALWAYS** join the run thread before destroying the bus.

### 4.5 Shutdown Sequencing (ASM-08)

```cpp
// Correct shutdown order:
stop_flag.store(true);      // 1. Signal run() to exit
run_thread.join();          // 2. Wait for run() to return
bus->close();               // 3. Close bus (signals subscriber threads)
// 4. subscriber_thread.join() if applicable
// 5. shared_ptr<bus> goes out of scope
```

Incorrect order (bus destroyed before threads exit) is undefined behaviour.

---

## 5. Anti-Patterns (Safety Violations)

| Anti-Pattern | Consequence | Correct Alternative |
|-------------|-------------|---------------------|
| Skip `validate_frame()` on hardware input | Invalid ID / oversized payload silently processed | Always call `validate_frame()` at hardware boundary |
| Ignore `E2EError` from `Receiver::unwrap()` | Corrupted or replayed payload propagates to actuator | Trigger safe state; log event |
| Destroy bus while subscriber threads running | Undefined behaviour (data race, crash) | Call `bus->close()` and join threads first |
| Use `lin::virt::Bus` as production transport | Virtual bus lacks physical LIN timing/collision detection | Implement physical `IBus` adapter for production |
| Call `SetSchedule` concurrently with `Run` | Potential schedule modification mid-cycle | Set schedule before starting `Run`; stop/restart if update needed |
| Multiple `master::Node` instances on same bus | Two masters violate LIN protocol (ASM-05) | One master per bus; validate hardware topology |
| Ignore `set_schedule()` return value | Invalid schedule silently installed | Always check `err` from `set_schedule()` |

---

## 6. Thread Safety Matrix

| Operation | Thread Safe? | Notes |
|-----------|-------------|-------|
| `virt::Bus::publish()` | Yes | Protected by `shared_mutex` |
| `virt::Bus::send_header()` | Yes | Protected by `shared_mutex` |
| `virt::Bus::subscribe()` | Yes | Protected by `shared_mutex` |
| `virt::Bus::close()` | Yes | Idempotent |
| `safety::Protector::protect()` | Yes | Atomic sequence counter |
| `safety::Receiver::unwrap()` | Yes (single consumer) | Mutex-protected; share one receiver per data flow |
| `master::Node::run()` | Single thread only | Call from exactly one thread |
| `master::Node::set_schedule()` | Not during `run()` | Configure before or after; not concurrently |
| `slave::Node::set_response()` | Yes (after construction) | Delegates to bus::publish |

---

## 7. Performance and Memory Budget

| Component | Stack | Heap (peak) | Notes |
|-----------|-------|-------------|-------|
| `virt::Bus::create()` | — | ~1 KB + 64 B/subscriber | Map + vector per subscriber |
| `Chan<Frame>` (depth=64) | — | ~8 KB | 64 × sizeof(Frame) ≈ 64 × 128 B |
| `safety::Protector` | ~16 B | None | Atomic counter only |
| `safety::Receiver` | ~32 B | None | mutex + two counters |
| `master::Node::run()` | ~512 B | None | Loop variables only |
| `ldf::parse()` | ~4 KB | Proportional to LDF size | One-time at startup |

**Recommendation:** Allocate all channels at startup; do not create/destroy `Chan`
objects during safety-critical operation (ASIL-B scope).

---

## 8. Integration with RELAY Router

When using cpp-LIN as a RELAY node (via `lin::adapt()`), the integrating system
must ensure:

1. The RELAY router is started before `adapt()` is called.
2. `linNode->close()` is called before the RELAY router shuts down.
3. The RELAY message envelope's `protocol` field is set to `relay::Protocol::LIN`.
4. The `id` field in `relay::Message` carries the decimal string representation
   of the LIN frame ID (e.g., `"16"` for frame 0x10).

See `RELAY Spec v1.11 §8.3` for the complete LIN-over-RELAY envelope specification.

---

## 9. Qualification Evidence Location

| Artifact | Location |
|----------|----------|
| ASIL-B requirements | `.fusa-reqs.json` |
| Hazard Analysis | `HARA.md` |
| SEooC Assumptions | `SEOOC.md` |
| Safety Plan | `SAFETY_PLAN.md` |
| TARA (cybersecurity) | `TARA.md`, `tara.json` |
| FMEA | `fmea.json` |
| Architecture Spec | `sas.md` |
| CI evidence | GitHub Actions (ASIL-B artifacts uploaded per run) |
| Qualification badge | CI artifact `fusa-badge.svg` |

---

## 10. Derived Safety Requirements for Integrators

Reproducing from `SEOOC.md` for convenience:

| DSR-ID | Requirement |
|--------|-------------|
| DSR-01 | Monitor heap usage; trigger safe state on heap exhaustion |
| DSR-02 | Enforce single-master topology at hardware level |
| DSR-03 | Apply hardware CRC for ASIL-C/D data paths |
| DSR-04 | Call `IBus::close()` before destroying bus with active subscriber threads |
| DSR-05 | Verify LDF file integrity via secure boot mechanism |
| DSR-06 | Implement watchdog timer to detect schedule runner starvation |

---

## 11. References

- `HARA.md` — Hazard Analysis and Risk Assessment
- `SEOOC.md` — Safety Element out of Context assumptions
- `SAFETY_PLAN.md` — Development safety plan
- `TARA.md` — Threat Analysis and Risk Assessment
- `fmea.json` — Failure Mode and Effect Analysis
- `sas.md` — Software Architecture Specification
- ISO 26262:2018 Part 6 §7 — Software integration and verification
- ISO 26262:2018 Part 10 §9 — Safety element out of context
- RELAY Specification v1.11 §8.3 — LIN bus binding
