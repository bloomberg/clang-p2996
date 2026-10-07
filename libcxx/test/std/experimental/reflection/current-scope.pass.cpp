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
using std::meta::current_namespace;
using std::meta::info;

                              // ================
                              // namespace_scope
                              // ================

static_assert(current_namespace() == ^^::);

namespace namespace_scope {
static_assert(current_namespace() == ^^namespace_scope);

namespace inner {
static_assert(current_namespace() == ^^inner);
}  // namespace inner

namespace {
static_assert(current_namespace() != ^^namespace_scope);
static_assert(is_namespace(current_namespace()));
static_assert(parent_of(current_namespace()) == ^^namespace_scope);
}  // namespace
}  // namespace namespace_scope

                               // ==============
                               // function_scope
                               // ==============

namespace function_scope {
consteval info fn() { return current_function(); }
static_assert(fn() == ^^fn);

consteval info ns_from_fn() { return current_namespace(); }
static_assert(ns_from_fn() == ^^function_scope);

// The scope is that of the function containing the call, not of any
// function it calls.
consteval info outer() { return fn(); }
static_assert(outer() == ^^fn);

template <typename T> consteval info tfn() { return current_function(); }
static_assert(tfn<int>() == ^^tfn<int>);
static_assert(tfn<char>() == ^^tfn<char>);
static_assert(template_of(tfn<int>()) == ^^tfn);

// A lambda's scope is its call operator, whose class is the closure type.
consteval info from_lambda() {
  return [] { return current_function(); }();
}
static_assert(is_function(from_lambda()));
static_assert(from_lambda() != ^^from_lambda);
static_assert(is_class_member(from_lambda()));
static_assert(is_class_type(parent_of(from_lambda())));

consteval info lambda_class() {
  return [] { return current_class(); }();
}
static_assert(is_class_type(lambda_class()));
static_assert(!has_identifier(lambda_class()));

consteval info lambda_namespace() {
  return [] { return current_namespace(); }();
}
static_assert(lambda_namespace() == ^^function_scope);
}  // namespace function_scope

                                // ===========
                                // class_scope
                                // ===========

namespace class_scope {
struct S {
  static constexpr info cls = current_class();
  static constexpr info ns = current_namespace();
  static_assert(current_class() == ^^S);

  static consteval info from_static_mfn() { return current_class(); }
  consteval info from_mfn() const { return current_class(); }
  static consteval info fn_from_static_mfn() { return current_function(); }
  static consteval info ns_from_static_mfn() { return current_namespace(); }

  struct Nested {
    static constexpr info cls = current_class();
    static constexpr info ns = current_namespace();
    static consteval info from_mfn() { return current_class(); }
  };
};
static_assert(S::cls == ^^S);
static_assert(S::ns == ^^class_scope);
static_assert(S::from_static_mfn() == ^^S);
static_assert(S{}.from_mfn() == ^^S);
static_assert(S::fn_from_static_mfn() == ^^S::fn_from_static_mfn);
static_assert(S::ns_from_static_mfn() == ^^class_scope);
static_assert(S::Nested::cls == ^^S::Nested);
static_assert(S::Nested::ns == ^^class_scope);
static_assert(S::Nested::from_mfn() == ^^S::Nested);

union U {
  static constexpr info cls = current_class();
  static consteval info from_mfn() { return current_class(); }
};
static_assert(U::cls == ^^U);
static_assert(U::from_mfn() == ^^U);

template <typename T> struct TCls {
  static constexpr info cls = current_class();
  static consteval info from_mfn() { return current_class(); }
  static consteval info fn() { return current_function(); }
};
static_assert(TCls<int>::cls == ^^TCls<int>);
static_assert(TCls<int>::from_mfn() == ^^TCls<int>);
static_assert(TCls<char>::from_mfn() == ^^TCls<char>);
static_assert(TCls<int>::fn() == ^^TCls<int>::fn);

// A local class inside a function.
consteval info local_class() {
  struct Local {
    static consteval info cls() { return current_class(); }
  };
  return Local::cls();
}
static_assert(is_class_type(local_class()));
static_assert(identifier_of(local_class()) == "Local");
}  // namespace class_scope

int main() { }
