# Changelog

All notable changes to cpp-LIN are documented here. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.4.0] — 2026-07-27

cpp-LIN v0.4.0 — RELAY ecosystem audit fixes. Fixes all 8 open issues filed by
the RELAY ecosystem audit (2026-07-27). See PR #12 for full details.

### Fixed — spec conformance

- `lin::Errc::invalid_frame` — a LIN-specific error category, distinct from
  `ErrPayloadTooLarge`, for out-of-range LIN frame IDs (spec §5.3:
  `ErrInvalidFrame` and `ErrPayloadTooLarge` are distinct; `ValidateFrame`
  MUST NOT return `ErrPayloadTooLarge`). Applied to
  `virtual::Bus::do_publish()`/`send_header()`,
  `master::Node::set_schedule()`/`run()`, and `slave::Node::set_response()`.
- `convert` now writes the correct `ErrInvalidFrame` sentinel name to stderr
  instead of the nonexistent `ErrInvalidInput` (spec §11.2).
- `capabilities` now reports an actual transport backend (`"virtual"`)
  instead of the protocol name (`"LIN"`) in the `transports` array
  (spec §12.2).
- `"tool"` field lowercased to `"cpp-lin"` to match the CLI-binary-name
  convention used by peers (`go-can`, `cpp-rcp`) (spec §13.2).
- `version`/`status` now support `--format text|json`, exiting `2` on an
  unrecognized format value or any other unrecognized argument (spec §11.1).
- `lin::mock` — the RELAY spec §13.7.1 canonical `mock` module, aliasing the
  existing `lin::virt::Bus`.

## [0.3.0] — 2026-06-19

RELAY v1.11 §17.7 — CLI waiver removed. RELAY v1.11 removes the waiver that
previously allowed CLI-less C++ library implementations to mark CLI
conformance requirements as "not applicable". Every conformant C++
implementation must now ship the `version` / `capabilities` / `status` CLI,
buildable via `-DRELAY_BUILD_CLI=ON`.

cpp-LIN has shipped the full CLI since v0.1.0 — no new code was required.
This release aligns the declared spec version and CMake interface with the
new spec text (`kSpecVersion "1.10"` → `"1.11"` in `relay.hpp`/`lin.hpp`,
`spec_version` in CLI JSON output, `RELAY_BUILD_CLI` CMake alias option,
RELAY test vectors updated to `spec_version: "1.11"`).

## [0.2.0] — 2026-06-19

Full Safety & Cyber Pack.

### Added

- **Safety documentation** (ISO 26262 / IEC 61508 / DO-178C): `TARA.md` +
  `tara.json` (7 threat scenarios, CVSS 3.1, 8 security controls, attack
  tree, residual risk acceptance — ISO/SAE 21434:2021 + IEC 62443-4-1 SL-2);
  `fmea.json` (14 failure mode entries linked to HARA hazards H-01..H-05, all
  closed — ISO 26262-9); `sas.md` (9 modules, invariants, public interfaces,
  data flow, memory safety strategy — ISO 26262-6 §7.4); `boundary.mermaid`
  (ISO 26262-6 §7.4.1); `SAFETY_MANUAL.md` (safe API patterns, anti-patterns,
  thread safety matrix, memory budget, shutdown sequencing — ISO 26262-6 §7 /
  Part 10 §9 SEooC); `SECURITY.md` and `INCIDENT-RESPONSE.md` (coordinated
  disclosure, ASIL impact assessment, ISN process).
- **Requirements traceability**: 19 IEC 62443 security controls in
  `.fusa-iec62443.json` (expanded from a 5-line stub); 32 new
  `REQ-RELAY-001..029,051,056,059` entries in `.fusa-reqs.json`, closing all
  orphan `fusa:req` annotations. All 109 pre-existing FuSa requirements
  retained; full traceability matrix intact.

## [0.1.0] — 2026-06-19

First release of the C++ LIN bus library — a complete port of go-LIN
implementing the RELAY spec v1.10, qualified to ISO 26262 ASIL-B.

### Added

- Core LIN types: `Frame`, `Filter`, `ScheduleEntry`, `IBus`, `IMasterBus` —
  full feature parity with go-LIN.
- Virtual bus (`lin::virt::Bus`): thread-safe, metrics, health provider,
  subscriber drainer.
- E2E safety (`lin::safety`): CRC-16/CCITT-FALSE end-to-end data protection
  with monotonic sequence counter.
- LDF parser (`lin::ldf`): parses LIN Description Files — frames, signals,
  schedule tables.
- Master node (`lin::master::Node`): schedule-driven header transmission
  with per-slot callbacks.
- Slave node (`lin::slave::Node`): response registry with frame
  subscription.
- RELAY adapter (`lin::adapt()`): wraps any `IBus` as a `relay::INode`.
- CLI binary (`cpp-lin-cli`): RELAY `version`, `capabilities`, `status`,
  `convert --protocol LIN`.

[Unreleased]: https://github.com/SoundMatt/cpp-LIN/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/SoundMatt/cpp-LIN/releases/tag/v0.4.0
[0.3.0]: https://github.com/SoundMatt/cpp-LIN/releases/tag/v0.3.0
[0.2.0]: https://github.com/SoundMatt/cpp-LIN/releases/tag/v0.2.0
[0.1.0]: https://github.com/SoundMatt/cpp-LIN/releases/tag/v0.1.0
