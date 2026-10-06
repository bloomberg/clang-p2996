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

                                // ==========
                                // properties
                                // ==========

namespace properties {
consteval void property(info type, sv name) {
  queue_injection(^^{
    private:
      \[:type:] \["m_", name]{};
    public:
      constexpr \[:type:] \["get_", name]() const { return \["m_", name]; }
      constexpr void \["set_", name](\[:type:] value) { \["m_", name] = value; }
  });
}

class Person {
  consteval {
    property(^^long, "id");
    property(^^int, "age");
  }
  // The access specifier in effect before the injection is restored.
  int still_private;
};

constexpr int test() {
  Person p;
  p.set_id(7);
  p.set_age(85);
  return p.get_id() + p.get_age();
}
static_assert(test() == 92);
static_assert(__is_same(decltype(Person().get_id()), long));

void access(Person p) {
  (void)p.m_age;          // expected-error {{'m_age' is a private member of 'properties::Person'}} \
                          // expected-note@* {{declared private here}}
  (void)p.still_private;  // expected-error {{'still_private' is a private member of 'properties::Person'}} \
                          // expected-note@* {{declared private here}}
}

struct Public {
  consteval { property(^^int, "x"); }
  int still_public;
};
constexpr int use_public = Public().still_public;
}  // namespace properties

                               // =============
                               // class_context
                               // =============

namespace class_context {
struct S {
  static_assert(nearest_token_queuing_context() == ^^S);
  static_assert(nearest_class_or_namespace() == ^^S);
  static_assert(nearest_namespace() == ^^class_context);

  consteval {
    queue_injection(^^{
      int a = 1;
      static constexpr int b = 2;
      constexpr int sum() const { return a + b + c; }  // 'c' is declared below.
      struct Nested { int n = 3; };
      template <typename T> constexpr T as() const { return T(a); }
      using type = long;
      enum { enumerator = 4 };
    });
  }
  int c = 10;
};
static_assert(S().sum() == 13);
static_assert(S::b == 2 && S::Nested().n == 3 && S().as<long>() == 1);
static_assert(__is_same(S::type, long) && S::enumerator == 4);

// Data members are laid out where they are injected.
struct Layout {
  char first;
  consteval { queue_injection(^^{ char second; char third; }); }
  char fourth;
};
static_assert(__builtin_offsetof(Layout, second) == 1);
static_assert(__builtin_offsetof(Layout, third) == 2);
static_assert(__builtin_offsetof(Layout, fourth) == 3);

// Injection into an enclosing class, and into the enclosing namespace.
struct Outer {
  struct Inner {
    consteval {
      queue_injection(^^Outer, ^^{ static constexpr int from_inner = 1; });
      queue_injection(^^class_context, ^^{ constexpr int from_class = 2; });
      queue_injection(^^{ static constexpr int own = 3; });
    }
  };
  static_assert(from_inner == 1);
};
static_assert(Outer::from_inner == 1 && from_class == 2 && Outer::Inner::own == 3);

// Queued right before the closing brace.
struct AtEnd {
  int a;
  consteval { queue_injection(^^{ int b; }); }
};
static_assert(sizeof(AtEnd) == 2 * sizeof(int));

// Constructors and special members.
struct Special {
  consteval {
    queue_injection(^^{
      constexpr Special(int v) : value(v) {}
      constexpr Special() : Special(5) {}
      int value;
      constexpr bool operator==(const Special &) const = default;
    });
  }
};
static_assert(Special().value == 5 && Special(3) == Special(3));

// Unions and local classes.
union U {
  consteval { queue_injection(^^{ int i; float f; }); }
};
static_assert(sizeof(U) == sizeof(int));

constexpr int local_class() {
  struct Local {
    consteval { queue_injection(^^{ int v = 9; }); }
  };
  return Local().v;
}
static_assert(local_class() == 9);
}  // namespace class_context

                                  // ======
                                  // errors
                                  // ======

namespace errors {
struct Complete {};

struct S {
  consteval { queue_injection(^^Complete, ^^{ int x; }); }
  // expected-error@-1 {{must be a constant expression}}
  // expected-note@-2 {{cannot inject tokens into a type: it is not a namespace, class or function whose body is currently being parsed}}
  // expected-note@-3 2 {{in call to}}
  // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}

  consteval { queue_injection(^^{ undeclared z; }); }
  // expected-error@-1 {{unknown type name 'undeclared'}}
  // expected-note@-2 {{in token sequence injected here}}

  // The class is complete in the bodies of its member functions.
  void f() {
    consteval { queue_injection(^^S, ^^{ int w; }); }
    // expected-error@-1 {{must be a constant expression}}
    // expected-note@-2 {{cannot inject tokens into a type}}
    // expected-note@-3 2 {{in call to}}
    // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
  }
};
}  // namespace errors
