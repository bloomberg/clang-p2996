//===- unittests/AST/MetafunctionLookupTest.cpp ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A CXXMetafunctionExpr carries a numeric metafunction ID that indexes the
// Metafunctions table. When the expression is read back from a serialized AST
// that ID arrives unvalidated, so Metafunction::Lookup and
// Sema::getMetafunctionCb are the two places that decide whether an unknown ID
// produces a usable callback. Lookup used to leave its out-parameter untouched
// on failure and getMetafunctionCb used to ignore the failure return, so an
// out-of-range ID left a stack-garbage Metafunction pointer captured in the
// callback and called through at constant evaluation time.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Metafunction.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Sema/Sema.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;

namespace {

/// An ID far past the end of the Metafunctions table; the value the security
/// report used as its example of what a crafted AST file would encode.
constexpr unsigned OutOfRangeID = 0xFFFF;

/// A recognisable non-null pointer to seed the out-parameter with. If Lookup
/// fails without writing it, this is what a caller would be left holding --
/// standing in for the uninitialized stack slot in the real defect.
const Metafunction *const Sentinel =
    reinterpret_cast<const Metafunction *>(static_cast<uintptr_t>(0xDEADBEEF));

/// The number of entries in the Metafunctions table, discovered by asking
/// Lookup. The table size is a file-local constant in ExprConstantMeta.cpp, so
/// there is nothing to compare against directly.
unsigned findTableSize() {
  for (unsigned ID = 0; ID < OutOfRangeID; ++ID) {
    const Metafunction *Metafn = Sentinel;
    if (Metafunction::Lookup(ID, Metafn))
      return ID;
  }
  return OutOfRangeID;
}

TEST(MetafunctionLookup, OutOfRangeIDNullsOutParameter) {
  const Metafunction *Metafn = Sentinel;

  EXPECT_TRUE(Metafunction::Lookup(OutOfRangeID, Metafn));
  EXPECT_EQ(nullptr, Metafn);
}

TEST(MetafunctionLookup, FailsExactlyPastTheEndOfTheTable) {
  unsigned Size = findTableSize();
  ASSERT_GT(Size, 0u);
  ASSERT_LT(Size, OutOfRangeID);

  // The last valid ID still resolves...
  const Metafunction *Last = Sentinel;
  EXPECT_FALSE(Metafunction::Lookup(Size - 1, Last));
  EXPECT_NE(nullptr, Last);
  EXPECT_NE(Sentinel, Last);

  // ...and the first invalid one fails with the out-parameter cleared, rather
  // than leaving the caller's pointer as it found it.
  const Metafunction *Past = Sentinel;
  EXPECT_TRUE(Metafunction::Lookup(Size, Past));
  EXPECT_EQ(nullptr, Past);
}

/// The smallest translation unit that gets a Sema with reflection enabled. No
/// metafunction is used: getMetafunctionCb is asked for IDs directly.
std::unique_ptr<ASTUnit> buildReflectionAST() {
  std::unique_ptr<ASTUnit> AU = tooling::buildASTFromCodeWithArgs(
      "using info = decltype(^^int);\n",
      {"-std=c++26", "-freflection"});
  if (AU && AU->getDiagnostics().hasErrorOccurred())
    return nullptr;
  return AU;
}

TEST(MetafunctionLookup, OutOfRangeIDYieldsNoCallback) {
  std::unique_ptr<ASTUnit> AU = buildReflectionAST();
  ASSERT_TRUE(AU);
  ASSERT_TRUE(AU->hasSema());
  Sema &S = AU->getSema();

  // This is the call ASTStmtReader::VisitCXXMetafunctionExpr makes with an ID
  // taken verbatim from the AST file.
  EXPECT_EQ(nullptr, S.getMetafunctionCb(OutOfRangeID));
  EXPECT_EQ(nullptr, S.getMetafunctionCb(findTableSize()));
}

TEST(MetafunctionLookup, InRangeIDYieldsCallableCallback) {
  std::unique_ptr<ASTUnit> AU = buildReflectionAST();
  ASSERT_TRUE(AU);
  ASSERT_TRUE(AU->hasSema());
  Sema &S = AU->getSema();

  // Guards against the previous test passing because getMetafunctionCb returns
  // null for everything.
  const CXXMetafunctionExpr::ImplFn *Impl = S.getMetafunctionCb(0);
  ASSERT_NE(nullptr, Impl);
  EXPECT_TRUE(static_cast<bool>(*Impl));

  // Repeated requests are memoized against one stable callback object; the
  // expression stores an unowned pointer to it.
  EXPECT_EQ(Impl, S.getMetafunctionCb(0));
}

} // namespace
