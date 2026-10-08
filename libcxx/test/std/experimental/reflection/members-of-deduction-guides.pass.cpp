//===----------------------------------------------------------------------===//
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
// Regression test: deduction guides are neither functions nor function
// templates ([temp.deduct.guide]), so they are not members-of-representable
// ([meta.reflection.member.queries]) and members_of must not return them.
// Returning them made reflections of guides reachable, which the manglers
// cannot encode (issues #298, #312).

#include <experimental/meta>

namespace demo {
template <class T> struct Box { Box(T); };
template <class T> Box(T *) -> Box<T>;  // a deduction-guide template
Box(int) -> Box<long>;                  // a non-template deduction guide
}

consteval bool returns_no_guides() {
  for (auto m : std::meta::members_of(^^demo, std::meta::access_context::unchecked()))
    if ((std::meta::is_function(m) || std::meta::is_function_template(m)) &&
        !std::meta::has_identifier(m))
      return false;
  return true;
}
static_assert(returns_no_guides());

// The class template itself is still a member.
static_assert(std::meta::members_of(^^demo, std::meta::access_context::unchecked()).size() == 1);

int main() {}
