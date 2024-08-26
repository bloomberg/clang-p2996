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
// RUN: %{exec} %t.exe -n WG21 --count 5 > %t.stdout

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


struct Flags {
  bool use_short;
  bool use_long;
};

template <typename T, Flags flags>
struct Option {
  std::optional<T> initializer;

  Option() = default;
  Option(T t) : initializer(t) { }

  static constexpr bool use_short = flags.use_short;
  static constexpr bool use_long = flags.use_long;
};

consteval auto spec_to_opts(std::meta::info opts,
                            std::meta::info spec) -> std::meta::info {
  std::vector<std::meta::info> new_members;
  for (auto member : nonstatic_data_members_of(spec)) {
    auto new_type = template_arguments_of(type_of(member))[0];
    new_members.push_back(
        data_member_spec(new_type, {.name=identifier_of(member)}));
  }
  return define_class(opts, new_members);
}

struct Clap {
  template <typename Spec>
  auto parse(this Spec const& spec, int argc, const char** argv) {
    std::vector<std::string_view> cmdline(argv + 1, argv + argc);

    // check if cmdline contains --help, etc.

    struct Opts;
    static_assert(is_type(spec_to_opts(^^Opts, ^^Spec)));
    Opts opts;

    struct Z {
      std::meta::info spec;
      std::meta::info opt;
    };

    [:expand([]() consteval {
      auto spec_members = nonstatic_data_members_of(^^Spec);
      auto opts_members = nonstatic_data_members_of(^^Opts);

      std::vector<Z> v;
      for (size_t i = 0; i != spec_members.size(); ++i) {
        v.push_back({.spec=spec_members[i], .opt=opts_members[i]});
      }
      return v;
    }()):] >> [&]<auto Pair> {
      constexpr auto sm = Pair.spec;
      constexpr auto om = Pair.opt;

      auto& cur = spec.[:sm:];

      // find the argument associated with this option
      auto it = std::find_if(cmdline.begin(), cmdline.end(),
          [&](std::string_view arg){
            return (cur.use_short && arg.size() == 2 && arg[0] == '-' &&
                   arg[1] == identifier_of(sm)[0])
                || (cur.use_long && arg.starts_with("--") &&
                    arg.substr(2) == identifier_of(sm));
          });

      if (it == cmdline.end()) {
        // no such argument
        //
        // The following needs to be decomposed, as clang seems to have a bug
        // around short-circuiting of compound 'if constexpr'-conditions:
        //   https://godbolt.org/z/9b4faz96T
        if constexpr (has_template_arguments(type_of(om))) {
          if (template_of(type_of(om)) == ^^std::optional)
            // the type is optional, so the argument is too
            return;
        } else if (cur.initializer) {
          // the type isn't optional, but an initializer is provided, use that
          opts.[:om:] = *cur.initializer;
          return;
        } else {
          std::cerr << "Missing required option "
                    << display_string_of(sm) << '\n';
          std::exit(EXIT_FAILURE);
        }
      } else if (it + 1 == cmdline.end()) {
        std::cout << "Option " << *it << " for " << display_string_of(sm)
                  << " is missing a value\n";
        std::exit(EXIT_FAILURE);
      }

      // alright, found our argument, try to parse it
      std::stringstream iss;
      iss << it[1];
      if (iss >> opts.[:om:]; !iss) {
        std::cerr << "Failed to parse " << it[1] << " into option "
                  << display_string_of(sm) << " of type "
                  << display_string_of(type_of(om)) << '\n';
        std::exit(EXIT_FAILURE);
      }
    };

    return opts;
  }
};

struct Args : Clap {
  Option<std::string, Flags{.use_short=true, .use_long=true}> name;
  Option<int, Flags{.use_short=true, .use_long=true}> count = 1;
};

int main(int argc, const char** argv) {
  auto opts = Args{}.parse(argc, argv);

  // RUN: grep "Hello WG21" %t.stdout | wc -l | grep 5
  for (int i = 0; i < opts.count; ++i) {  // opts.count has type int
    std::println("Hello {}", opts.name);  // opts.name has type std::string
  }
}
