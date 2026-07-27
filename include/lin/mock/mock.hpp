// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// mock/mock.hpp — the RELAY spec's canonical `mock` module (§13.7.1).
//
// §13.7.1 requires every implementation to provide, and name, a module
// literally called `mock`: "in-process mock implementation (§7 Form 2 New)".
// This is a distinct registry entry from `virtual` (§13.7.2, "in-process
// virtual bus/transport ... for bus protocols"), but cpp-LIN's virtual bus
// (lin::virt::Bus) already IS a full in-process IBus/IMasterBus
// implementation — it is used both for tests and as the documented
// development bus. Rather than duplicate that type, `mock` is provided as a
// thin alias so the canonical name is present without a second
// implementation to keep in sync.
//
// fusa:req REQ-LIN-011 REQ-LIN-012

#pragma once

#include <lin/virtual/bus.hpp>

namespace lin::mock {

// Bus is the RELAY-spec-canonical `mock` in-process implementation.
// Identical to lin::virt::Bus (§13.7.2) — see file header for rationale.
using Bus = lin::virt::Bus;

} // namespace lin::mock
