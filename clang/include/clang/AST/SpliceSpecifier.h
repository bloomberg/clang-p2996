//===- SpliceSpecifier.h - Core classes for C++ templates -------*- C++ -*-===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file provides classes for representing splice-specifiers and
//  splice-specialization-specifiers (C++26).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SPLICESPECIFIER_H
#define LLVM_CLANG_AST_SPLICESPECIFIER_H

#include "clang/AST/DependenceFlags.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

class ASTContext;
struct ASTTemplateArgumentListInfo;
class Expr;


/// Represents a C++26 'splice-specifier'.
/// MUST be 16-byte aligned to match NestedNameSpecifier's `FlagBits`
class alignas(16) SpliceSpecifier {
  SourceLocation LSpliceLoc;
  Expr *Operand;
  SourceLocation RSpliceLoc;

  const ASTTemplateArgumentListInfo *TemplateArgs;

  SpliceSpecifier(SourceLocation LSpliceLoc, Expr *Operand,
                  SourceLocation RSpliceLoc,
                  const ASTTemplateArgumentListInfo *TemplateArgs);

public:
  void *operator new(size_t bytes, const ASTContext &C,
                     unsigned alignment = 16);

  void *operator new(size_t bytes, const ASTContext *C,
                     unsigned alignment = 16) {
    return operator new(bytes, *C, alignment);
  }

public:
  static SpliceSpecifier *Create(
      ASTContext &C, SourceLocation LSpliceLoc, Expr *Operand,
      SourceLocation RSpliceLoc,
      const ASTTemplateArgumentListInfo *TemplateArgs);

  Expr *getOperand() const { return Operand; }
  void setOperand(Expr *E) { Operand = E; }

  SpliceSpecifierDependence getDependence() const;

  SourceLocation getLSpliceLoc() const { return LSpliceLoc; }
  void setLSpliceLoc(SourceLocation Loc) { LSpliceLoc = Loc; }

  SourceLocation getRSpliceLoc() const { return RSpliceLoc; }
  void setRSpliceLoc(SourceLocation Loc) { RSpliceLoc = Loc; }

  SourceLocation getLAngleLoc() const;
  SourceLocation getRAngleLoc() const;

  SourceLocation getBeginLoc() const { return LSpliceLoc; }

  SourceLocation getEndLoc() const {
    return isSpecialization() ? getRAngleLoc() : getRSpliceLoc();
  }

  SourceRange getSourceRange() const {
    return SourceRange(getBeginLoc(), getEndLoc());
  }

  bool isSpecialization() const {
    return TemplateArgs != nullptr;
  }

  const ASTTemplateArgumentListInfo *getTemplateArgs() const {
    return TemplateArgs;
  }
};

} // namespace clang

#endif // LLVM_CLANG_AST_TEMPLATEBASE_H
