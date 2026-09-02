//===- unittests/AST/AttributeScratchpadTest.cpp --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The P3385 attribute reflection metafunctions synthesize ParsedAttr nodes and
// keep them in a scratchpad owned by the ASTContext being evaluated. These
// tests pin that ownership down: the scratchpad used to be a function-local
// static shared by every ASTContext in the process, which raced in hosts that
// build several ASTs concurrently (clangd) and grew without bound.
//
//===----------------------------------------------------------------------===//

#include "../../lib/AST/AttributeScratchpad.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;

namespace {

/// Metafunction IDs, as spelled by the '__metafn_*' enumerators in
/// libcxx/include/meta. They index the Metafunctions table in
/// clang/lib/AST/ExprConstantMeta.cpp, so inserting a metafunction ahead of
/// these shifts them; the tests below then fail to compile their input.
constexpr unsigned MetafnGetIthAttributeOf = 115;
constexpr unsigned MetafnIsAttribute = 116;

const std::vector<std::string> ReflectionArgs = {
    "-std=c++26", "-freflection", "-fattribute-reflection"};

/// A translation unit that reaches toSyntacticForm(): reflecting on an
/// attribute of a class forces the metafunction to rebuild a ParsedAttr from
/// the semantic Attr, which is the only thing the scratchpad holds.
std::string attributeReflectionCode() {
  return R"cpp(
    struct Sentinel {};
    struct [[deprecated]] Widget {};
    using info = decltype(^^int);
    consteval info first_attribute_of_widget() {
      return __metafunction()cpp" +
         std::to_string(MetafnGetIthAttributeOf) + R"cpp(,
                            ^^Widget, ^^Sentinel, 0);
    }
    constexpr info A = first_attribute_of_widget();
    static_assert(__metafunction()cpp" +
         std::to_string(MetafnIsAttribute) + R"cpp(, A));
  )cpp";
}

std::unique_ptr<ASTUnit> buildAndCheck(const std::string &Code) {
  std::unique_ptr<ASTUnit> AU =
      tooling::buildASTFromCodeWithArgs(Code, ReflectionArgs);
  if (AU && AU->getDiagnostics().hasErrorOccurred())
    return nullptr;
  return AU;
}

TEST(AttributeScratchpad, IsOwnedPerASTContext) {
  std::unique_ptr<ASTUnit> First = buildAndCheck(attributeReflectionCode());
  ASSERT_TRUE(First);
  std::unique_ptr<ASTUnit> Second = buildAndCheck(attributeReflectionCode());
  ASSERT_TRUE(Second);

  AttributeScratchpad &FirstPad =
      First->getASTContext().getAttributeScratchpad();
  AttributeScratchpad &SecondPad =
      Second->getASTContext().getAttributeScratchpad();

  // Each context synthesized its own attribute, into its own scratchpad.
  EXPECT_NE(&FirstPad, &SecondPad);
  EXPECT_GT(FirstPad.pool.size(), 0u);

  // Two identical translation units allocate the same number of attributes:
  // the second does not inherit the first's.
  EXPECT_EQ(FirstPad.pool.size(), SecondPad.pool.size());
}

TEST(AttributeScratchpad, DoesNotLeakIntoOtherASTContexts) {
  // Keep the producing units alive, so that a scratchpad shared with them
  // would still be holding their attributes.
  std::unique_ptr<ASTUnit> Producer = buildAndCheck(attributeReflectionCode());
  ASSERT_TRUE(Producer);
  ASSERT_GT(Producer->getASTContext().getAttributeScratchpad().pool.size(), 0u);

  std::unique_ptr<ASTUnit> Bystander = buildAndCheck("struct Unrelated {};");
  ASSERT_TRUE(Bystander);

  // A context that never evaluated an attribute reflection sees nothing.
  EXPECT_EQ(Bystander->getASTContext().getAttributeScratchpad().pool.size(),
            0u);
}

} // namespace
