// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-VIRT-001 REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005
// fusa:test REQ-VIRT-006 REQ-VIRT-007 REQ-VIRT-008 REQ-VIRT-009 REQ-VIRT-010
// fusa:test REQ-VIRT-011 REQ-VIRT-012 REQ-VIRT-013 REQ-VIRT-014 REQ-VIRT-015
// fusa:test REQ-VIRT-016 REQ-VIRT-017 REQ-VIRT-018 REQ-VIRT-019
// fusa:test REQ-RELAY-023 REQ-RELAY-024 REQ-RELAY-025 REQ-RELAY-026 REQ-RELAY-027
// fusa:test REQ-RELAY-028 REQ-RELAY-029

#include <lin/virtual/bus.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

using namespace lin;
using namespace lin::virt;

TEST_CASE("Bus::create returns non-null", "[virtual][REQ-VIRT-001]") {
    auto bus = Bus::create();
    REQUIRE(bus != nullptr);
    bus->close();
}

TEST_CASE("publish and send_header round-trip", "[virtual][REQ-VIRT-002][REQ-VIRT-006][REQ-VIRT-007]") {
    auto bus = Bus::create();
    REQUIRE_FALSE(bus->publish(0x10, {0x01, 0x02, 0x03, 0x04}));

    auto [f, err] = bus->send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.id == 0x10);
    CHECK(f.data == std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04});
    CHECK(f.checksum_type == ChecksumType::Enhanced);
    // Verify checksum
    uint8_t pid = protect_id(0x10);
    uint8_t expected_cs = calc_checksum(pid, f.data, ChecksumType::Enhanced);
    CHECK(f.checksum == expected_cs);
    bus->close();
}

TEST_CASE("publish_classic uses classic checksum", "[virtual][REQ-VIRT-007]") {
    auto bus = Bus::create();
    REQUIRE_FALSE(bus->publish_classic(0x10, {0x01, 0x02}));
    auto [f, err] = bus->send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.checksum_type == ChecksumType::Classic);
    uint8_t pid = protect_id(0x10);
    uint8_t expected_cs = calc_checksum(pid, f.data, ChecksumType::Classic);
    CHECK(f.checksum == expected_cs);
    bus->close();
}

TEST_CASE("send_header broadcasts to subscribers", "[virtual][REQ-VIRT-008]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {0xAA});
    bus->send_header(0x10);

    auto got = ch->recv();
    REQUIRE(got.has_value());
    CHECK(got->id == 0x10);
    bus->close();
}

TEST_CASE("multiple subscribers each receive independently", "[virtual][REQ-VIRT-014]") {
    auto bus = Bus::create();
    auto [ch1, e1] = bus->subscribe({}, {});
    auto [ch2, e2] = bus->subscribe({}, {});
    REQUIRE_FALSE(e1);
    REQUIRE_FALSE(e2);

    bus->publish(0x10, {0x42});
    bus->send_header(0x10);

    auto g1 = ch1->recv();
    auto g2 = ch2->recv();
    REQUIRE(g1.has_value());
    REQUIRE(g2.has_value());
    CHECK(g1->id == 0x10);
    CHECK(g2->id == 0x10);
    bus->close();
}

TEST_CASE("send_header returns ErrNoResponse when not published", "[virtual][REQ-VIRT-009]") {
    auto bus = Bus::create();
    auto [f, err] = bus->send_header(0x20);
    CHECK(err);
    bus->close();
}

TEST_CASE("send_header rejects ID > 0x3F", "[virtual][REQ-VIRT-010]") {
    auto bus = Bus::create();
    auto [f, err] = bus->send_header(0x40);
    CHECK(err);
    bus->close();
}

TEST_CASE("publish rejects ID > 0x3F", "[virtual][REQ-VIRT-004]") {
    auto bus = Bus::create();
    auto err = bus->publish(0x40, {0x01});
    CHECK(err);
    bus->close();
}

TEST_CASE("publish(id, {}) removes registration", "[virtual][REQ-VIRT-003]") {
    auto bus = Bus::create();
    REQUIRE_FALSE(bus->publish(0x10, {0xAA}));
    REQUIRE_FALSE(bus->publish(0x10, {}));  // remove
    auto [f, err] = bus->send_header(0x10);
    CHECK(err);  // ErrNoResponse
    bus->close();
}

TEST_CASE("publish after Close returns error", "[virtual][REQ-VIRT-005]") {
    auto bus = Bus::create();
    bus->close();
    auto err = bus->publish(0x10, {0x01});
    CHECK(err == relay::ErrClosed());
}

TEST_CASE("send_header after Close returns error", "[virtual][REQ-VIRT-017]") {
    auto bus = Bus::create();
    bus->close();
    auto [f, err] = bus->send_header(0x10);
    CHECK(err == relay::ErrClosed());
}

TEST_CASE("subscribe on closed bus returns ErrClosed", "[virtual]") {
    auto bus = Bus::create();
    bus->close();
    auto [ch, err] = bus->subscribe({}, {});
    CHECK(err == relay::ErrClosed());
    CHECK(ch == nullptr);
}

TEST_CASE("Close is idempotent", "[virtual][REQ-VIRT-016]") {
    auto bus = Bus::create();
    REQUIRE_FALSE(bus->close());
    REQUIRE_FALSE(bus->close());
}

TEST_CASE("subscriber channel closes when bus closes", "[virtual][REQ-VIRT-015]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {});
    REQUIRE_FALSE(err);
    bus->close();
    auto got = ch->recv();
    CHECK_FALSE(got.has_value());
}

TEST_CASE("filter by exact ID", "[virtual][REQ-VIRT-011]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({{0x10, false}}, {});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {0x01});
    bus->publish(0x20, {0x02});
    bus->send_header(0x20);  // should not arrive
    bus->send_header(0x10);  // should arrive

    auto got = ch->recv();
    REQUIRE(got.has_value());
    CHECK(got->id == 0x10);
    auto none = ch->try_recv();
    CHECK_FALSE(none.has_value());
    bus->close();
}

TEST_CASE("Filter all=true receives every frame", "[virtual][REQ-VIRT-012]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({{0x00, true}}, {});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {1});
    bus->publish(0x20, {2});
    bus->send_header(0x10);
    bus->send_header(0x20);

    auto g1 = ch->recv();
    auto g2 = ch->recv();
    REQUIRE(g1.has_value());
    REQUIRE(g2.has_value());
    bus->close();
}

TEST_CASE("full subscriber channel drops frame without blocking", "[virtual][REQ-VIRT-013]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {relay::with_channel_depth(2),
                                          relay::with_back_pressure(relay::BackPressurePolicy::DropNewest)});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {1});
    bus->send_header(0x10);
    bus->send_header(0x10);
    bus->send_header(0x10);  // dropped

    CHECK(ch->size() == 2);
    bus->close();
}

TEST_CASE("DropOldest back-pressure evicts oldest", "[virtual]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {relay::with_channel_depth(2),
                                          relay::with_back_pressure(relay::BackPressurePolicy::DropOldest)});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {1});
    bus->publish(0x20, {2});
    bus->publish(0x30, {3});
    bus->send_header(0x10);
    bus->send_header(0x20);
    bus->send_header(0x30);  // evicts 0x10

    CHECK(ch->size() == 2);
    auto m1 = ch->recv(); CHECK(m1->id == 0x20);
    auto m2 = ch->recv(); CHECK(m2->id == 0x30);
    bus->close();
}

TEST_CASE("publish stores defensive copy", "[virtual][REQ-VIRT-019]") {
    auto bus = Bus::create();
    std::vector<uint8_t> data = {0xAA};
    bus->publish(0x10, data);
    data[0] = 0xFF;  // mutate caller's copy

    auto [f, err] = bus->send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.data[0] == 0xAA);  // stored copy unchanged
    bus->close();
}

TEST_CASE("health reports OK when open, Down when closed", "[virtual][REQ-RELAY-023][REQ-RELAY-024][REQ-RELAY-025]") {
    auto bus = Bus::create();
    CHECK(bus->health().status == HealthStatus::OK);
    bus->close();
    CHECK(bus->health().status == HealthStatus::Down);
}

TEST_CASE("metrics count sends and deliveries", "[virtual][REQ-RELAY-026][REQ-RELAY-027][REQ-RELAY-029]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {1, 2});
    bus->send_header(0x10);
    ch->recv();

    auto m = bus->metrics();
    CHECK(m.write_count   == 1);
    CHECK(m.deliver_count == 1);
    CHECK(m.bytes_written == 2);
    bus->close();
}

TEST_CASE("close_with_drain returns ok when all consumed", "[virtual][REQ-RELAY-028]") {
    auto bus = Bus::create();
    auto [ch, err] = bus->subscribe({}, {});
    REQUIRE_FALSE(err);

    bus->publish(0x10, {1});
    bus->send_header(0x10);
    ch->recv();

    CHECK_FALSE(bus->close_with_drain(std::chrono::milliseconds{100}));
}

TEST_CASE("concurrent publish and send_header are data-race free", "[virtual][REQ-VIRT-018]") {
    auto bus = Bus::create();
    bus->publish(0x10, {0x01});

    std::thread t1([&]{
        for (int i = 0; i < 50; ++i) bus->publish(0x10, {static_cast<uint8_t>(i)});
    });
    std::thread t2([&]{
        for (int i = 0; i < 50; ++i) bus->send_header(0x10);
    });
    t1.join();
    t2.join();
    bus->close();
}

TEST_CASE("set_schedule stores schedule", "[virtual]") {
    auto bus = Bus::create();
    std::vector<ScheduleEntry> sched = {{0x10, 10}, {0x20, 20}};
    REQUIRE_FALSE(bus->set_schedule(sched));
    bus->close();
}
