#!/usr/bin/env bash
set -euo pipefail

INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/p2996/clang}"
LIB_SUBDIR="${LIB_SUBDIR:-aarch64-unknown-linux-gnu}"

CLANG="${INSTALL_PREFIX}/bin/clang++"
mkdir -p bin

"${CLANG}" p2996_examples.cpp -o bin/p2996_examples \
  -std=c++26 -freflection-latest -stdlib=libc++ \
  -L"${INSTALL_PREFIX}/lib/${LIB_SUBDIR}" \
  -Wl,-rpath,"${INSTALL_PREFIX}/lib/${LIB_SUBDIR}"

bin/p2996_examples --file_name data.csv --count 3  --name John

