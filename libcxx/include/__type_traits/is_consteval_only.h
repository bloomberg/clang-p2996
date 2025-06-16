//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_CONSTEVAL_ONLY_H
#define _LIBCPP___TYPE_TRAITS_IS_CONSTEVAL_ONLY_H

#include <__config>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS is_consteval_only
    : integral_constant<bool, __is_consteval_only(_Tp)> {};

template <class _Tp>
_LIBCPP_NO_SPECIALIZATIONS
inline constexpr bool is_consteval_only_v = __is_consteval_only(_Tp);

#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_CONSTEVAL_ONLY_H
