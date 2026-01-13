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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// <experimental/reflection>
//
// [reflection]

#include <meta>

                  // ========================================
                  // bb_clang_p2996_issue_241_regression_test
                  // ========================================

namespace bb_clang_p2996_issue_241_regression_test {
using F1 [[=1]] = int ;
static_assert(std::meta::annotations_of(^^F1).size() == 1);

using [[=1]] F2 = int ;  // expected-error {{an attribute list cannot appear here}}
static_assert(std::meta::annotations_of(^^F2).size() == 1);

struct Bar {};
using F3 [[=1]] = [[=2]] Bar ;  // expected-error {{annotations are not permitted on defining-type-id}}
static_assert(std::meta::annotations_of(^^F3).size() == 1);
static_assert(std::meta::annotations_of(^^Bar).size() == 0);

using F4 = [[=1]] int ;  // expected-error {{annotations are not permitted on defining-type-id}} // expected-error {{an attribute list cannot appear here}}
static_assert(std::meta::annotations_of(^^F4).size() == 0);
using F5 = int [[=1]];  // expected-error {{annotations are not permitted on defining-type-id}}
static_assert(std::meta::annotations_of(^^F5).size() == 0);

}  // namespace bb_clang_p2996_issue_241_regression_test

int main() {}
