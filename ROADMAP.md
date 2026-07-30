# Roadmap — cpp-LIN

## v0.1.0 — Initial Release

- [x] Virtual bus with Classic and Enhanced checksum support
- [x] Protected Identifier (PID) computation and verification
- [x] LDF parser (nodes, signals, frames, schedule tables, signal decode)
- [x] Master node with schedule table runner (`std::stop_token`)
- [x] Slave node with per-ID response registration
- [x] E2E safety: Protector and Receiver (CRC-16/CCITT-FALSE, 10-byte header)
- [x] RELAY v1.10 adapter (`relay::INode`)
- [x] CLI binary (`cpp-lin-cli`) for relay conform/interop
- [x] ISO 26262 ASIL-B requirements traceability
- [x] CI pipeline: build, test, coverage, sanitizers, RELAY conform, cpfusa
- [x] Docker multi-stage image

## v0.2.0 — Full Safety & Cyber Pack

- [x] TARA (ISO/SAE 21434, IEC 62443-4-1 SL-2): threats, controls, `tara.json`
- [x] FMEA (ISO 26262-9) linked to hazards, `fmea.json`
- [x] SAS (ISO 26262-6 §7.4): 9-module architecture specification
- [x] Boundary diagram (ISO 26262-6 §7.4.1): `boundary.mermaid`
- [x] Safety Manual (ISO 26262-6 §7, Part 10 §9): `SAFETY_MANUAL.md`
- [x] `SECURITY.md` + `INCIDENT-RESPONSE.md`: coordinated disclosure, ISN process
- [x] IEC 62443: expanded `.fusa-iec62443.json` security requirements
- [x] Full requirements traceability closure in `.fusa-reqs.json`
- [x] CI: `cpfusa lint`/`cyber` hard gates; ThreadSanitizer gate added

## v0.3.0 — RELAY spec v1.11 conformance

- [x] `kSpecVersion` bumped 1.10 → 1.11 (`relay.hpp`, `lin.hpp`)
- [x] CLI version/capabilities JSON report `spec_version` 1.11
- [x] `RELAY_BUILD_CLI` CMake alias (§17.7, since removed — see v0.4.0)
- [x] Test vectors updated to spec_version 1.11

## v0.4.0 — RELAY ecosystem audit fixes

- [x] LIN-specific error category (`lin::Errc::invalid_frame`) distinct from
      `ErrPayloadTooLarge` for out-of-range frame IDs (spec §5.3)
- [x] `Adapt()` subscription thread no longer captures a raw `this` — fixes a
      use-after-free if the adapter is dropped before the bus is closed
- [x] `convert` writes the correct `ErrInvalidFrame` sentinel name to stderr
- [x] `capabilities` reports an actual transport backend (`virtual`), not the
      protocol name
- [x] `version`/`capabilities`/`status` self-report the real release version
      (driven by `PROJECT_VERSION`, not a second hard-coded literal)
- [x] `version`/`status` support `--format text|json` with exit `2` on an
      unrecognized format or unknown argument (spec §11.1)
- [x] `tool` field lowercased to `cpp-lin` (spec §13.2 CLI-binary-name
      convention), matching peers like `go-can`
- [x] `lin::mock` canonical module (spec §13.7.1), aliasing `lin::virt::Bus`

## v0.4.1 — RELAY ecosystem audit fixes, pass 2 (current)

- [x] Declared RELAY spec version bumped 1.11 → 2.0
- [x] `verify_checksum()` added; HARA SG-03 downgraded from "Implemented" to
      "Partial" pending a verified-checksum path in a real (non-virtual) bus
      backend
- [x] `lin::Errc::no_response` distinct sentinel replaces the generic
      timeout previously returned by `send_header` on no slave response
- [x] `from_message` rejects unrecognized `lin.checksum_type` instead of
      silently defaulting to Classic
- [x] `master::Node::set_schedule` accepts an empty schedule (spec §8.3)
- [x] DropOldest back-pressure counts an eviction as a drop, not a delivery
- [x] `Frame::checksum_type` defaults to Classic (enum zero value)
- [x] E2E `Receiver::unwrap` no longer resyncs its sequence counter after a
      single rejected frame
- [x] Requirement/HARA traceability corrections: `REQ-CLI-001..006` /
      `REQ-SEC-013` defined, three misattributed SG rationale references
      fixed, HARA §15→§5 citation fixed
- [x] `CHANGELOG.md` added

## v0.5.0 — Enhanced Bus Features

- [ ] Sleep/wake frame support (LIN 2.x go-to-sleep command)
- [ ] Break field and sync field simulation in virtual bus
- [ ] Configurable baud rate metadata on virtual bus
- [ ] `IMasterBus::set_schedule` hot-swap without restarting `run()`
- [ ] `IBus::unsubscribe` to cancel an active subscription

## v0.6.0 — Diagnostics

- [ ] ISO 15765-3 / LIN Transport Layer (segmented diagnostic frames)
- [ ] UDS (ISO 14229) over LIN TP adapter
- [ ] Diagnostic session management in master node

## v0.7.0 — Hardware Abstraction

- [ ] Serial/UART hardware bus implementation (Linux `termios`)
- [ ] SocketLIN adapter (if Linux kernel support is available)
- [ ] PEAK LIN hardware adapter

## v0.8.0 — AUTOSAR-Lite

- [ ] LIN Interface (LINIf) / LIN Driver (LINDrv) API-compatible shim
- [ ] COM-stack signal packing / unpacking with endianness support
- [ ] PDU router integration point

## v1.0.0 — Production Hardening

- [ ] ASIL-D dual-channel redundancy pattern example
- [ ] Formal verification of checksum and PID algorithms (SPARK/Frama-C)
- [ ] DO-178C DAL-C qualification evidence package
- [ ] MISRA C++ 2023 compliance report
- [ ] ISO/SAE 21434 TARA update (v1.0 threat model)

## Future — LIN Bus Simulator

LIN is in a different position than this ecosystem's other buses. CAN has
Linux's real `vcan` kernel interface plus `can-utils` as an independent
oracle; DDS has CycloneDDS as a genuine third-party peer (see cpp-DDS's
`interop/` CycloneDDS-peer harness and rust-DDS's `cyclone-interop` feature).
LIN has neither: no OS-native virtual LIN device, and no widely-used
third-party LIN stack anywhere in this ecosystem to test against. A
deliberately-designed simulator is therefore worth more here than it would be
for CAN or DDS — it is not just a convenience, it is close to the only
practical way to develop and test master/slave LIN logic without hardware,
and (if a transport phase is ever added) the only plausible route to real
interop testing between go-LIN, cpp-LIN, and rust-LIN.

**What already exists, precisely.** `lin::virt::Bus` (`include/lin/virtual/bus.hpp`,
aliased as `lin::mock::Bus` per spec §13.7.1) is an in-process,
thread-safe `IMasterBus` — the only one in the codebase. `master::Node::run()`
(`include/lin/master/node.hpp`) already drives a schedule table
(`std::vector<ScheduleEntry>`) in a loop, calling `IMasterBus::send_header()`
per slot and invoking `on_frame`/`on_error` callbacks, and that schedule can
already be sourced straight from a parsed `.ldf` file via
`lin::ldf::DB::schedule(name)`. `slave::Node::set_response()`
(`include/lin/slave/node.hpp`) is a thin wrapper over `IBus::publish()`. None
of this amounts to a simulator, for three concrete reasons visible directly
in `src/virtual/bus.cpp`:

- **Responses are one flat map, not independent slave identities.**
  `virt::Bus::responses_` is a single `std::map<uint8_t, ResponseEntry>`
  keyed by frame ID. Any number of `slave::Node` instances can share one
  `Bus`, but they all write into the same map — two "slaves" registering the
  same ID silently clobber each other (last `publish()` wins) with no
  conflict reported. There is no notion of a slave as a distinct simulated
  ECU with its own behavior.
- **`send_header()` cannot produce a bad frame.** `Bus::send_header()` always
  calls `protect_id()` and `calc_checksum()` itself from the stored data —
  correctness is baked into the transport. There is no code path to make a
  simulated slave answer with a wrong checksum, a wrong PID, or silence on a
  frame it's normally registered for, which means today's error-handling
  tests can only construct bad `Frame` structs by hand; they never exercise
  `master::Node::on_error` or `lin::safety::Receiver::unwrap`'s `E2EError`
  path against a wire-realistic fault.
- **It is strictly in-process.** `Bus::create()` returns a
  `std::shared_ptr<Bus>` that only means anything inside one process's
  address space. There is no transport that lets two separate OS processes —
  two `cpp-lin-cli` instances, or a `cpp-lin` master talking to a `go-lin` or
  `rust-lin` slave — exchange frames at all.

### Phase 1 — Minimal useful simulator (in-process, deterministic)

The smallest cut that is genuinely useful on its own, with no transport work:

- New `lin::sim` namespace (`include/lin/sim/*.hpp`, `src/sim/*.cpp`),
  following this repo's existing convention of one namespace per concern
  (`lin::virt`, `lin::master`, `lin::slave`, `lin::ldf`).
- `sim::ISlaveBehavior` — a pure virtual base class, one required method:
  `virtual std::optional<Frame> on_header(uint8_t id, uint8_t pid) = 0;`,
  giving each simulated ECU a real identity instead of a shared map entry.
  `sim::StaticSlaveBehavior` ships as the default implementation and
  internally delegates to `lin::slave::Node::set_response()`, so existing
  callers of `slave::Node` are not obsoleted — they become the trivial case
  of a behavior-driven slave.
- `sim::Simulator` — an RAII owner (constructor wires a `shared_ptr<virt::Bus>`,
  a `master::Node`, and N registered behaviors together; destructor calls
  `bus->close()`) with `std::error_code add_slave(uint8_t id,
  std::unique_ptr<ISlaveBehavior>)`. Unlike today's `virt::Bus::responses_`,
  `add_slave` fails with a distinct error instead of silently overwriting an
  existing registration for the same ID — the first concrete conflict check
  this simulator adds over what exists today.
- A `MasterSchedule` helper adding an explicit `on_slot_complete` hook around
  `master::Node::run()`, so a test can step one schedule cycle and assert on
  it, rather than only being able to run the loop until an
  `std::atomic<bool>` flag flips (`run()`'s only stop mechanism today).
- Tests under `tests/test_sim.cpp`; links against the existing `cpp-lin`
  CMake target, no new build option required.

### Phase 2 — Fault injection

- `sim::FaultInjector`, a decorator implementing `ISlaveBehavior` and wrapping
  any other behavior (composition, not a change to the `ISlaveBehavior`
  interface), adding: no response (return `std::nullopt`), wrong checksum,
  wrong/garbled PID, and configurable response latency/jitter.
- Delivering a deliberately-wrong checksum or PID needs a raw-injection path
  on the transport, since `virt::Bus::send_header()` currently recomputes
  both from the registered data unconditionally (see above) — this phase's
  transport-side prerequisite is a `virt::Bus` (or `sim`-owned) entry point
  that accepts a fully-formed, un-recomputed `Frame` for the wire.
- Fault schedules (e.g. "corrupt every 5th response to 0x21", "0x30 never
  responds") so error-path tests are reproducible rather than probabilistic.
- Payoff: this is what finally lets `master::Node::on_error` and
  `lin::safety::Receiver::unwrap`'s `E2EError` throw path be exercised
  against realistic simulated wire faults, not just hand-built bad `Frame`
  values in unit tests.

### Phase 3 — Multi-slave scheduling conflicts

- Model real bus-contention scenarios beyond simple registration collisions:
  overlapping schedule tables assigning the same slot to two slaves, slot
  overruns (a slave's simulated response arrives after the master's
  configured `ScheduleEntry::delay_ms` — invisible today, since
  `master::Node::run()` just sleeps a fixed `delay_ms` after each
  `send_header()` with no deadline enforcement), and diagnostic ID
  (`kLINDiagRequestID` 0x3C / `kLINDiagResponseID` 0x3D) arbitration when more
  than one simulated ECU is configured to answer a diagnostic request.
- Surface these as `on_error` conditions or a `sim::ConflictReport`, so a
  schedule table parsed straight out of an `.ldf` file
  (`lin::ldf::DB::schedule()`) can be validated against a simulated slave
  population before it ever reaches real hardware.

### Phase 4 (stretch) — Cross-process transport for interop testing

- A second `IMasterBus`/`IBus` implementation, `lin::ipc::Bus`
  (`include/lin/ipc/bus.hpp` + `src/ipc/bus.cpp`), backed by a Unix domain
  socket (or POSIX shared memory + semaphore for lower-latency same-host
  use), framing `Frame` records — reusing `cli/json.hpp`'s existing JSON
  helpers for the wire encoding would keep it consistent with the CLI's own
  `convert` command rather than inventing a second format. Gated behind a new
  CMake option following this repo's existing `CPPLIN_BUILD_CLI` /
  `RELAY_BUILD_CLI` naming convention, e.g. `CPPLIN_SIM_IPC` (default `OFF`).
- With that in place, two separate processes — a `sim-master` and a
  `sim-slave` binary, or subcommands added to `cpp-lin-cli` — opening the same
  socket path would give cpp-LIN its own two-process self-interop test,
  mirroring rust-DDS's `rtps-interop-peer` pattern (one proven binary run
  twice as independent OS processes) and the shape of go-DDS/cpp-DDS's
  CycloneDDS-peer harness (opt-in `interop/` directory, `docker-compose.yml`,
  `INTEROP_*` env vars, a probe-gated CI job that skips rather than fails
  when no peer is reachable). The difference from DDS's version: there is no
  third-party LIN stack to play the CycloneDDS role, so the peer is a second
  instance of cpp-LIN itself — and, longer term, `go-lin`/`rust-lin` speaking
  the same wire framing over the same socket, which is the actual path to
  real cross-language LIN interop testing.
- Explicitly not a claim that `lin::ipc::Bus` is a real LIN transport — real
  LIN is a single-wire UART-level serial bus (see v0.7.0's planned
  `termios`/SocketLIN/PEAK hardware work for that). This is a test/interop
  *simulation* transport crossing a process boundary, the same role
  `virt::Bus` already plays inside one process.
- This phase is explicitly a stretch goal, not a prerequisite: Phases 1–3
  deliver full standalone dev/test value with zero process-boundary or IPC
  work. Cross-process transport only matters once real interop testing
  between go-LIN/cpp-LIN/rust-LIN is prioritized.
