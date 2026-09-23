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

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

static consteval bool streq8(const char8_t* A, const char8_t* B) {
  while (*A && *A == *B) {
    ++A;
    ++B;
  }
  return *A == *B;
}

namespace ordinary_and_wide {
constexpr const char* Hello = std::define_encoded_static_string<char>(u8"Hello");
static_assert(Hello[0] == 'H' && Hello[1] == 'e' && Hello[2] == 'l' && Hello[3] == 'l' && Hello[4] == 'o' &&
              Hello[5] == '\0');
static_assert(std::char_traits<char>::length(Hello) == 5);

constexpr const wchar_t* WHello = std::define_encoded_static_string<wchar_t>(u8"Hello");
static_assert(WHello[0] == L'H' && WHello[1] == L'e' && WHello[2] == L'l' && WHello[3] == L'l' && WHello[4] == L'o' &&
              WHello[5] == L'\0');
static_assert(std::char_traits<wchar_t>::length(WHello) == 5);
} // namespace ordinary_and_wide

namespace utf_encodings {
constexpr const char8_t* U8 = std::define_encoded_static_string<char8_t>(u8"Hello");
static_assert(streq8(U8, u8"Hello"));
static_assert(std::char_traits<char8_t>::length(U8) == 5);

constexpr const char16_t* U16 = std::define_encoded_static_string<char16_t>(u8"Hello");
static_assert(U16[0] == u'H' && U16[4] == u'o' && U16[5] == u'\0');
static_assert(std::char_traits<char16_t>::length(U16) == 5);

constexpr const char32_t* U32 = std::define_encoded_static_string<char32_t>(u8"Hello");
static_assert(U32[0] == U'H' && U32[4] == U'o' && U32[5] == U'\0');
static_assert(std::char_traits<char32_t>::length(U32) == 5);
} // namespace utf_encodings

namespace unicode {
constexpr const char* Cafe = std::define_encoded_static_string<char>(u8"caf\u00e9");
static_assert(std::char_traits<char>::length(Cafe) == 5); // UTF-8: c a f c3 a9

constexpr const char16_t* Cafe16 = std::define_encoded_static_string<char16_t>(u8"caf\u00e9");
static_assert(Cafe16[0] == u'c' && Cafe16[1] == u'a' && Cafe16[2] == u'f' && Cafe16[3] == u'\u00e9' &&
              Cafe16[4] == u'\0');

constexpr const char32_t* Cafe32 = std::define_encoded_static_string<char32_t>(u8"caf\u00e9");
static_assert(Cafe32[0] == U'c' && Cafe32[3] == U'\u00e9' && Cafe32[4] == U'\0');

// U+1F600 GRINNING FACE: one UTF-32 code unit, a UTF-16 surrogate pair.
constexpr const char32_t* Grin32 = std::define_encoded_static_string<char32_t>(u8"\U0001F600");
static_assert(Grin32[0] == U'\U0001F600' && Grin32[1] == U'\0');

constexpr const char16_t* Grin16 = std::define_encoded_static_string<char16_t>(u8"\U0001F600");
static_assert(Grin16[0] == 0xD83D && Grin16[1] == 0xDE00 && Grin16[2] == 0);

constexpr const wchar_t* GrinW = std::define_encoded_static_string<wchar_t>(u8"\U0001F600");
template <std::size_t WWidth>
constexpr bool grin_w_ok(const wchar_t* S) {
  if constexpr (WWidth > 2)
    return S[0] == wchar_t(0x1F600) && S[1] == 0;
  else
    return S[0] == wchar_t(0xD83D) && S[1] == wchar_t(0xDE00) && S[2] == 0;
}
static_assert(grin_w_ok<sizeof(wchar_t)>(GrinW));
} // namespace unicode

namespace empty_string {
constexpr const char* Empty = std::define_encoded_static_string<char>(u8"");
static_assert(Empty[0] == '\0');

constexpr const char32_t* Empty32 = std::define_encoded_static_string<char32_t>(u8"");
static_assert(Empty32[0] == U'\0');
} // namespace empty_string

namespace from_range {
constexpr const char* FromView = std::define_encoded_static_string<char>(std::u8string_view(u8"Hi"));
static_assert(FromView[0] == 'H' && FromView[1] == 'i' && FromView[2] == '\0');

constexpr const char* FromVec = std::define_encoded_static_string<char>(std::vector<char8_t>{u8'B', u8'y', u8'e'});
static_assert(FromVec[0] == 'B' && FromVec[1] == 'y' && FromVec[2] == 'e' && FromVec[3] == '\0');

constexpr char8_t Arr[]       = {u8'A', u8'B', 0};
constexpr const char* FromArr = std::define_encoded_static_string<char>(Arr);
static_assert(FromArr[0] == 'A' && FromArr[1] == 'B' && FromArr[2] == '\0');

// Non-contiguous input is copied into a vector, then read from that allocation.
constexpr char8_t Raw[]        = {u8'O', u8'K'};
constexpr const char* FromFilt = std::define_encoded_static_string<char>(
    Raw | std::views::filter([](char8_t) { return true; }));
static_assert(FromFilt[0] == 'O' && FromFilt[1] == 'K' && FromFilt[2] == '\0');
} // namespace from_range

// P3867's STATICALLY_WIDEN: one UTF-8 source, many associated encodings.
namespace statically_widen {
template <class CharT>
constexpr const CharT* Hello = std::define_encoded_static_string<CharT>(u8"Hello");

static_assert(Hello<char>[0] == 'H');
static_assert(Hello<wchar_t>[0] == L'H');
static_assert(Hello<char8_t>[0] == u8'H');
static_assert(Hello<char16_t>[0] == u'H');
static_assert(Hello<char32_t>[0] == U'H');
} // namespace statically_widen

int main() { return 0; }
