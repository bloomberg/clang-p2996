//===--- ParseInject.cpp - Injection of token sequences -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the parsing of injected token sequences (P3294), i.e.,
// the parser side of 'std::meta::queue_injection' and of
// 'std::meta::namespace_inject'.
//
// Injected tokens are pushed in front of the current token, followed by an
// 'eof' token that marks the end of the injection. They are then parsed by
// the very loop that parses the declarations (or statements) of the context
// they are injected into.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/TokenSequence.h"
#include "clang/Basic/DiagnosticParse.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/CXXFieldCollector.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/ScopeExit.h"
#include <stack>
using namespace clang;

void Parser::EnterInjectedTokens(const TokenSequence *Tokens,
                                 const void *EofTag) {
  ArrayRef<Token> Toks = Tokens->tokens();
  auto Buffer = std::make_unique<Token[]>(Toks.size() + 2);

  for (unsigned I = 0; I < Toks.size(); ++I) {
    Token T = Toks[I];
    // The spelling of a string literal that was synthesized (rather than
    // lexed from a source file) has to be given a source location of its own,
    // for the sake of whoever needs the location of one of its characters.
    if (T.is(tok::string_literal) && T.stringifiedInMacro()) {
      SourceLocation Loc = T.getLocation();
      PP.CreateString(StringRef(T.getLiteralData(), T.getLength()), T, Loc,
                      Loc);
    }
    Buffer[I] = T;
  }

  // The end of the injected tokens.
  Token &Eof = Buffer[Toks.size()];
  Eof.startToken();
  Eof.setKind(tok::eof);
  Eof.setLocation(Tok.getLocation());
  Eof.setEofData(EofTag);

  // The current token, to be seen again after the injected ones.
  Buffer[Toks.size() + 1] = Tok;

  PP.EnterTokenStream(std::move(Buffer), Toks.size() + 2,
                      /*DisableMacroExpansion=*/true, /*IsReinject=*/true);

  // Fetch the first of the injected tokens. Do not go through the 'Consume'
  // methods: the token being replaced has not been consumed.
  PP.Lex(Tok);
}

void Parser::StartTokenInjection(const TokenSequence *Tokens,
                                 SourceLocation Loc, AccessSpecifier AS) {
  const void *EofTag =
      reinterpret_cast<const void *>(uintptr_t(++NumTokenInjections));
  ActiveTokenInjections.push_back(
      {EofTag, getCurScope(), AS, Actions.NextTokenInjectionSeq});
  Actions.pushTokenInjectionContext(Loc);
  EnterInjectedTokens(Tokens, EofTag);
}

bool Parser::HandleTokenInjectionBoundarySlow(TokenInjectionKind Kind,
                                              AccessSpecifier *AS) {
  Scope *Owner = getCurScope();
  unsigned MinSeq = 0;

  if (!ActiveTokenInjections.empty() &&
      ActiveTokenInjections.back().Owner == Owner) {
    // While these tokens are being parsed, only the injections requested
    // by the tokens themselves (e.g., by a consteval block among them) are
    // started, right after the declaration requesting them. Older requests
    // wait for the end of this injection.
    MinSeq = ActiveTokenInjections.back().StartSeq;

    // A closing brace that does not match anything in the injected tokens
    // must not end the scope they are injected into.
    if (Tok.is(tok::r_brace)) {
      Diag(Tok, diag::err_injected_tokens_extraneous)
          << static_cast<unsigned>(Kind);
      while (Tok.isNot(tok::eof))
        PP.Lex(Tok);
    }

    if (isAtEndOfTokenInjection()) {
      // Get back the token that was current when the injection started.
      PP.Lex(Tok);
      if (AS)
        *AS = ActiveTokenInjections.back().SavedAS;
      ActiveTokenInjections.pop_back();
      Actions.popCodeSynthesisContext();
      return true;
    }
  }

  // Tokens may have been queued for this context by the declaration that was
  // just parsed.
  if (auto Pending =
          Actions.takePendingTokenInjection(Actions.CurContext, MinSeq)) {
    StartTokenInjection(Pending->Tokens, Pending->Loc, AS ? *AS : AS_none);
    return true;
  }

  // In between two top-level declarations, no namespace is open: whatever
  // is still queued for a namespace (by the instantiation of a consteval
  // block) will not be picked up by the loop parsing that namespace.
  if (Kind == TokenInjectionKind::Declaration && Owner == Actions.TUScope &&
      ActiveTokenInjections.empty()) {
    if (auto Pending = Actions.takePendingNamespaceTokenInjection()) {
      InjectTokensIntoNamespace(Pending->Target, Pending->Tokens,
                                Pending->Loc);
      return true;
    }
  }

  return false;
}

/// Sets aside the state of the parser (and the part of the state of Sema that
/// the parser is responsible for), which may be in the middle of anything,
/// and restores it on destruction.
class Parser::ReentrantParseRAII {
  Parser &P;
  GreaterThanIsOperatorScope GreaterThan;
  ColonProtectionRAIIObject ColonProtection;
  InMessageExpressionRAIIObject InMessage;

  SourceLocation SavedPrevTokLocation;
  unsigned SavedTemplateParameterDepth;
  unsigned short SavedParenCount, SavedBracketCount, SavedBraceCount,
      SavedSpliceCount;
  decltype(Parser::TemplateIds) SavedTemplateIds;
  decltype(Parser::ClassStack) SavedClassStack;
  AngleBracketTracker SavedAngleBrackets;
  Scope *SavedScope;

public:
  /// 'StartScope' is the scope to start from.
  ReentrantParseRAII(Parser &P, Scope *StartScope)
      : P(P), GreaterThan(P.GreaterThanIsOperator, true),
        ColonProtection(P, false), InMessage(P, false),
        SavedPrevTokLocation(P.PrevTokLocation),
        SavedTemplateParameterDepth(P.TemplateParameterDepth),
        SavedParenCount(P.ParenCount), SavedBracketCount(P.BracketCount),
        SavedBraceCount(P.BraceCount), SavedSpliceCount(P.SpliceCount),
        SavedScope(P.Actions.CurScope) {
    P.TemplateParameterDepth = 0;
    P.ParenCount = P.BracketCount = P.BraceCount = P.SpliceCount = 0;
    std::swap(SavedTemplateIds, P.TemplateIds);
    std::swap(SavedClassStack, P.ClassStack);
    std::swap(SavedAngleBrackets, P.AngleBrackets);
    P.Actions.CurScope = StartScope;
  }

  ~ReentrantParseRAII() {
    P.Actions.CurScope = SavedScope;
    P.DestroyTemplateIds();
    std::swap(SavedTemplateIds, P.TemplateIds);
    std::swap(SavedClassStack, P.ClassStack);
    std::swap(SavedAngleBrackets, P.AngleBrackets);
    P.TemplateParameterDepth = SavedTemplateParameterDepth;
    P.ParenCount = SavedParenCount;
    P.BracketCount = SavedBracketCount;
    P.BraceCount = SavedBraceCount;
    P.SpliceCount = SavedSpliceCount;
    P.PrevTokLocation = SavedPrevTokLocation;
  }
};

void Parser::ParseInjectedDeclarations(const TokenSequence *Tokens,
                                       SourceLocation Loc,
                                       bool HandToConsumer) {
  unsigned Depth = ActiveTokenInjections.size();
  StartTokenInjection(Tokens, Loc, AS_none);
  while (ActiveTokenInjections.size() > Depth) {
    if (HandleTokenInjectionBoundary(TokenInjectionKind::Declaration))
      continue;
    if (Tok.is(tok::eof)) {
      // Not our 'eof': give up on what is left of the tokens.
      ActiveTokenInjections.pop_back();
      Actions.popCodeSynthesisContext();
      break;
    }

    ParsedAttributes DeclAttrs(AttrFactory);
    MaybeParseCXX11Attributes(DeclAttrs);
    ParsedAttributes EmptyDeclSpecAttrs(AttrFactory);
    DeclGroupPtrTy Group =
        ParseExternalDeclaration(DeclAttrs, EmptyDeclSpecAttrs);
    if (Group && HandToConsumer)
      Actions.getASTConsumer().HandleTopLevelDecl(Group.get());
  }
}

void Parser::ParseInjectedMembers(CXXRecordDecl *RD,
                                  const TokenSequence *Tokens,
                                  SourceLocation Loc, AccessSpecifier AS) {
  DeclSpec::TST TagType = DeclSpec::TST_struct;
  switch (RD->getTagKind()) {
  case TagTypeKind::Class:
    TagType = DeclSpec::TST_class;
    break;
  case TagTypeKind::Union:
    TagType = DeclSpec::TST_union;
    break;
  case TagTypeKind::Interface:
    TagType = DeclSpec::TST_interface;
    break;
  default:
    break;
  }
  if (AS == AS_none)
    AS = RD->isClass() ? AS_private : AS_public;

  ParsedAttributes AccessAttrs(AttrFactory);
  unsigned Depth = ActiveTokenInjections.size();
  StartTokenInjection(Tokens, Loc, AS);
  while (ActiveTokenInjections.size() > Depth) {
    if (HandleTokenInjectionBoundary(TokenInjectionKind::Member, &AS))
      continue;
    if (Tok.is(tok::eof)) {
      ActiveTokenInjections.pop_back();
      Actions.popCodeSynthesisContext();
      break;
    }

    ParseCXXClassMemberDeclarationWithPragmas(AS, AccessAttrs, TagType, RD);
    MaybeDestroyTemplateIds();
  }
}

Sema::TokenInjectionStatus
Parser::TokenInjectionCallback(void *P, DeclContext *NS,
                               const TokenSequence *Tokens,
                               SourceLocation Loc) {
  return static_cast<Parser *>(P)->InjectTokensIntoNamespace(NS, Tokens, Loc);
}

Sema::TokenInjectionStatus
Parser::ClassTokenInjectionCallback(void *P, CXXRecordDecl *RD,
                                    const TokenSequence *Tokens,
                                    SourceLocation Loc, AccessSpecifier AS) {
  return static_cast<Parser *>(P)->InjectTokensIntoClass(RD, Tokens, Loc, AS);
}

void Parser::FinishClassTokenInjectionCallback(void *P, CXXRecordDecl *RD) {
  static_cast<Parser *>(P)->FinishTokenInjectionIntoClass(RD);
}

Sema::TokenInjectionStatus
Parser::InjectTokensIntoNamespace(DeclContext *NS, const TokenSequence *Tokens,
                                  SourceLocation Loc) {
  assert(NS->isFileContext() && "expected a namespace");

  // Tokens that are parsed now would be parsed again, were the parser to
  // backtrack.
  if (PP.isBacktrackEnabled())
    return Sema::TokenInjectionStatus::UnsupportedParserState;

  // Find the innermost namespace enclosing the target (or the target itself)
  // that is currently being parsed, and the namespaces to reopen from there.
  SmallVector<NamespaceDecl *, 4> ToReopen;
  Scope *OpenScope = nullptr;
  for (DeclContext *DC = NS; !OpenScope;) {
    for (Scope *S = getCurScope(); S && !OpenScope; S = S->getParent())
      if (DeclContext *Entity = S->getEntity();
          Entity && Entity->getPrimaryContext() == DC->getPrimaryContext())
        OpenScope = S;
    if (OpenScope)
      break;

    auto *ND = dyn_cast<NamespaceDecl>(DC);
    if (!ND)
      return Sema::TokenInjectionStatus::UnsupportedParserState;
    ToReopen.push_back(ND);
    DC = ND->getParent()->getRedeclContext();
  }
  DeclContext *OpenCtx = OpenScope->getEntity();

  // Declarations of a namespace that is still open reach the AST consumer
  // along with that namespace. Anything else has to be handed over here.
  bool HandToConsumer = OpenCtx->isTranslationUnit();

  DiagnosticsEngine &Diags = PP.getDiagnostics();
  unsigned NumErrorsBefore = Diags.getNumErrors();

  {
    ReentrantParseRAII Reentrant(*this, OpenScope);

    // The injected declarations are not part of whatever is being analyzed
    // at the point of injection.
    Sema::ContextRAII SavedContext(Actions, OpenCtx);
    EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);
    Sema::FPFeaturesStateRAII SavedFPFeatures(Actions);

    // Parse in a scope of its own: the declarations must not end up in front
    // of the declarations of the scopes nested in the open namespace when
    // names are looked up from those.
    ParseScope InjectionScope(this, Scope::DeclScope);
    getCurScope()->setEntity(OpenCtx);

    // Reopen the namespaces leading to the target, outermost first.
    auto ParseInNamespace = [&](auto &&Self, unsigned Level) -> void {
      if (Level == 0) {
        ParseInjectedDeclarations(Tokens, Loc,
                                  HandToConsumer && ToReopen.empty());
        return;
      }

      NamespaceDecl *ND = ToReopen[Level - 1];
      ParseScope NamespaceScope(this, Scope::DeclScope);
      ParsedAttributes Attrs(AttrFactory);
      UsingDirectiveDecl *ImplicitUsingDirective = nullptr;
      Decl *Reopened = Actions.ActOnStartNamespaceDef(
          getCurScope(), ND->isInline() ? Loc : SourceLocation(), Loc, Loc,
          ND->getIdentifier(), Loc, Attrs, ImplicitUsingDirective,
          /*IsNested=*/false);
      if (!Reopened)
        return;

      Self(Self, Level - 1);

      NamespaceScope.Exit();
      Actions.ActOnFinishNamespaceDef(Reopened, Loc);

      if (HandToConsumer && Level == ToReopen.size())
        Actions.getASTConsumer().HandleTopLevelDecl(
            Actions.ConvertDeclToDeclGroup(Reopened).get());
    };
    ParseInNamespace(ParseInNamespace, ToReopen.size());
  }

  return Diags.getNumErrors() == NumErrorsBefore
             ? Sema::TokenInjectionStatus::Success
             : Sema::TokenInjectionStatus::Failed;
}

/// Enters scopes for the contexts enclosing 'RD' and for 'RD' itself,
/// starting from the scope of the translation unit. Returns false if that is
/// not possible (i.e., for a local class).
bool Parser::EnterScopesOfInjectedClass(MultiParseScope &Scopes,
                                        CXXRecordDecl *RD) {
  SmallVector<DeclContext *, 4> Contexts;
  for (DeclContext *DC = RD; DC && !DC->isTranslationUnit();
       DC = DC->getLexicalParent()) {
    if (DC->isFunctionOrMethod())
      return false;
    if (DC->isFileContext() || DC->isRecord())
      Contexts.push_back(DC);
  }

  for (DeclContext *DC : llvm::reverse(Contexts)) {
    Scopes.Enter(DC->isRecord() ? Scope::ClassScope | Scope::DeclScope
                                : Scope::DeclScope);
    getCurScope()->setEntity(DC);
  }
  return true;
}

Sema::TokenInjectionStatus
Parser::InjectTokensIntoClass(CXXRecordDecl *RD, const TokenSequence *Tokens,
                              SourceLocation Loc, AccessSpecifier AS) {
  if (PP.isBacktrackEnabled())
    return Sema::TokenInjectionStatus::UnsupportedParserState;

  DiagnosticsEngine &Diags = PP.getDiagnostics();
  unsigned NumErrorsBefore = Diags.getNumErrors();

  {
    ReentrantParseRAII Reentrant(*this, Actions.TUScope);
    MultiParseScope Scopes(*this);
    if (!EnterScopesOfInjectedClass(Scopes, RD))
      return Sema::TokenInjectionStatus::UnsupportedParserState;

    Sema::ContextRAII SavedContext(Actions, RD);
    EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);
    Sema::FPFeaturesStateRAII SavedFPFeatures(Actions);

    // The parts of the members that have to wait for the class to be
    // complete are collected here, to be parsed once the instantiation of
    // the class is over.
    ParsingClass *&Class = InjectedParsingClasses[RD];
    if (!Class)
      Class = new ParsingClass(RD, /*TopLevelClass=*/true,
                               /*IsInterface=*/RD->isInterface());
    ClassStack.push(Class);
    Sema::ParsingClassState State = Actions.PushParsingClass();
    // Sema collects the fields of a class that is being parsed.
    Actions.FieldCollector->StartClass();

    ParseInjectedMembers(RD, Tokens, Loc, AS);

    Actions.FieldCollector->FinishClass();
    Actions.PopParsingClass(State);
    ClassStack.pop();
  }

  return Diags.getNumErrors() == NumErrorsBefore
             ? Sema::TokenInjectionStatus::Success
             : Sema::TokenInjectionStatus::Failed;
}

void Parser::FinishTokenInjectionIntoClass(CXXRecordDecl *RD) {
  auto It = InjectedParsingClasses.find(RD);
  if (It == InjectedParsingClasses.end())
    return;
  ParsingClass *Class = It->second;
  InjectedParsingClasses.erase(It);

  {
    ReentrantParseRAII Reentrant(*this, Actions.TUScope);
    MultiParseScope Scopes(*this);
    if (EnterScopesOfInjectedClass(Scopes, RD)) {
      Sema::ContextRAII SavedContext(Actions, RD);
      EnterExpressionEvaluationContext EvalContext(
          Actions, Sema::ExpressionEvaluationContext::PotentiallyEvaluated);
      Sema::FPFeaturesStateRAII SavedFPFeatures(Actions);

      ClassStack.push(Class);
      Sema::ParsingClassState State = Actions.PushParsingClass();

      // As at the end of a member-specification.
      ParseLexedPragmas(*Class);
      ParseLexedAttributes(*Class);
      ParseLexedMethodDeclarations(*Class);
      Actions.ActOnFinishCXXMemberDecls();
      ParseLexedMemberInitializers(*Class);
      ParseLexedMethodDefs(*Class);

      Actions.PopParsingClass(State);
      ClassStack.pop();
    }
  }

  DeallocateParsedClasses(Class);
}
