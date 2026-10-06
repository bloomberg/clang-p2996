//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Token sequences survive a round trip through an AST file: both as the
// values of constants (written as text) and as expressions in templates.
//
// RUN: %clang_cc1 -std=c++26 -freflection -ftoken-injection -x c++-header \
// RUN:   -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -ftoken-injection -include-pch %t.pch \
// RUN:   -fsyntax-only -verify %s

#ifndef HEADER
#define HEADER

#include "Inputs/token-injection.h"

using namespace std::meta;

constexpr auto ts = ^^{ a + "str" 42 'c' [: :] ^^ };
constexpr auto empty = ^^{};

template <typename T>
consteval info declare(sv name, int n) {
  return ^^{ constexpr \[:^^T:] \[name, n] = \val(n) + \str("s")[0]; };
}

consteval info plain() { return ^^{ constexpr int x = 1; }; }

#else

// expected-no-diagnostics

static_assert(ts == ^^{ a + "str" 42 'c' [: :] ^^ });
static_assert(ts != ^^{ a + "str" 42 'c' [: :] });
static_assert(empty == ^^{});
static_assert(is_token_sequence(ts) && is_empty_token_sequence(empty));
static_assert(declare<int>("v", 3) == ^^{ constexpr \[:^^int:] v3 = \val(3) + \str("s")[0]; });

consteval {
  queue_injection(plain());
  queue_injection(^^{ constexpr auto copy = ^^{ \{ts} }; });
}
static_assert(x == 1 && copy == ts);

consteval { queue_injection(declare<long>("w", 7)); }
static_assert(w7 == 7 + 's');
static_assert(__is_same(decltype(w7), const long));

#endif
