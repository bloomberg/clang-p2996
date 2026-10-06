//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Minimal stand-in for the token injection part of <meta> (P3294), so that
// the tests in this directory do not depend on a standard library.

#ifndef TOKEN_INJECTION_H
#define TOKEN_INJECTION_H

namespace std::meta {

using info = decltype(^^int);

// Keep these in sync with the table of metafunctions in ExprConstantMeta.cpp.
consteval bool is_token_sequence(info r) { return __metafunction(127, r); }
consteval bool is_empty_token_sequence(info r) {
  return __metafunction(128, r);
}
consteval info nearest_token_queuing_context() { return __metafunction(129); }
consteval info nearest_class_or_namespace() { return __metafunction(130); }
consteval info nearest_namespace() { return __metafunction(131); }
consteval void __report_tokens(info tokens) {
  (void)__metafunction(132, tokens);
}
consteval void queue_injection(info context, info tokens) {
  (void)__metafunction(133, context, tokens);
}
consteval void queue_injection(info tokens) {
  queue_injection(nearest_token_queuing_context(), tokens);
}
consteval void namespace_inject(info ns, info tokens) {
  (void)__metafunction(134, ns, tokens);
}
consteval void namespace_inject(info tokens) {
  namespace_inject(nearest_namespace(), tokens);
}

struct list_builder {
  consteval list_builder() {}
  consteval list_builder(info separator) : separator(separator) {}
  consteval operator info() const { return tokens; }
  consteval void operator+=(info next_tokens) {
    if (is_empty_token_sequence(next_tokens))
      return;
    if (first) {
      tokens = next_tokens;
      first = false;
    } else {
      tokens = ^^{ \{tokens} \{separator} \{next_tokens} };
    }
  }

private:
  bool first = true;
  info tokens = ^^{};
  info separator = ^^{,};
};

} // namespace std::meta

// A string-like class, in the manner of 'std::string_view'.
struct sv {
  const char *ptr;
  decltype(sizeof(0)) len;

  consteval sv(const char *s) : ptr(s), len(0) {
    while (s[len])
      ++len;
  }
  constexpr sv(const char *s, decltype(sizeof(0)) n) : ptr(s), len(n) {}

  constexpr const char *data() const { return ptr; }
  constexpr decltype(sizeof(0)) size() const { return len; }
};

#endif
