//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++26 -freflection -ftoken-injection -verify
// RUN: %clang_cc1 %s -std=c++26 -freflection -verify=noflag -DNO_FLAG
// RUN: not %clang_cc1 %s -std=c++26 -ftoken-injection -fsyntax-only 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NOREFL

// NOREFL: cannot specify '-ftoken-injection' without '-freflection'

#ifdef NO_FLAG

// Without -ftoken-injection, '^^{' does not introduce a token sequence and a
// backslash is not a token.
constexpr auto t = ^^{ int x; };  // noflag-error {{cannot reflect the provided operand}}
static_assert(!__has_feature(token_injection));

#else

#include "Inputs/token-injection.h"

using namespace std::meta;

static_assert(__has_feature(token_injection));

                              // =============
                              // basic_literals
                              // =============

namespace basic_literals {
constexpr auto t1 = ^^{ a + b };
constexpr info t2 = ^^{};
constexpr auto t3 = ^^{ int f() { return { 1 }; } };

static_assert(__is_same(decltype(t1), const info));
static_assert(__is_same(decltype(^^{ }), info));

static_assert(is_token_sequence(t1));
static_assert(is_token_sequence(t2));
static_assert(is_token_sequence(t3));
static_assert(!is_token_sequence(^^int));
static_assert(!is_token_sequence(^^::));
static_assert(!is_token_sequence(info{}));

static_assert(!is_empty_token_sequence(t1));
static_assert(is_empty_token_sequence(t2));
static_assert(!is_empty_token_sequence(^^int));

// Only braces have to be balanced.
constexpr auto t4 = ^^{ ( [ < };
constexpr auto t5 = ^^{ ) ] > :] [: };
static_assert(is_token_sequence(t4) && is_token_sequence(t5));

// Arbitrary tokens: keywords, literals, punctuators.
constexpr auto t6 = ^^{ template <typename T> struct S { T t = 1.5f; }; };
constexpr auto t7 = ^^{ "string" 'c' 42 0x2a 1'000 u8"x" R"(raw)" nullptr };
}  // namespace basic_literals

                                 // ========
                                 // equality
                                 // ========

namespace equality {
// Whitespace and comments are not significant.
constexpr auto t1 = ^^{ hello  = /* world */   "world" };
constexpr auto t2 = ^^{ hello="world" };
static_assert(t1 == t2);

static_assert(^^{ a + b } == ^^{a+b});
static_assert(^^{ a + b } != ^^{ a - b });
static_assert(^^{ a + b } != ^^{ a + c });
static_assert(^^{ a + b } != ^^{ a + b + c });
static_assert(^^{} == ^^{});
static_assert(^^{} != ^^{;});
static_assert(^^{ 1 } != ^^{ 1u });
static_assert(^^{ "a" } != ^^{ "b" });
static_assert(^^{ "a" } == ^^{ "a" });
static_assert(^^{ abc } != ^^{ ab c });
static_assert(^^{ int } != ^^int);
static_assert(^^{ >> } != ^^{ > > });

// Concatenation preserves the identity of tokens.
constexpr auto abc = ^^{ abc };
constexpr auto def = ^^{ def };
constexpr auto both = ^^{ \{abc} \{def} };
static_assert(both == ^^{ abc def });
static_assert(both != ^^{ abcdef });

// Macros are expanded before the sequence is formed.
#define PLUS +
#define TWICE(x) x x
static_assert(^^{ a PLUS b } == ^^{ a + b });
static_assert(^^{ TWICE(z) } == ^^{ z z });
}  // namespace equality

                              // =============
                              // report_tokens
                              // =============

namespace report_tokens {
consteval { __report_tokens(^^{ int   x = 42 ; }); }
// expected-warning@-1 {{token sequence: int x = 42 ;}}
consteval { __report_tokens(^^{}); }
// expected-warning-re@-1 {{token sequence: {{$}}}}
consteval { __report_tokens(^^{ f("a b", 'c', 1.5e3) -> x }); }
// expected-warning@-1 {{token sequence: f ( "a b" , 'c' , 1.5e3 ) -> x}}
consteval { __report_tokens(^^{ \[:^^int:] \["id", 1] = \val(42) + \str("s"); }); }
// expected-warning@-1 {{token sequence: [: ^^(type) :] id1 = 42 + "s" ;}}

consteval {  // expected-error {{must be a constant expression}} \
             // expected-note {{in call to}}
  __report_tokens(^^int);
  // expected-note@-1 {{expected a reflection of a token sequence, but got a type}}
  // expected-note@-2 {{in call to}}
  // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
}
}  // namespace report_tokens

                           // ====================
                           // ill_formed_literals
                           // ====================

namespace ill_formed_literals {
constexpr auto t1 = ^^{ a \ b };
// expected-error@-1 {{expected an interpolator}}

int x = \val(1);
// expected-error@-1 {{expected expression}}
}  // namespace ill_formed_literals

constexpr auto unterminated = ^^{ abc { def };
// expected-error@* {{expected '}' to end token sequence}}
// expected-note@-2 {{to match this '{'}}
// expected-error@-3 {{expected ';' after top level declarator}}

#endif
