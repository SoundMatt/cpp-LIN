// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// master/node.hpp — LIN master node.
//
// The master drives the schedule table: it transmits break+sync+PID for
// each slot in turn, collects slave responses, and enforces inter-frame
// slot timing. In a real system the master owns the bit-rate clock and
// is the only node that may transmit headers.
//
// fusa:req REQ-MASTER-001 REQ-MASTER-002 REQ-MASTER-003 REQ-MASTER-004
// fusa:req REQ-MASTER-005 REQ-MASTER-006 REQ-MASTER-007 REQ-MASTER-008
// fusa:req REQ-MASTER-009 REQ-MASTER-010 REQ-MASTER-011 REQ-MASTER-012
// fusa:req REQ-MASTER-013

#pragma once

#include <lin/lin.hpp>
#include <functional>
#include <memory>
#include <stop_token>
#include <vector>

namespace lin::master {

// Node is a LIN master node. It drives a schedule table over an IMasterBus.
//
// fusa:req REQ-MASTER-001
class Node {
public:
    // Creates a LIN master node backed by bus.
    // fusa:req REQ-MASTER-001
    explicit Node(std::shared_ptr<IMasterBus> bus);

    // Replaces the active schedule table. Validates all entries before storing
    // a defensive copy.
    // fusa:req REQ-MASTER-010 REQ-MASTER-011 REQ-MASTER-012
    std::error_code set_schedule(std::vector<ScheduleEntry> entries);

    // Registers a callback invoked for every successfully received frame.
    // The callback is called synchronously from run(); it must not block.
    // fusa:req REQ-MASTER-006
    void on_frame(std::function<void(Frame)> fn);

    // Registers a callback invoked when a slot produces an error.
    // fusa:req REQ-MASTER-007
    void on_error(std::function<void(std::error_code)> fn);

    // Triggers a single frame exchange for id outside the normal schedule.
    // fusa:req REQ-MASTER-002
    std::pair<Frame, std::error_code> send_header(uint8_t id);

    // Executes the schedule table repeatedly until the stop_token is requested.
    // Each slot transmits a header, waits for a slave response, then sleeps
    // for the slot's configured delay. Per-slot errors invoke on_error but do
    // not abort the schedule.
    // Returns an error immediately if the schedule is empty.
    // fusa:req REQ-MASTER-003 REQ-MASTER-004 REQ-MASTER-005 REQ-MASTER-006
    // fusa:req REQ-MASTER-007 REQ-MASTER-008 REQ-MASTER-009 REQ-MASTER-013
    std::error_code run(std::stop_token token);

private:
    std::shared_ptr<IMasterBus>          bus_;
    std::vector<ScheduleEntry>           schedule_;
    std::function<void(Frame)>           on_frame_;
    std::function<void(std::error_code)> on_error_;
};

} // namespace lin::master
