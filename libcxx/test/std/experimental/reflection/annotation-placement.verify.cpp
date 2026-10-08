//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fannotation-attributes

// <experimental/reflection>
//
// [dcl.attr.annotation]/1: an annotation may not be applied to a declaration
// whose host scope differs from its target scope, nor to a non-defining
// friend declaration.

#include <meta>

namespace N {
void f();
extern int v;
struct S { void m(); static int s; };
[[=1]] void g();  // OK
}

void N::f [[=1]] () {}
  // expected-error@-1 {{annotation applied to a declaration of 'f' outside its scope}}
[[=2]] int N::v = 0;
  // expected-error@-1 {{annotation applied to a declaration of 'v' outside its scope}}
void N::S::m [[=3]] () {}
  // expected-error@-1 {{annotation applied to a declaration of 'm' outside its scope}}
[[=4]] int N::S::s = 0;
  // expected-error@-1 {{annotation applied to a declaration of 's' outside its scope}}

namespace N {
void N::g [[=5]] ();  // OK: the host scope is the target scope
  // expected-warning@-1 {{extra qualification on member 'g'}}
}

struct Friends {
  friend void ff [[=1]] ();
    // expected-error@-1 {{annotation applied to the non-defining friend declaration of 'ff'}}
  friend void fh [[=3]] () {}  // OK: a definition
  template <typename T> friend void ft [[=4]] (T);
    // expected-error@-1 {{annotation applied to the non-defining friend declaration of 'ft'}}
};
void fh();
static_assert(std::meta::annotations_of(^^fh).size() == 1);
static_assert(std::meta::annotations_of(^^N::g).size() == 2);
