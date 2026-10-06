//===--- ParseReflect.cpp - C++2c Reflection Parsing (P2996) --------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for reflection facilities.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/LocInfoType.h"
#include "clang/Basic/DiagnosticParse.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/ParsedAttr.h"
#include "llvm/ADT/ScopeExit.h"
using namespace clang;

ExprResult Parser::ParseCXXReflectExpression(SourceLocation OpLoc) {
  // A '^^' followed by a brace introduces a token sequence (P3294).
  if (getLangOpts().TokenInjection && Tok.is(tok::l_brace))
    return ParseCXXTokenSequenceExpression(OpLoc);

  SourceLocation OperandLoc = Tok.getLocation();

  Sema::ConstevalOnlyRecorder RecordConstevalOnly(Actions);
  EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ReflectionContext);

  // Parse a leading nested-name-specifier, e.g.,
  //
  CXXScopeSpec SS;
  if (ParseOptionalCXXScopeSpecifier(SS, /*ObjectType=*/nullptr,
                                     /*ObjectHasErrors=*/false,
                                     /*EnteringContext=*/false)) {
    SkipUntil(tok::semi, StopAtSemi | StopBeforeMatch);
    return ExprError();
  }

  // Start the tentative parse: This will be reverted if the operand is found
  // to be a type (or rather: a type whose name is more complicated than a
  // single identifier).
  //
  TentativeParsingAction TentativeAction(*this);

  // Next, check for an unqualified-id.
  if (Tok.isOneOf(tok::identifier, tok::kw_operator, tok::kw_template,
                  tok::tilde, tok::annot_template_id)) {
    // Try parsing the operand name as an 'unqualified-id'.

    SourceLocation TemplateKWLoc;
    UnqualifiedId UnqualName;
    if (!ParseUnqualifiedId(SS, ParsedType{}, /*ObjectHadError=*/false,
                            /*EnteringContext=*/false,
                            /*AllowDestructorName=*/true,
                            /*AllowConstructorName=*/false,
                            /*AllowDeductionGuide=*/false,
                            SS.isSet() ? &TemplateKWLoc : nullptr,
                            UnqualName)) {
      bool AssumeType = false;
      if (UnqualName.getKind() == UnqualifiedIdKind::IK_TemplateId &&
          UnqualName.TemplateId->Kind == TNK_Type_template)
        AssumeType = true;
      else if (Tok.isOneOf(tok::l_square, tok::l_paren, tok::star, tok::amp,
                           tok::ampamp, tok::kw_const, tok::kw_volatile,
                           tok::kw_restrict))
        AssumeType = true;

      if (!AssumeType) {
        TentativeAction.Commit();
        return RecordConstevalOnly.RecordAndReturn(
                Actions.ActOnCXXReflectExpr(OpLoc, TemplateKWLoc, SS,
                                            UnqualName));
      }
    }
  } else if (SS.isValid() &&
             SS.getScopeRep()->getKind() == NestedNameSpecifier::Global) {
    // Check for '^::'.
    TentativeAction.Commit();

    Decl *TUDecl = Actions.getASTContext().getTranslationUnitDecl();
    return RecordConstevalOnly.RecordAndReturn(
            Actions.ActOnCXXReflectExpr(OpLoc, SourceLocation(), TUDecl));
  }
  TentativeAction.Revert();

  // Check for attribute
  {
    size_t last = Attrs.size();
    if (MaybeParseCXX11Attributes(Attrs)) {
      size_t newLast = Attrs.size();

      // Reflect expression of empty attribute list is ill formed
      if (last == newLast) {
        Diag(OperandLoc, diag::p3385_trace_empty_attributes_list);
        return ExprError();
      }
      // Reflect expression of multiple attributes is ill formed
      if (newLast - last > 1) {
        Diag(OperandLoc, diag::p3385_err_attributes_list) << (newLast - last);
        return ExprError();
      }
      // Reflects expression of unsupported attribute is ill formed
      auto * attribute = &Attrs.back();
      bool isReflectable = isAttributeWithReflectableVariant(attribute->getParsedKind());
      if (!isReflectable) {
        Diag(OpLoc, diag::p3385_warn_unsupported_attribute) << attribute->getAttrName()->getName();
        return ExprError();
      }
      return Actions.ActOnCXXReflectExpr(OpLoc, attribute);
    }
  }

  if (SS.isSet() &&
      TryAnnotateTypeOrScopeTokenAfterScopeSpec(SS, true,
                                                ImplicitTypenameContext::No)) {
    SkipUntil(tok::semi, StopAtSemi | StopBeforeMatch);
    return ExprError();
  }

  // Anything else must be a type-id (e.g., 'const int', 'Cls(*)(int)'.
  if (isCXXTypeId(TentativeCXXTypeIdContext::AsReflectionOperand)) {
    TypeResult TR = ParseTypeName(nullptr, DeclaratorContext::ReflectOperator);
    if (TR.isInvalid())
      return ExprError();

    std::string refKind;
    if (QualType QT = cast<LocInfoType>(TR.get().get())->getType();
        QT->isLValueReferenceType()) {
      refKind = "&";
    } else if (QT->isRValueReferenceType()) {
      refKind = "&&";
    } else if (auto *FPT = dyn_cast<FunctionProtoType>(QT)) {
      if (FPT->getRefQualifier() == RQ_LValue)
        refKind = "&";
      else if (FPT->getRefQualifier() == RQ_RValue)
        refKind = "&&";
    }

    if (!refKind.empty() &&
        !Tok.isOneOf(tok::r_paren, tok::greater, tok::greatergreater,
                     tok::comma, tok::r_brace, tok::r_square, tok::r_splice,
                     tok::semi, tok::ellipsis, tok::colon, tok::question)) {
      TypeLoc TL = cast<LocInfoType>(TR.get().get())
          ->getTypeSourceInfo()->getTypeLoc();

      Diag(OperandLoc, diag::warn_meant_parenthesize_reflection)
        << refKind << TL.getSourceRange();
    }

    return RecordConstevalOnly.RecordAndReturn(
            Actions.ActOnCXXReflectExpr(OpLoc, TR));
  }

  Diag(OperandLoc, diag::err_cannot_reflect_operand);
  return ExprError();
}

/// Parse an interpolator of a token sequence, after the backslash has been
/// seen (but not consumed):
///
///   interpolator:
///     '\' '[' assignment-expression-list ']'
///     '\' '[:' assignment-expression ':]'
///     '\' '{' assignment-expression '}'
///     '\' 'val' '(' assignment-expression ')'
///     '\' 'str' '(' assignment-expression ')'
///
/// The operands are ordinary expressions of the context in which the token
/// sequence appears. Returns true on error, in which case the tokens of the
/// malformed interpolator have been skipped.
bool Parser::ParseTokenSequenceInterpolator(
    unsigned TokenPos,
    SmallVectorImpl<CXXTokenSequenceExpr::Interpolator> &Interpolators,
    SmallVectorImpl<Expr *> &Operands) {
  assert(Tok.is(tok::backslash) && "expected an interpolator");

  CXXTokenSequenceExpr::Interpolator Interp;
  Interp.TokenPos = TokenPos;
  Interp.FirstOperand = Operands.size();
  Interp.NumOperands = 0;
  Interp.BeginLoc = ConsumeToken();

  tok::TokenKind Open;
  bool AllowList = false;
  if (Tok.is(tok::l_square)) {
    Interp.Kind = CXXTokenSequenceExpr::IK_Identifier;
    Open = tok::l_square;
    AllowList = true;
  } else if (Tok.is(tok::l_splice)) {
    Interp.Kind = CXXTokenSequenceExpr::IK_Splice;
    Open = tok::l_splice;
  } else if (Tok.is(tok::l_brace)) {
    Interp.Kind = CXXTokenSequenceExpr::IK_Tokens;
    Open = tok::l_brace;
  } else if (Tok.is(tok::identifier) && NextToken().is(tok::l_paren) &&
             (Tok.getIdentifierInfo()->isStr("val") ||
              Tok.getIdentifierInfo()->isStr("str"))) {
    Interp.Kind = Tok.getIdentifierInfo()->isStr("val")
                      ? CXXTokenSequenceExpr::IK_Value
                      : CXXTokenSequenceExpr::IK_String;
    ConsumeToken();
    Open = tok::l_paren;
  } else {
    Diag(Tok, diag::err_expected_interpolator);
    return true;
  }

  bool Invalid = false;
  if (Open == tok::l_splice) {
    // There is no balanced delimiter tracker for splice brackets.
    SourceLocation LSpliceLoc = ConsumeSplice();
    ExprResult Operand = ParseAssignmentExpression();
    if (Operand.isInvalid()) {
      Invalid = true;
      SkipUntil(tok::r_splice, StopAtSemi | StopBeforeMatch);
    } else {
      Operands.push_back(Operand.get());
    }
    if (Tok.is(tok::r_splice)) {
      Interp.EndLoc = ConsumeSplice();
    } else {
      Diag(Tok, diag::err_expected) << tok::r_splice;
      Diag(LSpliceLoc, diag::note_matching) << tok::l_splice;
      Invalid = true;
    }
  } else {
    BalancedDelimiterTracker T(*this, Open);
    T.consumeOpen();
    do {
      ExprResult Operand = ParseAssignmentExpression();
      if (Operand.isInvalid()) {
        Invalid = true;
        break;
      }
      Operands.push_back(Operand.get());
    } while (AllowList && TryConsumeToken(tok::comma));

    if (Invalid)
      T.skipToEnd();
    else if (T.consumeClose())
      Invalid = true;
    Interp.EndLoc = T.getCloseLocation();
  }

  if (Invalid) {
    Operands.truncate(Interp.FirstOperand);
    return true;
  }

  Interp.NumOperands = Operands.size() - Interp.FirstOperand;
  Interpolators.push_back(Interp);
  return false;
}

/// Parse a token sequence expression, after the '^^' has been consumed:
///
///   token-sequence-expression:
///     '^^' '{' balanced-brace-token-seq[opt] '}'
///
/// Only braces have to be balanced within the sequence. The tokens are not
/// analyzed in any way until the sequence is injected, with the exception of
/// interpolators, whose operands are parsed and analyzed right away.
ExprResult Parser::ParseCXXTokenSequenceExpression(SourceLocation OpLoc) {
  assert(Tok.is(tok::l_brace) && "expected '{'");

  Sema::ConstevalOnlyRecorder RecordConstevalOnly(Actions);

  // Parentheses and brackets need not be balanced in a token sequence, so do
  // not let the tokens consumed below unbalance the enclosing construct.
  unsigned short SavedParenCount = ParenCount;
  unsigned short SavedBracketCount = BracketCount;
  unsigned short SavedBraceCount = BraceCount;
  unsigned short SavedSpliceCount = SpliceCount;
  auto RestoreCounts = llvm::make_scope_exit([&] {
    ParenCount = SavedParenCount;
    BracketCount = SavedBracketCount;
    BraceCount = SavedBraceCount;
    SpliceCount = SavedSpliceCount;
  });

  SourceLocation LBraceLoc = ConsumeBrace();

  SmallVector<Token, 32> Toks;
  SmallVector<CXXTokenSequenceExpr::Interpolator, 4> Interpolators;
  SmallVector<Expr *, 4> Operands;
  unsigned Depth = 0;
  bool Invalid = false;

  while (true) {
    if (Tok.isOneOf(tok::eof, tok::annot_module_begin, tok::annot_module_end,
                    tok::annot_module_include, tok::annot_repl_input_end)) {
      Diag(Tok, diag::err_token_sequence_unterminated);
      Diag(LBraceLoc, diag::note_matching) << tok::l_brace;
      return ExprError();
    }
    if (Tok.is(tok::code_completion)) {
      cutOffParsing();
      return ExprError();
    }

    if (Tok.is(tok::backslash)) {
      if (ParseTokenSequenceInterpolator(Toks.size(), Interpolators, Operands))
        Invalid = true;
      continue;
    }

    if (Tok.is(tok::l_brace)) {
      ++Depth;
    } else if (Tok.is(tok::r_brace)) {
      if (Depth == 0)
        break;
      --Depth;
    } else if (Tok.isAnnotation() && Tok.isNot(tok::annot_token_value)) {
      // The only annotation that can be part of a token sequence is the value
      // of an interpolator of an enclosing token sequence that is being
      // injected. Anything else was produced by a tentative parse of the
      // tokens of the sequence.
      if (!Invalid)
        Diag(Tok, diag::err_token_sequence_annotation_token);
      Invalid = true;
      ConsumeAnyToken();
      continue;
    }

    Toks.push_back(Tok);
    ConsumeAnyToken();
  }

  SourceLocation RBraceLoc = ConsumeBrace();
  if (Invalid)
    return ExprError();

  return RecordConstevalOnly.RecordAndReturn(
      Actions.ActOnCXXTokenSequenceExpr(OpLoc, LBraceLoc, RBraceLoc, Toks,
                                        Interpolators, Operands));
}

ExprResult Parser::ParseCXXMetafunctionExpression() {
  assert(Tok.is(tok::kw___metafunction) && "expected '___metafunction'");
  SourceLocation KwLoc = ConsumeToken();

  // Balance any number of arguments in parens.
  BalancedDelimiterTracker Parens(*this, tok::l_paren);
  if (Parens.expectAndConsume())
    return ExprError();

  SmallVector<Expr *, 2> Args;
  do {
    ExprResult Expr = ParseConstantExpression();
    if (Expr.isInvalid()) {
      Parens.skipToEnd();
      return ExprError();
    }
    Args.push_back(Expr.get());
  } while (TryConsumeToken(tok::comma));

  if (Parens.consumeClose())
    return ExprError();

  SourceLocation LPLoc = Parens.getOpenLocation();
  SourceLocation RPLoc = Parens.getCloseLocation();
  return Actions.ActOnCXXMetafunction(KwLoc, LPLoc, Args, RPLoc);
}

bool Parser::ParseSpliceSpecifier(bool TryParseSpecialization) {
  assert(Tok.is(tok::l_splice) && "expected '[:'");

  BalancedDelimiterTracker SpliceTokens(*this, tok::l_splice);
  if (SpliceTokens.expectAndConsume())
    return true;

  ExprResult ER = ParseConstantExpression();
  if (ER.isInvalid() || ER.get()->containsErrors()) {
    SpliceTokens.skipToEnd();
    return true;
  }
  Expr *Operand = ER.get();

  Token end = Tok;
  if (SpliceTokens.consumeClose())
    return true;

  SourceLocation LSplice = SpliceTokens.getOpenLocation();
  SourceLocation RSplice = SpliceTokens.getCloseLocation();

  SpliceResult SR;
  if (TryParseSpecialization && Tok.is(tok::less)) {
    SourceLocation LAngleLoc, RAngleLoc;

    // 'TArgs' must outlive the 'ASTTemplateArgsPtr' aliasing its storage, and
    // therefore the call to 'ActOnSpliceSpecifier' which reads through it.
    TemplateArgList TArgs;
    if (ParseTemplateIdAfterTemplateName(/*ConsumeLastToken=*/false, LAngleLoc,
                                         TArgs, RAngleLoc,
                                         /*Template=*/nullptr))
      return true;

    ASTTemplateArgsPtr TArgsPtr(TArgs.data(), TArgs.size());
    end = Tok;
    ConsumeToken();

    SR = Actions.ActOnSpliceSpecifier(LSplice, Operand, RSplice, LAngleLoc,
                                      TArgsPtr, RAngleLoc);
  } else {
    SR = Actions.ActOnSpliceSpecifier(LSplice, Operand, RSplice);
  }
  if (SR.isInvalid())
    return true;
  SpliceSpecifier *Splice = SR.get();

  UnconsumeToken(end);
  Tok.setKind(tok::annot_splice);
  setSpliceAnnotation(Tok, Splice);
  Tok.setLocation(Splice->getBeginLoc());
  Tok.setAnnotationEndLoc(Splice->getEndLoc());
  PP.AnnotateCachedTokens(Tok);

  return false;
}

ExprResult Parser::ParseCXXSpliceAsExpr(SourceLocation TemplateKWLoc,
                                        bool AllowMemberReference) {
  assert(Tok.is(tok::annot_splice) && "expected a splice annotation");

  SpliceResult SR = getSpliceAnnotation(Tok);
  if (SR.isInvalid())
    return ExprError();
  SpliceSpecifier *Splice = SR.get();

  assert((!Splice->isSpecialization() || TemplateKWLoc.isValid()) &&
         "splice-specialization-specifier required leading 'template'");
  ConsumeAnnotationToken();

  return Actions.ActOnCXXSpliceExpression(TemplateKWLoc, Splice,
                                          AllowMemberReference);
}

TypeResult Parser::ParseCXXSpliceAsType(SourceLocation TypenameKWLoc,
                                        bool AllowDependent, bool Complain) {
  assert(Tok.is(tok::annot_splice) && "expected a splice annotation");

  SpliceResult SR = getSpliceAnnotation(Tok);
  if (SR.isInvalid())
    return TypeError();
  SpliceSpecifier *Splice = SR.get();

  TypeResult Result = Actions.ActOnCXXSpliceTypeSpecifier(TypenameKWLoc,
                                                          Splice, Complain);
  if (!Result.isInvalid())
    ConsumeAnnotationToken();

  return Result;
}

DeclResult Parser::ParseCXXSpliceAsNamespace() {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  SpliceResult SR = getSpliceAnnotation(Tok);
  if (SR.isInvalid())
    return DeclError();
  SpliceSpecifier *Splice = SR.get();

  assert(!Splice->isSpecialization() &&
         "splice-specialization-specifier cannot represent a namespace");
  ConsumeAnnotationToken();

  return Actions.ActOnCXXSpliceExpectingNamespace(Splice);
}
