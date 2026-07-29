// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/safety/e2e.hpp>
#include <cstring>

// fusa:req REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
// fusa:req REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-007 REQ-SAFETY-008
// fusa:req REQ-SAFETY-009 REQ-SAFETY-010 REQ-SAFETY-011 REQ-SAFETY-012
// fusa:req REQ-SAFETY-013 REQ-SAFETY-014 REQ-SAFETY-015 REQ-SEOOC-001

namespace lin::safety {

// ── CRC-16/CCITT-FALSE (poly=0x1021, init=0xFFFF) ───────────────────────────

// fusa:req REQ-SAFETY-005
uint16_t crc16(const uint8_t* data, std::size_t len) noexcept {
    constexpr uint16_t poly = 0x1021;
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ poly)
                                 : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

// Builds the 3-byte header ([0]=SequenceCounter, [1:2]=CRC) with CRC filled
// in. DataID/SourceID are never written to the header — they exist only as
// CRC input, so a mismatched Config on either side surfaces as a CRC error
// rather than being silently accepted or spending header bytes on identity
// fields that a LIN frame's 8-byte budget cannot afford (see cpp-LIN#17).
static std::vector<uint8_t> build_header(uint16_t data_id, uint16_t source_id,
                                          uint8_t seq,
                                          const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> hdr(kHeaderSize, 0);
    hdr[0] = seq;
    // hdr[1:2] = 0 during CRC computation

    constexpr uint16_t poly = 0x1021;
    uint16_t crc_val = 0xFFFF;
    auto crc_byte = [&crc_val](uint8_t b) {
        crc_val ^= static_cast<uint16_t>(b) << 8;
        for (int j = 0; j < 8; ++j) {
            crc_val = (crc_val & 0x8000)
                ? static_cast<uint16_t>((crc_val << 1) ^ poly)
                : static_cast<uint16_t>(crc_val << 1);
        }
    };

    // CRC input: DataID (2B) + SourceID (2B) + SequenceCounter (1B) + payload.
    crc_byte(static_cast<uint8_t>(data_id));
    crc_byte(static_cast<uint8_t>(data_id >> 8));
    crc_byte(static_cast<uint8_t>(source_id));
    crc_byte(static_cast<uint8_t>(source_id >> 8));
    crc_byte(seq);
    for (auto b : payload) crc_byte(b);

    // bytes 1-2: CRC little-endian
    hdr[1] = static_cast<uint8_t>(crc_val);
    hdr[2] = static_cast<uint8_t>(crc_val >> 8);
    return hdr;
}

// ── Protector ─────────────────────────────────────────────────────────────────

// fusa:req REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
// fusa:req REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-012 REQ-SAFETY-014
std::vector<uint8_t> Protector::protect(const std::vector<uint8_t>& payload) {
    // On-wire SequenceCounter is a single byte (wraps mod 256) — see
    // e2e.hpp's wire-format note for why the header must stay this small.
    uint8_t seq = static_cast<uint8_t>(seq_++);
    auto hdr = build_header(cfg_.data_id, cfg_.source_id, seq, payload);
    std::vector<uint8_t> out;
    out.reserve(kHeaderSize + payload.size());
    out.insert(out.end(), hdr.begin(), hdr.end());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// ── Receiver ─────────────────────────────────────────────────────────────────

// fusa:req REQ-SAFETY-007 REQ-SAFETY-008 REQ-SAFETY-009 REQ-SAFETY-010
// fusa:req REQ-SAFETY-011 REQ-SAFETY-013 REQ-SAFETY-015
std::vector<uint8_t> Receiver::unwrap(const std::vector<uint8_t>& data) {
    if (data.size() < kHeaderSize)
        throw E2EError(E2EErrorKind::HeaderTooShort, 0,
                       "need " + std::to_string(kHeaderSize) +
                       " bytes, got " + std::to_string(data.size()));

    uint8_t seq = data[0];

    uint16_t received_crc = static_cast<uint16_t>(data[1])
                          | static_cast<uint16_t>(data[2]) << 8;

    std::vector<uint8_t> payload(data.begin() + kHeaderSize, data.end());
    auto expected_hdr = build_header(cfg_.data_id, cfg_.source_id, seq, payload);
    uint16_t expected_crc = static_cast<uint16_t>(expected_hdr[1])
                          | static_cast<uint16_t>(expected_hdr[2]) << 8;

    if (received_crc != expected_crc)
        throw E2EError(E2EErrorKind::CRCMismatch, seq, "CRC mismatch");

    std::lock_guard<std::mutex> lk(mu_);
    // Sequence comparison wraps mod 256 to match the on-wire counter width.
    if (!first_ && seq != static_cast<uint8_t>(last_seq_ + 1)) {
        uint8_t expected_seq = static_cast<uint8_t>(last_seq_ + 1);
        last_seq_ = seq;
        throw E2EError(E2EErrorKind::SequenceGap, seq,
                       "expected " + std::to_string(expected_seq) +
                       ", got " + std::to_string(seq));
    }
    first_    = false;
    last_seq_ = seq;

    // return independent copy — REQ-SAFETY-015
    return payload;
}

} // namespace lin::safety
