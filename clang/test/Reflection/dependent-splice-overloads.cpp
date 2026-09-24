//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// RUN: %clang_cc1 %s -std=c++26 -freflection -fentity-proxy-reflection -verify

using info = decltype(^^int);

template <info R0, info R1>
struct DistinctSplices {
  using t0 = [:R0:];
  using t1 = [:R1:];

  static constexpr int f(t0) { return 0; }
  static constexpr int f(t1) { return 1; }
};

static_assert(DistinctSplices<^^int, ^^long>::f(0) == 0);
static_assert(DistinctSplices<^^int, ^^long>::f(0L) == 1);

template <info R>
struct SameSplice {
  using t0 = [:R:];
  using t1 = [:R:];

  static int f(t0); // expected-note {{previous declaration is here}}
  static int f(t1); // expected-error {{class member cannot be redeclared}}
};

template <typename T>
struct TBox {};

template <info R, typename T1, typename T2>
struct DistinctSpliceSpecializations {
  using t0 = typename [:R:]<T1>;
  using t1 = typename [:R:]<T2>;

  static constexpr int f(t0) { return 0; }
  static constexpr int f(t1) { return 1; }
};

static_assert(DistinctSpliceSpecializations<^^TBox, int, long>::f(TBox<int>{}) ==
              0);
static_assert(DistinctSpliceSpecializations<^^TBox, int, long>::f(TBox<long>{}) ==
              1);
