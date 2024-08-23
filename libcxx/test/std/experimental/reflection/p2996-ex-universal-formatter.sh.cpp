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
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>
#include <format>
#include <print>

namespace __impl {
  template<auto... vals>
  struct replicator_type {
    template<typename F>
      constexpr void operator>>(F body) const {
        (body.template operator()<vals>(), ...);
      }
  };

  template<auto... vals>
  replicator_type<vals...> replicator = {};
}

template<typename R>
consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(reflect_value(r));
  }
  return substitute(^^__impl::replicator, args);
}

struct universal_formatter {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }

  template <typename T>
  auto format(T const& t, auto& ctx) const {
    auto out = std::format_to(ctx.out(), "{}{{", identifier_of(^^T));

    auto delim = [first=true, &out]() mutable {
      if (!first) {
        *out++ = ',';
        *out++ = ' ';
      }
      first = false;
    };

    [: expand(bases_of(^^T)) :] >> [&]<auto base>{
        delim();
        out = std::format_to(out, "{}",
                             (typename [: type_of(base) :] const&)(t));
    };

    [: expand(nonstatic_data_members_of(^^T)) :] >> [&]<auto mem>{
      delim();
      out = std::format_to(out, ".{}={}", identifier_of(mem), t.[:mem:]);
    };

    *out++ = '}';
    return out;
  }
};

struct B { int m0 = 0; };
struct X : B { int m1 = 1; };
struct Y : B { int m2 = 2; };
class Z : public X, private Y { int m3 = 3; int m4 = 4; };

template <> struct std::formatter<B> : universal_formatter { };
template <> struct std::formatter<X> : universal_formatter { };
template <> struct std::formatter<Y> : universal_formatter { };
template <> struct std::formatter<Z> : universal_formatter { };

int main() {
  // RUN: grep "Z{X{B{.m0=0}, .m1=1}, Y{B{.m0=0}, .m2=2}, .m3=3, .m4=4}" \
  // RUN:     %t.stdout | wc -l
  std::println("{}", Z());
}
