//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify=good %s
// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s -DTEST_ERRORS
// good-no-diagnostics

using info = decltype(^^int);

// Keep in sync with std::meta::detail::__metafn_define_encoded_static_string
// in libcxx/include/meta.
enum : unsigned { __metafn_define_encoded_static_string = 126 };

template <class CharT>
consteval const CharT *encode(const char8_t *Data, unsigned Size) {
  return __metafunction(__metafn_define_encoded_static_string, ^^const CharT *, // expected-note 2 {{subexpression not valid in a constant expression}}
                        Data, Size);
}

#ifndef TEST_ERRORS

constexpr const char *Hello = encode<char>(u8"Hello", 5);
static_assert(Hello[0] == 'H' && Hello[4] == 'o' && Hello[5] == '\0');

constexpr const wchar_t *WHello = encode<wchar_t>(u8"Hello", 5);
static_assert(WHello[0] == L'H' && WHello[4] == L'o' && WHello[5] == L'\0');

constexpr const char8_t *U8Hello = encode<char8_t>(u8"Hello", 5);
static_assert(U8Hello[0] == u8'H' && U8Hello[5] == u8'\0');

constexpr const char16_t *U16Hello = encode<char16_t>(u8"Hello", 5);
static_assert(U16Hello[0] == u'H' && U16Hello[5] == u'\0');

constexpr const char32_t *U32Hello = encode<char32_t>(u8"Hello", 5);
static_assert(U32Hello[0] == U'H' && U32Hello[5] == U'\0');

constexpr const char32_t *Cafe = encode<char32_t>(u8"caf\u00e9", 5);
static_assert(Cafe[0] == U'c' && Cafe[3] == U'\u00e9' && Cafe[4] == U'\0');

constexpr const char16_t *Grin = encode<char16_t>(u8"\U0001F600", 4);
static_assert(Grin[0] == 0xD83D && Grin[1] == 0xDE00 && Grin[2] == 0);

constexpr const char *Empty = encode<char>(u8"", 0);
static_assert(Empty[0] == '\0');

// Pointer into a local constexpr array: not a converted constant expression.
consteval const char *from_local() {
  char8_t buf[] = {u8'X', u8'Y'};
  return encode<char>(buf, 2);
}
constexpr const char *FromLocal = from_local();
static_assert(FromLocal[0] == 'X' && FromLocal[1] == 'Y' && FromLocal[2] == '\0');

// Pointer into a constexpr allocation.
consteval const char *from_heap() {
  char8_t *P = new char8_t[2]{u8'P', u8'Q'};
  const char *R = encode<char>(P, 2);
  delete[] P;
  return R;
}
constexpr const char *FromHeap = from_heap();
static_assert(FromHeap[0] == 'P' && FromHeap[1] == 'Q' && FromHeap[2] == '\0');

#else

constexpr const char8_t Bad[] = {char8_t(0x80)};
constexpr auto Invalid = encode<char>(Bad, 1); // expected-error {{must be initialized by a constant expression}} \
                                               // expected-note {{cannot encode the provided UTF-8 string}} \
                                               // expected-note {{in call to 'encode<char>}}

constexpr auto BadType = encode<int>(u8"x", 1); // expected-error {{must be initialized by a constant expression}} \
                                                // expected-note {{is not a valid character type}} \
                                                // expected-note {{in call to 'encode<int>}}

#endif
