//===--- TokenSequence.h - Token sequences for code injection ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the TokenSequence class, which represents the value of a
// token sequence expression (P3294), e.g., '^^{ int x = 42; }'.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_TOKENSEQUENCE_H
#define LLVM_CLANG_AST_TOKENSEQUENCE_H

#include "clang/AST/APValue.h"
#include "clang/AST/Type.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/TrailingObjects.h"

namespace llvm {
class FoldingSetNodeID;
class raw_ostream;
} // namespace llvm

namespace clang {

class ASTContext;
struct PrintingPolicy;

/// \brief A constant value interpolated into a token sequence.
///
/// This is the payload of a 'tok::annot_token_value' token: when such a token
/// is injected, the parser treats it as a primary expression having the given
/// type and value.
struct InterpolatedValue {
  QualType Ty;
  APValue Val;

  InterpolatedValue(QualType Ty, const APValue &Val) : Ty(Ty), Val(Val) {}
};

/// \brief An immutable sequence of tokens: the result of evaluating a token
/// sequence expression.
///
/// The tokens are regular preprocessed tokens (translation phase 7), with two
/// peculiarities:
///  - a 'tok::annot_token_value' token carries a 'InterpolatedValue *' as annotation
///    value; and
///  - a 'tok::string_literal' token flagged 'Token::StringifiedInMacro' was
///    synthesized by a '\str(...)' interpolator: its literal data is owned by
///    the ASTContext rather than by a source buffer.
class TokenSequence final
    : private llvm::TrailingObjects<TokenSequence, Token> {
  friend TrailingObjects;

  unsigned NumTokens;

  explicit TokenSequence(ArrayRef<Token> Toks);

public:
  static const TokenSequence *Create(const ASTContext &C, ArrayRef<Token> Toks);

  /// Builds a token sequence by lexing the given text. This is how a token
  /// sequence is read back from an AST file, into which it was written as
  /// printed by 'print'. The tokens have no source location.
  static const TokenSequence *CreateFromText(const ASTContext &C,
                                             StringRef Text);

  /// Whether the sequence contains the value of a '\val' or '\[: :]'
  /// interpolator, which has no textual representation.
  bool hasInterpolatedValues() const;

  ArrayRef<Token> tokens() const {
    return {getTrailingObjects(), NumTokens};
  }
  unsigned size() const { return NumTokens; }
  bool empty() const { return NumTokens == 0; }

  /// Profiles the contents of the sequence. Whitespace, source locations and
  /// token flags do not participate.
  void Profile(llvm::FoldingSetNodeID &ID) const;

  /// Prints the tokens separated by single spaces.
  void print(llvm::raw_ostream &OS, const PrintingPolicy &Policy,
             const ASTContext *Ctx = nullptr) const;

  /// Returns the spelling of a single token of a token sequence.
  static void printToken(llvm::raw_ostream &OS, const Token &Tok,
                         const PrintingPolicy &Policy,
                         const ASTContext *Ctx = nullptr);

  /// Builds an identifier token.
  static Token makeIdentifier(IdentifierInfo *II, SourceLocation Loc);

  /// Builds a pseudo-literal token carrying the given value.
  static Token makeValue(const ASTContext &C, QualType Ty, const APValue &Val,
                         SourceLocation Loc);

  /// Builds a string literal token whose value is the given (unescaped)
  /// string.
  static Token makeStringLiteral(const ASTContext &C, StringRef Str,
                                 SourceLocation Loc);

  /// Builds a punctuator token.
  static Token makePunctuator(tok::TokenKind Kind, SourceLocation Loc);
};

} // namespace clang

#endif
