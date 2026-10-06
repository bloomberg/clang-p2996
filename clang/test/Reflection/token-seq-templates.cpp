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

                             // ==================
                             // class_instantiation
                             // ==================

namespace class_instantiation {
// Members are injected into each specialization when it is instantiated.
template <typename T, int N>
struct Tuple {
  consteval {
    for (int i = 0; i < N; ++i)
      queue_injection(^^{ \[:^^T:] \["m", i] = \val(i * 10); });
  }
  int own = -1;
};

static_assert(Tuple<int, 3>().m0 == 0 && Tuple<int, 3>().m2 == 20);
static_assert(Tuple<long, 1>().m0 == 0);
static_assert(__is_same(decltype(Tuple<long, 1>::m0), long));
static_assert(sizeof(Tuple<int, 3>) == 4 * sizeof(int));
static_assert(sizeof(Tuple<int, 0>) == sizeof(int));
static_assert(Tuple<int, 2>{1, 2, 3}.own == 3);

// Member functions, whose bodies see the complete class.
template <typename T>
struct Wrapper {
  consteval {
    queue_injection(^^{
      constexpr \[:^^T:] get() const { return value + later; }
      constexpr void set(\[:^^T:] v) { value = v; }
      \[:^^T:] with_init = sizeof(Wrapper);
      static constexpr int constant = 5;
      struct Nested { int n = 6; };
    });
  }
  T value = 1;
  T later = 100;
};

constexpr int use() {
  Wrapper<int> w;
  w.set(5);
  return w.get();
}
static_assert(use() == 105);
static_assert(Wrapper<long>::constant == 5 && Wrapper<long>::Nested().n == 6);
static_assert(Wrapper<long>().with_init == sizeof(Wrapper<long>));

// The access specifier in effect at the consteval block applies.
template <typename T>
class Access {
  consteval { queue_injection(^^{ T *never_named; int hidden; }); }
  // expected-error@-1 {{unknown type name 'T'}}
  // expected-note@-2 {{in token sequence injected here}}
  // expected-error@-3 {{must be a constant expression}}
  // expected-note@Inputs/token-injection.h:* {{injected tokens are ill-formed}}
  // expected-note@Inputs/token-injection.h:* {{in call to}}
  // expected-note@-6 2 {{in call to}}
  // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
public:
  consteval { queue_injection(^^{ int shown; }); }
};
int access(Access<int> a) {  // expected-note 2 {{in instantiation of template class}}
  return a.shown;
}

template <typename T>
class Private {
  consteval { queue_injection(^^{ int hidden; }); }
public:
  consteval { queue_injection(^^{ int shown; }); }
};
int access(Private<int> p) {
  (void)p.hidden;  // expected-error {{'hidden' is a private member}} \
                   // expected-note@* {{declared private here}}
  return p.shown;
}

// Member classes of class templates.
template <typename T>
struct Outer {
  struct Inner {
    consteval { queue_injection(^^{ static constexpr int from_inner = sizeof(typename \[:^^T:]); }); }
  };
};
static_assert(Outer<char>::Inner::from_inner == 1);
static_assert(Outer<int>::Inner::from_inner == sizeof(int));

// Explicit specializations are parsed, not instantiated.
template <typename T> struct Spec;
template <> struct Spec<int> {
  consteval { queue_injection(^^{ static constexpr int v = 1; }); }
};
static_assert(Spec<int>::v == 1);
}  // namespace class_instantiation

                           // =======================
                           // namespace_from_templates
                           // =======================

namespace namespace_from_templates {
// Namespace-scope declarations requested by the instantiation of a class.
template <int N>
struct Registrar {
  consteval {
    queue_injection(^^namespace_from_templates,
                    ^^{ constexpr int \["registered_", N] = \val(N); });
  }
};
Registrar<1> r1;
Registrar<2> r2;
static_assert(registered_1 == 1 && registered_2 == 2);

template <int N>
struct Immediate {
  consteval {
    namespace_inject(^^namespace_from_templates,
                     ^^{ constexpr int \["immediate_", N] = \val(N); });
  }
  static constexpr int value = N;
};
static_assert(Immediate<7>::value == 7 && immediate_7 == 7);
}  // namespace namespace_from_templates

namespace closed_target {}
namespace user {
template <int N>
struct IntoClosed {
  consteval {
    queue_injection(^^closed_target, ^^{ constexpr int \["c", N] = \val(N); });
  }
};
IntoClosed<3> ic;
}  // namespace user
static_assert(closed_target::c3 == 3);

                           // ======================
                           // function_instantiation
                           // ======================

namespace function_instantiation {
template <typename T>
constexpr T f(T t) {
  consteval { queue_injection(^^{ return t; }); }
  // expected-error@-1 {{must be a constant expression}}
  // expected-note@Inputs/token-injection.h:* {{cannot inject tokens into a block scope during template instantiation}}
  // expected-note@Inputs/token-injection.h:* {{in call to}}
  // expected-note@-4 2 {{in call to}}
  // expected-note@Inputs/token-injection.h:* {{subexpression not valid in a constant expression}}
  return t;
}
constexpr int x = f(1);  // expected-note {{in instantiation of function template specialization}}

// Token sequences computed by templates are fine.
template <typename T, int N>
consteval info member(const char *name) {
  return ^^{ \[:^^T:] \[name][\val(N)]; };
}
struct S {
  consteval {
    queue_injection(member<int, 2>("ints"));
    queue_injection(member<char, 3>("chars"));
  }
};
static_assert(sizeof(S::ints) == 2 * sizeof(int) && sizeof(S::chars) == 3);
}  // namespace function_instantiation
