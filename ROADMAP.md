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

## v0.4.0 — RELAY ecosystem audit fixes (current)

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
