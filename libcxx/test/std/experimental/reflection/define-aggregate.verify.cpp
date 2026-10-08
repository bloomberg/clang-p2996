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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration
// ADDITIONAL_COMPILE_FLAGS: -Wno-unused-private-field

// <experimental/reflection>
//
// [reflection]

#include <meta>

#include <print>


constexpr auto ctx = std::meta::access_context::unchecked();

                                 // ===========
                                 // well_formed
                                 // ===========

namespace well_formed {
struct I1;
consteval { define_aggregate(^^I1, {}); }

struct S {
  struct I2;
  consteval { define_aggregate(^^I2, {}); }
};

template <typename>
struct TCls {
  struct I3;
  consteval { define_aggregate(^^I3, {}); }
};
TCls<int>::I3 i1;

consteval auto fn1(std::meta::info r) {
  return define_aggregate(r, {});
}
struct I4;
consteval { fn1(^^I4); }
I4 i2;

template <typename>
consteval auto fn2(std::meta::info r) {
  return define_aggregate(r, {});
}
struct I5;
consteval { fn2<int>(^^I5); }
I5 i3;

consteval void fn3() {
  struct S;
  consteval { define_aggregate(^^S, {}); }
}

template <typename>
consteval auto fn4() {
  struct S;
  consteval { define_aggregate(^^S, {}); }
  return ^^S;
}
constexpr auto i4 = fn4<int>();

}  // namespace well_formed


                       // ==============================
                       // non_plainly_constant_evaluated
                       // ==============================

namespace non_plainly_constant_evaluated {
struct I1;
constexpr auto u1 = define_aggregate(^^I1, {});
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{non-plainly constant-evaluated context}}

template <typename>
struct S1 {
  struct I1;

  void mfn1() requires((define_aggregate(^^I1, {}), false)) {}
  // expected-error@-1 {{non-constant expression}}
  // expected-note@-2 {{non-plainly constant-evaluated context}}

  void mfn1() requires(true) {}

  struct I2;
  void mfn2() noexcept((define_aggregate(^^I2, {}), false)) {}
  // expected-error@-1 {{not a constant expression}}
  // expected-note@-2 {{non-plainly constant-evaluated}}
};

void trigger_instantiations() {
  S1<int> s;
  s.mfn1();
  s.mfn2();
}

template <typename Ty>
constexpr auto Completion = define_aggregate(^^Ty, {});
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{non-plainly constant-evaluated}}

struct I4;
constexpr auto D = Completion<I4>;
// expected-error@-1 {{must be initialized by a constant expression}}

void fn() {
  struct I5;
  [[maybe_unused]] static constexpr auto r = define_aggregate(^^I5, {});
    // expected-error@-1 {{must be initialized by a constant expression}}
    // expected-note@-2 {{non-plainly constant-evaluated}}
}

}  // namespace non_plainly_constant_evaluated


                            // =====================
                            // scope_rule_violations
                            // =====================

namespace scope_rule_violations {
struct I1;
void fn1() {
  consteval { define_aggregate(^^I1, {}); }
  // expected-error@-1 {{function 'fn1' does not enclose both declarations}}
}

consteval auto fn2() {
  struct I;
  return ^^I;
}
consteval { define_aggregate(fn2(), {}); }
// expected-error@-1 {{function 'fn2' does not enclose both declarations}}

struct I2;
consteval {
  [] consteval {
    consteval { define_aggregate(^^I2, {}); }
  }();
}
// expected-error-re@-3 {{function '{{.*}}' does not enclose both declarations}}

struct I3;
template <typename>
struct TCls1 {
  consteval { define_aggregate(^^I3, {}); }
  // expected-error@-1 {{class 'TCls1<int>' does not enclose both}}

  struct I;
};
consteval { define_aggregate(^^TCls1<int>::I, {}); }
// expected-error@-1 {{class 'TCls1<int>' does not enclose both}}

template <std::meta::info R>
consteval auto tfn() {
  struct I;
  if constexpr (R != std::meta::info{}) {
    consteval { define_aggregate(R, {}); }
    // expected-error-re@-1 {{function 'tfn<{{.*}}>' does not enclose both}}
  }
  return ^^I;
}
consteval { tfn<tfn<std::meta::info{}>()>(); }

struct I4;
struct S1 {
  consteval { define_aggregate(^^I4, {}); }
  // expected-error@-1 {{class 'S1' does not enclose both}}
};
}  // namespace scope_rule_violations

                      // =================================
                      // speculative_and_trial_evaluations
                      // =================================

namespace speculative_and_trial_evaluations {
struct S1;
consteval auto fn() {
  return members_of(define_aggregate(^^S1, {}), ctx).empty() ? new int :
                                                               nullptr;
};

consteval void fn2() {
  delete fn();
};

consteval { fn2(); }
[[maybe_unused]] S1 s1;


consteval int fn3(int p) { return p; }

struct S2;
consteval {
  (void) [](int p) {
    [[maybe_unused]] const auto v = (define_aggregate(^^S2, {}), fn3(p));
    return p;
  }(5);
}

[[maybe_unused]] S2 s2;

}  // namespace speculative_and_trial_evaluations

                                // ============
                                // repeat_calls
                                // ============

namespace repeat_calls {
struct S1;
consteval { // expected-error {{consteval block must be a constant expression}}
  define_aggregate(^^S1, {});
  define_aggregate(^^S1, {});  // expected-note {{already a complete type}}
}

struct S2;
consteval { // expected-error {{consteval block must be a constant expression}}
  define_aggregate(^^S2, {
    data_member_spec(^^int, {.name="member1"}),
    data_member_spec(^^bool, {.name="member2"}),
  });
  define_aggregate(^^S2, {  // expected-note {{already a complete type}}
    data_member_spec(^^int, {.name="member1"}),
    data_member_spec(^^bool, {.name="member2"}),
  });
}

}  // namespace repeat_calls

                            // ====================
                            // invalid_descriptions
                            // ====================

// [meta.reflection.define.aggregate]/5: data_member_spec throws for a type
// that is not an object or reference type, a name that is not an identifier,
// a description with neither name nor bit width, annotations on an unnamed
// bit-field, and a cv-qualified unnamed bit-field.
namespace invalid_descriptions {
constexpr auto d1 = data_member_spec(^^void(), {.name="f"});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{the type of a data member must be an object or reference type}}
constexpr auto d2 = data_member_spec(^^void, {.name="v"});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{the type of a data member must be an object or reference type}}
constexpr auto d3 = data_member_spec(^^int, {.name="int"});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided name 'int' is not a valid identifier}}
constexpr auto d4 = data_member_spec(^^int, {.name="1x"});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided name '1x' is not a valid identifier}}
constexpr auto d5 = data_member_spec(^^int, {.name=""});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{provided name '' is not a valid identifier}}
constexpr auto d6 = data_member_spec(^^int, {});
  // expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto d7 = data_member_spec(^^int, {.bit_width=3,
                                             .annotations={std::meta::reflect_constant(1)}});
  // expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto d8 = data_member_spec(^^const int, {.bit_width=3});
  // expected-error@-1 {{must be initialized by a constant expression}} \
  // expected-note@-1 {{an unnamed bit-field cannot have a cv-qualified type}}
constexpr auto d9 = data_member_spec(^^int, {.name="a", .annotations={^^int}});
  // expected-error@-1 {{must be initialized by a constant expression}}
int arr[2];
constexpr auto d10 = data_member_spec(^^int, {.name="a",
                                              .annotations={std::meta::reflect_object(arr)}});
  // expected-error@-2 {{must be initialized by a constant expression}}

// A reference type and an incomplete object type are allowed in a
// description (/5.1); define_aggregate rejects the latter (/8.4).
struct Incomplete;
constexpr auto ok1 = data_member_spec(^^int &, {.name="r"});
constexpr auto ok2 = data_member_spec(^^Incomplete, {.name="i"});
constexpr auto ok3 = data_member_spec(^^int[], {.name="a"});
}  // namespace invalid_descriptions

                            // ====================
                            // invalid_definitions
                            // ====================

// [meta.reflection.define.aggregate]/8.1, /8.4, /8.5: the class type must be
// cv-unqualified, every member type complete, and names other than "_" unique.
namespace invalid_definitions {
struct S1; struct S2; struct S3; struct S4;
consteval { define_aggregate(^^const S1, {}); }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{cannot define 'const invalid_definitions::S1'; the class type must be cv-unqualified}}
consteval { define_aggregate(^^S2, {data_member_spec(^^int[], {.name="a"})}); }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{cannot define a class with a data member of incomplete type 'int[]'}}
struct Incomplete;
consteval { define_aggregate(^^S3, {data_member_spec(^^Incomplete, {.name="a"})}); }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{cannot define a class with a data member of incomplete type 'invalid_definitions::Incomplete'}}
constexpr auto dup = data_member_spec(^^int, {.name="a"});
consteval { define_aggregate(^^S4, {dup, dup}); }
  // expected-error@-1 {{must be a constant expression}} \
  // expected-note@-1 {{cannot produce a class definition with multiple members named 'a'}}
}  // namespace invalid_definitions


int main() { }
