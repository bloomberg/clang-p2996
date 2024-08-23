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

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <print>
#include <utility>
#include <vector>
#include <initializer_list>
#include <tuple>


namespace std::meta {
    consteval auto type_tuple_size(info type) -> size_t {
        return extract<size_t>(substitute(^^std::tuple_size_v, {type}));
    }
}

template<std::pair<std::size_t, std::size_t>... indices>
struct Indexer {
   template<typename Tuples>
   // Can use tuple indexing instead of tuple of tuples
   auto operator()(Tuples&& tuples) const {
     using ResultType = std::tuple<
       std::tuple_element_t<
         indices.second,
         std::remove_cvref_t<std::tuple_element_t<indices.first,
                                                  std::remove_cvref_t<Tuples>>>
       >...
     >;
     return ResultType(std::get<indices.second>(
                 std::get<indices.first>(std::forward<Tuples>(tuples)))...);
   }
};

template <class T>
consteval auto subst_by_value(std::meta::info tmpl, std::vector<T> args)
    -> std::meta::info
{
    std::vector<std::meta::info> a2;
    for (T x : args) {
        a2.push_back(std::meta::reflect_value(x));
    }

    return substitute(tmpl, a2);
}

consteval auto make_indexer(std::vector<std::size_t> sizes)
    -> std::meta::info
{
    std::vector<std::pair<int, int>> args;

    for (std::size_t tidx = 0; tidx < sizes.size(); ++tidx) {
        for (std::size_t eidx = 0; eidx < sizes[tidx]; ++eidx) {
            args.push_back({tidx, eidx});
        }
    }

    return subst_by_value(^^Indexer, args);
}

template<typename... Tuples>
auto my_tuple_cat(Tuples&&... tuples) {
    constexpr typename [: make_indexer({
        type_tuple_size(type_remove_cvref(^^Tuples))...
    }) :] indexer;
    return indexer(std::forward_as_tuple(std::forward<Tuples>(tuples)...));
}

int r;
auto x = my_tuple_cat(std::make_tuple(10, std::ref(r)),
                      std::make_tuple(21.1, 22, 23, 24));
static_assert(dealias(^^decltype(x)) ==
              ^^std::tuple<int, int&, double, int, int, int>);

int main() {
  // RUN: grep "10, 21.1, 22, 23, 24" %t.stdout
  std::println("{}, {}, {}, {}, {}",
               get<0>(x), get<2>(x), get<3>(x), get<4>(x), get<5>(x));
}
