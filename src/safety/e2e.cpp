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

// Builds the 10-byte header with CRC filled in.
static std::vector<uint8_t> build_header(uint16_t data_id, uint16_t source_id,
                                          uint32_t seq,
                                          const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> hdr(kHeaderSize, 0);
    // bytes 0-1: DataID little-endian
    hdr[0] = static_cast<uint8_t>(data_id);
    hdr[1] = static_cast<uint8_t>(data_id >> 8);
    // bytes 2-3: SourceID little-endian
    hdr[2] = static_cast<uint8_t>(source_id);
    hdr[3] = static_cast<uint8_t>(source_id >> 8);
    // bytes 4-7: SequenceCounter little-endian
    hdr[4] = static_cast<uint8_t>(seq);
    hdr[5] = static_cast<uint8_t>(seq >> 8);
    hdr[6] = static_cast<uint8_t>(seq >> 16);
    hdr[7] = static_cast<uint8_t>(seq >> 24);
    // hdr[8:9] = 0 during CRC computation

    // CRC over header bytes 0–7 (CRC slot zeroed) and payload
    constexpr uint16_t poly = 0x1021;
    uint16_t crc_val = 0xFFFF;
    for (int i = 0; i < 8; ++i) {
        crc_val ^= static_cast<uint16_t>(hdr[i]) << 8;
        for (int j = 0; j < 8; ++j) {
            crc_val = (crc_val & 0x8000)
                ? static_cast<uint16_t>((crc_val << 1) ^ poly)
                : static_cast<uint16_t>(crc_val << 1);
        }
    }
    for (auto b : payload) {
        crc_val ^= static_cast<uint16_t>(b) << 8;
        for (int j = 0; j < 8; ++j) {
            crc_val = (crc_val & 0x8000)
                ? static_cast<uint16_t>((crc_val << 1) ^ poly)
                : static_cast<uint16_t>(crc_val << 1);
        }
    }

    // bytes 8-9: CRC little-endian
    hdr[8] = static_cast<uint8_t>(crc_val);
    hdr[9] = static_cast<uint8_t>(crc_val >> 8);
    return hdr;
}

// ── Protector ─────────────────────────────────────────────────────────────────

// fusa:req REQ-SAFETY-001 REQ-SAFETY-002 REQ-SAFETY-003 REQ-SAFETY-004
// fusa:req REQ-SAFETY-005 REQ-SAFETY-006 REQ-SAFETY-012 REQ-SAFETY-014
std::vector<uint8_t> Protector::protect(const std::vector<uint8_t>& payload) {
    uint32_t seq = seq_++;
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

    uint32_t seq = static_cast<uint32_t>(data[4])
                 | static_cast<uint32_t>(data[5]) << 8
                 | static_cast<uint32_t>(data[6]) << 16
                 | static_cast<uint32_t>(data[7]) << 24;

    uint16_t received_crc = static_cast<uint16_t>(data[8])
                          | static_cast<uint16_t>(data[9]) << 8;

    std::vector<uint8_t> payload(data.begin() + kHeaderSize, data.end());
    auto expected_hdr = build_header(cfg_.data_id, cfg_.source_id, seq, payload);
    uint16_t expected_crc = static_cast<uint16_t>(expected_hdr[8])
                          | static_cast<uint16_t>(expected_hdr[9]) << 8;

    if (received_crc != expected_crc)
        throw E2EError(E2EErrorKind::CRCMismatch, seq, "CRC mismatch");

    std::lock_guard<std::mutex> lk(mu_);
    if (!first_ && seq != last_seq_ + 1) {
        uint32_t expected_seq = last_seq_ + 1;
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
