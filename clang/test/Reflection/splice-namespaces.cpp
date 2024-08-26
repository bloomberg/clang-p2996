//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

using info = decltype(^^int);

int global_decl;

constexpr int x = 1;

namespace myns {
  namespace inner { int y; constexpr int z = 3; }
  constexpr int x = 2;
}  // namespace myns

                                 // ===========
                                 // idempotency
                                 // ===========

namespace idempotency {
namespace inner {
  constexpr int x = 3;
}  // namespace inner

static_assert(&[:^^:::]::global_decl == &::global_decl);
static_assert(&[:^^idempotency:]::inner::x == &inner::x);
static_assert(&[:^^inner:]::x == &inner::x);
}  // namespace idempotency

                                   // ======
                                   // in_nns
                                   // ======

namespace in_nns {
namespace Alias = ::idempotency::inner;

constexpr info r_global = ^^::;
constexpr info r_myns = ^^::myns;
constexpr info r_alias = ^^Alias;

static_assert([:r_global:]::x == 1);
static_assert([:r_myns:]::x == 2);
static_assert([:r_alias:]::x == 3);

// Splicing dependent reflection of a namespace in a nested name specifier.
template <info R> consteval int getX() { return [:R:]::x; }
static_assert(getX<r_myns>() == 2);
}  // namespace in_nns

                               // ==============
                               // in_alias_defns
                               // ==============

namespace in_alias_defns {
constexpr info r_global = ^^::;
constexpr info r_myns = ^^myns;

namespace Alias1 = [:r_myns:];
static_assert(Alias1::x == 2);

constexpr auto r_Alias1 = ^^Alias1;
namespace Alias2 = [:r_Alias1:];
static_assert(&myns::x == &Alias2::x);

namespace Alias3 = [:r_global:]::idempotency::inner;
static_assert(Alias3::x == 3);

template <info R>
consteval int XPlusY() {
  namespace Alias = [:R:];
  namespace ReAlias = Alias;
  namespace InnerAlias = [:R:]::inner;
  namespace ReAliasInner = InnerAlias;

  return ReAlias::x + ReAliasInner::z;
}
static_assert(XPlusY<^^myns>() == 5);
}  // namespace in_alias_defns

                             // ===================
                             // in_using_directives
                             // ===================

namespace in_using_directives {
namespace Alias = ::myns::inner;

constexpr info r_global = ^^::;
constexpr info r_myns = ^^myns;
constexpr info r_Alias = ^^Alias;
void test1() {
  using namespace [:r_global:]::idempotency;
  static_assert(inner::x == 3);
}
void test2() {
  using namespace [:r_myns:]::inner;
  static_assert(&y == &myns::inner::y);
}
void test3() {
  using namespace [:r_Alias:];
  static_assert(&y == &myns::inner::y);
}
}  // namespace in_using_directives
