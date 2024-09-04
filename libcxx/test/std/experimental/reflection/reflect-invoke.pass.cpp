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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>

#include <array>


                               // ===============
                               // basic_functions
                               // ===============

namespace basic_functions {
// No parameters.
consteval int fn0() { return 42; }
static_assert([:reflect_invoke(^^fn0, {}):] == 42);
static_assert(extract<int>(reflect_invoke(^^fn0, {})) == 42);

// Single parameter.
consteval int fn1(int i1) { return i1 + 42; }
static_assert([:reflect_invoke(^^fn1,
                               {std::meta::reflect_value(fn0())}):] == 84);
static_assert(extract<int>(reflect_invoke(^^fn1,
                                          {reflect_invoke(^^fn0, {})})) == 84);

// Multiple parameters.
consteval int f2(int i1, int i2) { return 42 + i1 + i2; }
static_assert([:reflect_invoke(^^f2, {std::meta::reflect_value(1),
                                     std::meta::reflect_value(2)}):] == 45);
static_assert(
    extract<int>(reflect_invoke(^^f2, {std::meta::reflect_value(1),
                                      std::meta::reflect_value(2)})) == 45);

// 'std::meta::info'-type parameter.
using Alias = int;
consteval bool isType(std::meta::info R) { return is_type(R); }
static_assert([:reflect_invoke(^^isType, {reflect_value(^^int)}):]);
static_assert(![:reflect_invoke(^^isType, {reflect_value(^^isType)}):]);
static_assert(extract<bool>(reflect_invoke(^^isType,
                                           {reflect_value(^^Alias)})));

// Static member function.
struct Cls {
  static consteval int fn(int p) { return p * p; }
};
static_assert([:reflect_invoke(^^Cls::fn,
                               {std::meta::reflect_value(4)}):] == 16);

// With reflection of constexpr variable as an argument.
static constexpr int five = 5;

static_assert([:reflect_invoke(^^fn1, {^^five}):] == 47);

}  // namespace basic_functions

                              // =================
                              // default_arguments
                              // =================

namespace default_arguments {
consteval int fn(int i1, int i2 = 10) { return 42 + i1 + i2; }

// Explicitly providing all arguments.
static_assert([:reflect_invoke(^^fn, {std::meta::reflect_value(1),
                                     std::meta::reflect_value(2)}):] == 45);

// Leveraging default argument value for parameter 'i2'.
static_assert([:reflect_invoke(^^fn, {std::meta::reflect_value(5)}):] == 57);
}  // namespace default_arguments

                             // ==================
                             // lambda_expressions
                             // ==================

namespace lambda_expressions {
// Ordinary lambda.
static_assert(
    [:reflect_invoke(std::meta::reflect_value([](int p) { return p * p; }),
                     {std::meta::reflect_value(3)}):] == 9);

// Generic lambda.
static_assert(
    [:reflect_invoke(
        std::meta::reflect_value([]<typename T>(T t) requires (sizeof(T) > 1) {
                                   return t;
                                 }),
        {std::meta::reflect_value(4)}):] == 4);
}  // namespace lambda_expressions

                             // ==================
                             // function_templates
                             // ==================

namespace function_templates {
template <typename T1, typename T2>
consteval bool sumIsEven(T1 p1, T2 p2) { return (p1 + p2) % 2 == 0; }

// Fully specialized function call.
static_assert(![:reflect_invoke(^^sumIsEven<int, long>,
                                {std::meta::reflect_value(3),
                                 std::meta::reflect_value(4l)}):]);
static_assert([:reflect_invoke(^^sumIsEven<int, long>,
                               {std::meta::reflect_value(3),
                                std::meta::reflect_value(7)}):]);

// Without specified template arguments.
static_assert([:reflect_invoke(^^sumIsEven,
                               {std::meta::reflect_value(3),
                                std::meta::reflect_value(4)}):] == false);

// With a type parameter pack.
template <typename... Ts>
consteval bool sumIsOdd(Ts... ts) { return (... + ts) % 2 == 1; }
static_assert([:reflect_invoke(^^sumIsOdd,
                               {std::meta::reflect_value(2),
                                std::meta::reflect_value(3l),
                                std::meta::reflect_value(4ll)}):]);

// With a result of 'substitute'.
template <typename T, template <typename, size_t> class C, size_t Sz>
consteval bool FirstElemZero(C<T, Sz> Container) { return Container[0] == 0; }
static_assert(
        [:reflect_invoke(substitute(^^FirstElemZero,
                                    {^^int, ^^std::array,
                                     std::meta::reflect_value(4)}),
                         {std::meta::reflect_value(std::array{0,2,3,4})}):]);

}  // namespace function_templates

                           // ======================
                           // explicit_template_args
                           // ======================

namespace explicit_template_args {
template <template <typename, size_t> class C, typename T, size_t Sz>
consteval auto GetSubstitution() { return ^^C<T, Sz>; }

static_assert([:reflect_invoke(^^GetSubstitution,
                               {^^std::array, ^^int,
                                std::meta::reflect_value(5)}, {}):] ==
              ^^std::array<int, 5>);

template <typename... Ts> consteval auto sum(Ts... ts) { return (... + ts); }

static_assert(type_of(reflect_invoke(^^sum, {^^long, ^^long, ^^long},
              {std::meta::reflect_value(1), std::meta::reflect_value(2),
               std::meta::reflect_value(3)})) == ^^long);
}  // namespace explicit_template_args

                                // ============
                                // constructors
                                // ============

namespace constructors_and_destructors {
struct Cls {
  int value;

  consteval Cls(int value) : value(value) {}
  template <typename T> consteval Cls(T) : value(sizeof(T)) {}
};

constexpr auto ctor = 
  (members_of(^^Cls) |
      std::views::filter(std::meta::is_constructor) |
      std::views::filter(std::meta::is_user_provided)).front();

constexpr auto ctor_template =
  (members_of(^^Cls) |
      std::views::filter(std::meta::is_constructor_template)).front();

// Non-template constructor.
static_assert([:reflect_invoke(ctor,
                               {std::meta::reflect_value(25)}):].value == 25);

// Template constructor with template arguments specified.
static_assert([:reflect_invoke(substitute(ctor_template, {^^int}),
                               {std::meta::reflect_value(4ll)}):].value ==
              sizeof(int));

// Template constructor with template arguments inferred.
static_assert([:reflect_invoke(ctor_template,
                               {std::meta::reflect_value('c')}):].value == 1);

}  // namespace constructors_and_destructors

                            // ====================
                            // returning_references
                            // ====================

namespace returning_references {
const int K = 0;
consteval const int &fn() { return K; }

constexpr auto r = reflect_invoke(^^fn, {});
static_assert(is_object(r) && !is_value(r));
static_assert(type_of(r) == ^^const int);
static_assert(!is_variable(r));
static_assert(r != std::meta::reflect_value(0));

constexpr auto v = value_of(r);
static_assert(is_value(v) && !is_object(v));
static_assert(type_of(v) == ^^int);
static_assert(!is_variable(v));
static_assert(v == std::meta::reflect_value(0));

consteval int &second(std::pair<int, int> &p) {
  return p.second;
}

std::pair<int, int> p;
static_assert(&[:reflect_invoke(^^second, {^^p}):] == &p.second);

}  // namespace returning_references

                         // ==========================
                         // with_non_contiguous_ranges
                         // ==========================

namespace with_non_contiguous_ranges {
consteval auto sum(auto... vs) { return (... + vs); }

static_assert(
    std::meta::reflect_value(20) ==
    reflect_invoke(^^sum, std::ranges::iota_view{1, 10} |
                         std::views::filter([](int v) {
                           return v % 2 == 0;
                         }) |
                         std::views::transform(std::meta::reflect_value<int>)));
}  // namespace with_non_contiguous_ranges

namespace non_static_member_functions {

struct Number {
public:
  consteval Number(int v) : value(v) {}

  consteval int plus(int a) const { return plus_impl(a); }

  constexpr int get_value() const { return value; }

  consteval Number operator+(int num) const { return Number(plus_impl(num)); }

  template <typename T>
  consteval T multiply(T x) const {
    return value * x;
  }

  const int value;

private:
  consteval int plus_impl(int a) const { return value + a; }
};

constexpr Number num{42};

// member function with input arguments
static_assert(std::meta::reflect_value(84) ==
              reflect_invoke(^^Number::plus,
                             {^^num, std::meta::reflect_value(42)}));

// operator overload
static_assert(std::meta::reflect_value(84) ==
              reflect_invoke(^^Number::get_value,
                             {reflect_invoke(^^Number::operator+,
                                                {^^num, std::meta::reflect_value(42)})}));

// member function without input arguments
static_assert(std::meta::reflect_value(42) ==
              reflect_invoke(^^Number::get_value,
                             {^^num}));

// member function called with object reference
constexpr auto num_ref = &num;
static_assert(std::meta::reflect_value(42) ==
              reflect_invoke(^^Number::get_value,
                             {^^num_ref}));

// template member function
static_assert(std::meta::reflect_value(84) ==
              reflect_invoke(^^Number::multiply,
                             {^^int},
                             {^^num, std::meta::reflect_value(2)}));

// Invoking Base::fn() with an object of type Child
struct IsReal {
  consteval IsReal(bool v): value(v){}

  consteval bool is_real() const {
    return value;
  }

  const bool value;
};

struct FloatNumber : public Number, IsReal{
  consteval FloatNumber(int v) : Number(v), IsReal(true) {}
};

constexpr FloatNumber childNumber{42};
static_assert(std::meta::reflect_value(42) ==
              reflect_invoke(^^Number::get_value,
                             {^^childNumber}));
static_assert(std::meta::reflect_value(true) ==
              reflect_invoke(^^IsReal::is_real,
                             {^^childNumber}));

} // namespace non_static_member_functions

namespace function_pointer {
// pointer to simple function
constexpr int foo(int a) {
  return a + 42;
}

constexpr int (*foo_pointer)(int) = &foo;
static_assert(reflect_invoke(^^foo_pointer, {std::meta::reflect_value(0)})
              == std::meta::reflect_value(42));

constexpr static int (*foo_static_pointer)(int) = &foo;
static_assert(reflect_invoke(^^foo_static_pointer, {std::meta::reflect_value(2)})
              == std::meta::reflect_value(44));

// pointer to template function
template <typename T>
constexpr T bar(T a) {
  return a + 42;
}

constexpr int (*bar_pointer)(int) = &bar<int>;
static_assert(reflect_invoke(^^bar_pointer, {std::meta::reflect_value(1)})
              == std::meta::reflect_value(43));

// pointer to method
struct Cls {
public:
  constexpr Cls(int data) : data(data) {}

  static constexpr int fn(int p) { return p * p; }

  constexpr int get() const { return data; }

  const int data;
};

// pointer to static method
constexpr int (*fn_pointer)(int) = &Cls::fn;
static_assert(reflect_invoke(^^fn_pointer, {std::meta::reflect_value(2)})
              == std::meta::reflect_value(4));

// pointer to non-static method
constexpr Cls data(42);
constexpr int (Cls::*get_pointer)() const = &Cls::get;
static_assert(reflect_invoke(^^get_pointer, {^^data}) == std::meta::reflect_value(42));

// object with static storage duration holding a pointer to a constexpr function
constexpr static int (Cls::*get_static_pointer)() const = &Cls::get;
static_assert(reflect_invoke(^^get_static_pointer, {^^data}) == std::meta::reflect_value(42));

} // namespace function_pointer

int main() { }
