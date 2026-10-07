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
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <experimental/reflection>
//
// [meta.reflection.annotation]

#include <meta>

                        // ==============================
                        // annotations_of_with_type_throws
                        // ==============================

namespace annotations_of_with_type_throws {
[[=1]] void fn();
struct Incomplete;

// 'type' must represent a complete type.
constexpr auto n1 = annotations_of_with_type(^^fn, ^^Incomplete).size();
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto n2 = annotations_of_with_type(^^fn, ^^fn).size();
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto n3 = annotations_of_with_type(^^fn, ^^::).size();
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto n4 = annotations_of_with_type(^^fn, std::meta::info{}).size();
  // expected-error@-1 {{must be initialized by a constant expression}}

constexpr auto n5 = annotations_of_with_type(^^fn, ^^int).size();  // ok
static_assert(n5 == 1);
}  // namespace annotations_of_with_type_throws

                           // =======================
                           // deprecated_two_arg_form
                           // =======================

namespace deprecated_two_arg_form {
[[=1]] void fn();

constexpr auto n = annotations_of(^^fn, ^^int).size();
  // expected-warning@-1 {{'annotations_of' is deprecated: renamed to 'annotations_of_with_type' in P3394R4}}
static_assert(n == 1);
}  // namespace deprecated_two_arg_form
