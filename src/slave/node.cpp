// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/slave/node.hpp>
#include <algorithm>

// fusa:req REQ-SLAVE-001 REQ-SLAVE-002 REQ-SLAVE-003 REQ-SLAVE-004
// fusa:req REQ-SLAVE-005 REQ-SLAVE-006 REQ-SLAVE-007 REQ-SLAVE-008

namespace lin::slave {

// fusa:req REQ-SLAVE-001
Node::Node(std::shared_ptr<IBus> bus)
    : bus_(std::move(bus))
{}

// fusa:req REQ-SLAVE-002 REQ-SLAVE-003 REQ-SLAVE-004 REQ-SLAVE-008
std::error_code Node::set_response(uint8_t id, std::vector<uint8_t> data) {
    if (id > kLINMaxID)
        return relay::make_error_code(relay::Errc::payload_too_large);

    auto err = bus_->publish(id, data);
    if (err) return err;

    std::lock_guard<std::mutex> lk(mu_);
    auto it = std::find(ids_.begin(), ids_.end(), id);
    if (data.empty()) {
        if (it != ids_.end()) ids_.erase(it);
    } else {
        if (it == ids_.end()) ids_.push_back(id);
        // if already present, overwrite registration is handled by publish()
    }
    return {};
}

// fusa:req REQ-SLAVE-006
std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
Node::subscribe(std::vector<Filter> filters,
                std::vector<relay::SubscriberOption> opts) {
    return bus_->subscribe(std::move(filters), std::move(opts));
}

// fusa:req REQ-SLAVE-005 REQ-SLAVE-007
std::vector<uint8_t> Node::registered_ids() const {
    std::lock_guard<std::mutex> lk(mu_);
    return ids_;  // copy
}

} // namespace lin::slave
