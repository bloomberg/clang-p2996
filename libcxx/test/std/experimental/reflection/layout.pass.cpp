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

// <experimental/reflection>
//
// [reflection]

#include <meta>
#include <ranges>


using int_alias = int;

long x;
static_assert(size_of(std::meta::reflect_object(x)) == sizeof(long));
static_assert(size_of(std::meta::reflect_constant(38)) == sizeof(int));
static_assert(size_of(^^int) == sizeof(int));
static_assert(size_of(^^int&) == sizeof(int *));
static_assert(size_of(^^int_alias) == sizeof(int));
static_assert(bit_size_of(^^int) == 8 * sizeof(int));
static_assert(bit_size_of(^^int &) == 8 * sizeof(int *));
static_assert(bit_size_of(^^int_alias) == 8 * sizeof(int));
static_assert(alignment_of(^^int) == alignof(int));
static_assert(alignment_of(^^int &) == alignof(int *));
static_assert(alignment_of(^^int_alias) == alignof(int));


// [meta.reflection.layout]: total_bits is a const member function returning
// ptrdiff_t, so it must be callable on a const member_offset.
constexpr std::meta::member_offset const_off{3, 5};
static_assert(const_off.total_bits() == 3 * CHAR_BIT + 5);
static_assert(std::meta::member_offset{0, 0}.total_bits() == 0);
static_assert(std::is_same_v<decltype(const_off.total_bits()), std::ptrdiff_t>);

struct S1 { char mem; };
static_assert(offset_of(^^S1::mem) == std::meta::member_offset{0, 0});
static_assert(size_of(^^S1::mem) == 1);
static_assert(bit_size_of(^^S1::mem) == 8);
static_assert(size_of(^^S1) == 1);


struct BitField {
    char bf1 : 1;
    char bf2 : 2;
    char : 0;
    char bf3 : 3;
    int bf4 : 3;
};
static_assert(offset_of(^^BitField::bf1) == std::meta::member_offset{0, 0});
static_assert(offset_of(^^BitField::bf2) == std::meta::member_offset{0, 1});
static_assert(
    offset_of(
        nonstatic_data_members_of(^^BitField,
                                  std::meta::access_context::current())[2]) ==
    std::meta::member_offset{1, 0});
static_assert(offset_of(^^BitField::bf3) == std::meta::member_offset{1, 0});
static_assert(offset_of(^^BitField::bf4) == std::meta::member_offset{1, 3});
static_assert(bit_size_of(^^BitField::bf1) == 1);
static_assert(bit_size_of(^^BitField::bf2) == 2);
static_assert(
    bit_size_of(
        (members_of(^^BitField, std::meta::access_context::current()) |
            std::views::filter(std::meta::is_bit_field) |
            std::ranges::to<std::vector>())[2]) ==
    0);
static_assert(bit_size_of(^^BitField::bf3) == 3);
static_assert(bit_size_of(^^BitField::bf4) == 3);

// unnamed bitfield not included.
static_assert(
    nonstatic_data_members_of(
        ^^BitField,
        std::meta::access_context::current()).size() == 4);
static_assert(size_of(^^BitField) == 4);

alignas(64) int i1;
alignas(128) int &r1 = i1;

static_assert(alignment_of(^^i1) == 64);
// alignment_of(^^r1) is not defined for a variable of reference type
// ([meta.reflection.layout]/8.1); see layout.verify.cpp.

struct Align {
    alignas(1) char a1;
    alignas(2) char a2;
    alignas(4) char a4;
    alignas(8) char a8;

    alignas(16) int &r1;
    int &r2;
};
static_assert(alignment_of(^^Align::a1) == 1);
static_assert(alignment_of(^^Align::a2) == 2);
static_assert(alignment_of(^^Align::a4) == 4);
static_assert(alignment_of(^^Align::a8) == 8);
static_assert(alignment_of(^^Align::r1) == 16);
static_assert(alignment_of(^^Align::r2) == 8);
static_assert(alignment_of(^^Align) == 16);
static_assert(offset_of(^^Align::a1) == std::meta::member_offset{0, 0});
static_assert(offset_of(^^Align::a2) == std::meta::member_offset{2, 0});
static_assert(offset_of(^^Align::a4) == std::meta::member_offset{4, 0});
static_assert(offset_of(^^Align::a8) == std::meta::member_offset{8, 0});
static_assert(size_of(^^Align::a1) == sizeof(char));
static_assert(size_of(^^Align::a2) == sizeof(char));
static_assert(size_of(^^Align::a4) == sizeof(char));
static_assert(size_of(^^Align::a8) == sizeof(char));
static_assert(size_of(^^Align) == 32);

struct alignas(64) AlignedTo64 { int mem; };
static_assert(alignment_of(^^AlignedTo64) == 64);
static_assert(size_of(^^AlignedTo64) == 64);
static_assert(size_of(^^AlignedTo64::mem) == sizeof(int));

constexpr auto dms1 = data_member_spec(^^int, {});
constexpr auto dms2 = data_member_spec(^^int, {.bit_width=0});
constexpr auto dms3 = data_member_spec(^^int, {.bit_width=3});
constexpr auto dms4 = data_member_spec(^^int, {.alignment=8});
static_assert(alignment_of(dms1) == alignof(int));
static_assert(alignment_of(dms4) == 8);
static_assert(size_of(dms1) == sizeof(int));
// size_of(dms2) and size_of(dms3) are not defined for a data member
// description with a bit width ([meta.reflection.layout]/6.1).
static_assert(size_of(dms4) == sizeof(int));
static_assert(bit_size_of(dms1) == sizeof(int) * 8);
static_assert(bit_size_of(dms2) == 0);
static_assert(bit_size_of(dms3) == 3);
static_assert(bit_size_of(dms4) == sizeof(int) * 8);

                                // ============
                                // base_offsets
                                // ============

namespace base_offsets {
struct A { char a; };
struct B { bool b; };
struct C : A, B {};

static_assert(
    offset_of(bases_of(^^C, std::meta::access_context::current())[0]) ==
    std::meta::member_offset{0, 0});
static_assert(
    offset_of(bases_of(^^C, std::meta::access_context::current())[1]) ==
    std::meta::member_offset{1, 0});

struct V { };
struct D : V { virtual void fn() = 0; };
static_assert(
    offset_of(bases_of(^^D, std::meta::access_context::current())[0]) ==
    std::meta::member_offset{0, 0});

}  // namespace base_offsets

                           // =======================
                           // base_class_relationships
                           // =======================

// [meta.reflection.layout]/5, /7.3, /9.3: a direct base class relationship
// has the size, alignment and bit size of the base class type.
namespace base_class_relationships {
constexpr auto ctx = std::meta::access_context::unchecked();
struct Empty {};
struct B { int b; char c; };
struct alignas(16) Aligned { char c; };
struct D : Empty, B, Aligned { int d; };

static_assert(size_of(bases_of(^^D, ctx)[0]) == sizeof(Empty));
static_assert(size_of(bases_of(^^D, ctx)[0]) > 0);
static_assert(size_of(bases_of(^^D, ctx)[1]) == sizeof(B));
static_assert(size_of(bases_of(^^D, ctx)[2]) == sizeof(Aligned));
static_assert(alignment_of(bases_of(^^D, ctx)[1]) == alignof(B));
static_assert(alignment_of(bases_of(^^D, ctx)[2]) == 16);
static_assert(bit_size_of(bases_of(^^D, ctx)[1]) == CHAR_BIT * sizeof(B));
static_assert(bit_size_of(bases_of(^^D, ctx)[2]) == CHAR_BIT * sizeof(Aligned));
static_assert(size_of(bases_of(^^D, ctx)[1]) == size_of(type_of(bases_of(^^D, ctx)[1])));
}  // namespace base_class_relationships

                              // ===============
                              // other_operands
                              // ===============

namespace other_operands {
int g;
constexpr int cg = 3;
struct S { int bf : 3; int m; static constexpr int sm = 0; };
struct alignas(8) A8 { char c; };

// Variables of non-reference type, objects and values ([meta.reflection.layout]
// /6.1, /8.1, /10.1); a reference type itself is sized as a pointer (/5.2).
static_assert(size_of(^^g) == sizeof(int));
static_assert(size_of(^^S::sm) == sizeof(int));
static_assert(size_of(std::meta::reflect_object(g)) == sizeof(int));
static_assert(size_of(std::meta::reflect_constant(3)) == sizeof(int));
static_assert(size_of(std::meta::reflect_constant(A8{})) == sizeof(A8));
static_assert(size_of(^^int &) == sizeof(int *));
static_assert(size_of(^^A8 &&) == sizeof(A8 *));
static_assert(alignment_of(^^g) == alignof(int));
static_assert(alignment_of(std::meta::reflect_object(g)) == alignof(int));
static_assert(alignment_of(std::meta::reflect_constant(A8{})) == 8);
static_assert(alignment_of(^^A8 &) == alignof(A8 *));
static_assert(bit_size_of(^^g) == CHAR_BIT * sizeof(int));
static_assert(bit_size_of(std::meta::reflect_constant(cg)) == CHAR_BIT * sizeof(int));
static_assert(bit_size_of(^^S::bf) == 3);
static_assert(bit_size_of(^^S::m) == CHAR_BIT * sizeof(int));
static_assert(alignment_of(^^S::m) == alignof(int));

// A non-static data member of reference type is allowed (/6.1, /8.1, /10.1)
// and is sized as a pointer (/5.1).
struct WithRef { int &r; alignas(16) int &ar; };
static_assert(size_of(^^WithRef::r) == sizeof(int *));
static_assert(bit_size_of(^^WithRef::r) == CHAR_BIT * sizeof(int *));
static_assert(alignment_of(^^WithRef::r) == alignof(int *));
static_assert(alignment_of(^^WithRef::ar) == 16);
}  // namespace other_operands

int main() { }
