//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -ftoken-injection
// ADDITIONAL_COMPILE_FLAGS: -Wno-unused-private-field

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{exec} %t.exe > %t.stdout

#include <meta>

#include <array>
#include <string>
#include <string_view>

using namespace std::meta;
using namespace std::literals;

                              // ==============
                              // classification
                              // ==============

namespace classification {
constexpr auto t1 = ^^{ a + b };
constexpr info t2 = ^^{};

static_assert(std::same_as<decltype(t1), const info>);
static_assert(is_token_sequence(t1) && is_token_sequence(t2));
static_assert(!is_empty_token_sequence(t1) && is_empty_token_sequence(t2));
static_assert(!is_token_sequence(^^int) && !is_token_sequence(^^std));
static_assert(!is_empty_token_sequence(info{}));

// A token sequence is neither a type, nor a value, nor anything else.
static_assert(!is_type(t1) && !is_value(t1) && !is_object(t1) &&
              !is_variable(t1) && !is_namespace(t1) && !is_template(t1) &&
              !is_function(t1) && !has_identifier(t1));

// Token sequences are ordinary values of type 'info'.
consteval std::vector<info> pieces() { return {^^{ a }, ^^{ b }, ^^{ c }}; }
static_assert(pieces().size() == 3 && pieces()[1] == ^^{ b });
constexpr std::array arr = {^^{ x }, ^^{ y }};
static_assert(arr[0] != arr[1]);
}  // namespace classification

                                 // ========
                                 // equality
                                 // ========

namespace equality {
static_assert(^^{ hello  = /* world */   "world" } == ^^{ hello="world" });
static_assert(^^{ a + b } != ^^{ a - b });

constexpr auto t1 = ^^{ abc };
constexpr auto t2 = ^^{ def };
constexpr auto t3 = ^^{ \{t1} \{t2} };
static_assert(t3 == ^^{ abc def });
static_assert(t3 != ^^{ abcdef });
}  // namespace equality

                           // ====================
                           // standard_string_types
                           // ====================

namespace standard_string_types {
// 'std::string_view', 'std::string' and string literals are all accepted.
static_assert(^^{ \["abc"sv] } == ^^{ abc });
static_assert(^^{ \["abc"s] } == ^^{ abc });
static_assert(^^{ \["abc"] } == ^^{ abc });
static_assert(^^{ \[std::string("ab") + "c", 1, "_"sv, 2u] } == ^^{ abc1_2 });
static_assert(^^{ \["hello world"sv.substr(6)] } == ^^{ world });
static_assert(^^{ \str("x"sv) } == ^^{ \str("x"s) });
static_assert(^^{ \str(std::string(3, 'z')) } == ^^{ \str("zzz") });

consteval info field(info type, std::string_view name) {
  return ^^{ \[:type:] \[name]; };
}
static_assert(field(^^int, "a") == field(^^int, "a"sv.substr(0, 1)));
static_assert(field(^^int, "a") != field(^^int, "b"));
static_assert(field(^^int, "a") != field(^^long, "a"));

// Names obtained through reflection.
struct S { int first; long second; };
consteval info copy_members(info type) {
  info result = ^^{};
  for (info m : nonstatic_data_members_of(type, access_context::unchecked()))
    result = ^^{ \{result} \[:type_of(m):] \[identifier_of(m)]; };
  return result;
}
static_assert(copy_members(^^S) ==
              ^^{ \[:^^int:] first; \[:^^long:] second; });
}  // namespace standard_string_types

                                // ============
                                // list_builder
                                // ============

namespace list_builder_tests {
consteval info params(int n) {
  list_builder b;
  for (int i = 0; i < n; ++i)
    b += ^^{ int \["p", i] };
  return b;
}
static_assert(params(0) == ^^{});
static_assert(params(1) == ^^{ int p0 });
static_assert(params(3) == ^^{ int p0, int p1, int p2 });

consteval info joined(info sep) {
  list_builder b(sep);
  b += ^^{ a };
  b += ^^{};      // empty pieces are skipped
  b += ^^{ b };
  b += ^^{ c d };
  return ^^{ [ \{b} ] };
}
static_assert(joined(^^{ | }) == ^^{ [ a | b | c d ] });
static_assert(joined(^^{}) == ^^{ [ a b c d ] });
static_assert(joined(^^{ , }) == ^^{ [ a, b, c d ] });
}  // namespace list_builder_tests

int main() { }
