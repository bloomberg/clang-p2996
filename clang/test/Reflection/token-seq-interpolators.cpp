//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -ftoken-injection -verify

#include "Inputs/token-injection.h"

using namespace std::meta;

                          // ======================
                          // identifier_interpolator
                          // ======================

namespace identifier_interpolator {
constexpr sv name = "x";
constexpr int idx = 5;

static_assert(^^{ int \[name, idx]; } == ^^{ int x5; });
static_assert(^^{ \["abc"] } == ^^{ abc });
static_assert(^^{ \["a", "b", "c"] } == ^^{ abc });
static_assert(^^{ \["a", 1, "b", 23] } == ^^{ a1b23 });
static_assert(^^{ \[sv("hello world", 5)] } == ^^{ hello });
static_assert(^^{ \["_", 0u, 1L, 2ull] } == ^^{ _012 });
static_assert(^^{ \["n", -1] } != ^^{ n1 });  // expected-error {{not an integral constant expression}} \
                                              // expected-note {{'n-1' does not spell an identifier}}

// Operands are ordinary expressions of the enclosing context.
consteval info make(sv prefix, int n) {
  info result = ^^{};
  for (int i = 0; i < n; ++i)
    result = ^^{ \{result} \[prefix, i] };
  return result;
}
static_assert(make("v", 3) == ^^{ v0 v1 v2 });

// The operand of an identifier interpolator is only evaluated once.
consteval info once() {
  const char *names[] = {"first", "second"};
  int i = 0;
  info r = ^^{ \[sv(names[i++])] };
  return i == 1 ? r : ^^{};
}
static_assert(once() == ^^{ first });

constexpr auto bad1 = ^^{ \["1abc"] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'1abc' does not spell an identifier}}
constexpr auto bad2 = ^^{ \[""] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'' does not spell an identifier}}
constexpr auto bad3 = ^^{ \["while"] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'while' is a keyword, not an identifier}}
constexpr auto bad4 = ^^{ \["a b"] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'a b' does not spell an identifier}}

constexpr auto bad5 = ^^{ \[1, "a"] };
// expected-error@-1 {{operand of '\[' interpolator must be a string, but has type 'int'}}
// expected-note@-2 {{a string operand is}}
constexpr auto bad6 = ^^{ \["a", 1.5] };
// expected-error@-1 {{operand of '\[' interpolator must be a string or an integer, but has type 'double'}}
// expected-note@-2 {{a string operand is}}
struct NotAString { int size() const; };
constexpr auto bad7 = ^^{ \[NotAString{}] };
// expected-error@-1 {{operand of '\[' interpolator must be a string, but has type 'NotAString'}}
// expected-note@-2 {{a string operand is}}
constexpr auto bad8 = ^^{ \[] };
// expected-error@-1 {{expected expression}}
constexpr auto bad9 = ^^{ \["a" };
// expected-error@-1 {{expected ']'}}
// expected-note@-2 {{to match this '['}}
}  // namespace identifier_interpolator

                            // ===================
                            // tokens_interpolator
                            // ===================

namespace tokens_interpolator {
constexpr auto inner = ^^{ a + b };
constexpr auto outer = ^^{ return \{inner}; };
static_assert(outer == ^^{ return a + b; });
static_assert(^^{ \{^^{}} } == ^^{});
static_assert(^^{ ( \{inner} ) * \{inner} } == ^^{ ( a + b ) * a + b });

// A class type operand is converted to 'info', even explicitly.
struct Wrapper {
  info tokens;
  consteval explicit operator info() const { return tokens; }
};
static_assert(^^{ [ \{Wrapper{inner}} ] } == ^^{ [ a + b ] });

consteval info join() {
  list_builder args;
  args += ^^{ int a };
  args += ^^{};
  args += ^^{ int b };
  list_builder sum(^^{ + });
  sum += ^^{ a };
  sum += ^^{ b };
  return ^^{ int f(\{args}) { return \{sum}; } };
}
static_assert(join() == ^^{ int f(int a, int b) { return a + b; } });

constexpr auto bad1 = ^^{ \{^^int} };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{operand of '\{' interpolator does not represent a token sequence}}
constexpr auto bad2 = ^^{ \{42} };
// expected-error@-1 {{operand of '\{' interpolator must be convertible to 'std::meta::info', but has type 'int'}}
}  // namespace tokens_interpolator

                             // ==================
                             // value_interpolator
                             // ==================

namespace value_interpolator {
constexpr int val = 42;
static_assert(^^{ int x = \val(val); } == ^^{ int x = \val(42); });
static_assert(^^{ \val(1) } != ^^{ \val(2) });
static_assert(^^{ \val(1) } != ^^{ \val(1L) });
// A value is not the same token as the literal spelling it.
static_assert(^^{ \val(1) } != ^^{ 1 });

struct NonStructural { private: int x = 0; public: constexpr NonStructural() {} };
constexpr auto bad1 = ^^{ \val(NonStructural{}) };
// expected-error@-1 {{operand of '\val' interpolator has type 'NonStructural', which is not a structural type}}
constexpr auto bad2 = ^^{ \val() };
// expected-error@-1 {{expected expression}}
}  // namespace value_interpolator

                             // ===================
                             // string_interpolator
                             // ===================

namespace string_interpolator {
constexpr sv text = "hello";
static_assert(^^{ \str(text) } == ^^{ \str("hello") });
static_assert(^^{ \str("a") } != ^^{ \str("b") });
static_assert(^^{ \str(sv("hello world", 5)) } == ^^{ \str(text) });

constexpr auto bad1 = ^^{ \str(42) };
// expected-error@-1 {{operand of '\str' interpolator must be a string, but has type 'int'}}
// expected-note@-2 {{a string operand is}}
}  // namespace string_interpolator

                             // ===================
                             // splice_interpolator
                             // ===================

namespace splice_interpolator {
static_assert(^^{ \[:^^int:] } == ^^{ \[:^^int:] });
static_assert(^^{ \[:^^int:] } != ^^{ \[:^^long:] });
static_assert(^^{ \[:^^int:] } != ^^{ int });

constexpr auto bad1 = ^^{ \[: 42 :] };
// expected-error@-1 {{operand of '\[:' interpolator must be convertible to 'std::meta::info', but has type 'int'}}
constexpr auto bad2 = ^^{ \[: ^^int ] };
// expected-error@-1 {{expected ':]'}}
// expected-note@-2 {{to match this '[:'}}
}  // namespace splice_interpolator

                              // =================
                              // dependent_operands
                              // =================

namespace dependent_operands {
template <typename T, int N>
consteval info make() {
  return ^^{ \[:^^T:] \["arr", N][\val(N)]; };
}
static_assert(make<int, 3>() == ^^{ \[:^^int:] arr3[\val(3)]; });
static_assert(make<char, 4>() != make<char, 5>());

template <typename S>
consteval info named(S s) { return ^^{ \[s] }; }
static_assert(named("lit") == ^^{ lit });
static_assert(named(sv("view")) == ^^{ view });

template <typename T>
consteval info bad(T t) { return ^^{ \[t] }; }
// expected-error@-1 {{operand of '\[' interpolator must be a string, but has type 'double'}}
// expected-note@-2 {{a string operand is}}
constexpr auto b = bad(1.5);
// expected-note@-1 {{in instantiation of function template specialization}}
// expected-error@-2 {{must be initialized by a constant expression}}
}  // namespace dependent_operands
