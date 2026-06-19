# Coding Standard — cpp-LIN

## Language and Standard

- C++17 is the minimum supported standard.
- C++20 features may be used only if guarded by `#if __cplusplus >= 202002L`.
- All code must compile cleanly with `-Wall -Wextra -Wpedantic -Werror` under
  GCC 12 and Clang 14.

## File Layout

Every source and header file must start with:

```cpp
// Copyright (c) 2026 Matt Jones. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0.
```

Followed by:
1. File-level `// fusa:req` annotation listing requirements implemented here.
2. Standard library `#include`s, sorted alphabetically, separated from project
   includes by a blank line.
3. Project `#include`s, sorted alphabetically.
4. Namespace declarations.

## Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Types | `PascalCase` | `ScheduleEntry` |
| Functions | `snake_case` | `calc_checksum` |
| Variables | `snake_case` | `checksum_type` |
| Constants | `kPascalCase` | `kLINMaxDataLen` |
| Private members | `snake_case_` | `closed_` |
| Namespaces | `snake_case` | `lin::virt` |
| Macros | `SCREAMING_SNAKE_CASE` | (avoid macros) |
| Enum values | `PascalCase` | `ChecksumType::Enhanced` |

## Namespaces

- All LIN types live in `namespace lin`.
- Virtual bus lives in `namespace lin::virt`.
- Safety types live in `namespace lin::safety`.
- LDF parser lives in `namespace lin::ldf`.
- Master node lives in `namespace lin::master`.
- Slave node lives in `namespace lin::slave`.
- RELAY types live in `namespace relay` (shared with cpp-CAN).
- Anonymous namespaces are preferred over `static` for file-local helpers.

## Error Handling

- Use exceptions for programmer errors (invalid arguments, contract violations).
- Use `std::error_code` for expected runtime failures in APIs.
- Never use `abort()`, `assert()`, or `exit()` in library code — only in tests.
- Error types must inherit from `std::runtime_error` or `std::system_error`.
- Every public function that can fail must document its failure modes.

## Thread Safety

- All public APIs must document their thread-safety guarantees.
- Use `std::shared_mutex` for read-heavy, write-rare data (subscriptions).
- Use `std::atomic` for metrics counters to avoid lock contention.
- Never hold a lock across an external callback invocation.

## Memory

- Prefer `std::unique_ptr` for exclusive ownership.
- Use `std::shared_ptr` only when shared ownership is genuinely needed.
- No raw `new`/`delete` in library code.
- No `malloc`/`free`.

## FuSa Annotations

Every function that implements a requirement from `.fusa-reqs.json` must carry:

```cpp
// fusa:req REQ-LIN-001
```

Multiple requirements on the same function:

```cpp
// fusa:req REQ-LIN-001 REQ-LIN-002
```

Every `TEST_CASE` or `SECTION` that verifies a requirement must carry:

```cpp
// fusa:test REQ-LIN-001
```

## clang-tidy

The project's `.clang-tidy` enables:
- `clang-analyzer-*`
- `bugprone-*`
- `modernize-use-override`
- `modernize-use-nullptr`
- `cppcoreguidelines-no-malloc`

All warnings from enabled checks must be resolved (not suppressed) before merge.

## Testing

- Every public API must have a corresponding unit test.
- Line coverage must remain at or above 70%.
- Tests must not have external dependencies (no network, no filesystem access
  except reading test fixtures from `testdata/`).
- Use Catch2 `REQUIRE` for assertions that must pass for the test to be valid;
  use `CHECK` for non-fatal assertions.
- Each `TEST_CASE` must carry a unique tag `[lin]`, `[virtual]`, `[e2e]`,
  `[ldf]`, `[master]`, `[slave]`, `[relay]`, or `[cli]`.

## Documentation

- All public types and functions must have Doxygen-style `///` comments.
- Comments must explain *why*, not *what*.
- Do not comment-out dead code — delete it.
