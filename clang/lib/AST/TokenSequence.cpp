//===--- TokenSequence.cpp - Token sequences for code injection -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the TokenSequence class (P3294).
//
//===----------------------------------------------------------------------===//

#include "clang/AST/TokenSequence.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

TokenSequence::TokenSequence(ArrayRef<Token> Toks) : NumTokens(Toks.size()) {
  std::uninitialized_copy(Toks.begin(), Toks.end(),
                          getTrailingObjects());
}

const TokenSequence *TokenSequence::Create(const ASTContext &C,
                                           ArrayRef<Token> Toks) {
  void *Mem = C.Allocate(totalSizeToAlloc<Token>(Toks.size()),
                         alignof(TokenSequence));
  return new (Mem) TokenSequence(Toks);
}

const TokenSequence *TokenSequence::CreateFromText(const ASTContext &C,
                                                   StringRef Text) {
  // The literal tokens point into the lexed buffer, which thus has to live
  // as long as the token sequence.
  char *Buffer = new (C) char[Text.size() + 1];
  std::copy(Text.begin(), Text.end(), Buffer);
  Buffer[Text.size()] = '\0';

  Lexer Lex(SourceLocation(), C.getLangOpts(), Buffer, Buffer,
            Buffer + Text.size());
  SmallVector<Token, 32> Toks;
  while (true) {
    Token Tok;
    Lex.LexFromRawLexer(Tok);
    if (Tok.is(tok::eof))
      break;

    if (Tok.is(tok::raw_identifier)) {
      IdentifierInfo &II = C.Idents.get(Tok.getRawIdentifier());
      Tok.setKind(II.getTokenID());
      Tok.setIdentifierInfo(&II);
    } else if (Tok.is(tok::string_literal)) {
      // Have the parser assign a location to the spelling of the literal.
      Tok.setFlag(Token::StringifiedInMacro);
    }
    Tok.setLocation(SourceLocation());
    Toks.push_back(Tok);
  }
  return Create(C, Toks);
}

bool TokenSequence::hasInterpolatedValues() const {
  return llvm::any_of(tokens(), [](const Token &Tok) {
    return Tok.is(tok::annot_token_value);
  });
}

static StringRef literalSpelling(const Token &Tok) {
  assert(Tok.isLiteral() && Tok.getLiteralData() &&
         "literal token without literal data");
  return StringRef(Tok.getLiteralData(), Tok.getLength());
}

void TokenSequence::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(NumTokens);
  for (const Token &Tok : tokens()) {
    ID.AddInteger(static_cast<unsigned>(Tok.getKind()));
    if (Tok.is(tok::annot_token_value)) {
      auto *TV = static_cast<const InterpolatedValue *>(Tok.getAnnotationValue());
      TV->Ty.getCanonicalType().Profile(ID);
      TV->Val.Profile(ID);
    } else if (Tok.isLiteral()) {
      ID.AddString(literalSpelling(Tok));
    } else if (!Tok.isAnnotation()) {
      if (const IdentifierInfo *II = Tok.getIdentifierInfo())
        ID.AddPointer(II);
    }
  }
}

void TokenSequence::printToken(llvm::raw_ostream &OS, const Token &Tok,
                               const PrintingPolicy &Policy,
                               const ASTContext *Ctx) {
  if (Tok.is(tok::annot_token_value)) {
    auto *TV = static_cast<const InterpolatedValue *>(Tok.getAnnotationValue());
    TV->Val.printPretty(OS, Policy, TV->Ty, Ctx);
  } else if (Tok.isLiteral()) {
    OS << literalSpelling(Tok);
  } else if (Tok.isAnnotation()) {
    OS << '<' << Tok.getName() << '>';
  } else if (const IdentifierInfo *II = Tok.getIdentifierInfo()) {
    OS << II->getName();
  } else if (const char *Spelling = tok::getPunctuatorSpelling(Tok.getKind())) {
    OS << Spelling;
  } else {
    OS << Tok.getName();
  }
}

void TokenSequence::print(llvm::raw_ostream &OS, const PrintingPolicy &Policy,
                          const ASTContext *Ctx) const {
  bool First = true;
  for (const Token &Tok : tokens()) {
    if (!First)
      OS << ' ';
    First = false;
    printToken(OS, Tok, Policy, Ctx);
  }
}

Token TokenSequence::makeIdentifier(IdentifierInfo *II, SourceLocation Loc) {
  Token Tok;
  Tok.startToken();
  Tok.setKind(tok::identifier);
  Tok.setIdentifierInfo(II);
  Tok.setLocation(Loc);
  Tok.setLength(II->getLength());
  return Tok;
}

Token TokenSequence::makeValue(const ASTContext &C, QualType Ty,
                               const APValue &Val, SourceLocation Loc) {
  auto *TV = new (C) InterpolatedValue(Ty, Val);
  // The APValue may own memory which the ASTContext's allocator knows nothing
  // about.
  if (Val.needsCleanup())
    C.addDestruction(&TV->Val);

  Token Tok;
  Tok.startToken();
  Tok.setKind(tok::annot_token_value);
  Tok.setLocation(Loc);
  Tok.setAnnotationEndLoc(Loc);
  Tok.setAnnotationValue(TV);
  return Tok;
}

Token TokenSequence::makeStringLiteral(const ASTContext &C, StringRef Str,
                                       SourceLocation Loc) {
  llvm::SmallString<64> Spelling;
  {
    llvm::raw_svector_ostream OS(Spelling);
    OS << '"';
    for (unsigned char Ch : Str) {
      switch (Ch) {
      case '"':  OS << "\\\""; break;
      case '\\': OS << "\\\\"; break;
      case '\n': OS << "\\n"; break;
      case '\t': OS << "\\t"; break;
      case '\r': OS << "\\r"; break;
      default:
        if (Ch < 0x20 || Ch == 0x7f) {
          // Emit other control characters as three-digit octal escapes, which
          // cannot swallow the characters that follow.
          OS << '\\' << char('0' + ((Ch >> 6) & 7)) << char('0' + ((Ch >> 3) & 7))
             << char('0' + (Ch & 7));
        } else {
          OS << char(Ch);
        }
        break;
      }
    }
    OS << '"';
  }

  char *Data = new (C) char[Spelling.size() + 1];
  std::copy(Spelling.begin(), Spelling.end(), Data);
  Data[Spelling.size()] = '\0';

  Token Tok;
  Tok.startToken();
  Tok.setKind(tok::string_literal);
  Tok.setLocation(Loc);
  Tok.setLength(Spelling.size());
  Tok.setLiteralData(Data);
  Tok.setFlag(Token::StringifiedInMacro);
  return Tok;
}

Token TokenSequence::makePunctuator(tok::TokenKind Kind, SourceLocation Loc) {
  Token Tok;
  Tok.startToken();
  Tok.setKind(Kind);
  Tok.setLocation(Loc);
  if (const char *Spelling = tok::getPunctuatorSpelling(Kind))
    Tok.setLength(strlen(Spelling));
  return Tok;
}
