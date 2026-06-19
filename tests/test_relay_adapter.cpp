// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-LIN-011 REQ-LIN-012 REQ-LIN-019 REQ-LIN-020

#include <lin/lin.hpp>
#include <lin/virtual/bus.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace lin;
using namespace lin::virt;

TEST_CASE("adapt: protocol returns LIN", "[adapter][REQ-LIN-011]") {
    auto bus = Bus::create();
    auto node = adapt(bus);
    CHECK(node->protocol() == relay::Protocol::LIN);
    bus->close();
}

TEST_CASE("adapt: send publishes payload to bus", "[adapter][REQ-LIN-011]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    relay::Message msg;
    msg.protocol = relay::Protocol::LIN;
    msg.id = "16";
    msg.payload = {0xAA, 0xBB};
    auto err = node->send(msg);
    REQUIRE_FALSE(err);

    // Verify response was registered
    auto [f, ferr] = bus->send_header(0x10);
    REQUIRE_FALSE(ferr);
    CHECK(f.id == 0x10);
    CHECK(f.data == std::vector<uint8_t>{0xAA, 0xBB});
    bus->close();
}

TEST_CASE("adapt: subscribe delivers frames as relay::Message", "[adapter][REQ-LIN-012]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    auto [ch, err] = node->subscribe({});
    REQUIRE_FALSE(err);

    bus->publish(0x20, {0x01, 0x02});
    bus->send_header(0x20);

    // Give the bridge goroutine time to relay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto got = ch->try_recv();
    REQUIRE(got.has_value());
    CHECK(got->id == "32");
    CHECK(got->protocol == relay::Protocol::LIN);
    CHECK(got->payload == std::vector<uint8_t>{0x01, 0x02});
    CHECK(got->meta.at("lin.checksum_type") == "enhanced");
    bus->close();
}

TEST_CASE("adapt: close closes the bus", "[adapter]") {
    auto bus = Bus::create();
    auto node = adapt(bus);
    auto err = node->close();
    REQUIRE_FALSE(err);
    // Second close should be idempotent
    REQUIRE_FALSE(bus->close());
}
