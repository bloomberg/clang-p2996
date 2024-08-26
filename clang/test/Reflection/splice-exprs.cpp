//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax

using info = decltype(^^int);

                                 // ===========
                                 // idempotency
                                 // ===========

namespace idempotency {
int x;
void fn();
struct S {
  int x;
  static constexpr int s_x = 11;
  void fn();
  static void s_fn();
};
enum Enum { A, B, C };
enum class EnumCls { A, B, C };

static_assert(&[:^^x:] == &x);
static_assert([:^^fn:] == fn);

static_assert(&[:^^S::x:] == &S::x);
static_assert(&[:^^S::s_x:] == &S::s_x);
static_assert(&[:^^S::fn:] == &S::fn);
static_assert([:^^S::s_fn:] == S::s_fn);

static_assert([:^^Enum::B:] == Enum::B);
static_assert([:^^EnumCls::B:] == EnumCls::B);
}  // namespace idempotency

                               // ==============
                               // with_variables
                               // ==============

namespace with_variables {
consteval int fn() {
  int x = 32;

  constexpr auto rx = ^^x;
  ++[:rx:];

  return x;
}
static_assert(fn() == 33);
}  // namespace with_variables

                               // ==============
                               // with_functions
                               // ==============

namespace with_functions {
consteval int vanilla_fn() { return 42; }

constexpr info r_vanilla_fn = ^^vanilla_fn;
static_assert([:r_vanilla_fn:]() == 42);

// With a dependent reflection.
template <info R> consteval int fn() { return [:R:](); }
static_assert(fn<r_vanilla_fn>() == 42);
}  // namespace with_functions

                        // ============================
                        // with_shadowed_function_names
                        // ============================

namespace with_shadowed_function_names {
struct B { consteval char fn() const { return 'B'; } };
struct D : B { consteval char fn() const { return 'D'; } };

constexpr auto rBfn = ^^B::fn;
constexpr auto rDfn = ^^D::fn;

constexpr D d;
constexpr auto rd = ^^d;

static_assert([:rd:].[:rBfn:]() == 'B');
static_assert([:rd:].[:rDfn:]() == 'D');

}  // namespace with_shadowed_function_names

                             // ==================
                             // with_member_access
                             // ==================

// Check use of splices in member access expressions.
namespace with_member_access {
struct S {
  int j;
  int k;

  consteval int getJ() const { return j; }

  template <int N>
  consteval int getJPlusN() const { return j + N; }

  static consteval int eleven() { return 11; }

  template <int N>
  static consteval int constant() { return N; }
};

// Splicing dependent member references.
template <info RMem>
consteval int fn() {
  S s = {11, 13};
  return s.[:RMem:] + (&s)->[:RMem:];
}
static_assert(fn<^^S::j>() == 22);
static_assert(fn<^^S::k>() == 26);

// Splicing dependent member references with arrow syntax.
template <info RMem>
consteval int fn2() {
  S s = {11, 13};
  return s.*(&[:RMem:]) + (&s)->*(&[:RMem:]);
}
static_assert(fn<^^S::j>() == 22);
static_assert(fn<^^S::k>() == 26);

// Splicing member functions.
constexpr info r_getJ = ^^S::getJ;
static_assert(S{2, 4}.[:r_getJ:]() == 2);

// Splicing static member functions.
constexpr auto rEleven = ^^S::eleven;
static_assert([:rEleven:]() == 11);

// Splicing static member template function instantiation.
constexpr auto rConst14 = ^^S::constant<14>;
static_assert([:rConst14:]() == 14);

// Splicing member function template instanstiations.
constexpr auto rgetJPlus5 = ^^S::getJPlusN<5>;
static_assert(S{2, 4}.[:rgetJPlus5:]() == 7);

// Splicing member function template instantiations with spliced objects.
constexpr S instance {1, 4};
constexpr info rInstance = ^^instance;
static_assert([:rInstance:].[:rgetJPlus5:]() == 6);
static_assert((&[:rInstance:])->[:rgetJPlus5:]() == 6);

// Splicing dependent object in a member access expression.
template <info RObj>
consteval int fn3() {
  return [:RObj:].k;
}
static_assert(fn3<^^instance>() == 4);

// Passing address of a spliced operand as an argument.
consteval int getMem(const S *s, int S::* mem) {
  return s->*mem;
}
constexpr info rJ = ^^S::j;
static_assert(getMem(&instance, &[:rJ:]) == 1);

// Member access through a splice of a private member.
class WithPrivateBase : S {} d;
int dK = d.[:^^S::k:];

}  // namespace with_member_access

                         // ===========================
                         // with_implicit_member_access
                         // ===========================

namespace with_implicit_member_access {

// Non-dependent case
struct S {
  static constexpr int l = 3;

  int k;

  void fn2() { }

  void fn() {
    static_assert([:^^l:] == 3);
    static_assert([:^^S:]::l == 3);
    (void) this->[:^^k:];
    (void) this->[:^^S:]::k;
    this->[:^^fn2:]();
    this->[:^^S:]::fn2();
  }
};

// Dependent case
struct D {
  static constexpr int l = 3;

  int k;

  void fn2() { }

  template <typename T>
  void fn() {
    static_assert([:^^T:]::l == 3);
    (void) this->[:^^T:]::l;
    (void) this->[:^^T:]::fn2();
  }
};

}  // namespace with_implicit_member_access

                           // ======================
                           // with_overridden_memfns
                           // ======================

namespace with_overridden_memfns {
struct B { consteval virtual int fn() const { return 1; } };
struct D : B { consteval int fn() const override { return 2; } };

constexpr D d;
static_assert(d.[:^^D::fn:]() == 2);
static_assert(d.[:^^B::fn:]() == 2);
static_assert(d.[:^^B:]::fn() == 1);

// Splicing member as intermediate component of a member-access expression.
struct T { struct Inner { int v; } inner; };
constexpr auto r_inner = ^^T::inner;
constexpr T t = {{4}};
static_assert(t.[:r_inner:].v == 4);
}  // namespace with_overridden_memfns

                                 // ==========
                                 // with_enums
                                 // ==========

namespace with_enums {
enum Enum { A, B, C };
enum class EnumCls { A, B, C };

constexpr info rB = ^^B, rClsB = ^^EnumCls::B;
static_assert(rB != rClsB);
static_assert(int([:rB:]) == int([:rClsB:]));
static_assert(static_cast<Enum>([:rClsB:]) == B);
}  // namespace with_enums

                                // =============
                                // colon_parsing
                                // =============

// Check that parsing correctly handles successions of ':'-characters.
namespace colon_parsing {
constexpr int x = 4;
constexpr auto rx = ^^x;
static_assert([:rx:] == 4);

constexpr unsigned Idx = 1;
constexpr int arr[] = {1, 2, 3};
static_assert(arr[::colon_parsing::Idx] == 2);

constexpr info rIdx = ^^Idx;
static_assert([:::colon_parsing::rIdx:] == 1);

struct WithIndexOperator {
  bool operator[:>(int);  // Test interaction with ':>'-digraph (i.e., ']').
};
}  // namespace colon_parsing

                   // =======================================
                   // bb_clang_p2996_issue_22_regression_test
                   // =======================================

namespace bb_clang_p2996_issue_22_regression_test {
// Issue #22 invoked a crash involving CTAD in a double templated context.
// I wasn't able to find a more minimal reproduction of the crash, but am
// including this test to prevent regression.
template <decltype(^^::) FN>
struct Cls
{
    template <typename RESULT, typename... Args>
    struct Impl {
        Impl(decltype(&[:FN:]));
    };
    template <typename RESULT, typename... Args>
    Impl(RESULT (*)(Args...)) -> Impl<RESULT, Args...>;
};

void fn(int);
static_assert(^^decltype(Cls<^^fn>::Impl(&fn)) == ^^Cls<^^fn>::Impl<void, int>);
}  // namespace bb_clang_p2996_issue_22_regression_test
