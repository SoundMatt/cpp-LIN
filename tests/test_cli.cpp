// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-CLI-001 REQ-CLI-002 REQ-CLI-003 REQ-CLI-004 REQ-CLI-005 REQ-CLI-006

#include "json.hpp"
#include <lin/lin.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cli;
using namespace lin;

// ── parse_frame_json ── REQ-CLI-004 ──────────────────────────────────────────

TEST_CASE("parse_frame_json: basic frame", "[cli][REQ-CLI-004]") {
    std::string json = R"({"id":16,"data":"qrs=","checksum":73,"checksum_type":1})";
    auto f = parse_frame_json(json);
    CHECK(f.id == 16);
    CHECK(f.checksum_type == ChecksumType::Enhanced);
    CHECK(f.checksum == 73);
    CHECK(!f.data.empty());
}

TEST_CASE("parse_frame_json: missing id throws", "[cli][REQ-CLI-004]") {
    std::string json = R"({"data":"AA=="})";
    REQUIRE_THROWS(parse_frame_json(json));
}

TEST_CASE("parse_frame_json: classic checksum type", "[cli][REQ-CLI-004]") {
    std::string json = R"({"id":60,"data":"AQID","checksum":0,"checksum_type":0})";
    auto f = parse_frame_json(json);
    CHECK(f.checksum_type == ChecksumType::Classic);
    CHECK(f.id == 60);
}

// ── message_to_json ── REQ-CLI-005 ───────────────────────────────────────────

TEST_CASE("message_to_json: produces valid JSON structure", "[cli][REQ-CLI-005]") {
    relay::Message m;
    m.protocol = relay::Protocol::LIN;
    m.id = "16";
    m.payload = {0xAA, 0xBB, 0xCC};
    m.meta["lin.checksum_type"] = "enhanced";
    m.meta["lin.checksum"] = "73";

    auto json = message_to_json(m);
    CHECK(json.find("\"protocol\":3") != std::string::npos);
    CHECK(json.find("\"id\":\"16\"") != std::string::npos);
    CHECK(json.find("lin.checksum_type") != std::string::npos);
}

TEST_CASE("message_to_json: timestamp is zero string", "[cli][REQ-CLI-005]") {
    relay::Message m;
    m.protocol = relay::Protocol::LIN;
    m.id = "0";
    auto json = message_to_json(m);
    CHECK(json.find("0001-01-01T00:00:00Z") != std::string::npos);
}

// ── version_json ── REQ-CLI-001 ───────────────────────────────────────────────

TEST_CASE("version_json: contains expected fields", "[cli][REQ-CLI-001]") {
    auto json = version_json();
    CHECK(json.find("cpp-LIN") != std::string::npos);
    CHECK(json.find("\"protocol\":\"LIN\"") != std::string::npos);
    CHECK(json.find("\"protocol_int\":3") != std::string::npos);
    CHECK(json.find("\"version\"") != std::string::npos);
    CHECK(json.find("\"spec_version\"") != std::string::npos);
}

// ── capabilities_json ── REQ-CLI-002 ─────────────────────────────────────────

TEST_CASE("capabilities_json: contains expected fields", "[cli][REQ-CLI-002]") {
    auto json = capabilities_json();
    CHECK(json.find("capabilities") != std::string::npos);
    CHECK(json.find("\"protocol\":\"LIN\"") != std::string::npos);
    CHECK(json.find("\"protocol_int\":3") != std::string::npos);
    CHECK(json.find("convert") != std::string::npos);
    CHECK(json.find("version") != std::string::npos);
}

// ── status_json ── REQ-CLI-003 ────────────────────────────────────────────────

TEST_CASE("status_json: contains expected fields", "[cli][REQ-CLI-003]") {
    auto json = status_json();
    CHECK(json.find("\"protocol\":\"LIN\"") != std::string::npos);
    CHECK(json.find("\"healthy\":true") != std::string::npos);
}

// ── round-trip: parse → to_message → message_to_json ─────────────────────────

TEST_CASE("RELAY vector: lin-frame enhanced checksum", "[cli][REQ-CLI-004][REQ-CLI-005]") {
    // lin-frame.json: id=16, data="qrs=" (= {0xAA,0xAB,0xAC}?), checksum=73, checksum_type=1
    std::string json = R"({"id":16,"data":"qrs=","checksum":73,"checksum_type":1})";
    auto f = parse_frame_json(json);
    CHECK(f.id == 16);
    CHECK(f.checksum == 73);
    CHECK(f.checksum_type == ChecksumType::Enhanced);
    REQUIRE_NOTHROW(validate_frame(f));

    auto msg = to_message(f);
    msg.timestamp = {};
    auto out = message_to_json(msg);
    CHECK(out.find("\"protocol\":3") != std::string::npos);
    CHECK(out.find("\"id\":\"16\"") != std::string::npos);
    CHECK(out.find("enhanced") != std::string::npos);
}

TEST_CASE("RELAY error vector: lin-id-overflow rejected", "[cli][REQ-CLI-006]") {
    std::string json = R"({"id":64,"data":"AQ==","checksum":0,"checksum_type":0})";
    auto f = parse_frame_json(json);
    CHECK(f.id == 64);
    REQUIRE_THROWS_AS(validate_frame(f), ErrInvalidFrame);
}

TEST_CASE("RELAY error vector: lin-diagnostic-wrong-checksum rejected", "[cli][REQ-CLI-006]") {
    // id=60 (0x3C), checksum_type=1 (Enhanced) → invalid
    std::string json = R"({"id":60,"data":"AQIDBAUGBwg=","checksum":0,"checksum_type":1})";
    auto f = parse_frame_json(json);
    CHECK(f.id == 60);
    REQUIRE_THROWS_AS(validate_frame(f), ErrInvalidFrame);
}
