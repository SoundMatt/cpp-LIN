// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/lin.hpp>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

// fusa:req REQ-LIN-001 REQ-LIN-002 REQ-LIN-003 REQ-LIN-004 REQ-LIN-005
// fusa:req REQ-LIN-006 REQ-LIN-007 REQ-LIN-008 REQ-LIN-009 REQ-LIN-010
// fusa:req REQ-LIN-011 REQ-LIN-012 REQ-LIN-013 REQ-LIN-014 REQ-LIN-015
// fusa:req REQ-LIN-016 REQ-LIN-017 REQ-LIN-018 REQ-LIN-019 REQ-LIN-020
// fusa:req REQ-LIN-021

namespace lin {

// ── protect_id ── REQ-LIN-004 REQ-LIN-005 REQ-LIN-018 ────────────────────────

uint8_t protect_id(uint8_t id) noexcept {
    id &= kLINMaxID;  // mask to 6 bits
    uint8_t p0 = ((id >> 0) ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 0x01u;
    uint8_t p1 = static_cast<uint8_t>((~((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 0x01u);
    return static_cast<uint8_t>(id | (p0 << 6) | (p1 << 7));
}

// ── verify_pid ── REQ-LIN-006 REQ-LIN-007 ────────────────────────────────────

uint8_t verify_pid(uint8_t pid) {
    uint8_t id = pid & kLINMaxID;
    if (protect_id(id) != pid)
        throw ErrInvalidFrame("PID 0x" + [pid]{ char buf[8]; std::snprintf(buf, sizeof(buf), "%02X", pid); return std::string(buf); }() + " parity mismatch");
    return id;
}

// ── calc_checksum ── REQ-LIN-008 REQ-LIN-009 REQ-LIN-010 ─────────────────────

uint8_t calc_checksum(uint8_t pid, const std::vector<uint8_t>& data, ChecksumType ct) noexcept {
    uint16_t sum = 0;
    if (ct == ChecksumType::Enhanced) {
        sum = static_cast<uint16_t>(pid);
    }
    for (auto b : data) {
        sum += static_cast<uint16_t>(b);
        if (sum > 0xFF) {
            sum -= 0xFF;  // carry-around (not 0x100)
        }
    }
    return static_cast<uint8_t>(0xFF - static_cast<uint8_t>(sum));
}

// ── validate_frame ── REQ-LIN-001 REQ-LIN-002 REQ-LIN-003 REQ-LIN-015 REQ-LIN-016 REQ-LIN-017

void validate_frame(const Frame& f) {
    if (f.id > kLINMaxID)
        throw ErrInvalidFrame("frame ID 0x" + [f]{ char buf[8]; std::snprintf(buf, sizeof(buf), "%02X", f.id); return std::string(buf); }() + " exceeds maximum 0x3F");
    if (f.data.empty())
        throw ErrInvalidFrame("frame data must not be empty");
    if (f.data.size() > kLINMaxDataLen)
        throw ErrInvalidFrame("frame data length " + std::to_string(f.data.size()) + " exceeds maximum " + std::to_string(kLINMaxDataLen));
    if ((f.id == kLINDiagRequestID || f.id == kLINDiagResponseID) && f.checksum_type != ChecksumType::Classic)
        throw ErrInvalidFrame("diagnostic frame 0x" + [f]{ char buf[8]; std::snprintf(buf, sizeof(buf), "%02X", f.id); return std::string(buf); }() + " must use classic checksum");
}

// ── to_message / from_message ── REQ-LIN-011 REQ-LIN-012 ────────────────────

relay::Message to_message(const Frame& f) {
    relay::Message m;
    m.protocol = relay::Protocol::LIN;
    m.id       = std::to_string(static_cast<int>(f.id));
    m.payload  = f.data;
    m.meta["lin.checksum_type"] = (f.checksum_type == ChecksumType::Enhanced) ? "enhanced" : "classic";
    m.meta["lin.checksum"]      = std::to_string(static_cast<int>(f.checksum));
    return m;
}

Frame from_message(const relay::Message& m) {
    unsigned long long id_val{};
    try {
        id_val = std::stoull(m.id);
    } catch (...) {
        throw ErrInvalidFrame("invalid LIN ID: " + m.id);
    }
    if (id_val > kLINMaxID)
        throw ErrInvalidFrame("invalid LIN ID: " + m.id);

    Frame f;
    f.id   = static_cast<uint8_t>(id_val);
    f.data = m.payload;

    auto it = m.meta.find("lin.checksum_type");
    if (it != m.meta.end() && it->second == "enhanced")
        f.checksum_type = ChecksumType::Enhanced;
    else
        f.checksum_type = ChecksumType::Classic;

    it = m.meta.find("lin.checksum");
    if (it != m.meta.end() && !it->second.empty()) {
        try { f.checksum = static_cast<uint8_t>(std::stoull(it->second)); } catch (...) {}
    }
    return f;
}

// ── RELAY adapter ── REQ-LIN-011 ─────────────────────────────────────────────

namespace {

class LinAdapter : public relay::INode {
public:
    explicit LinAdapter(std::shared_ptr<IBus> bus) : bus_(std::move(bus)) {}

    relay::Protocol protocol() const noexcept override { return relay::Protocol::LIN; }

    std::error_code send(relay::Message msg) override {
        try {
            uint8_t id = static_cast<uint8_t>(std::stoull(msg.id));
            return bus_->publish(id, std::move(msg.payload));
        } catch (...) {
            return relay::make_error_code(relay::Errc::payload_too_large);
        }
    }

    std::pair<std::shared_ptr<Chan<relay::Message>>, std::error_code>
        subscribe(std::vector<relay::SubscriberOption> opts = {}) override
    {
        relay::SubscriberConfig cfg = relay::apply_options(opts);

        auto [frames, err] = bus_->subscribe({}, {});
        if (err) return {nullptr, err};

        int depth = cfg.effective_depth(64);
        auto out = std::make_shared<Chan<relay::Message>>(static_cast<std::size_t>(depth));

        std::thread([this, frame_ch = std::move(frames), out,
                     bp = cfg.back_pressure]() mutable
        {
            while (true) {
                auto opt_f = frame_ch->recv();
                if (!opt_f) break;

                relay::Message msg = to_message(*opt_f);
                msg.timestamp = std::chrono::system_clock::now();
                msg.seq = ++seq_;

                switch (bp) {
                case relay::BackPressurePolicy::DropNewest:
                    out->try_send(std::move(msg));
                    break;
                case relay::BackPressurePolicy::DropOldest:
                    out->send_drop_oldest(std::move(msg));
                    break;
                case relay::BackPressurePolicy::Block:
                    out->send(std::move(msg));
                    break;
                }
            }
            out->close();
        }).detach();

        return {out, {}};
    }

    std::error_code close() override { return bus_->close(); }

private:
    std::shared_ptr<IBus> bus_;
    std::atomic<uint64_t> seq_{0};
};

} // anonymous namespace

std::unique_ptr<relay::INode> adapt(std::shared_ptr<IBus> bus) {
    return std::make_unique<LinAdapter>(std::move(bus));
}

} // namespace lin
