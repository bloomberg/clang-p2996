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

                                // ===========
                                // block_scope
                                // ===========

namespace block_scope {
constexpr int f() {
  static_assert(nearest_token_queuing_context() == ^^f);
  static_assert(nearest_class_or_namespace() == ^^block_scope);
  static_assert(nearest_namespace() == ^^block_scope);

  consteval { queue_injection(^^{ int x = 40; }); }
  consteval { queue_injection(^^{ x += 1; ++x; }); }
  return x;
}
static_assert(f() == 42);

// Statements of all kinds.
constexpr int g(int n) {
  int result = 0;
  consteval {
    queue_injection(^^{
      for (int i = 0; i < n; ++i)
        result += i;
      if (result > 100) { return -1; }
      struct Local { int v = 1000; };
      result += Local().v;
    });
  }
  return result;
}
static_assert(g(4) == 1006);
static_assert(g(100) == -1);

// Nested blocks: the tokens go to the innermost block.
constexpr int nested() {
  int x = 1;
  {
    consteval { queue_injection(^^{ int x = 2; }); }
    if (x != 2)
      return -1;
  }
  return x;
}
static_assert(nested() == 1);

// Lambdas and member functions.
constexpr auto lambda = [] {
  consteval { queue_injection(^^{ return 7; }); }
};
static_assert(lambda() == 7);

struct S {
  constexpr int m() const {
    consteval { queue_injection(^^{ return v * 2; }); }
  }
  int v = 21;
};
static_assert(S().m() == 42);

// Generated statements using interpolators.
consteval void assign_all(const char *const *names, int n, int value) {
  for (int i = 0; i < n; ++i)
    queue_injection(^^{ \[names[i]] = \val(value + i); });
}
constexpr int assigned() {
  int a = 0, b = 0, c = 0;
  static constexpr const char *names[] = {"a", "b", "c"};
  consteval { assign_all(names, 3, 10); }
  return a * 10000 + b * 100 + c;
}
static_assert(assigned() == 101112);

// At the very end of the block.
constexpr int at_end() {
  consteval { queue_injection(^^{ return 3; }); }
}
static_assert(at_end() == 3);

// A namespace-scope declaration requested from a block.
void from_block() {
  consteval { queue_injection(^^block_scope, ^^{ constexpr int from_function = 5; }); }
}
static_assert(from_function == 5);
}  // namespace block_scope

                                  // ======
                                  // errors
                                  // ======

namespace errors {
void other();

void f() {
  consteval { queue_injection(^^{ undeclared(); }); }
  // expected-error@-1 {{use of undeclared identifier 'undeclared'}}
  // expected-note@-2 {{in token sequence injected here}}

  consteval { queue_injection(^^other, ^^{ int x; }); }
  // expected-error@-1 {{must be a constant expression}}
  // expected-note@-2 {{cannot inject tokens into a function: it is not a namespace, class or function whose body is currently being parsed}}
  // expected-note@-3 2 {{in call to}}
  // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
}
}  // namespace errors
