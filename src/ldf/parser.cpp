// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <lin/ldf/parser.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>

// fusa:req REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004 REQ-LDF-005
// fusa:req REQ-LDF-006 REQ-LDF-007 REQ-LDF-008 REQ-LDF-009 REQ-LDF-010
// fusa:req REQ-LDF-011 REQ-LDF-012 REQ-LDF-013 REQ-LDF-014 REQ-LDF-015

namespace lin::ldf {

// ── DB accessors ──────────────────────────────────────────────────────────────

// fusa:req REQ-LDF-005 REQ-LDF-012
const LDFFrame* DB::frame(uint8_t id) const {
    auto it = frames_.find(id);
    return it != frames_.end() ? &it->second : nullptr;
}

// fusa:req REQ-LDF-005 REQ-LDF-015
std::map<uint8_t, LDFFrame> DB::frames() const {
    return frames_;  // defensive copy
}

// fusa:req REQ-LDF-007 REQ-LDF-013
const Signal* DB::signal(const std::string& name) const {
    auto it = signals_.find(name);
    return it != signals_.end() ? &it->second : nullptr;
}

// fusa:req REQ-LDF-007 REQ-LDF-008
std::unordered_map<std::string, Signal> DB::signals() const {
    return signals_;  // defensive copy
}

// fusa:req REQ-LDF-011
std::vector<ScheduleEntry> DB::schedule(const std::string& name) const {
    auto it = schedules_.find(name);
    if (it == schedules_.end()) return {};
    return it->second;  // copy
}

// fusa:req REQ-LDF-009 REQ-LDF-010
std::unordered_map<std::string, uint64_t>
DB::decode(uint8_t id, const std::vector<uint8_t>& data) const {
    auto it = frames_.find(id);
    if (it == frames_.end()) return {};

    std::unordered_map<std::string, uint64_t> result;
    for (const auto& ref : it->second.signals) {
        auto sit = signals_.find(ref.name);
        if (sit == signals_.end()) continue;

        // LSB-first (Intel) bit extraction — REQ-LDF-009
        uint64_t val = 0;
        int bit_width = sit->second.bit_width;
        for (int i = 0; i < bit_width; ++i) {
            int byte_idx = (ref.bit_offset + i) / 8;
            int bit_idx  = (ref.bit_offset + i) % 8;
            if (byte_idx >= static_cast<int>(data.size())) break;
            if (data[byte_idx] & (1 << bit_idx))
                val |= static_cast<uint64_t>(1) << i;
        }
        result[ref.name] = val;
    }
    return result;
}

// ── Parser helpers ────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    auto l = s.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    auto r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

static std::string strip_comment(const std::string& line) {
    auto pos = line.find("//");
    if (pos != std::string::npos)
        return trim(line.substr(0, pos));
    return trim(line);
}

static std::string strip_semicolon(const std::string& s) {
    std::string r = trim(s);
    if (!r.empty() && r.back() == ';') r.pop_back();
    return trim(r);
}

// fusa:req REQ-LDF-002
static double extract_float_value(const std::string& line) {
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) {
        tok = strip_semicolon(tok);
        try { return std::stod(tok); } catch (...) {}
    }
    return 0.0;
}

static std::string extract_quoted_value(const std::string& line) {
    auto start = line.find('"');
    if (start == std::string::npos) return "";
    auto end = line.find('"', start + 1);
    if (end == std::string::npos) return "";
    return line.substr(start + 1, end - start - 1);
}

static int64_t parse_int(const std::string& s) {
    std::string t = strip_semicolon(s);
    if (t.size() > 2 && (t.substr(0, 2) == "0x" || t.substr(0, 2) == "0X"))
        return static_cast<int64_t>(std::stoll(t.substr(2), nullptr, 16));
    return std::stoll(t);
}

static uint64_t parse_uint(const std::string& s) {
    std::string t = strip_semicolon(s);
    if (t.size() > 2 && (t.substr(0, 2) == "0x" || t.substr(0, 2) == "0X"))
        return std::stoull(t.substr(2), nullptr, 16);
    // handle float strings like "10.0"
    try { return static_cast<uint64_t>(std::stod(t)); } catch (...) {}
    return std::stoull(t);
}

// ── parse() ───────────────────────────────────────────────────────────────────

namespace {

struct Parser {
    std::vector<std::string> lines;
    std::size_t              pos{};

    std::string peek() const {
        if (pos >= lines.size()) return "";
        return lines[pos];
    }
    std::string next() {
        auto l = peek();
        ++pos;
        return l;
    }

    void parse_nodes(DB& db) {
        if (!peek().empty() && peek().front() == '{') next();
        while (pos < lines.size()) {
            auto line = peek();
            if (line == "}") { next(); return; }
            next();
            if (line.substr(0, 7) == "Master:") {
                std::string rest = trim(line.substr(7));
                auto comma = rest.find(',');
                std::string master_part = comma != std::string::npos ? rest.substr(0, comma) : rest;
                db.master_node = strip_semicolon(master_part);
            } else if (line.substr(0, 7) == "Slaves:") {
                std::string rest = strip_semicolon(trim(line.substr(7)));
                std::istringstream ss(rest);
                std::string tok;
                while (std::getline(ss, tok, ',')) {
                    tok = trim(tok);
                    if (!tok.empty()) db.slave_nodes.push_back(tok);
                }
            }
        }
    }

    void parse_signals(DB& db) {
        if (!peek().empty() && peek().front() == '{') next();
        while (pos < lines.size()) {
            auto line = peek();
            if (line == "}") { next(); return; }
            next();
            // name : bitWidth, initValue, publisher, subscribers ;
            line = strip_semicolon(line);
            auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string name = trim(line.substr(0, colon));
            std::string rest = trim(line.substr(colon + 1));
            std::vector<std::string> parts;
            std::istringstream ss(rest);
            std::string tok;
            while (std::getline(ss, tok, ','))
                parts.push_back(trim(tok));
            if (parts.size() < 3) continue;

            Signal sig;
            sig.name = name;
            try { sig.bit_width = static_cast<int>(parse_int(parts[0])); } catch (...) {}
            try { sig.init_value = parse_uint(parts[1]); } catch (...) {}
            sig.publisher = trim(parts[2]);
            for (std::size_t i = 3; i < parts.size(); ++i) {
                std::istringstream ss2(parts[i]);
                std::string sub;
                while (std::getline(ss2, sub, ',')) {
                    sub = trim(sub);
                    if (!sub.empty()) sig.subscribers.push_back(sub);
                }
            }
            db.signals_[name] = sig;
        }
    }

    void parse_frames(DB& db) {
        if (!peek().empty() && peek().front() == '{') next();
        while (pos < lines.size()) {
            auto line = peek();
            if (line == "}") { next(); return; }
            if (line.find('{') != std::string::npos) {
                next();
                // frame header: name : id, publisher, length {
                std::string hdr = line;
                // strip trailing {
                auto brace = hdr.rfind('{');
                if (brace != std::string::npos) hdr = trim(hdr.substr(0, brace));
                auto colon = hdr.find(':');
                if (colon == std::string::npos) continue;
                LDFFrame fr;
                fr.name = trim(hdr.substr(0, colon));
                std::string rest = trim(hdr.substr(colon + 1));
                std::vector<std::string> parts;
                std::istringstream ss(rest);
                std::string tok;
                while (std::getline(ss, tok, ','))
                    parts.push_back(trim(tok));
                if (parts.size() >= 3) {
                    try { fr.id = static_cast<uint8_t>(parse_int(parts[0])); } catch (...) {}
                    fr.publisher = trim(parts[1]);
                    try { fr.length = static_cast<int>(parse_int(parts[2])); } catch (...) {}
                }
                // parse signal refs
                while (pos < lines.size()) {
                    auto inner = peek();
                    if (inner == "}") { next(); break; }
                    next();
                    inner = strip_semicolon(inner);
                    auto comma = inner.find(',');
                    if (comma != std::string::npos) {
                        SignalRef ref;
                        ref.name = trim(inner.substr(0, comma));
                        try { ref.bit_offset = static_cast<int>(parse_int(trim(inner.substr(comma + 1)))); } catch (...) {}
                        fr.signals.push_back(ref);
                    }
                }
                db.frames_[fr.id] = fr;
            } else {
                next();
            }
        }
    }

    void parse_schedule_tables(DB& db) {
        if (!peek().empty() && peek().front() == '{') next();
        while (pos < lines.size()) {
            auto line = peek();
            if (line == "}") { next(); return; }
            if (line.back() == '{') {
                std::string table_name = trim(line.substr(0, line.size() - 1));
                next();
                std::vector<ScheduleEntry> entries;
                while (pos < lines.size()) {
                    auto inner = peek();
                    if (inner == "}") { next(); break; }
                    next();
                    inner = strip_semicolon(inner);
                    if (inner.substr(0, 13) == "AssignFrameId") continue;
                    // frame_name delay N ms
                    std::vector<std::string> parts;
                    std::istringstream ss(inner);
                    std::string tok2;
                    while (ss >> tok2) parts.push_back(tok2);
                    if (parts.size() >= 3) {
                        // find delay keyword
                        bool found_delay = false;
                        for (std::size_t i = 1; i < parts.size(); ++i) {
                            std::string lc = parts[i];
                            for (auto& ch : lc) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                            if (lc == "delay" && i + 1 < parts.size()) {
                                uint32_t delay_ms = 0;
                                try { delay_ms = static_cast<uint32_t>(parse_uint(parts[i + 1])); } catch (...) {}
                                // resolve frame name to ID
                                uint8_t fid = 0xFF;
                                for (const auto& [fid_k, fr] : db.frames_) {
                                    if (fr.name == parts[0]) { fid = fid_k; break; }
                                }
                                if (fid <= kLINMaxID)
                                    entries.push_back({fid, delay_ms});
                                found_delay = true;
                                break;
                            }
                        }
                        (void)found_delay;
                    }
                }
                db.schedules_[table_name] = entries;
            } else {
                next();
            }
        }
    }

    void parse_top(DB& db) {
        while (pos < lines.size()) {
            auto line = peek();
            if (line.substr(0, 20) == "LIN_protocol_version") {
                db.protocol_version = extract_quoted_value(line);
                next();
            } else if (line.substr(0, 20) == "LIN_language_version") {
                db.language_version = extract_quoted_value(line);
                next();
            } else if (line.substr(0, 9) == "LIN_speed") {
                db.speed = extract_float_value(line);
                next();
            } else if (line.substr(0, 5) == "Nodes") {
                next();
                parse_nodes(db);
            } else if (line.substr(0, 7) == "Signals") {
                next();
                parse_signals(db);
            } else if (line.substr(0, 6) == "Frames") {
                next();
                parse_frames(db);
            } else if (line.substr(0, 15) == "Schedule_tables") {
                next();
                parse_schedule_tables(db);
            } else {
                next();
            }
        }
    }
};

} // anonymous namespace

// fusa:req REQ-LDF-001 REQ-LDF-002 REQ-LDF-003 REQ-LDF-004 REQ-LDF-014
std::unique_ptr<DB> parse(std::istream& r) {
    auto db = std::make_unique<DB>();
    try {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(r, line)) {
            line = strip_comment(line);
            if (!line.empty()) lines.push_back(line);
        }
        Parser p;
        p.lines = std::move(lines);
        p.parse_top(*db);
    } catch (...) {
        // never panic — REQ-LDF-014
    }
    return db;
}

} // namespace lin::ldf
