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
// [meta.reflection.scope]

#include <meta>

using std::meta::current_class;
using std::meta::current_function;
using std::meta::info;

                             // ==================
                             // current_function
                             // ==================

namespace current_function_throws {
// Throws unless the current scope is a function.
constexpr info v1 = current_function();
  // expected-error@-1 {{must be initialized by a constant expression}}

struct S {
  static constexpr info v2 = current_function();
    // expected-error@-1 {{must be initialized by a constant expression}}
};
}  // namespace current_function_throws

                               // =============
                               // current_class
                               // =============

namespace current_class_throws {
// Throws unless the current scope is a class or a member function.
constexpr info v1 = current_class();
  // expected-error@-1 {{must be initialized by a constant expression}}

consteval info free_fn() { return current_class(); }
constexpr info v2 = free_fn();
  // expected-error@-1 {{must be initialized by a constant expression}}
}  // namespace current_class_throws
