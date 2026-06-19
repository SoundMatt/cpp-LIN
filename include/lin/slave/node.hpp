// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// slave/node.hpp — LIN slave node.
//
// A slave node publishes response frames and subscribes to observe the bus.
// When the master transmits a header for a registered ID, the slave's
// response is sent by the underlying transport.
//
// fusa:req REQ-SLAVE-001 REQ-SLAVE-002 REQ-SLAVE-003 REQ-SLAVE-004
// fusa:req REQ-SLAVE-005 REQ-SLAVE-006 REQ-SLAVE-007 REQ-SLAVE-008

#pragma once

#include <lin/lin.hpp>
#include <memory>
#include <mutex>
#include <vector>

namespace lin::slave {

// Node is a LIN slave node. It registers response payloads on the bus and
// provides a subscription interface for observing frames.
//
// fusa:req REQ-SLAVE-001
class Node {
public:
    // Creates a LIN slave node backed by bus.
    // fusa:req REQ-SLAVE-001
    explicit Node(std::shared_ptr<IBus> bus);

    // Registers the response payload for the given frame ID.
    // When the master requests this ID, bus.publish delivers the payload.
    // Passing empty data removes the response.
    // Calling with an existing ID replaces the previous registration.
    // fusa:req REQ-SLAVE-002 REQ-SLAVE-003 REQ-SLAVE-004 REQ-SLAVE-008
    std::error_code set_response(uint8_t id, std::vector<uint8_t> data);

    // Returns a channel delivering frames that match any of the supplied filters.
    // Pass empty vector to receive all frames.
    // fusa:req REQ-SLAVE-006
    std::pair<std::shared_ptr<Chan<Frame>>, std::error_code>
        subscribe(std::vector<Filter> filters = {},
                  std::vector<relay::SubscriberOption> opts = {});

    // Returns the frame IDs for which this slave has a registered response.
    // Returns an empty (non-nil) vector when no responses are registered.
    // fusa:req REQ-SLAVE-005 REQ-SLAVE-007
    std::vector<uint8_t> registered_ids() const;

private:
    std::shared_ptr<IBus> bus_;
    mutable std::mutex    mu_;
    std::vector<uint8_t>  ids_;
};

} // namespace lin::slave
