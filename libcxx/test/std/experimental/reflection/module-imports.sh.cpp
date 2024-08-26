// FILE_DEPENDENCIES: example-module.cppm
//
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -freflection -freflection-new-syntax -fparameter-reflection \
// RUN:     --precompile example-module.cppm -o %t/example-module.pcm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 \
// RUN:     -freflection -freflection-new-syntax -fparameter-reflection \
// RUN:     -fmodule-file=Example=%t/example-module.pcm %t/example-module.pcm \
// RUN:     module-imports.sh.cpp -o %t/module-imports.sh.cpp.tsk
// RUN: %t/module-imports.sh.cpp.tsk > %t/stdout.txt

// expected-no-diagnostics
#include <experimental/meta>
#include <print>

import Example;


                              // ================
                              // Null reflections
                              // ================

static_assert(Example::rNull == std::meta::info{});

                            // ====================
                            // Reflections of types
                            // ====================

static_assert(is_type(Example::rAlias));
static_assert(is_type_alias(Example::rAlias));
static_assert(dealias(Example::rAlias) == ^^int);

                           // ======================
                           // Reflections of objects
                           // ======================

static_assert(is_object(Example::rObj));
static_assert(type_of(Example::rObj) == ^^int);

                            // =====================
                            // Reflections of values
                            // =====================

static_assert(is_value(Example::rValue));
static_assert(Example::rValue == std::meta::reflect_value(1));
static_assert(Example::rValue == [:Example::rRefl:]);
static_assert(Example::Splice == Example::rValue);

                         // ===========================
                         // Reflections of declarations
                         // ===========================

static_assert(is_variable(Example::r42));

                          // ========================
                          // Reflections of templates
                          // ========================

static_assert(is_template(Example::rTVar));

                          // =========================
                          // Reflections of namespaces
                          // =========================

static_assert(is_namespace(Example::rGlobalNS));
static_assert(Example::rGlobalNS == ^^::);

                       // ==============================
                       // Reflections of base specifiers
                       // ==============================

static_assert(is_private(Example::rBase1));
static_assert(is_public(Example::rBase2));

                      // ================================
                      // Reflections of data member specs
                      // ================================

static_assert(is_data_member_spec(Example::rTDMS));
static_assert(type_of(Example::rTDMS) == ^^int);

struct S;
static_assert(is_type(define_class(^^S, {Example::rTDMS})));

                               // ==============
                               // Driver program
                               // ==============

[:Example::rAlias:] main() {
  constexpr S s = {3};

  // RUN: grep "Value: 114" %t/stdout.txt
  std::println("Value: {}",
               [:Example::r42:] + template [:Example::rTVar:]<2> +
                   [:Example::rGlobalNS:]::Example::v42 + [:Example::rValue:] +
                   [:Example::rObj:] + [:type_of(Example::rBase2):]::K +
                   s.test + Example::fn<S, ^^S::test>(s));
}
