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
//
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <print>


                          // =========================
                          // completion_with_no_fields
                          // =========================

namespace completion_with_no_fields {
struct S;
class C;
union U;
static_assert(is_incomplete_type(^S));
static_assert(is_incomplete_type(^C));
static_assert(is_incomplete_type(^U));
static_assert(is_type(define_class(^S, {})));
static_assert(is_type(define_class(^C, {})));
static_assert(is_type(define_class(^U, {})));
static_assert(!is_incomplete_type(^S));
static_assert(!is_incomplete_type(^C));
static_assert(!is_incomplete_type(^U));
static_assert(nonstatic_data_members_of(^S).size() == 0);
static_assert(nonstatic_data_members_of(^C).size() == 0);
static_assert(nonstatic_data_members_of(^U).size() == 0);

S s;
C c;
U u;
}  // namespace completion_with_no_fields

                               // ==============
                               // test_all_flags
                               // ==============

namespace test_all_flags {
struct S;
static_assert(is_incomplete_type(^S));
static_assert(is_type(define_class(^S, {
                data_member_spec(^int, {.name="count", .alignment=16}),
                data_member_spec(^bool, {.name="flag"}),
                data_member_spec(^int, {.width=5}),
              })));
static_assert(!is_incomplete_type(^S));
static_assert(nonstatic_data_members_of(^S).size() == 3);
static_assert(alignment_of(^S::count) == 16);
static_assert(bit_size_of(nonstatic_data_members_of(^S)[2]) == 5);

constexpr S s = {14, true, 11};
static_assert(s.count == 14);
static_assert(s.flag);
static_assert(s.[:nonstatic_data_members_of(^S)[2]:] == 11);
}  // namespace test_all_flags

                              // ================
                              // class_completion
                              // ================
namespace class_completion {
class C;
static_assert(is_incomplete_type(^C));
static_assert(is_type(define_class(^C, {
                data_member_spec(^int, {.name="count"}),
                data_member_spec(^bool, {.name="flag"}),
              })));
static_assert(!is_incomplete_type(^C));
static_assert(nonstatic_data_members_of(^C).size() == 2);
static_assert(members_of(^C, std::meta::is_nonstatic_data_member,
                         std::meta::is_private).size() == 2);

C c;
}  // namespace class_completion

                              // ================
                              // union_completion
                              // ================

namespace union_completion {
union U;
static_assert(is_incomplete_type(^U));
static_assert(is_type(define_class(^U, {
                data_member_spec(^int, {.name="count"}),
                data_member_spec(^bool, {.name="flag"}),
              })));
static_assert(!is_incomplete_type(^U));
static_assert(size_of(^U) == size_of(^U::count));
static_assert(nonstatic_data_members_of(^U).size() == 2);
static_assert(members_of(^U, std::meta::is_nonstatic_data_member,
                         std::meta::is_public).size() == 2);

U u = {13};
}  // namespace union_completion

                     // ==================================
                     // template_specialization_completion
                     // ==================================

namespace template_specialization_completion {
template <int Idx> struct S;
template <> struct S<0> {};
template <> struct S<2> {};

consteval int nextIncompleteIdx() {
  for (int Idx = 0;; ++Idx)
    if (is_incomplete_type(substitute(^S, {std::meta::reflect_value(Idx)})))
      return Idx;
}
static_assert(is_type(define_class(^S<nextIncompleteIdx()>, {
                data_member_spec(^int, {.name="mem"}),
              })));
static_assert(is_type(define_class(^S<nextIncompleteIdx()>, {
                data_member_spec(^bool, {.name="mem"}),
              })));

static_assert(nonstatic_data_members_of(^S<0>).size() == 0);
static_assert(nonstatic_data_members_of(^S<1>).size() == 1);
static_assert(type_of(^S<1>::mem) == ^int);
static_assert(nonstatic_data_members_of(^S<2>).size() == 0);
static_assert(nonstatic_data_members_of(^S<3>).size() == 1);
static_assert(type_of(^S<3>::mem) == ^bool);
static_assert(is_incomplete_type(^S<4>));
}  // namespace template_specialization_completion

                        // ============================
                        // completion_of_dependent_type
                        // ============================

namespace completion_of_dependent_type {
template <typename T, std::meta::info... Mems>
consteval bool completeDefn() {
  return is_type(define_class(^T, {Mems...}));
}

struct S;
static_assert(is_incomplete_type(^S));
static_assert(completeDefn<S,
                           data_member_spec(^bool, {.name="flag"}),
                           data_member_spec(^int, {.name="count"})>());
static_assert(!is_incomplete_type(^S));
static_assert(nonstatic_data_members_of(^S).size() == 2);

S s;
}  // namespace completion_of_dependent_type

                          // =========================
                          // completion_of_local_class
                          // =========================

namespace completion_of_local_class {
consteval int fn() {
  struct S;
  static_assert(is_type(define_class(^S, {
    data_member_spec(^int, {.name="member"})
  })));

  S s = {13};
  return s.member;
}
static_assert(fn() == 13);
}  // namespace completion_of_local_class

                   // ======================================
                   // completion_of_template_with_pack_param
                   // ======================================

namespace completion_of_template_with_pack_param {
template <typename...>
struct foo;

static_assert(is_type(define_class(^foo<>, {
  data_member_spec(^int, {.name="mem1"})
})));
static_assert(is_type(define_class(^foo<int>, {
  data_member_spec(^int, {.name="mem2"})
})));
static_assert(is_type(define_class(^foo<bool, char>, {
  data_member_spec(^int, {.name="mem3"})
})));

constexpr foo<> f1 = {1};
constexpr foo<int> f2 = {2};
constexpr foo<bool, char> f3 = {3};
static_assert(f1.mem1 + f2.mem2 + f3.mem3 == 6);
}  // namespace completion_of_template_with_pack_param

                          // =========================
                          // with_non_contiguous_range
                          // =========================

namespace with_non_contiguous_range {
struct foo;
static_assert(is_type(define_class(
    ^foo,
    std::views::join(std::vector<std::vector<std::pair<bool,
                                                       std::meta::info>>> {
      {
        std::make_pair(true, std::meta::data_member_spec(^int, {.name="i"})),
      }, {
        std::make_pair(false, std::meta::data_member_spec(^std::string)),
        std::make_pair(true, std::meta::data_member_spec(^bool, {.name="b"})),
      }
    }) |
    std::views::filter([](auto P) { return P.first; }) |
    std::views::transform([](auto P) { return P.second; }))));

static_assert(type_of(^foo::i) == ^int);
static_assert(type_of(^foo::b) == ^bool);
static_assert(nonstatic_data_members_of(^foo).size() == 2);
}  // namespace with_non_contiguous_range

                             // ==================
                             // static_data_member
                             // ==================

namespace static_data_member {
struct S;
class C;
union U;

static_assert(is_incomplete_type(^S));
static_assert(is_type(define_class(^S, {
                data_member_spec(^int, {.name="count", .is_static=true}),
              })));
static_assert(!is_incomplete_type(^S));
decltype(S::count) S::count = 14;
}  // namespace static_data_member

                           // =======================
                           // utf8_name_of_roundtrips
                           // =======================

namespace utf8_name_of_roundtrip {
class Kühl { };

class Cls1;
static_assert(
    is_type(define_class(^Cls1, {data_member_spec(^int,
                                                  {.name=name_of(^Kühl)})})));
static_assert(name_of(nonstatic_data_members_of(^Cls1)[0]) == u8"Kühl");

class Cls2;

constexpr std::string_view sv = name_of<std::string_view>(^Kühl);
static_assert(sv == "K\\u{00FC}hl");
static_assert(is_type(define_class(^Cls2,
                                   {data_member_spec(^int, {.name=sv})})));
static_assert(name_of(nonstatic_data_members_of(^Cls2)[0]) == u8"Kühl");

}  // namespace utf8_name_of_roundtrip

                           // ======================
                           // member_names_with_ucns
                           // ======================

namespace member_names_with_ucns {
struct S;
constexpr auto U = define_class(^S, {
  data_member_spec(^int, {.name="i", .alignment=64}),
  data_member_spec(^int, {.name=u8"こんにち", .alignment=64}),
  data_member_spec(^int, {.name="v\\N{LATIN SMALL LETTER AE}rs\\u{E5}god"})
});
[[maybe_unused]] constexpr [:U:] s{.i=1, .こんにち=2, .værsågod=3};

}  // namespace member_names_with_ucns

int main() {
    // RUN: grep "S::count=14" %t.stdout
    std::print("S::count={}", static_data_member::S::count);
}
