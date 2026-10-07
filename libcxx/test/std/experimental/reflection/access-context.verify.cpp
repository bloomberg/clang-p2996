//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [meta.reflection.access.context]

#include <meta>

using std::meta::access_context;
using std::meta::info;

                                    // ===
                                    // via
                                    // ===

namespace via {
struct Incomplete;
struct Complete { };
enum class E { };
void fn();

// via(cls) requires cls to be the null reflection or a complete class type.
constexpr auto v1 = access_context::unchecked().via(^^Incomplete);
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto v2 = access_context::unchecked().via(^^int);
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto v3 = access_context::unchecked().via(^^E);
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto v4 = access_context::unchecked().via(^^fn);
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto v5 = access_context::unchecked().via(^^via);
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto v6 = access_context::unchecked().via(^^Complete);  // ok
}  // namespace via

                             // ==================
                             // not_constructible
                             // ==================

namespace not_constructible {
constexpr access_context ctx;
  // expected-error@-1 {{call to deleted constructor}}
}  // namespace not_constructible
