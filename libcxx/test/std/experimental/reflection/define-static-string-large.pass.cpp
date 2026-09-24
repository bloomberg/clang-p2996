//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fconstexpr-steps=268435456

// <experimental/meta>

// RUN: %{build}
// RUN: %{exec} %t.exe

// Large pack expansions: a substituted character pack for a string of
// 2^15 or more characters used to overflow the 15-bit PackIndex storage on
// SubstNonTypeTemplateParmExpr (index+1 encoding wrapped to zero), producing
// a bogus "excess elements in array initializer" from define_static_string's
// FixedArray. Exercise both sides of the old cliff.

#include <experimental/meta>
#include <cstring>
#include <string>

template <std::size_t N>
consteval const char* lift() {
  return std::define_static_string(std::string(N, 'x'));
}

int main(int, char**) {
  // Just under the old 15-bit cliff (always worked).
  constexpr const char* a = lift<32767>();
  if (std::strlen(a) != 32767)
    return 1;
  // At and past the old cliff (used to fail to compile).
  constexpr const char* b = lift<32768>();
  if (std::strlen(b) != 32768)
    return 2;
  constexpr const char* c = lift<50000>();
  if (std::strlen(c) != 50000)
    return 3;
  return 0;
}
