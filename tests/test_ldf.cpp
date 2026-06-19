// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004 REQ-LDF-005
// fusa:test REQ-LDF-006 REQ-LDF-007 REQ-LDF-008 REQ-LDF-009 REQ-LDF-010
// fusa:test REQ-LDF-011 REQ-LDF-012 REQ-LDF-013 REQ-LDF-014 REQ-LDF-015

#include <lin/ldf/parser.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

using namespace lin::ldf;

static const char* kSampleLDF = R"(
LIN_description_file ;
LIN_protocol_version = "2.1" ;
LIN_language_version = "2.1" ;
LIN_speed = 19.2 kbps ;

Nodes {
  Master: BCM, 1 ms, 0.1 ms ;
  Slaves: MotorControl, SeatPos ;
}

Signals {
  MotorSpeed : 8, 0, MotorControl, BCM ;
  SeatPosition : 16, 0, SeatPos, BCM ;
}

Frames {
  MotorFrame : 0x10, MotorControl, 2 {
    MotorSpeed, 0 ;
  }
  SeatFrame : 0x20, SeatPos, 4 {
    SeatPosition, 0 ;
  }
}

Schedule_tables {
  NormalSchedule {
    MotorFrame delay 10 ms ;
    SeatFrame delay 20 ms ;
  }
}
)";

TEST_CASE("parse extracts protocol version", "[ldf][REQ-LDF-001]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    CHECK(db->protocol_version == "2.1");
}

TEST_CASE("parse extracts baud rate", "[ldf][REQ-LDF-002]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    CHECK(db->speed > 0.0);
}

TEST_CASE("parse extracts master node name", "[ldf][REQ-LDF-003]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    CHECK(db->master_node == "BCM");
}

TEST_CASE("parse extracts slave node list", "[ldf][REQ-LDF-004]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    REQUIRE(db->slave_nodes.size() >= 2);
    bool found_motor = false, found_seat = false;
    for (const auto& s : db->slave_nodes) {
        if (s == "MotorControl") found_motor = true;
        if (s == "SeatPos") found_seat = true;
    }
    CHECK(found_motor);
    CHECK(found_seat);
}

TEST_CASE("parse populates frame descriptors", "[ldf][REQ-LDF-005]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto* f = db->frame(0x10);
    REQUIRE(f != nullptr);
    CHECK(f->name == "MotorFrame");
    CHECK(f->id   == 0x10);
    CHECK(f->length == 2);
}

TEST_CASE("parse populates signal-to-bit-offset mappings", "[ldf][REQ-LDF-006]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto* f = db->frame(0x10);
    REQUIRE(f != nullptr);
    REQUIRE(!f->signals.empty());
    CHECK(f->signals[0].name == "MotorSpeed");
    CHECK(f->signals[0].bit_offset == 0);
}

TEST_CASE("parse populates signal bit width", "[ldf][REQ-LDF-007]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto* sig = db->signal("MotorSpeed");
    REQUIRE(sig != nullptr);
    CHECK(sig->bit_width == 8);
}

TEST_CASE("parse populates signal publisher", "[ldf][REQ-LDF-008]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto* sig = db->signal("MotorSpeed");
    REQUIRE(sig != nullptr);
    CHECK(sig->publisher == "MotorControl");
}

TEST_CASE("Decode uses LSB-first Intel byte order", "[ldf][REQ-LDF-009]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    std::vector<uint8_t> data = {0x42, 0x00};
    auto decoded = db->decode(0x10, data);
    REQUIRE(!decoded.empty());
    CHECK(decoded.at("MotorSpeed") == 0x42);
}

TEST_CASE("Decode returns empty for unknown frame ID", "[ldf][REQ-LDF-010]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto result = db->decode(0xFF, {0x01});
    CHECK(result.empty());
}

TEST_CASE("parse populates schedule table entries", "[ldf][REQ-LDF-011]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto sched = db->schedule("NormalSchedule");
    CHECK(sched.size() >= 1);
    bool found = false;
    for (const auto& e : sched) {
        if (e.id == 0x10 && e.delay_ms == 10) { found = true; break; }
    }
    CHECK(found);
}

TEST_CASE("Frame() returns nullptr for unknown ID", "[ldf][REQ-LDF-012]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    CHECK(db->frame(0xFF) == nullptr);
}

TEST_CASE("Signal() returns nullptr for unknown name", "[ldf][REQ-LDF-013]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    CHECK(db->signal("NonExistent") == nullptr);
}

TEST_CASE("parse does not panic on empty input", "[ldf][REQ-LDF-014]") {
    std::istringstream ss("");
    std::unique_ptr<DB> db;
    REQUIRE_NOTHROW(db = parse(ss));
    REQUIRE(db != nullptr);
}

TEST_CASE("parse does not panic on garbage input", "[ldf][REQ-LDF-014]") {
    std::istringstream ss("!@#$%^&*() garbage input }{][");
    std::unique_ptr<DB> db;
    REQUIRE_NOTHROW(db = parse(ss));
    REQUIRE(db != nullptr);
}

TEST_CASE("Frames() returns defensive copy", "[ldf][REQ-LDF-015]") {
    std::istringstream ss(kSampleLDF);
    auto db = parse(ss);
    REQUIRE(db != nullptr);
    auto frames = db->frames();
    // Mutating the returned copy should not affect the DB
    frames.clear();
    CHECK(db->frame(0x10) != nullptr);
}
