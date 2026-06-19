# Copyright (c) 2026 Matt Jones. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.
#
# Multi-stage Dockerfile for cpp-LIN.
# Stage 1 (build): compiles the library, tests, and CLI.
# Stage 2 (test):  runs the test suite and exits.
# Stage 3 (cli):   minimal runtime image with just the CLI binary.

# ── Stage 1: build ───────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS build

ARG DEBIAN_FRONTEND=noninteractive
ARG CMAKE_BUILD_TYPE=Release
ARG CXX_STANDARD=17

RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    ca-certificates \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build \
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
      -DCMAKE_CXX_STANDARD=${CXX_STANDARD} \
      -G Ninja \
  && cmake --build build --parallel

# ── Stage 2: test ────────────────────────────────────────────────────────────
FROM build AS test

WORKDIR /src
ENTRYPOINT ["ctest", "--test-dir", "build", "--output-on-failure", "-j1"]

# ── Stage 3: cli (minimal runtime) ───────────────────────────────────────────
FROM ubuntu:22.04 AS cli

RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    libstdc++6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/cli/cpp-lin-cli /usr/local/bin/cpp-lin-cli
RUN chmod +x /usr/local/bin/cpp-lin-cli

# Non-root user for minimal attack surface
RUN useradd -r -s /bin/false linuser
USER linuser

ENTRYPOINT ["/usr/local/bin/cpp-lin-cli"]
CMD ["--help"]

LABEL org.opencontainers.image.title="cpp-LIN CLI" \
      org.opencontainers.image.description="C++ LIN bus library CLI — RELAY-conformant, ISO 26262 ASIL-B" \
      org.opencontainers.image.vendor="Matt Jones" \
      org.opencontainers.image.licenses="MPL-2.0" \
      org.opencontainers.image.source="https://github.com/SoundMatt/cpp-LIN"
