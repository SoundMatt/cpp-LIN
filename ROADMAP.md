# Roadmap — cpp-LIN

## v0.1.0 — Initial Release (current)

- [x] Virtual bus with Classic and Enhanced checksum support
- [x] Protected Identifier (PID) computation and verification
- [x] LDF parser (nodes, signals, frames, schedule tables, signal decode)
- [x] Master node with schedule table runner (`std::stop_token`)
- [x] Slave node with per-ID response registration
- [x] E2E safety: Protector and Receiver (CRC-16/CCITT-FALSE, 10-byte header)
- [x] RELAY v1.11 adapter (`relay::INode`)
- [x] CLI binary (`cpp-lin-cli`) for relay conform/interop
- [x] ISO 26262 ASIL-B requirements traceability
- [x] CI pipeline: build, test, coverage, sanitizers, RELAY conform, cpfusa
- [x] Docker multi-stage image

## v0.2.0 — Enhanced Bus Features

- [ ] Sleep/wake frame support (LIN 2.x go-to-sleep command)
- [ ] Break field and sync field simulation in virtual bus
- [ ] Configurable baud rate metadata on virtual bus
- [ ] `IMasterBus::set_schedule` hot-swap without restarting `run()`
- [ ] `IBus::unsubscribe` to cancel an active subscription

## v0.3.0 — Diagnostics

- [ ] ISO 15765-3 / LIN Transport Layer (segmented diagnostic frames)
- [ ] UDS (ISO 14229) over LIN TP adapter
- [ ] Diagnostic session management in master node

## v0.4.0 — Hardware Abstraction

- [ ] Serial/UART hardware bus implementation (Linux `termios`)
- [ ] SocketLIN adapter (if Linux kernel support is available)
- [ ] PEAK LIN hardware adapter

## v0.5.0 — AUTOSAR-Lite

- [ ] LIN Interface (LINIf) / LIN Driver (LINDrv) API-compatible shim
- [ ] COM-stack signal packing / unpacking with endianness support
- [ ] PDU router integration point

## v1.0.0 — Production Hardening

- [ ] ASIL-D dual-channel redundancy pattern example
- [ ] Formal verification of checksum and PID algorithms (SPARK/Frama-C)
- [ ] DO-178C DAL-C qualification evidence package
- [ ] MISRA C++ 2023 compliance report
- [ ] ISO/SAE 21434 TARA update (v1.0 threat model)
