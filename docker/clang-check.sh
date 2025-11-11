#!/usr/bin/env bash
set -euo pipefail

# Ensure INSTALL_PREFIX default
INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/p2996/clang}"
LIB_SUBDIR="${LIB_SUBDIR:-aarch64-unknown-linux-gnu}"

# Use the toolchain's clang++ to avoid host/system clang
CLANGPP="${INSTALL_PREFIX}/bin/clang++"

# Set CLANG_BUILD_STRING from the container toolchain's clang
export CLANG_BUILD_STRING="$($CLANGPP --version | tr '\n' ';')"

# Prepare output dir
mkdir -p bin

# Build the checker, passing CLANG_BUILD_STRING as a string literal
"$CLANGPP" check_clang_libc++.cpp -o bin/check_clang_libc++ \
  -std=c++26 -freflection-latest -stdlib=libc++ \
  -L"${INSTALL_PREFIX}/lib/${LIB_SUBDIR}" \
  -Wl,-rpath,"${INSTALL_PREFIX}/lib/${LIB_SUBDIR}" \
  -DCLANG_BUILD_STRING="\"${CLANG_BUILD_STRING}\""

# Run
bin/check_clang_libc++
