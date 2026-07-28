//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fparameter-reflection

// <experimental/meta>

// RUN: %{build}
// RUN: %{exec} %t.exe

// Parameter-name queries must not depend on instantiation state. A member
// whose in-class declaration and out-of-line definition name a parameter
// differently has NO consistent name; instantiating the definition (which
// replaces the parameters on the instantiated FunctionDecl in place) must
// not change the answer. The consistency walk goes through the template
// pattern's full declaration chain.

#include <experimental/meta>
#include <string_view>

using namespace std::meta;

template <class T> struct S {
  void f(int value);   // inconsistent: the definition says "val"
  void g(int width);   // consistent across declaration + definition
};
template <class T> void S<T>::f(int val) {}
template <class T> void S<T>::g(int width) {}

// Instantiate the definitions BEFORE the queries: pre-fix this flipped f's
// reported parameter name from "value" to "val".
template void S<int>::f(int);
template void S<int>::g(int);

consteval info param0(std::string_view name) {
  for (auto m : members_of(^^S<int>, access_context::unchecked()))
    if (is_function(m) && has_identifier(m) && identifier_of(m) == name)
      return parameters_of(m)[0];
  return {};
}

static_assert(!has_identifier(param0("f")));
static_assert(has_identifier(param0("g")));
static_assert(identifier_of(param0("g")) == "width");

int main(int, char**) { return 0; }
