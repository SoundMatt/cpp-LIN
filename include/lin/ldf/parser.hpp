// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// ldf/parser.hpp — LIN Description File (LDF) parser.
// Parses LDF files as defined by the LIN Consortium specification (LIN 2.x).
//
// fusa:req REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004 REQ-LDF-005
// fusa:req REQ-LDF-006 REQ-LDF-007 REQ-LDF-008 REQ-LDF-009 REQ-LDF-010
// fusa:req REQ-LDF-011 REQ-LDF-012 REQ-LDF-013 REQ-LDF-014 REQ-LDF-015

#pragma once

#include <lin/lin.hpp>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lin::ldf {

// SignalRef is a signal embedded within a frame at a given bit offset.
// fusa:req REQ-LDF-006
struct SignalRef {
    std::string name;
    int         bit_offset{};
};

// Frame describes a LIN frame as declared in the LDF Frames section.
// fusa:req REQ-LDF-005 REQ-LDF-006
struct LDFFrame {
    std::string           name;
    uint8_t               id{};
    std::string           publisher;
    int                   length{};
    std::vector<SignalRef> signals;
};

// Signal describes a signal as declared in the LDF Signals section.
// fusa:req REQ-LDF-007 REQ-LDF-008
struct Signal {
    std::string              name;
    int                      bit_width{};
    uint64_t                 init_value{};
    std::string              publisher;
    std::vector<std::string> subscribers;
    double                   scale{1.0};
    double                   offset{};
    std::string              unit;
    double                   min{};
    double                   max{};
};

// DB holds the parsed contents of an LDF file.
// fusa:req REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004
class DB {
public:
    std::string              protocol_version;
    std::string              language_version;
    double                   speed{};       // kbps
    std::string              master_node;
    std::vector<std::string> slave_nodes;

    // Returns the frame with the given ID, or nullptr if not found.
    // fusa:req REQ-LDF-005 REQ-LDF-012
    const LDFFrame* frame(uint8_t id) const;

    // Returns all frames, keyed by frame ID (defensive copy).
    // fusa:req REQ-LDF-005 REQ-LDF-015
    std::map<uint8_t, LDFFrame> frames() const;

    // Returns the signal with the given name, or nullptr if not found.
    // fusa:req REQ-LDF-007 REQ-LDF-013
    const Signal* signal(const std::string& name) const;

    // Returns all signals, keyed by name (defensive copy).
    // fusa:req REQ-LDF-007 REQ-LDF-008
    std::unordered_map<std::string, Signal> signals() const;

    // Returns the schedule table with the given name, or empty vector.
    // fusa:req REQ-LDF-011
    std::vector<ScheduleEntry> schedule(const std::string& name) const;

    // Extracts signal values from a raw frame payload.
    // Returns map of signal name → raw uint64 value (LSB-first bit order).
    // Returns empty map when frame ID is not present in the LDF.
    // fusa:req REQ-LDF-009 REQ-LDF-010
    std::unordered_map<std::string, uint64_t> decode(uint8_t id, const std::vector<uint8_t>& data) const;

    // Internal storage — populated by parse().
    std::map<uint8_t, LDFFrame>                          frames_;
    std::unordered_map<std::string, Signal>              signals_;
    std::unordered_map<std::string, std::vector<ScheduleEntry>> schedules_;
};

// Parses an LDF file from r.
// Never panics; malformed input results in a partial DB with a non-nil error.
// fusa:req REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004 REQ-LDF-014
std::unique_ptr<DB> parse(std::istream& r);

} // namespace lin::ldf
