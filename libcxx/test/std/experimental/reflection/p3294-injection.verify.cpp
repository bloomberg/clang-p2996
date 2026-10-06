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

// <experimental/reflection>
//
// [reflection]

#include <meta>

#include <string>
#include <string_view>

using namespace std::meta;
using namespace std::literals;

                               // ==============
                               // bad_identifiers
                               // ==============

namespace bad_identifiers {
constexpr auto t1 = ^^{ \["not an identifier"sv] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'not an identifier' does not spell an identifier}}

constexpr auto t2 = ^^{ \["class"s] };
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{'class' is a keyword, not an identifier}}

constexpr auto t3 = ^^{ \[42, "x"sv] };
// expected-error@-1 {{operand of '\[' interpolator must be a string, but has type 'int'}}
// expected-note@-2 {{a string operand is}}
}  // namespace bad_identifiers

                             // ==================
                             // bad_injection_sites
                             // ==================

namespace bad_injection_sites {
struct Complete {};

consteval { queue_injection(^^Complete, ^^{ int x; }); }
// expected-error@-1 {{must be a constant expression}}
// expected-note-re@-2 {{cannot inject tokens into {{.*}}: it is not a namespace, class or function whose body is currently being parsed}}

consteval { namespace_inject(^^Complete, ^^{ int x; }); }
// expected-error@-1 {{must be a constant expression}}
// expected-note-re@-2 {{cannot inject tokens into {{.*}}, which is not a namespace}}

consteval { queue_injection(^^int); }
// expected-error@-1 {{must be a constant expression}}
// expected-note@-2 {{expected a reflection of a token sequence, but got a type}}

// Injection is only possible from a consteval block.
constexpr int not_plain = (queue_injection(^^{ int y; }), 0);
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{cannot produce an injected declaration from a non-plainly constant-evaluated context}}
}  // namespace bad_injection_sites

                             // ===================
                             // ill_formed_injection
                             // ===================

namespace ill_formed_injection {
consteval { queue_injection(^^{ int x = undeclared; }); }
// expected-error@-1 {{use of undeclared identifier 'undeclared'}}
// expected-note@-2 {{in token sequence injected here}}

struct S {
  consteval { queue_injection(^^{ void f() { return 1; } }); }
  // expected-error@-1 {{void function 'f' should not return a value}}
};
}  // namespace ill_formed_injection
