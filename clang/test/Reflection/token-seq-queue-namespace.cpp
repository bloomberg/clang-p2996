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
                               // global_scope
                               // ============

consteval { queue_injection(^^{ int global_x = 42; }); }
int &global_ref = global_x;

consteval {
  queue_injection(^^{ constexpr int first = 1; });
  queue_injection(^^{ constexpr int second = first + 1; });
}
static_assert(first == 1 && second == 2);

// Several declarations in a single sequence.
consteval {
  queue_injection(^^{
    struct Injected { int member; };
    constexpr Injected injected{7};
    constexpr int get() { return injected.member; }
  });
}
static_assert(get() == 7);

// Nothing is injected before the consteval block is complete.
constexpr int before = 1;
consteval {
  queue_injection(^^{ constexpr int after = before + 1; });
  static_assert(before == 1);
}
static_assert(after == 2);

                              // ===============
                              // namespace_scope
                              // ===============

namespace ns {
consteval { queue_injection(^^{ constexpr int in_ns = 1; }); }
static_assert(in_ns == 1);
static_assert(nearest_namespace() == ^^ns);
static_assert(nearest_token_queuing_context() == ^^ns);
static_assert(nearest_class_or_namespace() == ^^ns);

namespace inner {
// An explicit context naming an enclosing namespace: the tokens are injected
// when the parser gets back to that namespace.
consteval { queue_injection(^^ns, ^^{ constexpr int from_inner = 2; }); }
consteval { queue_injection(^^::, ^^{ constexpr int from_inner_global = 3; }); }
constexpr int in_inner = 4;
}  // namespace inner
static_assert(from_inner == 2);
static_assert(inner::in_inner == 4);

// Queued at the very end of the namespace.
consteval { queue_injection(^^{ constexpr int last_in_ns = 5; }); }
}  // namespace ns
static_assert(ns::in_ns == 1);
static_assert(ns::from_inner == 2);
static_assert(ns::last_in_ns == 5);
static_assert(from_inner_global == 3);
static_assert(nearest_namespace() == ^^::);

                             // =================
                             // interpolated_code
                             // =================

namespace interpolated_code {
consteval void make_powers(int count) {
  for (int i = 0; i < count; ++i)
    queue_injection(^^{ constexpr long \["pow2_", i] = \val(1L << i); });
}
consteval { make_powers(5); }
static_assert(pow2_0 == 1 && pow2_1 == 2 && pow2_4 == 16);
static_assert(__is_same(decltype(pow2_3), const long));

consteval info make_sum(sv name, int n) {
  list_builder params;
  list_builder terms(^^{ + });
  for (int i = 0; i < n; ++i) {
    params += ^^{ long \["a", i] };
    terms += ^^{ \["a", i] };
  }
  return ^^{ constexpr long \[name](\{params}) { return \{terms}; } };
}
consteval { queue_injection(make_sum("sum4", 4)); }
static_assert(sum4(1, 2, 3, 4) == 10);

consteval void make_enum(sv name, const char *const *values, int n) {
  list_builder items;
  for (int i = 0; i < n; ++i)
    items += ^^{ \[values[i]] };
  queue_injection(^^{ enum class \[name] { \{items} }; });
}
constexpr const char *fruits[] = {"apple", "banana", "cherry"};
consteval { make_enum("Fruit", fruits, 3); }
static_assert(int(Fruit::apple) == 0 && int(Fruit::cherry) == 2);

// Splices and string literals.
consteval info declare(info type, sv name, int init) {
  return ^^{ constexpr \[:type:] \[name] = \val(init); };
}
consteval {
  queue_injection(declare(^^int, "spliced_int", 42));
  queue_injection(declare(^^long, "spliced_long", 43));
  queue_injection(^^{ constexpr const char *text = \str(sv("some \"text\"\n")); });
}
static_assert(spliced_int == 42 && __is_same(decltype(spliced_int), const int));
static_assert(spliced_long == 43 && __is_same(decltype(spliced_long), const long));
static_assert(text[0] == 's' && text[5] == '"' && text[11] == '\n' && text[12] == 0);

// Injected code can itself inject code.
consteval {
  queue_injection(^^{
    consteval { queue_injection(^^{ constexpr int nested = 1; }); }
    constexpr int uses_nested = nested + 1;
  });
}
static_assert(nested == 1 && uses_nested == 2);

// A token sequence whose interpolators are evaluated later.
consteval {
  queue_injection(^^{
    consteval { queue_injection(^^{ constexpr int \["late_", 7] = \val(7); }); }
  });
}
static_assert(late_7 == 7);

// Templates.
consteval {
  queue_injection(^^{
    template <typename T> constexpr T twice(T t) { return t + t; }
    template <typename T> struct Box { T value; };
  });
}
static_assert(twice(21) == 42 && Box<int>{3}.value == 3);
}  // namespace interpolated_code

                                  // ======
                                  // errors
                                  // ======

namespace errors {
consteval { queue_injection(^^{ int x = undeclared; }); }
// expected-error@-1 {{use of undeclared identifier 'undeclared'}}
// expected-note@-2 {{in token sequence injected here}}

consteval { queue_injection(^^{ int z = 1 }); }
// expected-error@-1 {{expected ';' after top level declarator}}

consteval { queue_injection(^^int, ^^{ int w; }); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@-2 {{cannot inject tokens into a type: it is not a namespace, class or function whose body is currently being parsed}}
// expected-note@-3 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

consteval { queue_injection(^^int); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@Inputs/token-injection.h:* {{expected a reflection of a token sequence, but got a type}}
// expected-note@-3 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

namespace closed {}
consteval { queue_injection(^^closed, ^^{ int v; }); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@-2 {{cannot inject tokens into a namespace: it is not a namespace, class or function whose body is currently being parsed}}
// expected-note@-3 2 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

// Injection requires a plainly constant-evaluated context.
constexpr int not_plain = (queue_injection(^^{ int u; }), 0);
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@Inputs/token-injection.h:* {{cannot produce an injected declaration from a non-plainly constant-evaluated context}}
// expected-note@-3 {{in call to}}
// expected-note@Inputs/token-injection.h:* {{in call to}}
// expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
}  // namespace errors

// Queued at the very end of the translation unit.
consteval { queue_injection(^^{ static_assert(ns::in_ns == 1); int at_eof = 0; }); }
