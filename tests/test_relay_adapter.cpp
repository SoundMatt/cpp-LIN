// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-ADAPT-001 REQ-ADAPT-002 REQ-ADAPT-003 REQ-ADAPT-004 REQ-ADAPT-005
// fusa:test REQ-LIN-011 REQ-LIN-012 REQ-LIN-019 REQ-LIN-020

#include <lin/lin.hpp>
#include <lin/virtual/bus.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace lin;
using namespace lin::virt;

TEST_CASE("adapt: protocol returns LIN", "[adapter][REQ-ADAPT-001]") {
    auto bus = Bus::create();
    auto node = adapt(bus);
    CHECK(node->protocol() == relay::Protocol::LIN);
    (void)bus->close();
}

TEST_CASE("adapt: send publishes payload to bus", "[adapter][REQ-ADAPT-002]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    relay::Message msg;
    msg.protocol = relay::Protocol::LIN;
    msg.id = "16";
    msg.payload = {0xAA, 0xBB};
    auto err = node->send(msg);
    REQUIRE_FALSE(err);

    auto [f, ferr] = bus->send_header(0x10);
    REQUIRE_FALSE(ferr);
    CHECK(f.id == 0x10);
    CHECK(f.data == std::vector<uint8_t>{0xAA, 0xBB});
    (void)bus->close();
}

TEST_CASE("adapt: send rejects out-of-range frame ID string", "[adapter][REQ-ADAPT-003]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    relay::Message msg;
    msg.id = "64";  // 0x40 > kLINMaxID
    msg.payload = {0x01};
    auto err = node->send(msg);
    CHECK(err);  // must reject
    (void)bus->close();
}

TEST_CASE("adapt: send rejects non-numeric ID string", "[adapter][REQ-ADAPT-003]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    relay::Message msg;
    msg.id = "bad";
    msg.payload = {0x01};
    auto err = node->send(msg);
    CHECK(err);
    (void)bus->close();
}

TEST_CASE("adapt: subscribe delivers frames as relay::Message", "[adapter][REQ-ADAPT-004]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    auto [ch, err] = node->subscribe({});
    REQUIRE_FALSE(err);

    (void)bus->publish(0x20, {0x01, 0x02});
    (void)bus->send_header(0x20);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto got = ch->try_recv();
    REQUIRE(got.has_value());
    CHECK(got->id == "32");
    CHECK(got->protocol == relay::Protocol::LIN);
    CHECK(got->payload == std::vector<uint8_t>{0x01, 0x02});
    CHECK(got->meta.at("lin.checksum_type") == "enhanced");
    (void)bus->close();
}

TEST_CASE("adapt: subscribe assigns monotonically increasing seq", "[adapter][REQ-ADAPT-004]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    auto [ch, err] = node->subscribe({});
    REQUIRE_FALSE(err);

    (void)bus->publish(0x10, {1});
    (void)bus->publish(0x20, {2});
    (void)bus->send_header(0x10);
    (void)bus->send_header(0x20);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto m1 = ch->try_recv();
    auto m2 = ch->try_recv();
    REQUIRE(m1.has_value());
    REQUIRE(m2.has_value());
    CHECK(m2->seq > m1->seq);
    (void)bus->close();
}

TEST_CASE("adapt: close closes the underlying bus", "[adapter][REQ-ADAPT-005]") {
    auto bus = Bus::create();
    auto node = adapt(bus);
    auto err = node->close();
    REQUIRE_FALSE(err);
    // Bus should now be closed — subsequent publish returns error
    auto perr = bus->publish(0x10, {1});
    CHECK(perr == relay::ErrClosed());
}

TEST_CASE("adapt: subscribe delivers frames after bus publish with nil removes registration", "[adapter][REQ-LIN-019]") {
    auto bus = Bus::create();
    auto node = adapt(bus);

    // Register then remove
    relay::Message reg; reg.id = "16"; reg.payload = {0xAA};
    REQUIRE_FALSE(node->send(reg));

    relay::Message rem; rem.id = "16"; rem.payload = {};
    REQUIRE_FALSE(node->send(rem));

    // Now send_header should return ErrNoResponse
    auto [f, ferr] = bus->send_header(0x10);
    CHECK(ferr);
    (void)bus->close();
}
