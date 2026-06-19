# Contributing to cpp-LIN

Thank you for your interest in contributing to cpp-LIN.

## Developer Certificate of Origin (DCO)

All contributions must be signed off using the
[Developer Certificate of Origin](https://developercertificate.org/).
Add a `Signed-off-by` trailer to every commit:

```sh
git commit -s -m "feat: describe your change"
```

This certifies that you wrote the code or have the right to submit it under
the MPL-2.0 license.

## Branching and Pull Requests

- Fork the repository and create a branch from `main`.
- Branch names: `feat/<short-description>`, `fix/<short-description>`,
  `docs/<short-description>`, `refactor/<short-description>`.
- Open a Pull Request against `main`.
- At least one review from `@SoundMatt` is required before merge (SLSA L3).

## Code Style

See [CODING_STANDARD.md](CODING_STANDARD.md) for the full coding standard.
Key points:
- C++17, `-Wall -Wextra -Wpedantic`, no warnings.
- `clang-tidy` must pass with the project's `.clang-tidy` config.
- Every source file must have the MPL-2.0 copyright header.
- Every function that implements a safety requirement must have a
  `// fusa:req REQ-XXX-NNN` annotation.
- Every test function that verifies a safety requirement must have a
  `// fusa:test REQ-XXX-NNN` annotation.

## Building and Testing

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=17
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Ensure all tests pass and coverage stays above 70%.

## Commit Message Format

```
<type>: <short summary (imperative, lower case, ≤ 72 chars)>

<body: why this change, not what — wrap at 72 chars>

Signed-off-by: Your Name <your@email.com>
```

Types: `feat`, `fix`, `refactor`, `test`, `docs`, `ci`, `build`, `chore`.

## Security Issues

Do not open public issues for security vulnerabilities. Email
`matt@jellybaby.com` directly. See `SECURITY.md` for the full disclosure
policy.

## License

By contributing you agree that your contributions are licensed under the
Mozilla Public License 2.0.
