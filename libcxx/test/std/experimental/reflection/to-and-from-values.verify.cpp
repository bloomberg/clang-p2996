//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]

#include <meta>


                             // ==================
                             // disallowed_results
                             // ==================

namespace disallowed_results {
constexpr auto v1 = std::meta::reflect_constant((const char *)"fails");
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided value cannot be represented}}

struct HoldsTemporary {
  const int &tmp;
};
constexpr HoldsTemporary htmp{42};
constexpr auto v2 = std::meta::reflect_constant(htmp);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided value cannot be represented}}

}  // namespace disallowed_results

                              // ================
                              // self_referential
                              // ================

namespace self_referential {
struct A {
  int *const p;
  consteval A(int *p) : p(p) {}
  consteval A(const A &oth) : p(0) {
    if (oth.p) {
      ++*oth.p;
    }
  }
};

consteval int f() {
  int x = 42;
  std::meta::reflect_constant<A>(&x);
  return x;
}

static_assert(f() == 43);
  // expected-error@-1 {{not an integral constant expression}}
}  // namespace self_referential

                           // ======================
                           // object_of_non_static
                           // ======================

// [meta.reflection.queries]/5: object_of throws unless r represents an object
// with static storage duration, or a variable that declares or refers to such
// an object.
namespace object_of_non_static {
thread_local int tl;
constexpr auto r1 = std::meta::object_of(^^tl);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the object of a variable that has thread storage duration}}

consteval std::meta::info automatic() {
  [[maybe_unused]] int loc = 0;
  return std::meta::object_of(^^loc);
    // expected-note@-1 {{cannot query the object of a variable that has automatic storage duration}}
}
constexpr auto r2 = automatic();
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{in call to 'automatic()'}}

consteval std::meta::info reference_to_automatic() {
  int loc = 0;
  [[maybe_unused]] int &ref = loc;
  return std::meta::object_of(^^ref);
    // expected-note@-1 {{cannot query the object of a variable that does not refer to an object with static storage duration}}
}
constexpr auto r3 = reference_to_automatic();
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{in call to 'reference_to_automatic()'}}

struct S { int m; };
constexpr auto r4 = std::meta::object_of(^^S::m);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the object of a non-static data member}}
constexpr auto r5 = std::meta::object_of(std::meta::reflect_constant(1));
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the object of a value}}
}  // namespace object_of_non_static

                          // =======================
                          // constant_of_non_constant
                          // =======================

// [meta.reflection.queries]/9: constant_of throws unless [:R:] is a valid
// splice-expression; a non-static member function cannot be spliced as an
// expression ([expr.prim.splice]).
namespace constant_of_non_constant {
struct S { void mfn(); };
constexpr auto r1 = std::meta::constant_of(^^S::mfn);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the value of a function}}

constexpr int md[2][2] = {{1, 2}, {3, 4}};
constexpr auto r2 = std::meta::constant_of(^^md);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the value of a multidimensional array}}
}  // namespace constant_of_non_constant

                          // ======================
                          // extract_disallowed
                          // ======================

namespace extract_disallowed {
struct S { int m; const int cm; void xo(this S &); void mf(); };
void fn(int);
int nonconst_array[2];
constexpr int carr[2] = {1, 2};
constexpr int const_global = 1;
constexpr const int *cip = &carr[0];

// [meta.reflection.extract]/5.2: a reference may only add qualifiers.
constexpr auto &r1 = std::meta::extract<int &>(^^const_global);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{reflected object of type 'const int' cannot be extracted as a reference to type 'int'}}
constexpr auto &r2 = std::meta::extract<int (&)[2]>(^^carr);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{reflected object of type 'const int[2]' cannot be extracted as a reference to type 'int[2]'}}

// /12, /10.3: an array is extracted through constant_of, which requires a
// variable usable in constant expressions, and as a pointer that may only add
// qualifiers.
constexpr auto r3 = std::meta::extract<int *>(^^nonconst_array);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{cannot query the value of a variable not usable in constant expressions}}
constexpr auto r4 = std::meta::extract<int *>(^^carr);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{reflected object of type 'const int *' cannot be extracted as a reference to type 'int *'}}

// /7.1, /7.2, /7.3: pointers to members and functions.
constexpr auto r5 = std::meta::extract<int S::*>(^^S::cm);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{a pointer of type 'int S::*' cannot be extracted from a reflection of a non-static data member whose pointer has type 'const int S::*'}}
constexpr auto r6 = std::meta::extract<void (*)(int) noexcept>(^^fn);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{reflected value of type 'void (int)' cannot be extracted as a value of type 'void (*)(int) noexcept'}}
constexpr auto r7 = std::meta::extract<void (S::*)()>(^^S::xo);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{a pointer of type 'void (S::*)()' cannot be extracted from a reflection of a function whose pointer has type 'void (*)(S &)'}}
constexpr auto r8 = std::meta::extract<void (S::*)() noexcept>(^^S::mf);
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{a pointer of type 'void (S::*)() noexcept' cannot be extracted from a reflection of a function whose pointer has type 'void (S::*)()'}}

// /10.1: a pointer value may only add qualifiers.
constexpr auto r9 = std::meta::extract<int *>(std::meta::reflect_constant(cip));
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{reflected value of type 'const int *' cannot be extracted as a value of type 'int *'}}
}  // namespace extract_disallowed

int main() { }
