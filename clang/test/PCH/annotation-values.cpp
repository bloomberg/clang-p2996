// This covers PCH serialization of CXX26AnnotationAttr values (bloomberg/clang-p2996#360).
// RUN: %clang_cc1 -std=c++2c -freflection -fannotation-attributes -ast-dump %s | FileCheck %s
// RUN: %clang_cc1 -std=c++2c -freflection -fannotation-attributes -emit-pch %s -o %t
// RUN: %clang_cc1 -std=c++2c -freflection -fannotation-attributes -include-pch %t -verify %s
// expected-no-diagnostics

// CHECK: CXX26AnnotationAttr

#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED

using info = decltype(^^int);
struct Sentinel {};

namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
}

// Keep in sync with std::meta::detail in libcxx/include/meta.
enum : unsigned { SourceLocationOf = 15, Extract = 24, GetIthAnnotationOf = 112 };

consteval info annotation_of(info entity, unsigned index) {
  return __metafunction(GetIthAnnotationOf, entity, ^^Sentinel, index);
}

template <typename T>
consteval T extract(info annotation) {
  return __metafunction(Extract, ^^T, annotation);
}

consteval unsigned line_of(info annotation) {
  const std::source_location::__impl *loc = __metafunction(SourceLocationOf, annotation);
  return loc->_M_line;
}

inline constexpr char kName[] = "at";

struct Column { int at; };
struct S {
#line 100
  [[=Column{7}, =42, =^^int, =static_cast<const char *>(kName)]] int a;
};

template <int N>
struct Dependent {
  [[=Column{N}]] int a;
};

static_assert(sizeof(Dependent<17>) > 0);

#else

static_assert(extract<Column>(annotation_of(^^S::a, 0)).at == 7);
static_assert(extract<int>(annotation_of(^^S::a, 1)) == 42);
static_assert(extract<info>(annotation_of(^^S::a, 2)) == ^^int);
static_assert(extract<const char *>(annotation_of(^^S::a, 3)) == kName);
static_assert(line_of(annotation_of(^^S::a, 0)) == 100);
static_assert(extract<Column>(annotation_of(^^Dependent<17>::a, 0)).at == 17);
static_assert(extract<Column>(annotation_of(^^Dependent<19>::a, 0)).at == 19);

#endif
