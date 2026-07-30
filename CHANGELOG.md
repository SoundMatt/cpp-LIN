# Changelog

All notable changes to cpp-LIN are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.1]
### Added
- `verify_checksum()`: validates a received checksum byte against the
  locally-computed value (ISO 17987 frame reception).
- `lin::Errc::no_response`, a distinct "no slave answered" sentinel that
  compares equivalent to a relay timeout, replacing the previous generic
  timeout return from `send_header`.
- `CHANGELOG.md` (this file).
### Changed
- Declared RELAY spec version bumped 1.11 → 2.0 across headers, CLI JSON,
  README, HARA, and requirements.
- `master::Node::set_schedule` now accepts an empty schedule (disables
  scheduled transmission), matching the virtual bus and the spec.
- `Chan::send_drop_oldest` reports whether it evicted an item so DropOldest
  back-pressure now increments the drop counter instead of the delivery
  counter on eviction.
- `Frame::checksum_type` now defaults to Classic (the enum's zero value),
  matching the canonical default used by the other language bindings.
- `capabilities` no longer duplicates `virtual` between `features` and
  `transports`.
- E2E `Receiver::unwrap` no longer advances its sequence counter on a
  rejected (out-of-order) frame, so a persistent shifted stream keeps being
  flagged instead of resyncing after a single error.
### Fixed
- `from_message` now rejects an unrecognized `lin.checksum_type` instead of
  silently coercing it to Classic.
- `testdata/relay-vectors/lin-frame.json` now encodes `lin.checksum_type` as
  `"enhanced"` instead of the numeral `"1"`.
- HARA SG-03 status corrected from "Implemented" to "Partial" pending a
  verified-checksum code path in a real (non-virtual) bus backend.
- Corrected the Safety Goal cross-references on three `.fusa-reqs.json`
  requirement rationales (SG-02/SG-01 instead of SG-05) and added the
  previously-undefined `REQ-CLI-001..006` / `REQ-SEC-013` requirement
  entries for existing `fusa:req` trace tags.
- HARA citation for error handling corrected from spec §15 to §5.

## [0.4.0]
### Added
- RELAY ecosystem audit fixes.

## [0.3.0]
### Added
- RELAY spec v1.11 conformance (see ROADMAP.md for detail).

## [0.2.0]
### Added
- Full safety and cyber pack: TARA, FMEA, SAS, boundary diagram, Safety
  Manual, IEC 62443 artifacts (see ROADMAP.md for detail).

## [0.1.0]
### Added
- Initial C++ LIN bus port: core frame types, PID/checksum, virtual bus,
  master/slave nodes, E2E safety layer, LDF parser, and RELAY adapter/CLI.
