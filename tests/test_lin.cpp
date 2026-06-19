// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-LIN-001 REQ-LIN-002 REQ-LIN-003 REQ-LIN-004 REQ-LIN-005
// fusa:test REQ-LIN-006 REQ-LIN-007 REQ-LIN-008 REQ-LIN-009 REQ-LIN-010
// fusa:test REQ-LIN-011 REQ-LIN-012 REQ-LIN-013 REQ-LIN-014 REQ-LIN-015
// fusa:test REQ-LIN-016 REQ-LIN-017 REQ-LIN-018 REQ-LIN-019 REQ-LIN-020
// fusa:test REQ-LIN-021

#include <lin/lin.hpp>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

using namespace lin;

// ── validate_frame ─────────────────────────────────────────────────────────────

TEST_CASE("validate_frame: valid frame ID 0x00", "[lin][REQ-LIN-016]") {
    REQUIRE_NOTHROW(validate_frame(Frame{0x00, {0xAA}}));
}

TEST_CASE("validate_frame: valid frame ID 0x3F", "[lin][REQ-LIN-015]") {
    REQUIRE_NOTHROW(validate_frame(Frame{0x3F, {0x01}}));
}

TEST_CASE("validate_frame: valid data length 1", "[lin][REQ-LIN-016]") {
    REQUIRE_NOTHROW(validate_frame(Frame{0x10, {0x01}}));
}

TEST_CASE("validate_frame: valid data length 8", "[lin][REQ-LIN-017]") {
    std::vector<uint8_t> data(8, 0xAB);
    REQUIRE_NOTHROW(validate_frame(Frame{0x10, data}));
}

TEST_CASE("validate_frame: rejects ID > 0x3F", "[lin][REQ-LIN-001]") {
    REQUIRE_THROWS_AS(validate_frame(Frame{0x40, {0x01}}), ErrInvalidFrame);
}

TEST_CASE("validate_frame: rejects empty data", "[lin][REQ-LIN-002]") {
    REQUIRE_THROWS_AS(validate_frame(Frame{0x10, {}}), ErrInvalidFrame);
}

TEST_CASE("validate_frame: rejects data > 8 bytes", "[lin][REQ-LIN-003]") {
    std::vector<uint8_t> data(9, 0);
    REQUIRE_THROWS_AS(validate_frame(Frame{0x10, data}), ErrInvalidFrame);
}

TEST_CASE("validate_frame: diagnostic 0x3C must use classic checksum", "[lin][REQ-LIN-001]") {
    Frame f;
    f.id = kLINDiagRequestID;
    f.data = {1, 2, 3, 4, 5, 6, 7, 8};
    f.checksum_type = ChecksumType::Enhanced;
    REQUIRE_THROWS_AS(validate_frame(f), ErrInvalidFrame);
}

TEST_CASE("validate_frame: diagnostic 0x3D must use classic checksum", "[lin][REQ-LIN-001]") {
    Frame f;
    f.id = kLINDiagResponseID;
    f.data = {1, 2, 3};
    f.checksum_type = ChecksumType::Enhanced;
    REQUIRE_THROWS_AS(validate_frame(f), ErrInvalidFrame);
}

TEST_CASE("validate_frame: diagnostic 0x3C with classic is valid", "[lin][REQ-LIN-001]") {
    Frame f;
    f.id = kLINDiagRequestID;
    f.data = {1, 2, 3};
    f.checksum_type = ChecksumType::Classic;
    REQUIRE_NOTHROW(validate_frame(f));
}

// ── protect_id ── REQ-LIN-004 REQ-LIN-005 REQ-LIN-018 ────────────────────────

TEST_CASE("protect_id: preserves lower 6 bits", "[lin][REQ-LIN-018]") {
    for (uint8_t id = 0; id <= kLINMaxID; ++id) {
        CHECK((protect_id(id) & kLINMaxID) == id);
    }
}

TEST_CASE("protect_id: known vector ID=0x10", "[lin][REQ-LIN-004][REQ-LIN-005]") {
    // ID=0x10 (16 decimal): bits 0-5 = 010000
    // P0 = 0^0^0^0 = 0 → bit 6 = 0
    // P1 = NOT(0^0^0^1) = NOT(1) = 0 → bit 7 = 0
    // PID = 0x10 | 0x00 | 0x00 = 0x10
    CHECK(protect_id(0x10) == 0x10);
}

TEST_CASE("protect_id: known vector ID=0x00", "[lin][REQ-LIN-004][REQ-LIN-005]") {
    // P0 = 0, P1 = NOT(0) = 1 → PID = 0x80
    CHECK(protect_id(0x00) == 0x80);
}

TEST_CASE("protect_id: ID>0x3F masks to 6 bits", "[lin][REQ-LIN-018]") {
    // 0x40 & 0x3F = 0x00
    CHECK((protect_id(0x40) & kLINMaxID) == 0x00);
}

// ── verify_pid ── REQ-LIN-006 REQ-LIN-007 ────────────────────────────────────

TEST_CASE("verify_pid: accepts correct PID", "[lin][REQ-LIN-006]") {
    for (uint8_t id = 0; id <= kLINMaxID; ++id) {
        uint8_t pid = protect_id(id);
        uint8_t got = 0;
        REQUIRE_NOTHROW(got = verify_pid(pid));
        CHECK(got == id);
    }
}

TEST_CASE("verify_pid: rejects corrupted PID", "[lin][REQ-LIN-007]") {
    uint8_t pid = protect_id(0x10);
    pid ^= 0x40;  // flip P0 bit
    REQUIRE_THROWS_AS(verify_pid(pid), ErrInvalidFrame);
}

// ── calc_checksum ── REQ-LIN-008 REQ-LIN-009 REQ-LIN-010 ──────────────────────

TEST_CASE("calc_checksum: classic ignores PID", "[lin][REQ-LIN-008]") {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    uint8_t cs1 = calc_checksum(0x10, data, ChecksumType::Classic);
    uint8_t cs2 = calc_checksum(0x20, data, ChecksumType::Classic);
    CHECK(cs1 == cs2);  // PID doesn't affect classic
}

TEST_CASE("calc_checksum: enhanced includes PID", "[lin][REQ-LIN-009]") {
    std::vector<uint8_t> data = {0x01, 0x02};
    uint8_t cs1 = calc_checksum(0x10, data, ChecksumType::Enhanced);
    uint8_t cs2 = calc_checksum(0x20, data, ChecksumType::Enhanced);
    CHECK(cs1 != cs2);  // different PIDs produce different checksums
}

TEST_CASE("calc_checksum: carry-around inversion", "[lin][REQ-LIN-010]") {
    // Known test: ID=0x10, data={0xAA, 0x72, 0x73} with enhanced checksum
    // This tests the carry-around addition
    std::vector<uint8_t> data = {0xAA, 0x72, 0x73};
    uint8_t pid = protect_id(0x10);
    uint8_t cs = calc_checksum(pid, data, ChecksumType::Enhanced);
    // Verify result can be used to validate frame
    CHECK(cs <= 0xFF);
    // The sum + checksum should equal 0xFF mod carry-around
    uint16_t sum = static_cast<uint16_t>(pid);
    for (auto b : data) {
        sum += b;
        if (sum > 0xFF) sum -= 0xFF;
    }
    sum += cs;
    if (sum > 0xFF) sum -= 0xFF;
    CHECK(sum == 0xFF);
}

// ── to_message / from_message ── REQ-LIN-011 REQ-LIN-012 ─────────────────────

TEST_CASE("to_message / from_message: enhanced checksum round-trip", "[lin]") {
    Frame orig;
    orig.id = 0x10;
    orig.data = {0xAA, 0xBB, 0xCC};
    orig.checksum = 0x49;
    orig.checksum_type = ChecksumType::Enhanced;

    auto msg = to_message(orig);
    CHECK(msg.protocol == relay::Protocol::LIN);
    CHECK(msg.id == "16");
    CHECK(msg.meta.at("lin.checksum_type") == "enhanced");
    CHECK(msg.meta.at("lin.checksum") == "73");

    auto f2 = from_message(msg);
    CHECK(f2.id == 0x10);
    CHECK(f2.data == orig.data);
    CHECK(f2.checksum_type == ChecksumType::Enhanced);
}

TEST_CASE("to_message / from_message: classic checksum round-trip", "[lin]") {
    Frame orig;
    orig.id = 0x3C;
    orig.data = {0x01, 0x02};
    orig.checksum = 0xFC;
    orig.checksum_type = ChecksumType::Classic;

    auto msg = to_message(orig);
    CHECK(msg.meta.at("lin.checksum_type") == "classic");
    auto f2 = from_message(msg);
    CHECK(f2.checksum_type == ChecksumType::Classic);
    CHECK(f2.id == 0x3C);
}

TEST_CASE("from_message: throws on ID > 0x3F", "[lin][REQ-LIN-001]") {
    relay::Message msg;
    msg.protocol = relay::Protocol::LIN;
    msg.id = "64";
    msg.payload = {0x01};
    REQUIRE_THROWS_AS(from_message(msg), ErrInvalidFrame);
}

TEST_CASE("from_message: throws on non-numeric ID", "[lin]") {
    relay::Message msg;
    msg.id = "bad";
    REQUIRE_THROWS_AS(from_message(msg), ErrInvalidFrame);
}

// ── Constants ─────────────────────────────────────────────────────────────────

TEST_CASE("kLINMaxDataLen is 8", "[lin]") {
    CHECK(kLINMaxDataLen == 8);
}

TEST_CASE("kLINMaxID is 0x3F", "[lin]") {
    CHECK(kLINMaxID == 0x3F);
}

TEST_CASE("kLINDiagRequestID is 0x3C", "[lin]") {
    CHECK(kLINDiagRequestID == 0x3C);
}

TEST_CASE("kLINDiagResponseID is 0x3D", "[lin]") {
    CHECK(kLINDiagResponseID == 0x3D);
}

// ── IBus / IMasterBus interface ────────────────────────────────────────────────

TEST_CASE("IBus is abstract", "[lin][REQ-LIN-011]") {
    CHECK(std::is_abstract<IBus>::value);
}

TEST_CASE("IMasterBus is abstract and extends IBus", "[lin][REQ-LIN-013]") {
    CHECK(std::is_abstract<IMasterBus>::value);
    CHECK((std::is_base_of<IBus, IMasterBus>::value));
}

// ── ChecksumType ──────────────────────────────────────────────────────────────

TEST_CASE("ChecksumType Classic=0 Enhanced=1", "[lin]") {
    CHECK(static_cast<int>(ChecksumType::Classic)  == 0);
    CHECK(static_cast<int>(ChecksumType::Enhanced) == 1);
}

// ── Filter ────────────────────────────────────────────────────────────────────

TEST_CASE("Filter::matches: exact match", "[lin][REQ-LIN-012]") {
    Filter f{0x10, false};
    Frame frame;
    frame.id = 0x10;
    frame.data = {1};
    CHECK(f.matches(frame));
    frame.id = 0x11;
    CHECK_FALSE(f.matches(frame));
}

TEST_CASE("Filter::matches: all=true matches every frame", "[lin][REQ-LIN-020]") {
    Filter f{0x00, true};
    Frame frame;
    for (uint8_t id = 0; id <= kLINMaxID; ++id) {
        frame.id = id;
        frame.data = {1};
        CHECK(f.matches(frame));
    }
}

// ── Error sentinels ───────────────────────────────────────────────────────────

TEST_CASE("ErrClosed alias matches relay::ErrClosed", "[lin][REQ-LIN-021]") {
    CHECK(ErrClosed() == relay::ErrClosed());
}

TEST_CASE("ErrInvalidFrame is throwable", "[lin][REQ-LIN-001]") {
    REQUIRE_THROWS_AS(throw ErrInvalidFrame("test"), ErrInvalidFrame);
}

TEST_CASE("ErrNoResponse is throwable", "[lin][REQ-LIN-014][REQ-LIN-021]") {
    REQUIRE_THROWS_AS(throw ErrNoResponse(), ErrNoResponse);
}

TEST_CASE("kSpecVersion is non-empty", "[lin]") {
    CHECK(!std::string(kSpecVersion).empty());
}
