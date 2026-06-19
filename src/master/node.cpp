// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/master/node.hpp>
#include <chrono>
#include <thread>

// fusa:req REQ-MASTER-001 REQ-MASTER-002 REQ-MASTER-003 REQ-MASTER-004
// fusa:req REQ-MASTER-005 REQ-MASTER-006 REQ-MASTER-007 REQ-MASTER-008
// fusa:req REQ-MASTER-009 REQ-MASTER-010 REQ-MASTER-011 REQ-MASTER-012
// fusa:req REQ-MASTER-013

namespace lin::master {

// fusa:req REQ-MASTER-001
Node::Node(std::shared_ptr<IMasterBus> bus)
    : bus_(std::move(bus))
{}

// fusa:req REQ-MASTER-010 REQ-MASTER-011 REQ-MASTER-012
std::error_code Node::set_schedule(std::vector<ScheduleEntry> entries) {
    if (entries.empty())
        return relay::make_error_code(relay::Errc::payload_too_large);  // empty schedule

    for (const auto& e : entries) {
        if (e.id > kLINMaxID)
            return relay::make_error_code(relay::Errc::payload_too_large);
    }

    // defensive copy stored
    schedule_ = std::move(entries);
    return {};
}

// fusa:req REQ-MASTER-006
void Node::on_frame(std::function<void(Frame)> fn) {
    on_frame_ = std::move(fn);
}

// fusa:req REQ-MASTER-007
void Node::on_error(std::function<void(std::error_code)> fn) {
    on_error_ = std::move(fn);
}

// fusa:req REQ-MASTER-002
std::pair<Frame, std::error_code> Node::send_header(uint8_t id) {
    return bus_->send_header(id);
}

// fusa:req REQ-MASTER-003 REQ-MASTER-004 REQ-MASTER-005 REQ-MASTER-006
// fusa:req REQ-MASTER-007 REQ-MASTER-008 REQ-MASTER-009 REQ-MASTER-013
std::error_code Node::run(const std::atomic<bool>& stop) {
    if (schedule_.empty())
        return relay::make_error_code(relay::Errc::payload_too_large);

    while (!stop.load()) {
        for (const auto& slot : schedule_) {
            if (stop.load()) return {};

            auto [f, err] = bus_->send_header(slot.id);
            if (err) {
                if (on_error_) on_error_(err);
            } else {
                if (on_frame_) on_frame_(f);
            }

            if (slot.delay_ms > 0) {
                auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(slot.delay_ms);
                while (!stop.load() &&
                       std::chrono::steady_clock::now() < deadline) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }
    return {};
}

} // namespace lin::master
