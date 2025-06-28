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
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <experimental/reflection>
//
// [reflection]

#include <meta>
#include <ranges>

                                 // ==========
                                 // empty_case
                                 // ==========

namespace empty_case {
void fn();
struct S;
template <typename> void TFn();
template <typename> struct TCls {};
namespace NS {};

static_assert(annotations_of(^^int).size() == 0);
static_assert(annotations_of(^^fn).size() == 0);
static_assert(annotations_of(^^S).size() == 0);
static_assert(annotations_of(^^TFn<int>).size() == 0);
static_assert(annotations_of(^^TCls<int>).size() == 0);
static_assert(annotations_of(^^NS).size() == 0);
}  // namespace empty_case

                                // =============
                                // non_dependent
                                // =============

namespace non_dependent {
[[=1, =1, =2, =1.0f]] void fn();
struct [[=3, =3, =4, =2.0f]] S;
template <typename> struct [[=5, =5, =6, =3.0f]] TCls {};
template <typename> [[=5, =5, =6, =3.0f]] void TFn();
namespace [[=7, =7, =8, =4.0f]] NS {}

static_assert((annotations_of(^^fn) |
                  std::views::transform(std::meta::constant_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_constant(1),
                           std::meta::reflect_constant(1),
                           std::meta::reflect_constant(2),
                           std::meta::reflect_constant(1.0f)});
static_assert((annotations_of(^^S) |
                  std::views::transform(std::meta::constant_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_constant(3),
                           std::meta::reflect_constant(3),
                           std::meta::reflect_constant(4),
                           std::meta::reflect_constant(2.0f)});
static_assert((annotations_of(^^TCls<int>) |
                  std::views::transform(std::meta::constant_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_constant(5),
                           std::meta::reflect_constant(5),
                           std::meta::reflect_constant(6),
                           std::meta::reflect_constant(3.0f)});

static_assert((annotations_of(^^TFn<int>) |
                  std::views::transform(std::meta::constant_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_constant(5),
                           std::meta::reflect_constant(5),
                           std::meta::reflect_constant(6),
                           std::meta::reflect_constant(3.0f)});
static_assert((annotations_of(^^NS) |
                  std::views::transform(std::meta::constant_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_constant(7),
                           std::meta::reflect_constant(7),
                           std::meta::reflect_constant(8),
                           std::meta::reflect_constant(4.0f)});

static_assert(is_annotation(annotations_of(^^fn)[0]));
static_assert(is_annotation(annotations_of(^^S)[0]));
static_assert(is_annotation(annotations_of(^^TCls<int>)[0]));
static_assert(is_annotation(annotations_of(^^TFn<int>)[0]));

static_assert(type_of(annotations_of(^^fn)[0]) == ^^int);
static_assert(type_of(annotations_of(^^fn)[3]) == ^^float);

static_assert(annotations_of(^^fn, ^^int).size() == 3);
static_assert(annotations_of(^^fn, ^^float).size() == 1);
static_assert(annotation_of_type<float>(^^fn) == 1.0f);
static_assert(annotation_of_type<char *>(^^fn) == std::nullopt);

static_assert(source_location_of(annotations_of(^^S)[0]).line() ==
              source_location_of(^^S).line());

constexpr struct S {} s;

[[=s]] void fnWithS();
static_assert(type_of(annotations_of(^^fnWithS)[0]) == ^^S);
static_assert(type_of(constant_of(annotations_of(^^fnWithS)[0])) ==
              ^^const S);
}  // namespace non_dependent

                                  // =========
                                  // dependent
                                  // =========

namespace dependent {
template <std::meta::info R>
  [[=[:constant_of(annotations_of(R)[0]):]]] void TFn();
template <std::meta::info R>
  struct [[=[:constant_of(annotations_of(R)[0]):]]] TCls {};

static_assert(extract<int>(annotations_of(^^TFn<^^non_dependent::fn>)[0]) == 1);
static_assert(extract<int>(annotations_of(^^TCls<^^non_dependent::S>)[0]) == 3);
}  // namespace dependent

                                 // ==========
                                 // comparison
                                 // ==========

namespace comparison {
struct [[=42, =42.0f]] S1;
struct [[=42  =40]] S2;

static_assert(annotations_of(^^S1)[0] == annotations_of(^^S1)[0]);
static_assert(annotations_of(^^S1)[0] != annotations_of(^^S2)[0]);
static_assert(annotations_of(^^S1)[0] != annotations_of(^^S1)[1]);

static_assert(constant_of(annotations_of(^^S1)[0]) ==
              std::meta::reflect_constant(42));
static_assert(constant_of(annotations_of(^^S1)[0]) ==
              constant_of(annotations_of(^^S2)[0]));
}  // namespace comparison

                           // =======================
                           // conditional_annotations
                           // =======================

namespace conditional_annotations {
[[=1, =2, =3]] void fn();

template <std::meta::info R>
struct TCls {};

template <std::meta::info R> requires (is_function(R))
struct [[=int(annotations_of(R).size())]] TCls<R> {};

static_assert(annotations_of(^^TCls<^^::>).size() == 0);
static_assert(annotation_of_type<int>(^^TCls<^^fn>) == 3);
}  // namespace conditional_annotations

                            // ====================
                            // annotation_injection
                            // ====================

namespace annotation_injection {
void fn();
template <std::meta::info R>
    [[=annotations_of(R)[0]]] consteval std::meta::info tfn(std::meta::info V) {
  return annotate(R, V);
}

static_assert(annotations_of(^^fn).size() == 0);

consteval {
  annotate(^^fn, std::meta::reflect_constant(1));
}
static_assert(annotations_of(^^fn).size() == 1);
static_assert(extract<int>(annotations_of(^^fn)[0]) == 1);

consteval { tfn<^^fn>(std::meta::reflect_constant(2.0f)); }
static_assert(annotations_of(^^fn).size() == 2);
static_assert(annotation_of_type<float>(^^fn) == 2.0f);
}  // namespace annotation_injection

                       // ==============================
                       // accumulation_over_declarations
                       // ==============================

namespace accumulation_over_declarations {
void fn();
static_assert(annotations_of(^^fn).size() == 0);
[[=1, =2]] void fn();
static_assert(annotations_of(^^fn).size() == 2);
consteval { annotate(^^fn, std::meta::reflect_constant(3)); }
static_assert(annotations_of(^^fn).size() == 3);
[[=4, =5]] void fn();
static_assert(annotations_of(^^fn).size() == 5);
consteval { annotate(^^fn, std::meta::reflect_constant(6)); }
static_assert(annotations_of(^^fn).size() == 6);
void fn();
static_assert(annotations_of(^^fn).size() == 6);

constexpr auto idxOf = [](int v) consteval {
  auto annots = annotations_of(^^fn);
  for (size_t k = 0; k < annots.size(); ++k)
    if (extract<int>(annots[k]) == v)
      return k;

  std::unreachable();
};
constexpr auto p1 = idxOf(1), p4 = idxOf(4);

static_assert(extract<int>(annotations_of(^^fn)[p1]) == 1);
static_assert(extract<int>(annotations_of(^^fn)[p1 + 1]) == 2);
static_assert(extract<int>(annotations_of(^^fn)[p1 + 2]) == 3);

static_assert(extract<int>(annotations_of(^^fn)[p4]) == 4);
static_assert(extract<int>(annotations_of(^^fn)[p4 + 1]) == 5);
static_assert(extract<int>(annotations_of(^^fn)[p4 + 2]) == 6);
}  // namespace accumulation_over_declarations

                       // ===============================
                       // ledger_based_consteval_variable
                       // ===============================

namespace ledger_based_consteval_variable {
struct Counter {
private:

public:
  static consteval int current() {
    auto history = annotations_of(^^Counter);
    return history.empty() ? 0 : extract<int>(history.back());
  }

  static consteval int increment(int i = 1) {
    int value = current() + i;
    annotate(^^Counter, std::meta::reflect_constant(value));
    return value;
  }
};
constexpr auto c1 = Counter::current();
consteval { Counter::increment(); }
constexpr auto c2 = Counter::current();
consteval { Counter::increment(-2); }
constexpr auto c3 = Counter::current();
consteval { Counter::increment(5); }
constexpr auto c4 = Counter::current();

static_assert(c1 == 0);
static_assert(c2 == 1);
static_assert(c3 == -1);
static_assert(c4 == 4);
}  // namespace ledger_based_consteval_variable

                         // ===========================
                         // templated_class_annotations
                         // ===========================

namespace templated_class_annotations {
template <class T>
struct X {
    struct [[=1]] C;
    struct [[=2]] D { };
};

static_assert(annotations_of(^^X<int>::C).size() == 1);
static_assert(annotations_of(^^X<int>::D).size() == 1);
}  // namespace templated_class_annotations

                  // ========================================
                  // bb_clang_p2996_issue_143_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_143_regression_test {
struct test_struct {};

constexpr test_struct test;

[[=test]] void func() {}
[[=1]] void func2() {}

constexpr auto func_first = std::meta::constant_of(std::meta::annotations_of(^^func)[0]);
constexpr auto func2_first = std::meta::constant_of(std::meta::annotations_of(^^func2)[0]);

static_assert(std::meta::constant_of(^^test) == std::meta::reflect_constant(test));
static_assert(std::same_as<decltype([:func_first:]), const test_struct &>);
static_assert(func2_first == std::meta::reflect_constant(1));
static_assert(func_first == std::meta::reflect_constant(test));
}  // namespace bb_clang_p2996_issue_143_regression_test


int main() { }
