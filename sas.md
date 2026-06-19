# Software Architecture Specification (SAS) — cpp-LIN

**Document ID:** SAS-cpp-LIN-001  
**Revision:** 0.1  
**Date:** 2026-06-19  
**Author:** Matt Jones  
**Standard:** ISO 26262:2018 Part 6 §7.4, IEC 61508:2010 Part 3 §7.4  
**ASIL:** ASIL-B

---

## 1. Purpose

This SAS defines the decomposition of cpp-LIN into software modules, their
responsibilities, invariants, interfaces, and inter-module dependencies. It
serves as:

- The primary evidence for ISO 26262-6 §7.4.1 (software architectural design)
- The basis for the FMEA (see `fmea.json`)
- The design reference for the safety manual (see `SAFETY_MANUAL.md`)

---

## 2. Architectural Decomposition

### 2.1 Module List

| Module | Namespace | Files | ASIL | Role |
|--------|-----------|-------|------|------|
| M-01 Core | `lin::` | `src/lin.cpp`, `include/lin/lin.hpp` | ASIL-B | Frame validation, PID, checksum, message conversion |
| M-02 Virtual Bus | `lin::virt::` | `src/virtual/bus.cpp`, `include/lin/virtual/bus.hpp` | ASIL-B | Thread-safe in-memory LIN transport |
| M-03 Safety | `lin::safety::` | `src/safety.cpp`, `include/lin/safety.hpp` | ASIL-B | E2E protection (CRC-16, sequence counter) |
| M-04 Master | `lin::master::` | `src/master.cpp`, `include/lin/master.hpp` | ASIL-B | Schedule-driven frame exchange |
| M-05 Slave | `lin::slave::` | `src/slave.cpp`, `include/lin/slave.hpp` | ASIL-B | Response registration and subscription |
| M-06 LDF | `lin::ldf::` | `src/ldf.cpp`, `include/lin/ldf.hpp` | ASIL-A | LDF file parser |
| M-07 RELAY Adapter | `lin::` | `src/lin.cpp` (adapt()) | ASIL-B | Wraps IBus as relay::INode |
| M-08 RELAY Types | `relay::` | `src/relay.cpp`, `include/lin/relay.hpp` | ASIL-B | RELAY spec types, error codes, subscriber options |
| M-09 Channel | `lin::` | `include/lin/channel.hpp` | ASIL-B | Type-safe bounded SPSC/MPSC channel |

### 2.2 Module Dependencies

```
M-04 (Master) ──► M-01 (Core) ──► M-08 (RELAY Types)
M-05 (Slave)  ──► M-01 (Core)
M-02 (Virt)   ──► M-01 (Core)
M-07 (Adapt)  ──► M-01 (Core) ──► M-08 (RELAY Types)
M-07 (Adapt)  ──► M-09 (Channel)
M-03 (Safety) ──► (none — standalone, no LIN dependency)
M-06 (LDF)    ──► (none — parse-only, no bus dependency)
```

No circular dependencies. M-03 and M-06 are independently usable.

---

## 3. Module Specifications

### M-01 — Core (`lin::`)

**Responsibilities:**
- Frame validation (`validate_frame`)
- PID computation and verification (`protect_id`, `verify_pid`)
- Checksum computation (`calc_checksum`, `ChecksumType`)
- Message conversion (`to_message`, `from_message`)
- IBus abstract interface definition
- Error code definition (`Errc`, `ErrInvalidFrame`, `ErrNoResponse`)

**Invariants:**
- `validate_frame(f)` never modifies `f`
- `protect_id(id)` is a pure function: same input → same output, no side effects
- `protect_id(protect_id(id) & 0x3F)` is idempotent for any valid id
- `calc_checksum` never throws; result is always in [0x00, 0xFF]

**Public Interface:**

```cpp
// Boundary validation
void validate_frame(const Frame& f);  // throws ErrInvalidFrame

// PID
uint8_t protect_id(uint8_t id) noexcept;
uint8_t verify_pid(uint8_t pid);       // throws ErrInvalidFrame

// Checksum
uint8_t calc_checksum(uint8_t pid, const std::vector<uint8_t>& data,
                       ChecksumType ct) noexcept;

// Message conversion
relay::Message to_message(const Frame& f);
Frame from_message(const relay::Message& m);  // throws ErrInvalidFrame

// Bus abstraction
class IBus { ... };
std::unique_ptr<relay::INode> adapt(std::shared_ptr<IBus> bus);
```

**Requirements:** REQ-LIN-001..021, REQ-ADAPT-001..005

---

### M-02 — Virtual Bus (`lin::virt::`)

**Responsibilities:**
- Thread-safe in-memory simulation of a LIN bus
- Response registration and send_header frame exchange
- Subscriber fan-out with backpressure (drop-full-channel)
- Health reporting and metrics collection
- Close/drain lifecycle

**Invariants:**
- `virt::Bus` is safe for concurrent access from any number of threads
- `close()` is idempotent — calling it twice has no effect
- After `close()`, all subscriber channels are closed; blocking `Chan::recv()` returns `nullopt`
- `publish(id, data)` stores a defensive copy — mutations to caller's data do not affect stored response

**Internal Design:**
- `std::shared_mutex` for read/write access to response table and subscriber list
- Subscriber channels: `std::shared_ptr<Chan<Frame>>` with bounded capacity (default 64)
- Health/Metrics: atomic counters, no lock required for reading

**Public Interface:**

```cpp
static std::shared_ptr<Bus> create();

std::error_code publish(uint8_t id, std::vector<uint8_t> data);
std::pair<Frame, std::error_code> send_header(uint8_t id);
std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
    subscribe(std::vector<Filter> filters, std::vector<SubscriberOption> opts);
std::error_code close();
Health health() const;
Metrics metrics() const;
std::error_code close_with_drain(std::chrono::milliseconds timeout);
```

**Requirements:** REQ-VIRT-001..019

---

### M-03 — Safety (`lin::safety::`)

**Responsibilities:**
- E2E protection: CRC-16/CCITT-FALSE over 10-byte header + payload
- Sequence counter (monotonically increasing, atomic)
- Header embedding: DataID (bytes 0-1), SourceID (bytes 2-3), counter (bytes 4-7), CRC (bytes 8-9)
- E2E verification: CRC check, sequence gap detection

**Invariants:**
- `Protector::protect(payload)` output length is exactly `10 + payload.size()`
- `Protector::protect()` is thread-safe (uses `std::atomic<uint32_t>` for counter)
- `Receiver::unwrap()` never returns partial results — either full payload or throws `E2EError`
- `E2EError::kind()` is always one of: `ErrHeaderTooShort`, `ErrCRCMismatch`, `ErrSequenceGap`

**CRC Algorithm:**
- Polynomial: 0x1021 (CRC-16/CCITT-FALSE)
- Initial value: 0xFFFF
- Input reflection: No; Output reflection: No; XOR out: 0x0000
- CRC computed over `[header with CRC bytes zeroed] || payload`

**Public Interface:**

```cpp
struct Config { uint16_t data_id; uint16_t source_id; };

class Protector {
    explicit Protector(Config cfg);
    std::vector<uint8_t> protect(const std::vector<uint8_t>& payload);
};

class E2EError : public std::exception { ... };

class Receiver {
    explicit Receiver(Config cfg);
    std::vector<uint8_t> unwrap(const std::vector<uint8_t>& protected_data);
};
```

**Requirements:** REQ-SAFETY-001..015

---

### M-04 — Master (`lin::master::`)

**Responsibilities:**
- Schedule-driven LIN frame exchange: iterate table in order, loop
- Per-slot timing: `std::this_thread::sleep_for(DelayMs)`
- Callback dispatch: `OnFrame` on success, `OnError` on error
- Graceful shutdown via `std::atomic<bool>& stop`

**Invariants:**
- `run()` processes slots strictly in the order they appear in the schedule table
- `run()` returns without stopping the schedule on per-slot `ErrNoResponse` (REQ-MASTER-013)
- `run()` only returns when `stop.load()` is true or the bus is closed
- `set_schedule()` stores a defensive copy

**Public Interface:**

```cpp
class Node {
    explicit Node(std::shared_ptr<IBus> bus);
    std::error_code set_schedule(std::vector<ScheduleEntry> entries);
    std::error_code run(const std::atomic<bool>& stop);
    void on_frame(std::function<void(Frame)> cb);
    void on_error(std::function<void(std::error_code)> cb);
    std::pair<Frame, std::error_code> send_header(uint8_t id);
};
```

**Requirements:** REQ-MASTER-001..013

---

### M-05 — Slave (`lin::slave::`)

**Responsibilities:**
- Response registration via `IBus::publish()`
- Tracking registered IDs for `RegisteredIDs()` diagnostic
- Subscription delegation to `IBus::subscribe()`

**Invariants:**
- `set_response(id, nullptr)` removes `id` from the registered set
- `RegisteredIDs()` returns an empty vector (not null) when no responses registered
- `set_response` validates id ≤ 0x3F before calling `bus->publish()`

**Requirements:** REQ-SLAVE-001..008

---

### M-06 — LDF Parser (`lin::ldf::`)

**Responsibilities:**
- Parse LIN Description File (LDF) text
- Populate `DB` struct with: protocol version, baud rate, node names, frame
  descriptors, signal definitions, schedule tables
- Never panic on arbitrary input (REQ-LDF-014)

**Invariants:**
- `ldf::parse()` always returns a non-null `DB`; empty fields for unparseable sections
- `DB::Frame(id)` returns null for unknown IDs (not a crash)
- `DB::Signal(name)` returns null for unknown names (not a crash)
- `DB::Frames()` returns a defensive copy

**Requirements:** REQ-LDF-001..015

---

### M-07 — RELAY Adapter (`lin:: adapt()`)

**Responsibilities:**
- Wraps an `IBus` as a `relay::INode`
- `send()`: decodes frame ID from `relay::Message::id`, publishes to bus
- `subscribe()`: spawns a background thread that converts `Chan<Frame>` to `Chan<relay::Message>`
- `close()`: delegates to `IBus::close()`

**Invariants:**
- `protocol()` always returns `relay::Protocol::LIN`
- `send()` rejects non-numeric or out-of-range ID strings without publishing
- Background subscriber threads exit when the underlying `Chan<Frame>` is closed
- `seq` counter in relay::Messages is strictly monotonically increasing per `LinAdapter` instance

**Requirements:** REQ-ADAPT-001..005

---

### M-08 — RELAY Types (`relay::`)

**Responsibilities:**
- Protocol enum (CAN=1, DDS=2, LIN=3, MQTT=4, RCP=5, SOMEIP=6)
- `Message` envelope struct
- `Errc` error codes and `std::error_category`
- `BackPressurePolicy` enum
- `SubscriberOption` / `SubscriberConfig` / `apply_options()`
- `INode`, `ICaller` abstract interfaces
- Optional capability interfaces: `IHealthProvider`, `IMetricsProvider`, `IDrainer`
- kSpecVersion constant "1.10"

**Requirements:** REQ-RELAY-001..029, REQ-RELAY-051, REQ-RELAY-056, REQ-RELAY-059

---

### M-09 — Channel (`lin::Chan<T>`)

**Responsibilities:**
- Bounded, thread-safe, multi-producer / multi-consumer channel
- Blocking `send()` / `recv()` and non-blocking `try_send()` / `try_recv()`
- Drop-oldest policy: `send_drop_oldest()`
- `close()`: unblocks all waiters; subsequent `recv()` drains remaining items then returns `nullopt`

**Invariants:**
- `Chan` is constructed with a fixed capacity; it never grows dynamically
- After `close()`, `send()` returns immediately (channel closed)
- Items pushed before `close()` are still retrievable by `recv()` until drained

---

## 4. Safety Architecture Decisions

### 4.1 No Dynamic Dispatch in Safety-Critical Path

`validate_frame()`, `protect_id()`, `verify_pid()`, and `calc_checksum()` are
free functions with no virtual dispatch. This ensures deterministic execution
time and avoids virtual table corruption hazards.

### 4.2 E2E at Application, Not Transport, Layer

E2E protection (M-03) is applied above the bus transport (M-02). This ensures
protection even when using the virtual bus for test, and is consistent with
ISO 26262-6 E2E profile placement.

### 4.3 Single-Writer Channel Design

`Chan<T>` is designed for single-writer (producer) / single-reader (consumer)
use per channel instance. Multi-subscriber fan-out is implemented by the bus
layer (M-02), which holds one channel per subscriber.

### 4.4 Separation of Physical and Software Concerns

cpp-LIN deliberately provides no physical layer implementation. `IBus` is the
boundary. All hardware I/O is the integrating system's responsibility (ASM-09).
This enforces a clean separation that enables independent ASIL qualification.

---

## 5. Data Flow for Safety-Critical Path

```
Sensor ECU (slave)
  │
  ▼ lin::safety::Protector::protect(raw_payload)
  │ → header{DataID, SourceID, counter, CRC} + raw_payload
  │
  ▼ lin::virt::Bus::publish(frame_id, protected_payload)
  │ → stored in response table
  │
  ▼ lin::master::Node::run()
  │ → calls bus::send_header(frame_id)
  │ → receives Frame{id, data=protected_payload, pid, checksum}
  │
  ▼ lin::safety::Receiver::unwrap(frame.data)
  │ → verifies CRC, sequence counter
  │ → returns raw_payload or throws E2EError
  │
  ▼ Actuator ECU (integrating system)
```

---

## 6. Memory Safety Strategy

| Risk | Mitigation |
|------|-----------|
| Buffer overflow in frame parsing | `validate_frame()` checks payload length; no raw pointer arithmetic |
| Use-after-free on bus close | `shared_ptr<Bus>` keeps bus alive while subscribers hold references |
| Stack overflow in recursive code | No recursive functions in cpp-LIN core |
| Heap exhaustion | Bounded `Chan<T>` capacity; integrator monitors heap (DSR-01) |
| Data race | `shared_mutex` in `virt::Bus`; `atomic<uint32_t>` for safety counter |

**CI verification:** ASan+UBSan (sanitizers job), ThreadSanitizer (tsan job).

---

## 7. References

- `HARA.md` — Hazard Analysis
- `TARA.md` — Threat Analysis  
- `fmea.json` — Failure Mode and Effect Analysis
- `SAFETY_MANUAL.md` — Integration guidance
- `SEOOC.md` — SEooC assumptions
- ISO 26262:2018 Part 6 §7.4 — Software architectural design
- IEC 61508:2010 Part 3 §7.4 — Software architecture
