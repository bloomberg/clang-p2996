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

// Can we use a function parameter as the info parameter of __metafunction
// without generating warnings?
consteval auto metafn_info_as_func_param(info inf)
{
    class Sentinel;

    // This is unstable because it depends on metafunction#0 taking exactly
    // two arguments. Easy enough to fix, though.
    return __metafunction(0, inf, ^^Sentinel);
}
