//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/meta>

// RUN: %{build}
// RUN: %{exec} %t.exe

// The operator-symbol tables must spell every compound assignment correctly
// (op_caret_equals used to duplicate plain "^").

#include <experimental/meta>
#include <string_view>

using namespace std::meta;

static_assert(symbol_of(operators::op_plus_equals) == std::string_view("+="));
static_assert(symbol_of(operators::op_minus_equals) == std::string_view("-="));
static_assert(symbol_of(operators::op_star_equals) == std::string_view("*="));
static_assert(symbol_of(operators::op_slash_equals) == std::string_view("/="));
static_assert(symbol_of(operators::op_percent_equals) == std::string_view("%="));
static_assert(symbol_of(operators::op_caret_equals) == std::string_view("^="));
static_assert(symbol_of(operators::op_ampersand_equals) == std::string_view("&="));
static_assert(symbol_of(operators::op_pipe_equals) == std::string_view("|="));
static_assert(symbol_of(operators::op_less_less_equals) == std::string_view("<<="));
static_assert(symbol_of(operators::op_greater_greater_equals) == std::string_view(">>="));

static_assert(u8symbol_of(operators::op_caret_equals) == std::u8string_view(u8"^="));

int main(int, char**) { return 0; }
