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

#include <meta>

#include <print>
#include <ranges>

constexpr auto ctx = std::meta::access_context::unchecked();

                          // =========================
                          // completion_with_no_fields
                          // =========================

namespace completion_with_no_fields {
struct S;
class C;
union U;
static_assert(!is_complete_type(^^S));
static_assert(!is_complete_type(^^C));
static_assert(!is_complete_type(^^U));
consteval {
  define_aggregate(^^S, {});
  define_aggregate(^^C, {});
  define_aggregate(^^U, {});
}
static_assert(is_complete_type(^^S));
static_assert(is_complete_type(^^C));
static_assert(is_complete_type(^^U));
static_assert(nonstatic_data_members_of(^^S, ctx).size() == 0);
static_assert(nonstatic_data_members_of(^^C, ctx).size() == 0);
static_assert(nonstatic_data_members_of(^^U, ctx).size() == 0);

S s;
C c;
U u;
}  // namespace completion_with_no_fields

                               // ==============
                               // test_all_flags
                               // ==============

namespace test_all_flags {
struct S;
static_assert(!is_complete_type(^^S));
consteval {
  define_aggregate(^^S, {
    data_member_spec(^^int, {.name="count", .alignment=16}),
    data_member_spec(^^bool, {.name="flag"}),
    data_member_spec(^^int, {.width=0}),
    data_member_spec(^^int, {.width=5}),
  });
}
static_assert(is_complete_type(^^S));
// unnamed bitfields are not nonstatic data members.
static_assert(nonstatic_data_members_of(^^S, ctx).size() == 3);
static_assert(alignment_of(^^S::count) == 16);
static_assert(bit_size_of(nonstatic_data_members_of(^^S, ctx)[2]) == 5);
static_assert((members_of(^^S, ctx) |
               std::views::filter(std::meta::is_bit_field) |
               std::views::transform(std::meta::bit_size_of) |
               std::ranges::to<std::vector>()) == std::vector<size_t> {0, 5});

constexpr S s = {14, true, 11};
static_assert(s.count == 14);
static_assert(s.flag);
static_assert(s.[:nonstatic_data_members_of(^^S, ctx)[2]:] == 11);

struct Empty {};
struct WithEmpty;
consteval {
  define_aggregate(^^WithEmpty, {
    data_member_spec(^^int, {}),
    data_member_spec(^^Empty, {.no_unique_address=true}),
  });
}
static_assert(sizeof(WithEmpty) == sizeof(int));
}  // namespace test_all_flags

                              // ================
                              // class_completion
                              // ================
namespace class_completion {
class C;
static_assert(!is_complete_type(^^C));
consteval {
  define_aggregate(^^C, {
    data_member_spec(^^int, {.name="count"}),
    data_member_spec(^^bool, {.name="flag"}),
  });
}
static_assert(is_complete_type(^^C));
static_assert(nonstatic_data_members_of(^^C, ctx).size() == 2);
static_assert(
        (members_of(^^C, ctx) |
            std::views::filter(std::meta::is_nonstatic_data_member) |
            std::views::filter(std::meta::is_public) |
            std::ranges::to<std::vector>()).size() == 2);

C c;
}  // namespace class_completion

                              // ================
                              // union_completion
                              // ================

namespace union_completion {
union U;
static_assert(!is_complete_type(^^U));
consteval {
  define_aggregate(^^U, {
    data_member_spec(^^int, {.name="count"}),
    data_member_spec(^^bool, {.name="flag"}),
  });
}
static_assert(is_complete_type(^^U));
static_assert(size_of(^^U) == size_of(^^U::count));
static_assert(nonstatic_data_members_of(^^U, ctx).size() == 2);
static_assert(
        (members_of(^^U, ctx) |
            std::views::filter(std::meta::is_nonstatic_data_member) |
            std::ranges::to<std::vector>()).size() == 2);

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
    if (!is_complete_type(substitute(^^S, {std::meta::reflect_constant(Idx)})))
      return Idx;
}
consteval {
  define_aggregate(^^S<nextIncompleteIdx()>, {
    data_member_spec(^^int, {.name="mem"}),
  });
}
consteval {
  define_aggregate(^^S<nextIncompleteIdx()>, {
    data_member_spec(^^bool, {.name="mem"}),
  });
}

static_assert(nonstatic_data_members_of(^^S<0>, ctx).size() == 0);
static_assert(nonstatic_data_members_of(^^S<1>, ctx).size() == 1);
static_assert(type_of(^^S<1>::mem) == ^^int);
static_assert(nonstatic_data_members_of(^^S<2>, ctx).size() == 0);
static_assert(nonstatic_data_members_of(^^S<3>, ctx).size() == 1);
static_assert(type_of(^^S<3>::mem) == ^^bool);
static_assert(!is_complete_type(^^S<4>));
}  // namespace template_specialization_completion

                        // ============================
                        // completion_of_dependent_type
                        // ============================

namespace completion_of_dependent_type {
template <typename T, std::meta::info... Mems>
consteval bool completeDefn() {
  return is_type(define_aggregate(^^T, {Mems...}));
}

struct S;
static_assert(!is_complete_type(^^S));
consteval {
  completeDefn<S, data_member_spec(^^bool, {.name="flag"}),
                  data_member_spec(^^int, {.name="count"})>();
}
static_assert(is_complete_type(^^S));
static_assert(nonstatic_data_members_of(^^S, ctx).size() == 2);

S s;
}  // namespace completion_of_dependent_type

                          // =========================
                          // completion_of_local_class
                          // =========================

namespace completion_of_local_class {
consteval int fn() {
  struct S;
  consteval {
    define_aggregate(^^S, {
      data_member_spec(^^int, {.name="member"})
    });
  }

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

consteval {
  define_aggregate(^^foo<>, { data_member_spec(^^int, {.name="mem1"}) });
  define_aggregate(^^foo<int>, { data_member_spec(^^int, {.name="mem2"}) });
  define_aggregate(^^foo<bool, char>, {
    data_member_spec(^^int, {.name="mem3"})
  });
}

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
consteval {
  define_aggregate(
    ^^foo,
    std::views::join(std::vector<std::vector<std::pair<bool,
                                                       std::meta::info>>> {
      {
        std::make_pair(true, std::meta::data_member_spec(^^int, {.name="i"})),
      }, {
        std::make_pair(false, std::meta::data_member_spec(^^std::string)),
        std::make_pair(true, std::meta::data_member_spec(^^bool, {.name="b"})),
      }
    }) |
    std::views::filter([](auto P) { return P.first; }) |
    std::views::transform([](auto P) { return P.second;
  }));
}

static_assert(type_of(^^foo::i) == ^^int);
static_assert(type_of(^^foo::b) == ^^bool);
static_assert(nonstatic_data_members_of(^^foo, ctx).size() == 2);
}  // namespace with_non_contiguous_range

                        // =============================
                        // utf8_identifier_of_roundtrips
                        // =============================

namespace utf8_identifier_of_roundtrip {
class Kühl { };

class Cls1;
consteval {
  define_aggregate(^^Cls1, {
    data_member_spec(^^int, {.name=u8identifier_of(^^Kühl)})
  });
}
static_assert(u8identifier_of(nonstatic_data_members_of(^^Cls1, ctx)[0]) ==
              u8"Kühl");
static_assert(identifier_of(nonstatic_data_members_of(^^Cls1, ctx)[0]) ==
              "Kühl");
}  // namespace utf8_identifier_of_roundtrip

                         // ===========================
                         // data_member_spec_comparison
                         // ===========================

namespace data_member_spec_comparison {
static_assert(data_member_spec(^^int, {}) != ^^int);
static_assert(data_member_spec(^^int, {}) == data_member_spec(^^int, {}));
static_assert(data_member_spec(^^int, {}) !=
              data_member_spec(^^int, {.name="i"}));
static_assert(data_member_spec(^^int, {.name=u8"i"}) ==
              data_member_spec(^^int, {.name="i"}));
static_assert(data_member_spec(^^int, {.name="i", .alignment=4}) !=
              data_member_spec(^^int, {.name="i"}));
static_assert(data_member_spec(^^int, {.name=""}) ==
              data_member_spec(^^int, {}));

using Alias = int;
static_assert(data_member_spec(^^Alias, {}) != data_member_spec(^^int, {}));
}  // namespace data_member_spec_comparison

                            // ====================
                            // immediate_escalating
                            // ====================

namespace immediate_escalating {
struct S;

auto L = [] {
  define_aggregate(^^S, {});
  return 0;
};

consteval { L(); }
S s;

}  // namespace immediate_escalating

                             // ===================
                             // checks_for_overflow
                             // ===================

namespace check_for_overflow {
struct Incomplete;
struct S {
  int member = (define_aggregate(^^Incomplete, {}), 1);
};
static_assert(!is_complete_type(^^Incomplete));

}  // namespace check_for_overflow

                           // =======================
                           // out_of_scope_injections
                           // =======================

namespace out_of_scope_injections {
namespace {
[[maybe_unused]] void fn() {
  struct I;
  {
    consteval { define_aggregate(^^I, {}); }
    [[maybe_unused]] I i;
  }
  [[maybe_unused]] I i;
}
}  // namespace

namespace {
struct I;

consteval void *fn1() {
  static constexpr auto r = ^^I;
  return (void *)&r;
}

consteval int fn2(void *ptr) {
  define_aggregate(*(std::meta::info *)ptr, {});
  return 12;
}

namespace a {
consteval { (void) fn2(fn1()); }
}  // namespace
[[maybe_unused]] I i;

}  // namespace

}  // namespace out_of_scope_injections

                  // ========================================
                  // bb_clang_p2996_issue_145_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_145_regression_test {
template<int>
struct int_template;

struct int_holder { int x; };

template<int_holder>
struct int_holder_template;

consteval {
    std::meta::define_aggregate(^^int_template<0>, {});
    std::meta::define_aggregate(^^int_holder_template<int_holder{0}>, {});
}

int_template<0> o1;
int_holder_template<int_holder{0}> o2;

}  // namespace bb_clang_p2996_issue_145_regression_test

                  // ========================================
                  // bb_clang_p2996_issue_159_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_159_regression_test {
template <auto V>
struct VS;

template <>
struct VS<^^float>{};

consteval {
  std::meta::define_aggregate(^^VS<0>, {});
  std::meta::define_aggregate(
      std::meta::substitute(^^VS, {std::meta::reflect_constant(1)}), {});

  std::meta::define_aggregate(
      std::meta::substitute(^^VS, {std::meta::reflect_constant(^^int)}),
      {});
}

VS<1> v1;
VS<^^int> v2;

}  // namespace bb_clang_p2996_issue_159_regression_test


int main() { }
