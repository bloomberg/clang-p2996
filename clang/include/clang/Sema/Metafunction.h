//===-- Metafunction.h - Classes for representing metafunctions--*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines facilities for representing functions involving reflections.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SEMA_METAFUNCTION_H
#define LLVM_CLANG_SEMA_METAFUNCTION_H

#include "clang/AST/ExprCXX.h"
#include "clang/AST/Type.h"
#include <functional>


namespace clang {

class APValue;
class Sema;

class Metafunction {
public:
  // Enumerators identifying the return-type of a metafunction.
  enum ResultKind : unsigned {
    MFRK_bool,
    MFRK_metaInfo,
    MFRK_sizeT,
    MFRK_sourceLoc,
    MFRK_spliceFromArg,
  };

  using EvaluateFn = CXXMetafunctionExpr::EvaluateFn;

private:
  using impl_fn_t = bool (*)(APValue &Result,
                             Sema &SemaRef,
                             EvaluateFn Evaluator,
                             QualType ResultType,
                             SourceRange Range,
                             ArrayRef<Expr *> Args);

  ResultKind Kind;
  unsigned MinArgs;
  unsigned MaxArgs;
  impl_fn_t ImplFn;

public:
  constexpr Metafunction(ResultKind ResultKind,
                         unsigned MinArgs,
                         unsigned MaxArgs,
                         impl_fn_t ImplFn)
      : Kind(ResultKind), MinArgs(MinArgs), MaxArgs(MaxArgs),
        ImplFn(ImplFn) { }

  ResultKind getResultKind() const {
    return Kind;
  }

  unsigned getMinArgs() const {
    return MinArgs;
  }

  unsigned getMaxArgs() const {
    return MaxArgs;
  }

  bool evaluate(APValue &Result, Sema &S, EvaluateFn Evaluator,
                QualType ResultType, SourceRange Range,
                ArrayRef<Expr *> Args) const;

  // Get a pointer to the metafunction with the given ID.
  // Returns true in the case of error (i.e., no such metafunction exists).
  static bool Lookup(unsigned ID, const Metafunction *&result);
};

} // namespace clang

#endif
