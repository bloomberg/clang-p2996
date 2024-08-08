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

#include <experimental/meta>

#include <print>
#include <tuple>


                                // =============
                                // class_members
                                // =============

namespace class_members {
struct Empty {};
static_assert(members_of(^Empty).size() == 6);
static_assert(
    (members_of(^Empty) |
        std::views::filter(std::meta::is_defaulted) |
        std::views::filter(std::meta::is_special_member) |
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

static_assert(members_of(^Cls).size() == 13);
static_assert(nonstatic_data_members_of(^Cls) ==
              std::vector{^Cls::mem1, ^Cls::mem2});
static_assert(static_data_members_of(^Cls) == std::vector{^Cls::smem});
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_constructor) |
         std::ranges::to<std::vector>()).size() == 2);
static_assert(
    (members_of(^Cls) |
         std::views::filter(std::meta::is_destructor) |
         std::ranges::to<std::vector>()).size() == 1);
static_assert(
    (members_of(^Cls) | std::views::filter(std::meta::is_type) |
                        std::ranges::to<std::vector>()) ==
    std::vector{^Cls::Inner, ^Cls::Alias});
static_assert(
    (members_of(^Cls) | std::views::filter(std::meta::is_template) |
                        std::ranges::to<std::vector>()) ==
    std::vector{^Cls::TMemFn});
static_assert(
    (members_of(^Cls) | std::views::filter(std::meta::is_function) |
                        std::views::filter(std::meta::is_static_member) |
                        std::ranges::to<std::vector>()) ==
    std::vector{^Cls::sfn});
static_assert(
    (members_of(^Cls) | std::views::filter(std::meta::is_function) |
                        std::views::filter([](auto R) {
                          return !is_static_member(R) &&
                                 !is_special_member(R);
                        }) |
                        std::ranges::to<std::vector>()) ==
    std::vector{^Cls::memfn1, ^Cls::memfn2});
static_assert(
    (members_of(^Cls) | std::views::filter(std::meta::is_alias) |
                        std::ranges::to<std::vector>()) ==
    std::vector{^Cls::Alias});

template <typename T>
struct TCls {
  T mem;
};
static_assert(type_of(nonstatic_data_members_of(^TCls<int>)[0]) == ^int);

void usage_example() {
  constexpr auto getObj = []() consteval {
    Cls a {};
    constexpr auto first = nonstatic_data_members_of(^Cls)[0];
    constexpr auto second = nonstatic_data_members_of(^Cls)[1];
    a.[:first:] = 20;
    a.[:second:] = 1.4f;
    return a;
  };

  constinit static auto obj = getObj();

  // RUN: grep "obj=<20, 1.4>" %t.stdout
  std::println(R"(obj=<{}, {}>)", obj.memfn1(), obj.memfn2());

  static_assert((members_of(^Cls) |
                     std::views::filter(std::meta::is_type) |
                     std::ranges::to<std::vector>()).size() == 2);

  static_assert((members_of(^Cls) |
                     std::views::filter(std::meta::is_type)).front() ==
                ^Cls::Inner);

  constexpr auto getInnerObj = []() consteval {
    constexpr auto inner =
        (members_of(^Cls) | std::views::filter(std::meta::is_type)).front();
    constexpr auto innerFirst = nonstatic_data_members_of(inner)[0];

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
static_assert(members_of(^empty_ns).size() == 0);

namespace myns {
int var;
void fn();
struct Cls {};
using Alias = int;

template <typename T1, typename T2> class TCls {};
template <typename T> void TFn() {}
;  // Empty declarations should not be represented in 'members_of'.
template <typename T1, typename T2> int TVar = 0;
template <typename T> using TAlias = int;
template <typename T> concept Concept = requires { true; };

// Partial specializations are not representable in P2996.
template <typename T2> class TCls<int, T2> {};
template <typename T2> int TVar<int, T2> = 1;

}  // namespace myns

static_assert(members_of(^myns).size() == 9);
static_assert(members_of(^myns) ==
              std::vector{^myns::var, ^myns::fn, ^myns::Cls, ^myns::Alias,
                          ^myns::TCls, ^myns::TFn, ^myns::TVar, ^myns::TAlias,
                          ^myns::Concept});
static_assert((members_of(^myns) | std::views::filter(std::meta::is_template) |
                                   std::ranges::to<std::vector>()) ==
              std::vector{^myns::TCls, ^myns::TFn, ^myns::TVar, ^myns::TAlias,
                          ^myns::Concept});
static_assert((members_of(^myns) | std::views::filter(std::meta::is_template) |
                                   std::views::filter(std::meta::is_alias) |
                                   std::ranges::to<std::vector>()) ==
              std::vector{^myns::TAlias});
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

static_assert(members_of(^with_empty).size() == 0);
static_assert(members_of(^leading_node).size() == 2);
static_assert(members_of(^multiple_blocks).size() == 5);
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

static_assert(identifier_of(members_of(^Cls)[0]) == "mem1");
static_assert(identifier_of(members_of(^Cls)[1]) == "memfn");
static_assert(identifier_of(members_of(^Cls)[2]) == "mem2");

// Ensure these can be spliced.
constexpr Cls obj;
static_assert(obj.[:members_of(^Cls)[1]:]() == 5);
}  // namespace inaccessible_class_members

                                    // =====
                                    // bases
                                    // =====

namespace bases {
struct B1 {};
struct B2 {};
struct B3 {};
struct D1 : public B1, virtual protected B2, private B3 {};
static_assert(bases_of(^B1).size() == 0);
static_assert(type_of(bases_of(^D1)[0]) == ^B1);
static_assert(type_of(bases_of(^D1)[1]) == ^B2);
static_assert(type_of(bases_of(^D1)[2]) == ^B3);
static_assert(is_public(bases_of(^D1)[0]));
static_assert(!is_protected(bases_of(^D1)[0]));
static_assert(!is_private(bases_of(^D1)[0]));
static_assert(!is_virtual(bases_of(^D1)[0]));
static_assert(!is_public(bases_of(^D1)[1]));
static_assert(is_protected(bases_of(^D1)[1]));
static_assert(!is_private(bases_of(^D1)[1]));
static_assert(is_virtual(bases_of(^D1)[1]));
static_assert(!is_public(bases_of(^D1)[2]));
static_assert(!is_protected(bases_of(^D1)[2]));
static_assert(is_private(bases_of(^D1)[2]));
static_assert(!is_virtual(bases_of(^D1)[2]));

template <typename... Bases> struct D2 : Bases... {};
static_assert(type_of(bases_of(^D2<B1, B3>)[0]) == ^B1);
static_assert(type_of(bases_of(^D2<B1, B3>)[1]) == ^B3);
static_assert(is_public(bases_of(^D2<B1, B3>)[0]));
static_assert(is_public(bases_of(^D2<B1, B3>)[1]));
}  // namespace bases

                                 // ===========
                                 // enumerators
                                 // ===========

namespace enumerators {
enum class EnumCls { A, B, C };
static_assert(enumerators_of(^EnumCls) ==
              std::vector{^EnumCls::A, ^EnumCls::B, ^EnumCls::C});

struct Cls { enum Enum { A, B, C }; };
static_assert(enumerators_of(^Cls::Enum) ==
              std::vector{^Cls::A, ^Cls::B, ^Cls::C});

}  // namespace enumerators

                               // ==============
                               // all_subobjects
                               // ==============

namespace all_subobjects {
struct B1 {};
struct B2 {};
template <typename... Ts> struct TCls : Ts... {
  int mem;

  // Not subobjects.
  static int smem;
  void memfn();
  static void smemfn();
  TCls();
  ~TCls();
  struct Inner {};
  using Alias = int;
  template <typename> struct TInnerCls;
  template <typename> void TMemFn();
  template <typename> static void TSMemFn();
  template <typename> static int TSMem;
  template <typename> using TAlias = int;
};
static_assert(subobjects_of(^TCls<B1, B2>).size() == 3);
static_assert(subobjects_of(^TCls<B1, B2>)[0] == bases_of(^TCls<B1, B2>)[0]);
static_assert(subobjects_of(^TCls<B1, B2>)[1] == bases_of(^TCls<B1, B2>)[1]);
static_assert(subobjects_of(^TCls<B1, B2>)[2] ==
              nonstatic_data_members_of(^TCls<B1, B2>)[0]);
}  // namespace all_subobjects


int main() {
  class_members::usage_example();
}
