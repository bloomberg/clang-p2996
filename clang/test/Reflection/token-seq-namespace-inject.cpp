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

                                // ============
                                // paper_example
                                // ============

namespace paper_example {
consteval auto f(info r, int val, sv name) {
  return ^^{ constexpr \[:r:] \[name] = \val(val); };
}

namespace N {}

consteval {
  queue_injection(^^{ static_assert(N::x == 42); });
  namespace_inject(^^N, f(^^int, 42, "x"));
}
static_assert(N::x == 42);

// The injection is immediate: the namespace is complete once the function
// returns.
consteval void inject_then_check() {
  namespace_inject(^^N, ^^{ constexpr int y = x + 1; });
  namespace_inject(^^N, ^^{ static_assert(y == 43); });
}
consteval { inject_then_check(); }
static_assert(N::y == 43);
}  // namespace paper_example

                               // ===============
                               // open_namespaces
                               // ===============

consteval { namespace_inject(^^{ constexpr int global_injected = 1; }); }
static_assert(global_injected == 1);

namespace open {
consteval {
  namespace_inject(^^{ constexpr int a = 1; });
  namespace_inject(^^open, ^^{ constexpr int b = a + 1; });
  namespace_inject(^^::, ^^{ constexpr int from_open = 3; });
}
static_assert(a == 1 && b == 2 && from_open == 3);

struct S {
  consteval { namespace_inject(^^{ constexpr int from_class = 4; }); }
  static_assert(from_class == 4);
  // The class member of the same name is not hidden by the injection.
  static constexpr int a = 100;
  static_assert(a == 100);
};

constexpr int f() {
  int from_function = -1;
  consteval {
    namespace_inject(^^{ constexpr int from_function = 5; constexpr int a_copy = a; });
  }
  // The local variable still hides the injected one.
  return from_function;
}
static_assert(f() == -1 && from_function == 5 && a_copy == 1);
}  // namespace open
static_assert(open::from_class == 4 && from_open == 3);

                              // =================
                              // closed_namespaces
                              // =================

namespace closed {
namespace inner { constexpr int existing = 1; }
inline namespace inl { constexpr int in_inline = 2; }
namespace { constexpr int in_anon = 3; }
}  // namespace closed

consteval {
  namespace_inject(^^closed, ^^{ constexpr int a = 10; });
  namespace_inject(^^closed::inner, ^^{ constexpr int b = existing + 10; });
  namespace_inject(^^closed::inl, ^^{ constexpr int c = in_inline + 10; });
  namespace_inject(^^closed, ^^{
    namespace inner { constexpr int d = b + 1; }
    struct T { int t = 4; };
    template <typename U> constexpr U id(U u) { return u; }
  });
}
static_assert(closed::a == 10 && closed::inner::b == 11 && closed::c == 12);
static_assert(closed::inl::c == 12 && closed::inner::d == 12);
static_assert(closed::id(closed::T().t) == 4);
static_assert(^^closed::a != ^^closed::inner::b);

namespace alias = closed::inner;
consteval { namespace_inject(^^alias, ^^{ constexpr int via_alias = 6; }); }
static_assert(closed::inner::via_alias == 6);

// Injected code that injects.
consteval {
  namespace_inject(^^closed, ^^{
    consteval { queue_injection(^^{ constexpr int queued = 7; }); }
    consteval { namespace_inject(^^closed::inner, ^^{ constexpr int deep = 8; }); }
    constexpr int after = queued + inner::deep;
  });
}
static_assert(closed::after == 15);

                                  // ======
                                  // errors
                                  // ======

namespace errors {
struct S {};

consteval { namespace_inject(^^S, ^^{ int x; }); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@-2 {{cannot inject tokens into a type, which is not a namespace}}
// expected-note@-3 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

consteval { namespace_inject(^^errors, ^^int); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@-2 {{expected a reflection of a token sequence, but got a type}}
// expected-note@-3 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

consteval { namespace_inject(^^{ int y = undeclared; }); }
// expected-error@-1 {{use of undeclared identifier 'undeclared'}}
// expected-note@-2 {{in token sequence injected here}}
// expected-error@-3 {{must be a constant expression}}
// expected-note@Inputs/token-injection.h:* {{injected tokens are ill-formed}}
// expected-note@Inputs/token-injection.h:* {{in call to}}
// expected-note@-6 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

constexpr int not_plain = (namespace_inject(^^{ int u; }), 0);
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@Inputs/token-injection.h:* {{cannot produce an injected declaration from a non-plainly constant-evaluated context}}
// expected-note@Inputs/token-injection.h:* {{in call to}}
// expected-note@-4 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
}  // namespace errors
