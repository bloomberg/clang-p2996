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
//
// RUN: %{exec} %t.exe > %t.stdout

#include <meta>

#include <print>
#include <ranges>
#include <tuple>


using access_context = std::meta::access_context;

                                // =============
                                // class_members
                                // =============

namespace class_members {
struct Empty {};
static_assert(members_of(^^Empty, access_context::unchecked()).size() == 6);
static_assert(
    (members_of(^^Empty, access_context::unchecked()) |
        std::views::filter(std::meta::is_defaulted) |
        std::views::filter(std::meta::is_special_member_function) |
        std::ranges::to<std::vector>()).size() == 6);

struct NotAMember {};
struct Cls : NotAMember {
  int mem1;
  float mem2;

  Cls() = default;
  Cls(const Cls&) = default;
  constexpr ~Cls() {}
  Cls &operator=(const Cls&) = default;

  int memfn1() const { return mem1; }
  float memfn2() const { return mem2; }

  static int smem;
  static void sfn();

  template <typename T> void TMemFn();

  struct Inner { int c; };

  using Alias = int;
};

static_assert(members_of(^^Cls, access_context::unchecked()).size() == 13);
static_assert(nonstatic_data_members_of(^^Cls, access_context::unchecked()) ==
              std::vector{^^Cls::mem1, ^^Cls::mem2});
static_assert(static_data_members_of(^^Cls, access_context::unchecked()) ==
              std::vector{^^Cls::smem});
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
         std::views::filter(std::meta::is_constructor) |
         std::ranges::to<std::vector>()).size() == 2);
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
         std::views::filter(std::meta::is_destructor) |
         std::ranges::to<std::vector>()).size() == 1);
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
     std::views::filter(std::meta::is_type) |
     std::ranges::to<std::vector>()) ==
    std::vector{^^Cls::Inner, ^^Cls::Alias});
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
     std::views::filter(std::meta::is_template) |
     std::ranges::to<std::vector>()) ==
    std::vector{^^Cls::TMemFn});
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
     std::views::filter(std::meta::is_function) |
     std::views::filter(std::meta::is_static_member) |
     std::ranges::to<std::vector>()) ==
    std::vector{^^Cls::sfn});
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
     std::views::filter(std::meta::is_function) |
     std::views::filter([](auto R) {
       return !is_static_member(R) &&
              !is_special_member_function(R);
     }) |
     std::ranges::to<std::vector>()) ==
    std::vector{^^Cls::memfn1, ^^Cls::memfn2});
static_assert(
    (members_of(^^Cls, access_context::unchecked()) |
     std::views::filter(std::meta::is_type_alias) |
     std::ranges::to<std::vector>()) ==
    std::vector{^^Cls::Alias});

template <typename T>
struct TCls {
  T mem;
};
static_assert(
    type_of(nonstatic_data_members_of(^^TCls<int>,
                                      access_context::unchecked())[0]) ==
    ^^int);

void usage_example() {
  constexpr auto getObj = []() consteval {
    Cls a {};
    constexpr auto first =
        nonstatic_data_members_of(^^Cls, access_context::unchecked())[0];
    constexpr auto second =
        nonstatic_data_members_of(^^Cls, access_context::unchecked())[1];
    a.[:first:] = 20;
    a.[:second:] = 1.4f;
    return a;
  };

  constinit static auto obj = getObj();

  // RUN: grep "obj=<20, 1.4>" %t.stdout
  std::println(R"(obj=<{}, {}>)", obj.memfn1(), obj.memfn2());

  static_assert((members_of(^^Cls, access_context::unchecked()) |
                     std::views::filter(std::meta::is_type) |
                     std::ranges::to<std::vector>()).size() == 2);

  static_assert((members_of(^^Cls, access_context::unchecked()) |
                     std::views::filter(std::meta::is_type)).front() ==
                ^^Cls::Inner);

  constexpr auto getInnerObj = []() consteval {
    constexpr auto inner =
        (members_of(^^Cls, access_context::unchecked()) |
         std::views::filter(std::meta::is_type)).front();
    constexpr auto innerFirst =
        nonstatic_data_members_of(inner, access_context::unchecked())[0];

    Cls::Inner ic {};
    ic.[:innerFirst:] = 5;
    return ic;
  };
  // RUN: grep "c=5" %t.stdout
  constinit static auto innerObj = getInnerObj();
  std::println("c={}", innerObj.c);
}
}  // namespace class_members

                              // =================
                              // namespace_members
                              // =================

namespace namespace_members {
namespace empty_ns {}
static_assert(members_of(^^empty_ns, access_context::unchecked()).size() == 0);

namespace myns {
int var;
void fn();
struct Cls {};
using Alias = int;

template <typename T1, typename T2> class TCls {};
template <typename T2> class TCls<bool, T2> {};
template <> class TCls<int, bool> {};
  // Specializations should not be represented in 'members_of'.

template <typename T> void TFn() {}
;  // Empty declarations should not be represented in 'members_of'.
template <typename T1, typename T2> int TVar = 0;
template <typename T2> int TVar<bool, T2> = 1;
template <> int TVar<int, bool> = 2;
  // Specializations should not be represented in 'members_of'.

template <typename T> using TAlias = int;
template <typename T> concept Concept = requires { true; };

// Partial specializations are not representable in P2996.
template <typename T2> class TCls<int, T2> {};
template <typename T2> int TVar<int, T2> = 1;

[[maybe_unused]] auto l = [] {};

}  // namespace myns

static_assert(members_of(^^myns, access_context::unchecked()).size() == 10);
static_assert(members_of(^^myns, access_context::unchecked()) ==
              std::vector{^^myns::var, ^^myns::fn, ^^myns::Cls, ^^myns::Alias,
                          ^^myns::TCls, ^^myns::TFn, ^^myns::TVar,
                          ^^myns::TAlias, ^^myns::Concept, ^^myns::l});
static_assert((members_of(^^myns, access_context::unchecked()) |
                   std::views::filter(std::meta::is_template) |
                   std::ranges::to<std::vector>()) ==
              std::vector{^^myns::TCls, ^^myns::TFn, ^^myns::TVar,
                          ^^myns::TAlias, ^^myns::Concept});
static_assert((members_of(^^myns, access_context::unchecked()) |
                   std::views::filter(std::meta::is_template) |
                   std::views::filter(std::meta::is_alias_template) |
                   std::ranges::to<std::vector>()) ==
              std::vector{^^myns::TAlias});
}  // namespace_members

                         // ===========================
                         // language_linkage_specifiers
                         // ===========================

namespace language_linkage_specifiers {
namespace with_empty {
extern "C" { }
}  // namespace with_empty

namespace leading_node {
extern "C" {
void fn1();
}  // extern "C"

void fn2();
}  // namespace leading_node

namespace multiple_blocks {
extern "C" {
void fn3();
}  // extern "C"

void fn4();

extern "C" {
void fn5();
}  // extern "C"
}  // namespace multiple_blocks

namespace multiple_blocks {
void fn6();

extern "C" {
void fn7();
}
}  // namespace multiple_blocks

static_assert(members_of(^^with_empty,
                         access_context::unchecked()).size() == 0);
static_assert(members_of(^^leading_node,
                         access_context::unchecked()).size() == 2);
static_assert(members_of(^^multiple_blocks,
                         access_context::unchecked()).size() == 5);
}  // namespace language_linkage_specifiers

                         // ==========================
                         // inaccessible_class_members
                         // ==========================

namespace inaccessible_class_members {
class Cls {
private:
  [[maybe_unused]] int mem1 = 0;
  consteval int memfn() const { return 5; }

public:
  int mem2 = 0;
};

static_assert(
        identifier_of(members_of(^^Cls,
                                 access_context::unchecked())[0]) == "mem1");
static_assert(
        identifier_of(members_of(^^Cls,
                                 access_context::unchecked())[1]) == "memfn");
static_assert(
        identifier_of(members_of(^^Cls,
                                 access_context::unchecked())[2]) == "mem2");

// Ensure these can be spliced.
constexpr Cls obj;
static_assert(obj.[:members_of(^^Cls, access_context::unchecked())[1]:]() == 5);
}  // namespace inaccessible_class_members

                           // =======================
                           // unsatisfied_constraints
                           // =======================

namespace unsatisfied_constraints {
template <typename T>
struct Cls {
    ~Cls() requires (false) {}
    ~Cls() {}

    void fn() requires (^^T == ^^int) {}
    void fn() requires (^^T != ^^bool) {}
};

static_assert(
    (members_of(^^Cls<int>, access_context::unchecked()) |
     std::views::filter(std::meta::is_destructor) |
     std::ranges::to<std::vector>()).size() == 1);
static_assert(
    (members_of(^^Cls<int>, access_context::unchecked()) |
     std::views::filter([](auto R) { return !is_destructor(R); }) |
     std::views::filter(std::meta::is_user_provided) |
     std::ranges::to<std::vector>()).size() == 2);
static_assert(
    (members_of(^^Cls<short>, access_context::unchecked()) |
     std::views::filter([](auto R) { return !is_destructor(R); }) |
     std::views::filter(std::meta::is_user_provided) |
     std::ranges::to<std::vector>()).size() == 1);
static_assert(
    (members_of(^^Cls<bool>, access_context::unchecked()) |
     std::views::filter([](auto R) { return !is_destructor(R); }) |
     std::views::filter(std::meta::is_user_provided) |
     std::ranges::to<std::vector>()).size() == 0);

}  // namespace unsatisfied_constraints

                                    // =====
                                    // bases
                                    // =====

namespace bases {
struct B1 {};
struct B2 {};
struct B3 {};
struct D1 : public B1, virtual protected B2, private B3 {};
using Alias = D1;
static_assert(bases_of(^^B1, access_context::unchecked()).size() == 0);
static_assert(type_of(bases_of(^^D1, access_context::unchecked())[0]) == ^^B1);
static_assert(type_of(bases_of(^^D1, access_context::unchecked())[1]) == ^^B2);
static_assert(type_of(bases_of(^^D1, access_context::unchecked())[2]) == ^^B3);
static_assert(is_public(bases_of(^^D1, access_context::unchecked())[0]));
static_assert(!is_protected(bases_of(^^D1, access_context::unchecked())[0]));
static_assert(!is_private(bases_of(^^D1, access_context::unchecked())[0]));
static_assert(!is_virtual(bases_of(^^D1, access_context::unchecked())[0]));
static_assert(!is_public(bases_of(^^D1, access_context::unchecked())[1]));
static_assert(is_protected(bases_of(^^D1, access_context::unchecked())[1]));
static_assert(!is_private(bases_of(^^D1, access_context::unchecked())[1]));
static_assert(is_virtual(bases_of(^^D1, access_context::unchecked())[1]));
static_assert(!is_public(bases_of(^^D1, access_context::unchecked())[2]));
static_assert(!is_protected(bases_of(^^D1, access_context::unchecked())[2]));
static_assert(is_private(bases_of(^^D1, access_context::unchecked())[2]));
static_assert(!is_virtual(bases_of(^^D1, access_context::unchecked())[2]));

static_assert(type_of(bases_of(^^Alias,
              access_context::unchecked())[0]) == ^^B1);

template <typename... Bases> struct D2 : Bases... {};
static_assert(type_of(bases_of(^^D2<B1, B3>, access_context::unchecked())[0]) ==
              ^^B1);
static_assert(type_of(bases_of(^^D2<B1, B3>, access_context::unchecked())[1]) ==
              ^^B3);
static_assert(is_public(bases_of(^^D2<B1, B3>,
                                 access_context::unchecked())[0]));
static_assert(is_public(bases_of(^^D2<B1, B3>,
                                 access_context::unchecked())[1]));
}  // namespace bases

                                 // ===========
                                 // enumerators
                                 // ===========

namespace enumerators {
enum class EnumCls;
static_assert(!is_enumerable_type(^^EnumCls));

enum class EnumCls {
    A,
    B = is_enumerable_type(^^EnumCls) ? 1 : 0,
    C,
};
static_assert(int(EnumCls::B) == 0);
static_assert(is_enumerable_type(^^EnumCls));
static_assert(enumerators_of(^^EnumCls) ==
              std::vector{^^EnumCls::A, ^^EnumCls::B, ^^EnumCls::C});

struct Cls { enum Enum { A, B, C }; };
static_assert(!is_enumerable_type(^^::));
static_assert(is_enumerable_type(^^Cls::Enum));
static_assert(enumerators_of(^^Cls::Enum) ==
              std::vector{^^Cls::A, ^^Cls::B, ^^Cls::C});

}  // namespace enumerators

                              // ================
                              // enumerable_types
                              // ================

namespace enumerable_types {
enum E : int;
static_assert(!is_enumerable_type(^^E));

enum E : int { A = is_enumerable_type(^^E) ? 1 : 0 };
static_assert(E::A == 0);
static_assert(is_enumerable_type(^^E));

struct S;
static_assert(!is_enumerable_type(^^S));

struct S {
  void fn() {
    static_assert(is_enumerable_type(^^S));
  }
  static_assert(!is_enumerable_type(^^S));
};
static_assert(is_enumerable_type(^^S));

void fn();
static_assert(!is_enumerable_type(^^fn));

void fn() {
  static_assert(!is_enumerable_type(^^fn));
}
static_assert(!is_enumerable_type(^^fn));

static_assert(!is_enumerable_type(^^::));
static_assert(!is_enumerable_type(^^int));
static_assert(!is_enumerable_type(^^std::vector));

}  // namespace enumrable_types

                            // ====================
                            // deduced_return_types
                            // ====================

namespace deduced_return_type {
namespace NS { auto fn(); }
static_assert(members_of(^^NS,
                         std::meta::access_context::unchecked()).size() == 0);

}  // namespace deduced_return_type

                          // ========================
                          // out_of_line_declarations
                          // ========================

namespace out_of_line_declarations {
constexpr auto ctx = std::meta::access_context::current();

namespace NS1 {
struct A;

}  // namespace NS1

struct NS1::A {
static void f(class C *);
static auto g();
};
void NS1::A::f(NS1::C *) {}
void gq(NS1::C *) {}

static_assert(
    *std::ranges::find(
        [](const auto &r) -> const auto & { return r; }(
            members_of(^^NS1, ctx)),
        ^^NS1::C) ==
    ^^NS1::C);

namespace NS2 {
namespace Inner {
struct S {
  friend void f();
};
} // namespace Inner
void Inner::f() {}

} // namespace NS2

static_assert(1 == members_of(^^NS2, ctx).size());
static_assert(2 == members_of(^^NS2::Inner, ctx).size());
}  // namespace out_of_line_declarations

int main() {
  class_members::usage_example();
}
