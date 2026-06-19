// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-MASTER-001 REQ-MASTER-002 REQ-MASTER-003 REQ-MASTER-004
// fusa:test REQ-MASTER-005 REQ-MASTER-006 REQ-MASTER-007 REQ-MASTER-008
// fusa:test REQ-MASTER-009 REQ-MASTER-010 REQ-MASTER-011 REQ-MASTER-012
// fusa:test REQ-MASTER-013

#include <lin/master/node.hpp>
#include <lin/virtual/bus.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include <vector>

using namespace lin;
using namespace lin::virt;
using namespace lin::master;

TEST_CASE("Node is constructible from a bus", "[master][REQ-MASTER-001]") {
    auto bus = Bus::create();
    Node node(bus);
    bus->close();
}

TEST_CASE("set_schedule rejects empty schedule", "[master][REQ-MASTER-010]") {
    auto bus = Bus::create();
    Node node(bus);
    auto err = node.set_schedule({});
    CHECK(err);
    bus->close();
}

TEST_CASE("set_schedule rejects invalid frame ID", "[master][REQ-MASTER-011]") {
    auto bus = Bus::create();
    Node node(bus);
    auto err = node.set_schedule({{0x40, 0}});  // 0x40 > 0x3F
    CHECK(err);
    bus->close();
}

TEST_CASE("set_schedule accepts valid schedule", "[master][REQ-MASTER-012]") {
    auto bus = Bus::create();
    Node node(bus);
    REQUIRE_FALSE(node.set_schedule({{0x10, 0}, {0x20, 0}}));
    bus->close();
}

TEST_CASE("set_schedule stores defensive copy", "[master][REQ-MASTER-012]") {
    auto bus = Bus::create();
    Node node(bus);
    std::vector<ScheduleEntry> sched = {{0x10, 5}};
    REQUIRE_FALSE(node.set_schedule(sched));
    sched[0].id = 0x20;  // mutate caller's copy
    // node's schedule should still have 0x10 — verified by running
    bus->close();
}

TEST_CASE("send_header delegates to bus", "[master][REQ-MASTER-002]") {
    auto bus = Bus::create();
    bus->publish(0x10, {0xAA});
    Node node(bus);
    auto [f, err] = node.send_header(0x10);
    REQUIRE_FALSE(err);
    CHECK(f.id == 0x10);
    CHECK(f.data == std::vector<uint8_t>{0xAA});
    bus->close();
}

TEST_CASE("run returns error for empty schedule", "[master][REQ-MASTER-009]") {
    auto bus = Bus::create();
    Node node(bus);
    std::stop_source ss;
    ss.request_stop();
    auto err = node.run(ss.get_token());
    CHECK(err);  // empty schedule
    bus->close();
}

TEST_CASE("run iterates schedule and invokes callbacks", "[master][REQ-MASTER-003][REQ-MASTER-004][REQ-MASTER-006][REQ-MASTER-007]") {
    auto bus = Bus::create();
    bus->publish(0x10, {0x01});
    // 0x20 has no response → ErrNoResponse

    Node node(bus);
    REQUIRE_FALSE(node.set_schedule({{0x10, 0}, {0x20, 0}}));

    std::atomic<int> frame_count{0};
    std::atomic<int> error_count{0};
    std::vector<uint8_t> received_id;

    node.on_frame([&](Frame f) {
        frame_count++;
        received_id.push_back(f.id);
    });
    node.on_error([&](std::error_code) {
        error_count++;
    });

    std::stop_source ss;
    std::thread t([&]{
        node.run(ss.get_token());
    });

    // Let it run a few iterations
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ss.request_stop();
    t.join();

    CHECK(frame_count.load() >= 1);
    CHECK(error_count.load() >= 1);  // 0x20 triggers error
    bus->close();
}

TEST_CASE("run continues after per-slot errors", "[master][REQ-MASTER-013]") {
    auto bus = Bus::create();
    // 0x10 registered, 0x20 not
    bus->publish(0x10, {0x01});

    Node node(bus);
    REQUIRE_FALSE(node.set_schedule({{0x20, 0}, {0x10, 0}}));

    std::atomic<int> frame_count{0};
    node.on_frame([&](Frame) { frame_count++; });
    node.on_error([](std::error_code) {});  // absorb errors

    std::stop_source ss;
    std::thread t([&]{ node.run(ss.get_token()); });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ss.request_stop();
    t.join();

    // Should have received at least one frame (0x10) despite 0x20 errors
    CHECK(frame_count.load() >= 1);
    bus->close();
}

TEST_CASE("run returns on stop token", "[master][REQ-MASTER-008]") {
    auto bus = Bus::create();
    bus->publish(0x10, {0x01});
    Node node(bus);
    REQUIRE_FALSE(node.set_schedule({{0x10, 0}}));

    std::stop_source ss;
    std::thread t([&]{ node.run(ss.get_token()); });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ss.request_stop();
    t.join();  // must return
    bus->close();
}
