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
// RUN: %{exec} %t.exe --count 42 > %t.stdout

#include <experimental/meta>

#include <iostream>
#include <sstream>
#include <vector>


// start 'expand' definition
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
    args.push_back(std::meta::reflect_value(r));
  }
  return substitute(^^__impl::replicator, args);
}
// end 'expand' definition


template<typename Opts>
auto parse_options(std::span<std::string_view const> args) -> Opts {
  Opts opts;

  [: expand(nonstatic_data_members_of(^^Opts)) :] >> [&]<auto dm>{
    auto it = std::find_if(args.begin(), args.end(),
      [](std::string_view arg){
        return arg.starts_with("--") && arg.substr(2) == identifier_of(dm);
      });

    if (it == args.end()) {
      // no option provided, use default
      return;
    } else if (it + 1 == args.end()) {
      std::cerr << "Option " << *it << " is missing a value\n";
      std::exit(EXIT_FAILURE);
    }

    using T = typename[:type_of(dm):];
    std::stringstream iss;
    iss << it[1];
    if (iss >> opts.[:dm:]; !iss) {
      std::cerr << "Failed to parse option " << *it << " into a "
                << display_string_of(^^T) << '\n';
      std::exit(EXIT_FAILURE);
    }
  };
  return opts;
}

struct MyOpts {
  std::string file_name = "input.txt";  // Option "--file_name <string>"
  int    count = 1;                     // Option "--count <int>"
};

int main(int argc, const char *argv[]) {
  MyOpts opts = parse_options<MyOpts>(std::vector<std::string_view>(argv+1, argv+argc));

  // RUN: grep "opts.file=input.txt" %t.stdout
  std::cout << "opts.file=" << opts.file_name << '\n';
  // RUN: grep "opts.count=42" %t.stdout
  std::cout << "opts.count=" << opts.count << '\n';
}
