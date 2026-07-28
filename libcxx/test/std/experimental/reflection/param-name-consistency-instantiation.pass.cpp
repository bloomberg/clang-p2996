//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fparameter-reflection -Wno-unused-parameter -Wno-deprecated-declarations

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

template <class T> struct P {
    template <class... Args> void h(int value, Args... rest);
};
template <class T> template <class... Args>
void P<T>::h(int val, Args... rest) {}

template void P<int>::h<float>(int, float);

// The non-pack parameter "value"/"val" is inconsistently named. The pack sits at
// pattern index 1, i.e. AFTER the queried index 0, so the query still walks the
// pattern's declaration chain and detects the inconsistency.
static_assert(!has_identifier(parameters_of(^^P<int>::h<float>)[0]));

// A named parameter positioned AFTER a function parameter pack. Instantiating
// with TWO pack elements makes "tail" land at instantiation index 3, while it
// sits at index 2 in the pattern (where the pack is a single parameter). Because
// a pack precedes the queried index, the query cannot safely index the pattern's
// shorter parameter list and falls back to the instantiation. This must neither
// misindex nor crash -- getParamDecl(3) on the 3-parameter pattern would be
// out-of-bounds.
template <class T> struct Q {
  template <class... Args> void k(int value, Args... rest, int tail);
};
template <class T> template <class... Args>
void Q<T>::k(int val, Args... rest, int tail) {}

template void Q<int>::k<float, double>(int, float, double, int);

// Pre-pack parameter (index 0): inconsistent name, detected via the pattern.
static_assert(!has_identifier(parameters_of(^^Q<int>::k<float, double>)[0]));

// Trailing parameter after the pack (instantiation index 3, pattern index 2):
// must not crash; reports its consistently-spelled name.
static_assert(has_identifier(parameters_of(^^Q<int>::k<float, double>)[3]));
static_assert(identifier_of(parameters_of(^^Q<int>::k<float, double>)[3]) == "tail");

int main(int, char**) { return 0; }
