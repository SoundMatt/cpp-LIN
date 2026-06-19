// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-SLAVE-001 REQ-SLAVE-002 REQ-SLAVE-003 REQ-SLAVE-004
// fusa:test REQ-SLAVE-005 REQ-SLAVE-006 REQ-SLAVE-007 REQ-SLAVE-008

#include <lin/slave/node.hpp>
#include <lin/virtual/bus.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace lin;
using namespace lin::virt;
using namespace lin::slave;

TEST_CASE("Node is constructible from a bus", "[slave][REQ-SLAVE-001]") {
    auto bus = Bus::create();
    Node node(bus);
    bus->close();
}

TEST_CASE("registered_ids returns empty initially", "[slave][REQ-SLAVE-007]") {
    auto bus = Bus::create();
    Node node(bus);
    auto ids = node.registered_ids();
    CHECK(ids.empty());
    bus->close();
}

TEST_CASE("set_response registers ID", "[slave][REQ-SLAVE-002]") {
    auto bus = Bus::create();
    Node node(bus);
    REQUIRE_FALSE(node.set_response(0x10, {0xAA, 0xBB}));

    auto ids = node.registered_ids();
    REQUIRE(ids.size() == 1);
    CHECK(ids[0] == 0x10);

    // Bus can now serve it
    auto [f, err] = bus->send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.data == std::vector<uint8_t>{0xAA, 0xBB});
    bus->close();
}

TEST_CASE("set_response(id, {}) removes registration", "[slave][REQ-SLAVE-003]") {
    auto bus = Bus::create();
    Node node(bus);
    REQUIRE_FALSE(node.set_response(0x10, {0xAA}));
    REQUIRE_FALSE(node.set_response(0x10, {}));  // remove

    auto ids = node.registered_ids();
    CHECK(ids.empty());

    auto [f, err] = bus->send_header(0x10);
    CHECK(err);  // ErrNoResponse
    bus->close();
}

TEST_CASE("set_response rejects ID > 0x3F", "[slave][REQ-SLAVE-004]") {
    auto bus = Bus::create();
    Node node(bus);
    auto err = node.set_response(0x40, {0x01});
    CHECK(err);
    bus->close();
}

TEST_CASE("registered_ids reflects current state", "[slave][REQ-SLAVE-005]") {
    auto bus = Bus::create();
    Node node(bus);
    node.set_response(0x10, {1});
    node.set_response(0x20, {2});
    node.set_response(0x30, {3});
    node.set_response(0x20, {});  // remove 0x20

    auto ids = node.registered_ids();
    CHECK(ids.size() == 2);
    bool found_10 = false, found_30 = false;
    for (auto id : ids) {
        if (id == 0x10) found_10 = true;
        if (id == 0x30) found_30 = true;
    }
    CHECK(found_10);
    CHECK(found_30);
    bus->close();
}

TEST_CASE("subscribe delegates to bus", "[slave][REQ-SLAVE-006]") {
    auto bus = Bus::create();
    Node node(bus);
    auto [ch, err] = node.subscribe({}, {});
    REQUIRE_FALSE(err);
    REQUIRE(ch != nullptr);

    node.set_response(0x10, {0xAA});
    bus->send_header(0x10);

    auto got = ch->recv();
    REQUIRE(got.has_value());
    CHECK(got->id == 0x10);
    bus->close();
}

TEST_CASE("set_response overwrites previous registration", "[slave][REQ-SLAVE-008]") {
    auto bus = Bus::create();
    Node node(bus);
    node.set_response(0x10, {0xAA});
    node.set_response(0x10, {0xBB});  // overwrite

    auto ids = node.registered_ids();
    CHECK(ids.size() == 1);  // still only one

    auto [f, err] = bus->send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.data == std::vector<uint8_t>{0xBB});
    bus->close();
}
