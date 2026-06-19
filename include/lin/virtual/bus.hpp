// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// virtual/bus.hpp — In-process broadcast LIN bus.
// Zero OS dependencies; default transport for development and testing.
// Mirrors go-LIN/virtual package.
//
// fusa:req REQ-VIRT-001 through REQ-VIRT-019

#pragma once

#include <lin/lin.hpp>
#include <atomic>
#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace lin::virt {

// Bus is an in-process LIN bus.
//
// Simulates both master and slave behaviour in a single process.
// Call publish() to register slave responses; call send_header() to trigger
// frame exchanges as a master node. All observed frames are delivered to
// matching subscribers.
// Multiple threads may call send_header(), publish(), and subscribe() concurrently.
//
// fusa:req REQ-VIRT-001 REQ-VIRT-002 REQ-VIRT-003 REQ-VIRT-004 REQ-VIRT-005
// fusa:req REQ-VIRT-006 REQ-VIRT-007 REQ-VIRT-008 REQ-VIRT-009 REQ-VIRT-010
// fusa:req REQ-VIRT-011 REQ-VIRT-012 REQ-VIRT-013 REQ-VIRT-014 REQ-VIRT-015
// fusa:req REQ-VIRT-016 REQ-VIRT-017 REQ-VIRT-018 REQ-VIRT-019
class Bus : public IMasterBus,
            public IHealthProvider,
            public IMetricsProvider,
            public IDrainer
{
public:
    // Factory constructor — always heap-allocated.
    static std::shared_ptr<Bus> create();

    // IBus
    std::error_code publish(uint8_t id, std::vector<uint8_t> data) override;
    std::error_code publish_classic(uint8_t id, std::vector<uint8_t> data) override;
    std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
        subscribe(std::vector<Filter> filters = {},
                  std::vector<relay::SubscriberOption> opts = {}) override;
    std::error_code close() override;

    // IMasterBus
    std::pair<Frame, std::error_code> send_header(uint8_t id) override;
    std::error_code set_schedule(std::vector<ScheduleEntry> entries) override;

    // IHealthProvider
    Health health() const override;

    // IMetricsProvider
    Metrics metrics() const override;

    // IDrainer — waits until all subscriber channels are empty or timeout elapses.
    std::error_code close_with_drain(std::chrono::milliseconds timeout) override;

private:
    Bus() = default;

    struct ResponseEntry {
        std::vector<uint8_t> data;
        ChecksumType         checksum_type{ChecksumType::Enhanced};
    };

    struct Subscription {
        std::vector<Filter>                 filters;
        std::shared_ptr<Chan<Frame>>        ch;
        relay::BackPressurePolicy           back_pressure;
    };

    std::error_code do_publish(uint8_t id, std::vector<uint8_t> data, ChecksumType ct);
    void broadcast(const Frame& f);
    bool all_drained() const;

    mutable std::shared_mutex               mu_;
    std::map<uint8_t, ResponseEntry>        responses_;
    std::vector<ScheduleEntry>              schedule_;
    std::vector<Subscription>               subs_;
    bool                                    closed_{false};

    std::atomic<uint64_t>                   write_count_{0};
    std::atomic<uint64_t>                   deliver_count_{0};
    std::atomic<uint64_t>                   drop_count_{0};
    std::atomic<uint64_t>                   bytes_written_{0};
    std::atomic<uint64_t>                   bytes_delivered_{0};
    std::atomic<uint64_t>                   error_count_{0};
};

} // namespace lin::virt
