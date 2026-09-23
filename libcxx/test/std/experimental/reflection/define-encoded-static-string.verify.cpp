//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <meta>
//
// P3867R0: define_encoded_static_string

#include <meta>

#include <vector>

namespace invalid_utf8 {
constexpr auto BadLead = std::define_encoded_static_string<char>(std::vector<char8_t>{char8_t(0x80)});
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@*:* {{cannot encode the provided UTF-8 string}}

constexpr auto Incomplete = std::define_encoded_static_string<wchar_t>(std::vector<char8_t>{char8_t(0xC2)});
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@*:* {{cannot encode the provided UTF-8 string}}

constexpr auto Overlong = std::define_encoded_static_string<char32_t>(
    std::vector<char8_t>{char8_t(0xC0), char8_t(0x80)});
// expected-error@-2 {{must be initialized by a constant expression}}
// expected-note@*:* {{cannot encode the provided UTF-8 string}}
} // namespace invalid_utf8

namespace invalid_char_type {
constexpr auto NotAChar = std::define_encoded_static_string<int>(u8"nope");
// expected-error@*:* {{CharT must be char, wchar_t, char8_t, char16_t, or char32_t}}
// expected-error@-2 {{must be initialized by a constant expression}}
} // namespace invalid_char_type

namespace invalid_range {
constexpr auto Ascii = std::define_encoded_static_string<char>("ascii");
// expected-error@-1 {{no matching function for call to 'define_encoded_static_string'}}
} // namespace invalid_range
