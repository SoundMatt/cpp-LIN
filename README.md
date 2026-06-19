# cpp-LIN

C++ LIN bus library — RELAY-conformant, ISO 26262 ASIL-B.

[![CI](https://github.com/SoundMatt/cpp-LIN/actions/workflows/ci.yml/badge.svg)](https://github.com/SoundMatt/cpp-LIN/actions/workflows/ci.yml)
[![RELAY Conformant](https://img.shields.io/badge/RELAY-v1.10%20conformant-blue)](https://github.com/SoundMatt/RELAY)
[![ASIL-B](https://img.shields.io/badge/ISO%2026262-ASIL--B-orange)](SAFETY_PLAN.md)
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](LICENSE)

## Overview

cpp-LIN is a production-quality C++ implementation of the LIN (Local Interconnect
Network) protocol, following the RELAY specification v1.10. It is the C++ sibling
of [go-LIN](https://github.com/SoundMatt/go-LIN) and is patterned after
[cpp-CAN](https://github.com/SoundMatt/cpp-CAN).

### Features

- **Virtual bus** — in-process, thread-safe LIN bus for testing and simulation
- **LDF parser** — parse LIN Description Files (`.ldf`), decode signals
- **Master node** — schedule table runner with configurable slot timing
- **Slave node** — publish response payloads for specific LIN frame IDs
- **E2E safety** — End-to-End protection (10-byte header, CRC-16/CCITT-FALSE)
- **RELAY adapter** — exposes any `IBus` as a `relay::INode` for cross-protocol routing
- **CLI** — `cpp-lin-cli` binary for RELAY conform/interop test harness integration
- **ISO 26262 ASIL-B** — full requirements traceability via `.fusa-reqs.json`

## Quick Start

```sh
# Clone
git clone https://github.com/SoundMatt/cpp-LIN.git
cd cpp-LIN

# Build (requires CMake >= 3.21, C++17)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Test
ctest --test-dir build --output-on-failure

# CLI
./build/cli/cpp-lin-cli version
./build/cli/cpp-lin-cli capabilities
```

## Architecture

```
include/lin/
  channel.hpp        — Chan<T>: bounded, thread-safe FIFO
  relay.hpp          — RELAY v1.10 types (Protocol, Message, INode, …)
  lin.hpp            — IBus, IMasterBus, Frame, Filter, free functions
  virtual/bus.hpp    — In-process virtual LIN bus
  safety/e2e.hpp     — E2E Protector / Receiver
  ldf/parser.hpp     — LDF file parser / signal decoder
  master/node.hpp    — Master schedule runner
  slave/node.hpp     — Slave response publisher

src/
  relay.cpp          — RELAY type implementations
  lin.cpp            — PID, checksum, validation, RELAY adapter
  virtual/bus.cpp    — Virtual bus implementation
  safety/e2e.cpp     — CRC-16 and E2E framing
  ldf/parser.cpp     — LDF tokeniser and parser
  master/node.cpp    — Master node run loop
  slave/node.cpp     — Slave node

cli/
  main.cpp           — CLI entry point (version/capabilities/status/convert)
  json.hpp           — JSON helpers

tests/               — Catch2 unit tests (≥ 70% line coverage enforced)
testdata/            — RELAY spec test vectors
docker/              — Dockerfile + docker-compose.yml
```

## LIN Protocol Notes

| Property | Value |
|----------|-------|
| Max frame ID | 0x3F (6-bit) |
| Max data length | 8 bytes |
| Diagnostic request ID | 0x3C |
| Diagnostic response ID | 0x3D |
| Classic checksum | Data bytes only |
| Enhanced checksum | PID + data bytes |
| Diagnostic frames | Classic checksum only |

**Protected Identifier (PID):**
```
P0 = ID0 ^ ID1 ^ ID2 ^ ID4        (bit 6)
P1 = NOT(ID1 ^ ID3 ^ ID4 ^ ID5)   (bit 7)
```

## RELAY Integration

cpp-LIN exposes a RELAY v1.10 `INode` adapter:

```cpp
#include <lin/lin.hpp>
#include <lin/virtual/bus.hpp>

auto bus = std::make_shared<lin::virt::Bus>();
auto node = lin::adapt(bus);
// node is a relay::INode* — plug it into any RELAY router
```

The CLI exposes the RELAY test harness interface:

```sh
relay conform ./build/cli/cpp-lin-cli --strict
relay interop --protocol LIN ./build/cli/cpp-lin-cli
```

## E2E Safety

```cpp
#include <lin/safety/e2e.hpp>

lin::safety::Config cfg{.data_id = 0x0042, .source_id = 0x0001};
lin::safety::Protector prot(cfg);
lin::safety::Receiver recv(cfg);

std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
auto protected_msg = prot.protect(payload);
auto unwrapped = recv.unwrap(protected_msg);   // throws E2EError on failure
```

## LDF Parser

```cpp
#include <lin/ldf/parser.hpp>
#include <fstream>

std::ifstream f("example.ldf");
auto db = lin::ldf::parse(f);

auto& fr = db->frame(0x10);   // frame by ID
auto decoded = db->decode(0x10, {0xAB, 0xCD});  // signal values
```

## ISO 26262 ASIL-B

All requirements are tracked in `.fusa-reqs.json`. Every source file carries
`// fusa:req REQ-XXX-NNN` annotations; every test file carries
`// fusa:test REQ-XXX-NNN` annotations. The CI pipeline runs `cpfusa qualify`
to enforce ASIL-B traceability coverage.

See [SAFETY_PLAN.md](SAFETY_PLAN.md) for the full safety plan.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). All commits require a DCO sign-off:

```sh
git commit -s -m "feat: your change"
```

## License

Mozilla Public License 2.0 — see [LICENSE](LICENSE).

Copyright (c) 2026 Matt Jones. All rights reserved.
