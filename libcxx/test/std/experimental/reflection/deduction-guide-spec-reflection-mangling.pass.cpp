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
// Regression test: a deduction-guide SPECIALIZATION reflection (Declaration
// kind, e.g. obtained via substitute on the guide template) used as a
// non-type template argument must mangle, not crash. The Template-kind guide
// encoding ("dg" + deduced template + ODR hash) already existed; the
// Declaration-kind path routed through mangleFunctionEncoding ->
// mangleUnqualifiedName and hit "Can't mangle a deduction guide name!".
// Distinct specializations must also mangle DISTINCTLY (no linker folding).

#include <experimental/meta>

namespace demo {
template <class T> struct Box { Box(T); };
Box(int) -> Box<int>;
Box(double) -> Box<double>;
} // namespace demo

template <std::meta::info R> struct Holder { static int x; };
template <std::meta::info R> int Holder<R>::x = 0;

consteval bool is_guide(std::meta::info m) {
  return std::meta::is_function_template(m) &&
         !std::meta::has_identifier(m) &&
         !std::meta::is_operator_function_template(m) &&
         !std::meta::is_conversion_function_template(m) &&
         !std::meta::is_literal_operator_template(m) &&
         !std::meta::is_constructor_template(m);
}

template <class A>
consteval std::meta::info guide_spec() {
  for (auto m : std::meta::members_of(^^demo,
                                      std::meta::access_context::unchecked()))
    if (is_guide(m) && std::meta::can_substitute(m, {^^A}))
      return std::meta::substitute(m, {^^A});
  return ^^void;
}

int main() {
  static_assert(guide_spec<int>() != ^^void);
  static_assert(guide_spec<double>() != ^^void);
  static_assert(guide_spec<int>() != guide_spec<double>());

  // Each specialization's reflection must mangle (this is what crashed) and
  // the two must NOT fold into one symbol at link time.
  int *a = &Holder<guide_spec<int>()>::x;
  int *b = &Holder<guide_spec<double>()>::x;
  if (a == b)
    return 1;
  return 0;
}
