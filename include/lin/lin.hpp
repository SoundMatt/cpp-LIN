// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// lin.hpp — Core LIN bus types and IBus/IMasterBus interfaces.
// C++ translation of github.com/SoundMatt/go-LIN.
//
// LIN is a serial, single-wire, master-slave bus defined by the LIN
// Consortium (LIN 2.x) for low-bandwidth automotive subsystems.

#pragma once

#include "relay.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// fusa:req REQ-LIN-001
// fusa:req REQ-LIN-002
// fusa:req REQ-LIN-003
// fusa:req REQ-LIN-004
// fusa:req REQ-LIN-005

namespace lin {

// ── Spec version ─────────────────────────────────────────────────────────────

inline constexpr const char* kSpecVersion = "1.10";

// ── Constants ─────────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-001 REQ-LIN-002 REQ-LIN-003
inline constexpr uint8_t kLINMaxDataLen      = 8;
inline constexpr uint8_t kLINMaxID           = 0x3F;
inline constexpr uint8_t kLINDiagRequestID   = 0x3C;
inline constexpr uint8_t kLINDiagResponseID  = 0x3D;

// ── ChecksumType ──────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-008 REQ-LIN-009
enum class ChecksumType : int {
    Classic  = 0,  // LIN 1.x — data bytes only
    Enhanced = 1,  // LIN 2.x — PID + data bytes
};

// ── Frame ─────────────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-001 REQ-LIN-002 REQ-LIN-003
struct Frame {
    uint8_t              id{};           // 6-bit frame identifier (0x00–0x3F)
    std::vector<uint8_t> data;           // payload (1–8 bytes)
    uint8_t              checksum{};     // wire checksum byte
    ChecksumType         checksum_type{ChecksumType::Enhanced};
};

// ── Filter ────────────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-012 REQ-LIN-020
struct Filter {
    uint8_t id{};   // exact frame identifier to match
    bool    all{};  // when true, matches every frame regardless of id

    // Returns true when frame f passes this filter.
    bool matches(const Frame& f) const noexcept {
        if (all) return true;
        return f.id == id;
    }
};

// ── ScheduleEntry ─────────────────────────────────────────────────────────────

struct ScheduleEntry {
    uint8_t  id{};        // frame identifier transmitted by master in this slot
    uint32_t delay_ms{};  // slot duration in milliseconds
};

// ── Error types ───────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-001 REQ-LIN-002 REQ-LIN-003 REQ-LIN-015
class ErrInvalidFrame : public std::runtime_error {
public:
    explicit ErrInvalidFrame(std::string reason)
        : std::runtime_error("lin: invalid frame: " + reason)
        , reason_(std::move(reason))
    {}
    const std::string& reason() const noexcept { return reason_; }
private:
    std::string reason_;
};

// fusa:req REQ-LIN-014 REQ-LIN-021
class ErrNoResponse : public std::runtime_error {
public:
    ErrNoResponse()
        : std::runtime_error("lin: no slave response")
    {}
};

// ── Error sentinels (aliases for relay errors) ────────────────────────────────

// fusa:req REQ-LIN-021
inline std::error_code ErrClosed()          noexcept { return relay::ErrClosed(); }
inline std::error_code ErrNotConnected()    noexcept { return relay::ErrNotConnected(); }
inline std::error_code ErrTimeout()         noexcept { return relay::ErrTimeout(); }
inline std::error_code ErrPayloadTooLarge() noexcept { return relay::ErrPayloadTooLarge(); }

// ── Free functions ────────────────────────────────────────────────────────────

// Computes the Protected Identifier (PID) for a 6-bit LIN frame ID.
// P0 = ID0 ^ ID1 ^ ID2 ^ ID4  (bit 6)
// P1 = NOT(ID1 ^ ID3 ^ ID4 ^ ID5) (bit 7)
//
// fusa:req REQ-LIN-004 REQ-LIN-005 REQ-LIN-018
uint8_t protect_id(uint8_t id) noexcept;

// Verifies PID parity. Returns the raw 6-bit ID on success, or throws
// ErrInvalidFrame on parity mismatch.
//
// fusa:req REQ-LIN-006 REQ-LIN-007
uint8_t verify_pid(uint8_t pid);

// Computes the LIN checksum for the given PID and data.
// Classic: sum data bytes only (pid ignored).
// Enhanced: include the PID byte in the sum alongside data bytes.
// Both use inverted carry-around 8-bit addition.
//
// fusa:req REQ-LIN-008 REQ-LIN-009 REQ-LIN-010
uint8_t calc_checksum(uint8_t pid, const std::vector<uint8_t>& data, ChecksumType ct) noexcept;

// Validates f against LIN protocol constraints.
// Throws ErrInvalidFrame if any constraint is violated.
//
// fusa:req REQ-LIN-001 REQ-LIN-002 REQ-LIN-003 REQ-LIN-015 REQ-LIN-016 REQ-LIN-017
void validate_frame(const Frame& f);

// ── RELAY bridge ─────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-011 REQ-LIN-012
relay::Message to_message(const Frame& f);
Frame          from_message(const relay::Message& m);  // throws ErrInvalidFrame on bad ID

// ── IBus interface ────────────────────────────────────────────────────────────

// fusa:req REQ-LIN-011 REQ-LIN-012
class IBus {
public:
    virtual ~IBus() = default;

    // Registers a response payload for the given frame ID.
    // When the master requests that ID, the supplied data is sent.
    // Passing empty data (or calling with id > kLINMaxID) is an error.
    // publish(id, {}) with zero data removes a registration.
    //
    // fusa:req REQ-LIN-011 REQ-LIN-019
    virtual std::error_code publish(uint8_t id, std::vector<uint8_t> data) = 0;

    // Registers with the classic (LIN 1.x) checksum.
    virtual std::error_code publish_classic(uint8_t id, std::vector<uint8_t> data) = 0;

    // Returns a channel delivering frames that match any of the supplied filters.
    // With no filters (empty vector) all frames are delivered.
    // The channel is closed when the bus is closed.
    //
    // fusa:req REQ-LIN-012 REQ-LIN-020
    virtual std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
        subscribe(std::vector<Filter> filters = {},
                  std::vector<relay::SubscriberOption> opts = {}) = 0;

    // Idempotent close. Subsequent publish/subscribe return ErrClosed.
    virtual std::error_code close() = 0;
};

// fusa:req REQ-LIN-013 REQ-LIN-014
class IMasterBus : public IBus {
public:
    // Drives a frame exchange: transmit break+sync+PID for id, collect the
    // slave response (if any), verify checksum, and broadcast the resulting
    // Frame to all subscribers. Returns ErrNoResponse when no slave response
    // is registered for id.
    //
    // fusa:req REQ-LIN-013 REQ-LIN-014
    virtual std::pair<Frame, std::error_code> send_header(uint8_t id) = 0;

    // Installs a new LIN schedule table. An empty slice is valid and disables
    // scheduled transmission.
    virtual std::error_code set_schedule(std::vector<ScheduleEntry> entries) = 0;
};

// ── Optional capability type aliases ─────────────────────────────────────────

using HealthStatus    = relay::HealthStatus;
using Health          = relay::Health;
using IHealthProvider = relay::IHealthProvider;
using Metrics         = relay::Metrics;
using IMetricsProvider = relay::IMetricsProvider;
using IDrainer        = relay::IDrainer;

// ── RELAY adapter ─────────────────────────────────────────────────────────────

// Wraps an IBus as a relay::INode for cross-protocol routing.
// fusa:req REQ-LIN-011
std::unique_ptr<relay::INode> adapt(std::shared_ptr<IBus> bus);

} // namespace lin
