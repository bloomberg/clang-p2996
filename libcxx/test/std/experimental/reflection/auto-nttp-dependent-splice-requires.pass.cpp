//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// Regression test: a DEPENDENT splice used as the argument of an `auto`
// non-type template parameter inside a requires-expression must not crash the
// parser. A dependent CXXSpliceExpr carries no model expression; classifying
// it (Sema::DeduceAutoType -> Expr::ClassifyImpl) used to dyn_cast the null
// model and trip "dyn_cast on a non-existent value" at parse time. The
// classification now falls back to the splice's own value kind.

#include <experimental/meta>

template <auto> struct probe;
template <long double> struct probe_fixed;

template <std::meta::info mem>
consteval bool value_readable_auto() {
  // The crash fired while PARSING this requires-expression (no instantiation
  // needed); the constant-evaluation behavior is checked below for good
  // measure.
  if constexpr (requires { typename probe<([:mem:])>; })
    return true;
  return false;
}

template <std::meta::info mem>
consteval bool value_readable_fixed() {
  if constexpr (requires { typename probe_fixed<(long double)([:mem:])>; })
    return true;
  return false;
}

struct WithInit    { static const int K = 7; };
struct DeclaredOnly { static const int K; };

int main() {
  // Constant-readable in-class initializer: both probe forms succeed.
  static_assert(value_readable_auto<^^WithInit::K>());
  static_assert(value_readable_fixed<^^WithInit::K>());

  // Declared-but-never-defined: not readable; the requires-clause must be a
  // clean substitution failure, not a crash or a hard error.
  static_assert(!value_readable_auto<^^DeclaredOnly::K>());
  static_assert(!value_readable_fixed<^^DeclaredOnly::K>());
  return 0;
}
