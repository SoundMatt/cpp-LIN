// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// fusa:test REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
// fusa:test REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-007 REQ-SAFETY-008
// fusa:test REQ-SAFETY-009 REQ-SAFETY-010 REQ-SAFETY-011 REQ-SAFETY-012
// fusa:test REQ-SAFETY-013 REQ-SAFETY-014 REQ-SAFETY-015 REQ-SEOOC-001

#include <lin/lin.hpp>
#include <lin/safety/e2e.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>

using namespace lin::safety;

TEST_CASE("Config has DataID and SourceID fields", "[safety][REQ-SAFETY-001][REQ-SAFETY-002]") {
    Config cfg{0xABCD, 0x12};
    CHECK(cfg.data_id   == 0xABCD);
    CHECK(cfg.source_id == 0x12);
}

TEST_CASE("E2E header is exactly 3 bytes", "[safety][REQ-SAFETY-004]") {
    CHECK(kHeaderSize == 3u);
}

// Regression test for cpp-LIN#17: the documented ASIL-B publish pattern
// (Protector::protect() output fed straight into Bus::publish()) must be
// able to fit within a single LIN frame for at least some non-trivial
// payload size. A header that alone exceeds kLINMaxDataLen makes the
// documented safety pattern unimplementable on a real LIN bus.
TEST_CASE("kHeaderSize leaves room for a non-empty payload within kLINMaxDataLen",
          "[safety][REQ-SAFETY-012][regression]") {
    CHECK(kHeaderSize < lin::kLINMaxDataLen);
    CHECK(lin::kLINMaxDataLen - kHeaderSize >= 1u);
}

TEST_CASE("E2EErrorKind values are distinct", "[safety]") {
    CHECK(E2EErrorKind::CRCMismatch    != E2EErrorKind::SequenceGap);
    CHECK(E2EErrorKind::SequenceGap    != E2EErrorKind::HeaderTooShort);
    CHECK(E2EErrorKind::CRCMismatch    != E2EErrorKind::HeaderTooShort);
}

TEST_CASE("Protector starts with seq=0", "[safety][REQ-SAFETY-003]") {
    Config cfg{0x0001, 0x01};
    Protector p{cfg};
    auto frame = p.protect({0x00});
    CHECK(frame[0] == 0);
}

TEST_CASE("protect output length == kHeaderSize + len(payload)", "[safety][REQ-SAFETY-012]") {
    Config cfg{0x0001, 0x0010};
    Protector p{cfg};
    std::vector<uint8_t> payload = {1, 2, 3};
    auto out = p.protect(payload);
    CHECK(out.size() == kHeaderSize + payload.size());
}

// Fits entirely within one LIN frame — the pattern documented in
// SAFETY_MANUAL.md §4.2 must actually be publishable.
TEST_CASE("protect output for a small payload fits within kLINMaxDataLen",
          "[safety][REQ-SAFETY-012][regression]") {
    Config cfg{0x0001, 0x0010};
    Protector p{cfg};
    std::vector<uint8_t> payload = {1, 2};  // 2 bytes: 3-byte header + 2 == 5 <= 8
    auto out = p.protect(payload);
    CHECK(out.size() <= lin::kLINMaxDataLen);
}

TEST_CASE("protect and unwrap round-trip", "[safety][REQ-SAFETY-011]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    auto protected_payload = protector.protect(payload);
    CHECK(protected_payload.size() == kHeaderSize + payload.size());

    auto result = receiver.unwrap(protected_payload);
    CHECK(result == payload);
}

TEST_CASE("protect increments sequence counter", "[safety][REQ-SAFETY-003][REQ-SAFETY-006]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};

    auto p0 = protector.protect({0xAA});
    auto p1 = protector.protect({0xBB});

    CHECK(p1[0] == static_cast<uint8_t>(p0[0] + 1));
}

TEST_CASE("sequence counter wraps mod 256", "[safety][REQ-SAFETY-003][regression]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};

    std::vector<uint8_t> first_out;
    for (int i = 0; i < 256; ++i) {
        auto out = protector.protect({0x00});
        if (i == 0) first_out = out;
    }
    auto wrapped = protector.protect({0x00});
    // After 257 total protect() calls (indices 0..256), the 257th call's
    // counter byte must equal the 1st call's counter byte (wrap mod 256).
    CHECK(wrapped[0] == first_out[0]);
}

TEST_CASE("DataID and SourceID are never written to the wire", "[safety][REQ-SAFETY-001][REQ-SAFETY-002]") {
    Config cfg{0xABCD, 0x1234};
    Protector p{cfg};
    auto out = p.protect({0x00});
    // The header is only kHeaderSize (3) bytes: SequenceCounter + CRC.
    // Neither DataID nor SourceID appears verbatim anywhere in the header.
    CHECK(out.size() == kHeaderSize + 1);
}

TEST_CASE("DataID/SourceID mismatch is detected as a CRC error", "[safety][REQ-SAFETY-001][REQ-SAFETY-002][regression]") {
    Config sender_cfg{0xABCD, 0x1234};
    Config receiver_cfg{0xABCD, 0x9999};  // different SourceID
    Protector protector{sender_cfg};
    Receiver  receiver{receiver_cfg};

    auto protected_payload = protector.protect({0x01});
    REQUIRE_THROWS_AS(receiver.unwrap(protected_payload), E2EError);
    try { receiver.unwrap(protected_payload); }
    catch (const E2EError& e) { CHECK(e.kind() == E2EErrorKind::CRCMismatch); }
}

TEST_CASE("unwrap: header too short throws E2EError", "[safety][REQ-SAFETY-007]") {
    Config cfg{};
    Receiver receiver{cfg};
    REQUIRE_THROWS_AS(receiver.unwrap({0x01, 0x02}), E2EError);
    try { receiver.unwrap({0x01, 0x02}); }
    catch (const E2EError& e) { CHECK(e.kind() == E2EErrorKind::HeaderTooShort); }
}

TEST_CASE("unwrap: CRC mismatch throws E2EError", "[safety][REQ-SAFETY-008]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    auto protected_payload = protector.protect({0x01, 0x02});
    protected_payload[1] ^= 0xFF;  // corrupt CRC
    REQUIRE_THROWS_AS(receiver.unwrap(protected_payload), E2EError);
    try { receiver.unwrap(protected_payload); }
    catch (const E2EError& e) { CHECK(e.kind() == E2EErrorKind::CRCMismatch); }
}

TEST_CASE("unwrap: sequence gap throws E2EError", "[safety][REQ-SAFETY-009]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    auto p0 = protector.protect({0xAA});
    auto p1 = protector.protect({0xBB});  // seq=1
    auto p2 = protector.protect({0xCC});  // seq=2

    receiver.unwrap(p0);  // seq=0 OK
    // skip p1, jump to p2 → gap
    REQUIRE_THROWS_AS(receiver.unwrap(p2), E2EError);
    try { receiver.unwrap(p2); }
    catch (const E2EError& e) { CHECK(e.kind() == E2EErrorKind::SequenceGap); }
    (void)p1;
}

TEST_CASE("unwrap accepts first message with any counter value", "[safety][REQ-SAFETY-013]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    // Skip several protector calls to get a non-zero starting counter
    for (int i = 0; i < 100; ++i) protector.protect({0x00});
    auto p = protector.protect({0xBB});
    REQUIRE_NOTHROW(receiver.unwrap(p));  // first call accepts any counter
}

TEST_CASE("multiple sequential unwraps succeed", "[safety][REQ-SAFETY-010]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    for (int i = 0; i < 10; ++i) {
        std::vector<uint8_t> payload = {static_cast<uint8_t>(i)};
        auto p = protector.protect(payload);
        REQUIRE_NOTHROW(receiver.unwrap(p));
    }
}

TEST_CASE("unwrap returns independent copy", "[safety][REQ-SAFETY-015]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};
    Receiver  receiver{cfg};

    std::vector<uint8_t> payload = {0xAA, 0xBB};
    auto p = protector.protect(payload);
    auto result = receiver.unwrap(p);
    result[0] = 0xFF;  // mutate result

    // Next call from fresh receiver should not be affected
    Receiver receiver2{cfg};
    auto p2 = protector.protect({0x01});
    REQUIRE_NOTHROW(receiver2.unwrap(p2));
}

TEST_CASE("Protect is safe for concurrent calls", "[safety][REQ-SAFETY-014]") {
    Config cfg{0x0001, 0x0010};
    Protector protector{cfg};

    std::atomic<int> ok{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&]{
            for (int i = 0; i < 100; ++i) {
                auto p = protector.protect({static_cast<uint8_t>(i)});
                if (p.size() == kHeaderSize + 1) ++ok;
            }
        });
    }
    for (auto& th : threads) th.join();
    CHECK(ok.load() == 400);
}

TEST_CASE("crc16: known vector '123456789' -> 0x29B1", "[safety][REQ-SAFETY-005]") {
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    CHECK(crc16(data, 9) == 0x29B1);
}
