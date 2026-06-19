// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/virtual/bus.hpp>
#include <chrono>
#include <thread>

// fusa:req REQ-VIRT-001 REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005
// fusa:req REQ-VIRT-006 REQ-VIRT-007 REQ-VIRT-008 REQ-VIRT-009 REQ-VIRT-010
// fusa:req REQ-VIRT-011 REQ-VIRT-012 REQ-VIRT-013 REQ-VIRT-014 REQ-VIRT-015
// fusa:req REQ-VIRT-016 REQ-VIRT-017 REQ-VIRT-018 REQ-VIRT-019

namespace lin::virt {

static constexpr int kDefaultChanDepth = 64;

namespace {
    bool matches_any(const std::vector<Filter>& filters, const Frame& f) {
        if (filters.empty()) return true;
        for (const auto& fl : filters)
            if (fl.matches(f)) return true;
        return false;
    }
} // anonymous namespace

std::shared_ptr<Bus> Bus::create() {
    return std::shared_ptr<Bus>(new Bus());
}

// fusa:req REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005 REQ-VIRT-019
std::error_code Bus::do_publish(uint8_t id, std::vector<uint8_t> data, ChecksumType ct) {
    if (id > kLINMaxID) {
        error_count_.fetch_add(1);
        return relay::make_error_code(relay::Errc::payload_too_large);
    }

    std::unique_lock<std::shared_mutex> lk(mu_);
    if (closed_) {
        error_count_.fetch_add(1);
        return relay::ErrClosed();
    }

    if (data.empty()) {
        responses_.erase(id);
    } else {
        // defensive copy already done by value parameter
        responses_[id] = ResponseEntry{std::move(data), ct};
    }
    return {};
}

// fusa:req REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005 REQ-VIRT-019
std::error_code Bus::publish(uint8_t id, std::vector<uint8_t> data) {
    return do_publish(id, std::move(data), ChecksumType::Enhanced);
}

// fusa:req REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005 REQ-VIRT-019
std::error_code Bus::publish_classic(uint8_t id, std::vector<uint8_t> data) {
    return do_publish(id, std::move(data), ChecksumType::Classic);
}

// fusa:req REQ-VIRT-006 REQ-VIRT-007 REQ-VIRT-008 REQ-VIRT-009 REQ-VIRT-010 REQ-VIRT-017
std::pair<Frame, std::error_code> Bus::send_header(uint8_t id) {
    if (id > kLINMaxID) {
        error_count_.fetch_add(1);
        return {Frame{}, relay::make_error_code(relay::Errc::payload_too_large)};
    }

    std::shared_lock<std::shared_mutex> lk(mu_);
    if (closed_) {
        error_count_.fetch_add(1);
        return {Frame{}, relay::ErrClosed()};
    }

    auto it = responses_.find(id);
    if (it == responses_.end()) {
        error_count_.fetch_add(1);
        return {Frame{}, relay::ErrTimeout()};  // ErrNoResponse maps to timeout
    }

    uint8_t pid = protect_id(id);
    std::vector<uint8_t> data = it->second.data;  // copy
    ChecksumType ct = it->second.checksum_type;
    uint8_t cs = calc_checksum(pid, data, ct);

    Frame f;
    f.id           = id;
    f.data         = std::move(data);
    f.checksum     = cs;
    f.checksum_type = ct;

    write_count_.fetch_add(1);
    bytes_written_.fetch_add(f.data.size());

    // broadcast to subscribers (must hold at least shared lock, which we have)
    for (const auto& s : subs_) {
        if (!matches_any(s.filters, f)) continue;

        switch (s.back_pressure) {
        case relay::BackPressurePolicy::DropNewest: {
            auto r = s.ch->try_send(f);
            if (r == Chan<Frame>::SendResult::Ok) {
                deliver_count_.fetch_add(1);
                bytes_delivered_.fetch_add(f.data.size());
            } else if (r == Chan<Frame>::SendResult::Full) {
                drop_count_.fetch_add(1);
            }
            break;
        }
        case relay::BackPressurePolicy::DropOldest:
            s.ch->send_drop_oldest(f);
            deliver_count_.fetch_add(1);
            bytes_delivered_.fetch_add(f.data.size());
            break;
        case relay::BackPressurePolicy::Block:
            s.ch->send(f);
            deliver_count_.fetch_add(1);
            bytes_delivered_.fetch_add(f.data.size());
            break;
        }
    }

    return {f, std::error_code{}};
}

// fusa:req REQ-VIRT-011 REQ-VIRT-012 REQ-VIRT-013 REQ-VIRT-014
std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
Bus::subscribe(std::vector<Filter> filters,
               std::vector<relay::SubscriberOption> opts)
{
    relay::SubscriberConfig cfg = relay::apply_options(opts);
    int depth = cfg.effective_depth(kDefaultChanDepth);

    std::unique_lock<std::shared_mutex> lk(mu_);
    if (closed_) {
        error_count_.fetch_add(1);
        return {std::shared_ptr<Chan<Frame>>{}, relay::ErrClosed()};
    }

    auto ch = std::make_shared<Chan<Frame>>(static_cast<std::size_t>(depth));
    subs_.push_back({std::move(filters), ch, cfg.back_pressure});
    return {ch, std::error_code{}};
}

std::error_code Bus::set_schedule(std::vector<ScheduleEntry> entries) {
    std::unique_lock<std::shared_mutex> lk(mu_);
    if (closed_) return relay::ErrClosed();
    schedule_ = std::move(entries);
    return {};
}

// fusa:req REQ-VIRT-015 REQ-VIRT-016
std::error_code Bus::close() {
    std::unique_lock<std::shared_mutex> lk(mu_);
    if (closed_) return {};
    closed_ = true;
    for (auto& s : subs_) s.ch->close();
    subs_.clear();
    return {};
}

// fusa:req REQ-RELAY-023 REQ-RELAY-024 REQ-RELAY-025
Health Bus::health() const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    if (closed_) return {HealthStatus::Down, "bus closed"};
    return {HealthStatus::OK, {}};
}

// fusa:req REQ-RELAY-026 REQ-RELAY-027 REQ-RELAY-029
Metrics Bus::metrics() const {
    return {
        write_count_.load(),
        deliver_count_.load(),
        drop_count_.load(),
        bytes_written_.load(),
        bytes_delivered_.load(),
        error_count_.load(),
    };
}

// fusa:req REQ-RELAY-028
std::error_code Bus::close_with_drain(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (all_drained()) return close();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto err = close();
    return err ? err : relay::ErrTimeout();
}

bool Bus::all_drained() const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    for (const auto& s : subs_)
        if (s.ch->size() > 0) return false;
    return true;
}

} // namespace lin::virt
