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
// RUN: %clang_cc1 %s -std=c++23 -freflection -fannotation-attributes

using info = decltype(^^int);

                                // =============
                                // basic_parsing
                                // =============

namespace basic_parsing {

consteval int fn() { return 1; }

[[=42, =basic_parsing::fn()]]
void annFn();

struct [[=42, =basic_parsing::fn()]] S;

template <typename>
  struct [[=42, =basic_parsing::fn()]] TCls;

namespace [[=42, =basic_parsing::fn()]] NS {};

}  // namespace basic_parsing
