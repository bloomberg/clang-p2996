//===----------------------------------------------------------------------===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -Wno-deprecated-declarations

#include <meta>

using namespace std::meta;

               // ======================================
               // define_enum rejects an unscoped enum
               // ======================================

namespace define_enum_rejects_unscoped {

enum PlainEnum : int;
consteval {  // expected-error {{consteval block must be a constant expression}}
  define_enum(^^PlainEnum, {  // expected-note {{is not a scoped enumeration type}}
    enumerator_spec({.name = "A"}),
  });
}

}  // namespace define_enum_rejects_unscoped


          // ==============================================
          // define_unscoped_enum rejects a scoped enum
          // ==============================================

namespace define_unscoped_enum_rejects_scoped {

enum class ScopedEnum : int;
consteval {  // expected-error {{consteval block must be a constant expression}}
  define_unscoped_enum(^^ScopedEnum, {  // expected-note {{is not an unscoped enumeration type}}
    enumerator_spec({.name = "A"}),
  });
}

}  // namespace define_unscoped_enum_rejects_scoped

int main() {
  return 0;
}
