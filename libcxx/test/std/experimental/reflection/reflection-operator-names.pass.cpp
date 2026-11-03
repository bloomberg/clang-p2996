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
// Regression test: reflections of operator templates, whose names are not
// identifiers, can be used as template arguments. They can only be obtained
// via members_of (^^ rejects them as overload sets).

#include <experimental/meta>

namespace ops {
template <class T> struct S {};
template <class T> bool operator==(S<T>, S<T>) { return true; }
struct Callable { template <class T> bool operator()(T) const { return true; } };
}

constexpr auto ctx = std::meta::access_context::unchecked();

consteval std::meta::info first_function_template(std::meta::info scope) {
  for (auto m : std::meta::members_of(scope, ctx))
    if (std::meta::is_function_template(m))
      return m;
  return ^^void;
}

template <std::meta::info R> int probe() { return 1; }

int main() {
  // Each specialization must get a (distinct) mangled name.
  int (*eq)() = &probe<first_function_template(^^ops)>;
  int (*call)() = &probe<first_function_template(^^ops::Callable)>;
  return eq != call && eq() + call() == 2 ? 0 : 1;
}
