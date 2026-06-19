// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// safety/e2e.hpp — End-to-end data protection for LIN payloads.
//
// Wire format (little-endian, 10 bytes followed by original payload):
//   Bytes  0–1   DataID (uint16)
//   Bytes  2–3   SourceID (uint16)
//   Bytes  4–7   SequenceCounter (uint32, monotonically increasing)
//   Bytes  8–9   CRC-16/CCITT-FALSE over bytes 0–7 (CRC slot zeroed) plus payload
//   Bytes 10+    Original payload
//
// fusa:req REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
// fusa:req REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-007 REQ-SAFETY-008
// fusa:req REQ-SAFETY-009 REQ-SAFETY-010 REQ-SAFETY-011 REQ-SAFETY-012
// fusa:req REQ-SAFETY-013 REQ-SAFETY-014 REQ-SAFETY-015 REQ-SEOOC-001

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace lin::safety {

// fusa:req REQ-SAFETY-001 REQ-SAFETY-002
inline constexpr std::size_t kHeaderSize = 10;

// fusa:req REQ-SAFETY-001 REQ-SAFETY-002
struct Config {
    uint16_t data_id{};    // logical data element identifier (0–65535)
    uint16_t source_id{};  // sender node identifier (0–65535)
};

// fusa:req REQ-SAFETY-007 REQ-SAFETY-008 REQ-SAFETY-009
enum class E2EErrorKind {
    CRCMismatch,
    SequenceGap,
    HeaderTooShort,
};

// fusa:req REQ-SAFETY-007 REQ-SAFETY-008 REQ-SAFETY-009
class E2EError : public std::runtime_error {
public:
    E2EError(E2EErrorKind kind, uint32_t counter, std::string msg)
        : std::runtime_error("lin/safety: E2E error: " + msg)
        , kind_(kind)
        , counter_(counter)
        , msg_(std::move(msg))
    {}

    E2EErrorKind kind()    const noexcept { return kind_; }
    uint32_t     counter() const noexcept { return counter_; }

private:
    E2EErrorKind kind_;
    uint32_t     counter_;
    std::string  msg_;
};

// CRC-16/CCITT-FALSE helper (poly=0x1021, init=0xFFFF).
// fusa:req REQ-SAFETY-005
uint16_t crc16(const uint8_t* data, std::size_t len) noexcept;

// Protector adds an E2E header to payloads before transmission.
// Protect is safe for concurrent calls (atomic sequence counter).
//
// fusa:req REQ-SAFETY-003 REQ-SAFETY-004 REQ-SAFETY-012 REQ-SAFETY-014
class Protector {
public:
    explicit Protector(Config cfg) : cfg_(cfg) {}

    // Prepends the E2E header and returns the protected payload.
    // Output length is exactly kHeaderSize (10) + len(payload).
    // fusa:req REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
    // fusa:req REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-012 REQ-SAFETY-014
    std::vector<uint8_t> protect(const std::vector<uint8_t>& payload);

private:
    Config                cfg_;
    std::atomic<uint32_t> seq_{0};
};

// Receiver validates E2E headers on received payloads.
// The first Unwrap call accepts any counter value to seed the sequence.
//
// fusa:req REQ-SAFETY-007 REQ-SAFETY-008 REQ-SAFETY-009 REQ-SAFETY-010
// fusa:req REQ-SAFETY-013 REQ-SAFETY-015
class Receiver {
public:
    explicit Receiver(Config cfg) : cfg_(cfg) {}

    // Validates the E2E header and returns a copy of the original payload.
    // Throws E2EError on CRC mismatch, sequence gap, or short payload.
    // fusa:req REQ-SAFETY-007 REQ-SAFETY-008 REQ-SAFETY-009 REQ-SAFETY-010
    // fusa:req REQ-SAFETY-011 REQ-SAFETY-013 REQ-SAFETY-015
    std::vector<uint8_t> unwrap(const std::vector<uint8_t>& data);

private:
    Config       cfg_;
    std::mutex   mu_;
    uint32_t     last_seq_{0};
    bool         first_{true};
};

} // namespace lin::safety
