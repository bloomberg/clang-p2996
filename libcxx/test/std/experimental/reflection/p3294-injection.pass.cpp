//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -ftoken-injection
// ADDITIONAL_COMPILE_FLAGS: -fparameter-reflection
// ADDITIONAL_COMPILE_FLAGS: -Wno-unused-private-field

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{exec} %t.exe > %t.stdout

#include <meta>

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

using namespace std::meta;
using namespace std::literals;

                               // =============
                               // paper_example
                               // =============

namespace paper_example {
consteval auto f(info r, int val, std::string_view name) {
  return ^^{ constexpr \[:r:] \[name] = \val(val); };
}

namespace N {}

consteval {
  queue_injection(^^{ static_assert(N::x == 42); });
  namespace_inject(^^N, f(^^int, 42, "x"));
}
static_assert(N::x == 42);
}  // namespace paper_example

                               // ==============
                               // queuing_contexts
                               // ==============

namespace queuing_contexts {
static_assert(nearest_namespace() == ^^queuing_contexts);
static_assert(nearest_class_or_namespace() == ^^queuing_contexts);
static_assert(nearest_token_queuing_context() == ^^queuing_contexts);

struct S {
  static_assert(nearest_class_or_namespace() == ^^S);
  static_assert(nearest_token_queuing_context() == ^^S);
  static_assert(nearest_namespace() == ^^queuing_contexts);
  consteval { queue_injection(^^{ int injected = 1; }); }
};
static_assert(S().injected == 1);

constexpr int f() {
  static_assert(nearest_token_queuing_context() == ^^f);
  static_assert(nearest_class_or_namespace() == ^^queuing_contexts);
  consteval { queue_injection(^^{ return 2; }); }
}
static_assert(f() == 2);
}  // namespace queuing_contexts

                          // ========================
                          // tuple_like_data_members
                          // ========================

namespace tuple_like {
template <typename... Ts>
struct Tuple {
  consteval {
    std::array types{^^Ts...};
    for (size_t i = 0; i < types.size(); ++i)
      queue_injection(^^{ [[no_unique_address]] \[:types[i]:] \["_", i]; });
  }
};

static_assert(sizeof(Tuple<int, char, char>) == 2 * sizeof(int));
static_assert(nonstatic_data_members_of(^^Tuple<int, long>,
                                        access_context::unchecked())
                  .size() == 2);
constexpr Tuple<int, long, char> t{1, 2L, 'c'};
static_assert(t._0 == 1 && t._1 == 2 && t._2 == 'c');
static_assert(std::same_as<decltype(t._1), long>);
}  // namespace tuple_like

                      // ================================
                      // wrapper_with_cloned_signatures
                      // ================================

namespace cloned_signatures {
struct Counter {
  int value = 0;
  int add(int a, int b) { value += a + b; return value; }
  int get() const { return value; }
  void reset() { value = 0; }
};

// A wrapper forwarding every member function of 'T', while counting calls.
template <typename T>
class Logged {
  T impl;
public:
  int calls = 0;

  consteval {
    for (info fn : members_of(^^T, access_context::unchecked())) {
      if (!is_function(fn) || is_special_member_function(fn) ||
          !has_identifier(fn))
        continue;

      list_builder params, args;
      int k = 0;
      for (info p : parameters_of(fn)) {
        params += ^^{ \[:type_of(p):] \["p", k] };
        args += ^^{ \["p", k] };
        ++k;
      }
      info quals = is_const(fn) ? ^^{ const } : ^^{};

      queue_injection(^^{
        \[:return_type_of(fn):] \[identifier_of(fn)](\{params}) \{quals} {
          ++const_cast<Logged &>(*this).calls;
          return impl.\[:fn:](\{args});
        }
      });
    }
  }
};
}  // namespace cloned_signatures

                           // =====================
                           // generated_dispatcher
                           // =====================

namespace dispatcher {
enum class Color { red, green, blue };

consteval void make_to_string(info e) {
  info cases = ^^{};
  for (info v : enumerators_of(e))
    cases = ^^{ \{cases} case \[:v:]: return \str(identifier_of(v)); };
  queue_injection(^^{
    constexpr std::string_view to_string(\[:e:] value) {
      switch (value) { \{cases} }
      return "<unknown>";
    }
  });
}
consteval { make_to_string(^^Color); }

static_assert(to_string(Color::red) == "red");
static_assert(to_string(Color::blue) == "blue");
static_assert(to_string(Color(42)) == "<unknown>");
}  // namespace dispatcher

int main() {
  cloned_signatures::Logged<cloned_signatures::Counter> c;
  assert(c.add(1, 2) == 3);
  assert(c.add(3, 4) == 10);
  assert(c.get() == 10);
  c.reset();
  assert(c.get() == 0);
  assert(c.calls == 5);
}
