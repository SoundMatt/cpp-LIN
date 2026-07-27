// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-LIN-011 REQ-LIN-012

// Verifies the RELAY spec §13.7.1 canonical `mock` module is present and
// usable as a full IMasterBus implementation, same as lin::virt::Bus.

#include <lin/mock/mock.hpp>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

using namespace lin;

TEST_CASE("lin::mock::Bus is the same type as lin::virt::Bus", "[mock]") {
    STATIC_CHECK(std::is_same_v<mock::Bus, virt::Bus>);
}

TEST_CASE("lin::mock::Bus is usable as a full IMasterBus", "[mock]") {
    auto bus = mock::Bus::create();
    REQUIRE(bus != nullptr);

    auto err = bus->publish(0x10, {0xAA, 0xBB});
    CHECK_FALSE(err);

    auto [f, herr] = bus->send_header(0x10);
    CHECK_FALSE(herr);
    CHECK(f.id == 0x10);
    CHECK(f.data == std::vector<uint8_t>{0xAA, 0xBB});

    (void)bus->close();
}
