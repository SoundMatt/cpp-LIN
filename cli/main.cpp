// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// cpp-lin-cli — RELAY-conformant CLI for cpp-LIN.
//
// Subcommands (RELAY spec §11.1 / §11.2):
//   version        — print spec + implementation version as JSON
//   capabilities   — print supported protocols and commands as JSON
//   status         — print health status as JSON
//   convert        — read lin.Frame JSON from stdin, write relay.Message JSON to stdout
//
// REQ-CLI-001 through REQ-CLI-006

#include "json.hpp"
#include <lin/lin.hpp>
#include <iostream>
#include <string>

// Parses an optional "--format <value>" flag from argv[2..argc) for commands
// that accept it (spec §11.1: `version [--format text|json]`, `status
// [--format text|json]`). Defaults `fmt` to "json". Any other argument
// (including an unrecognized --format value) is an invalid-args error: the
// caller is responsible for validating `fmt` against the command's allowed
// set and exiting 2 per spec §11.1/§11.3.
static bool parse_format_flag(int argc, char* argv[], std::string& fmt) {
    fmt = "json";
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" && i + 1 < argc) {
            fmt = argv[++i];
        } else {
            std::cerr << "ErrInvalidArgs: unrecognized argument '" << arg << "'\n";
            return false;
        }
    }
    return true;
}

// fusa:req REQ-CLI-001
static int cmd_version(const std::string& fmt) {
    if (fmt == "json") { std::cout << cli::version_json() << "\n"; return 0; }
    if (fmt == "text") { std::cout << cli::version_text();          return 0; }
    std::cerr << "ErrInvalidArgs: unsupported --format '" << fmt << "' (want text|json)\n";
    return 2;
}

static int cmd_capabilities() { std::cout << cli::capabilities_json() << "\n"; return 0; }

// fusa:req REQ-CLI-003
static int cmd_status(const std::string& fmt) {
    if (fmt == "json") { std::cout << cli::status_json() << "\n"; return 0; }
    if (fmt == "text") { std::cout << cli::status_text();          return 0; }
    std::cerr << "ErrInvalidArgs: unsupported --format '" << fmt << "' (want text|json)\n";
    return 2;
}

// fusa:req REQ-CLI-006
static int cmd_convert(const std::string& protocol) {
    if (protocol != "LIN") {
        std::cerr << "ErrUnsupportedProtocol\n";
        return 2;
    }

    std::string input;
    std::string line;
    while (std::getline(std::cin, line)) input += line;

    lin::Frame f;
    try {
        f = cli::parse_frame_json(input);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    try {
        lin::validate_frame(f);
    } catch (const lin::ErrInvalidFrame& e) {
        // spec §11.2: convert MUST write the sentinel error name (§5) to
        // stderr — the thrown type here is lin::ErrInvalidFrame, so that is
        // the name relay interop/relay conform expect, not a made-up one.
        std::cerr << "ErrInvalidFrame: " << e.what() << "\n";
        return 1;
    }

    auto msg      = lin::to_message(f);
    msg.timestamp = {};

    std::cout << cli::message_to_json(msg) << "\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: cpp-lin-cli <command> [options]\n";
        std::cerr << "commands: version capabilities status convert\n";
        return 2;
    }

    std::string cmd = argv[1];

    if (cmd == "version") {
        std::string fmt;
        if (!parse_format_flag(argc, argv, fmt)) return 2;
        return cmd_version(fmt);
    }
    if (cmd == "capabilities") return cmd_capabilities();
    if (cmd == "status") {
        std::string fmt;
        if (!parse_format_flag(argc, argv, fmt)) return 2;
        return cmd_status(fmt);
    }

    if (cmd == "convert") {
        std::string protocol;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--protocol" && i + 1 < argc) {
                protocol = argv[++i];
            } else if (arg == "--format" && i + 1 < argc) {
                std::string fmt = argv[++i];
                if (fmt != "json") {
                    std::cerr << "ErrUnsupportedFormat: " << fmt << "\n";
                    return 2;
                }
            }
        }
        if (protocol.empty()) {
            std::cerr << "ErrMissingArg: --protocol required\n";
            return 2;
        }
        return cmd_convert(protocol);
    }

    std::cerr << "ErrUnknownCommand: " << cmd << "\n";
    return 2;
}
