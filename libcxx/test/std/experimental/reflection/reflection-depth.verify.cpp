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
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]

#include <meta>


                            // ===================
                            // nested_reflect_test
                            // ===================

// 'std::meta::info' is a structural type, so 'reflect_constant' accepts one and
// returns a reflection of it. The representation counts these nestings in a
// fixed-width field; the count must saturate into a diagnostic rather than wrap
// back to "not a reflection".

namespace nested_reflect_test {

// Returns a reflection nested 'N' levels deep over the integer 0.
consteval std::meta::info nest(int N) {
  std::meta::info R = std::meta::reflect_constant(0);
  for (int Idx = 1; Idx < N; ++Idx)
    R = std::meta::reflect_constant(R);
      // expected-note@-1 {{reflection would nest more than 255 levels deep}}
  return R;
}

// The deepest representable nesting is still a reflection of a value.
static_assert(std::meta::is_value(nest(255)));

// Peeling exactly 254 layers back off must land on the reflection of the
// original integer: one layer too few or too many fails to extract. This pins
// the depth of 'nest(255)' at 255, so the cap below is at the representable
// limit and not somewhere short of it.
consteval int unnest_255(std::meta::info R) {
  for (int Idx = 1; Idx < 255; ++Idx)
    R = std::meta::extract<std::meta::info>(R);
  return std::meta::extract<int>(R);
}
static_assert(unnest_255(nest(255)) == 0);

// One level further is refused.
constexpr auto too_deep = nest(256);
  // expected-error@-1 {{must be initialized by a constant expression}}

}  // namespace nested_reflect_test

int main() { }
