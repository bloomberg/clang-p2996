//===--- Metafunctions.cpp - Functions targeting reflections ----*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements all metafunctions from the <experimental/meta> header.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Reflection.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Metafunction.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/TemplateDeduction.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/UnicodeCharRanges.h"
#include "llvm/Support/raw_ostream.h"


namespace clang {

using EvalFn = Metafunction::EvaluateFn;

// -----------------------------------------------------------------------------
// P2996 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_begin_enumerator_decl_of(APValue &Result, Sema &S,
                      EvalFn Evaluator, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool get_next_enumerator_decl_of(APValue &Result, Sema &S,
                                EvalFn Evaluator, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool get_ith_base_of(APValue &Result, Sema &S,
                            EvalFn Evaluator, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args);

static bool get_ith_template_argument_of(APValue &Result, Sema &S,
                                         EvalFn Evaluator, QualType ResultTy,
                                         SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool get_begin_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                     QualType ResultTy, SourceRange Range,
                                     ArrayRef<Expr *> Args);

static bool get_next_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args);

static bool map_decl_to_entity(APValue &Result, Sema &S, EvalFn Evaluator,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args);

static bool name_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool display_name_of(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args);

static bool source_location_of(APValue &Result, Sema &S, EvalFn Evaluator,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args);

static bool type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool parent_of(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool dealias(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool value_of(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool template_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool can_substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

static bool substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool extract(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_public(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool is_protected(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_private(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool access_context(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

static bool is_accessible(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args);

static bool is_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_pure_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args);

static bool is_override(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_deleted(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_defaulted(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_explicit(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_noexcept(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_bit_field(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool has_static_storage_duration(APValue &Result, Sema &S,
                                        EvalFn Evaluator, QualType ResultTy,
                                        SourceRange Range,
                                        ArrayRef<Expr *> Args);

static bool has_internal_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool has_module_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args);

static bool has_external_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool has_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_class_member(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args);

static bool is_namespace_member(APValue &Result, Sema &S, EvalFn Evaluator,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args);

static bool is_nonstatic_data_member(APValue &Result, Sema &S,
                                     EvalFn Evaluator, QualType ResultTy,
                                     SourceRange Range,
                                     ArrayRef<Expr *> Args);

static bool is_static_member(APValue &Result, Sema &S, EvalFn Evaluator,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args);

static bool is_base(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_namespace(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_function(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_variable(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_type(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_alias(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool is_incomplete_type(APValue &Result, Sema &S, EvalFn Evaluator,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args);

static bool is_template(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool is_variable_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool is_class_template(APValue &Result, Sema &S, EvalFn Evaluator,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args);

static bool is_alias_template(APValue &Result, Sema &S, EvalFn Evaluator,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args);

static bool is_concept(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_structured_binding(APValue &Result, Sema &S, EvalFn Evaluator,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args);

static bool is_value(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool is_object(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool has_template_arguments(APValue &Result, Sema &S, EvalFn Evaluator,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args);

static bool is_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

static bool is_destructor(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args);

static bool is_special_member(APValue &Result, Sema &S, EvalFn Evaluator,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args);

static bool is_user_provided(APValue &Result, Sema &S, EvalFn Evaluator,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args);

static bool reflect_result(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

static bool reflect_invoke(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

static bool data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args);

static bool define_class(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool offset_of(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool size_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool bit_offset_of(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args);

static bool bit_size_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool alignment_of(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

// -----------------------------------------------------------------------------
// P3096 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_ith_parameter_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool has_consistent_name(APValue &Result, Sema &S, EvalFn Evaluator,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args);

static bool has_ellipsis_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args);

static bool has_default_argument(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args);

static bool is_explicit_object_parameter(APValue &Result, Sema &S,
                                         EvalFn Evaluator, QualType ResultTy,
                                         SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool is_function_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args);

static bool return_type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args);

// -----------------------------------------------------------------------------
// Metafunction table
//
// Order of entries MUST be kept in sync with order of declarations in the
//   <experimental/meta>
// header file.
// -----------------------------------------------------------------------------

static constexpr Metafunction Metafunctions[] = {
  // Kind, MinArgs, MaxArgs, Impl

  // non-exposed metafunctions
  { Metafunction::MFRK_metaInfo, 2, 2, get_begin_enumerator_decl_of },
  { Metafunction::MFRK_metaInfo, 2, 2, get_next_enumerator_decl_of },
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_base_of },
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_template_argument_of },
  { Metafunction::MFRK_metaInfo, 2, 2, get_begin_member_decl_of },
  { Metafunction::MFRK_metaInfo, 2, 2, get_next_member_decl_of },
  { Metafunction::MFRK_metaInfo, 1, 1, map_decl_to_entity },

  // exposed metafunctions
  { Metafunction::MFRK_spliceFromArg, 4, 4, name_of },
  { Metafunction::MFRK_spliceFromArg, 3, 3, display_name_of },
  { Metafunction::MFRK_sourceLoc, 1, 1, source_location_of },
  { Metafunction::MFRK_metaInfo, 1, 1, type_of },
  { Metafunction::MFRK_metaInfo, 1, 1, parent_of },
  { Metafunction::MFRK_metaInfo, 1, 1, dealias },
  { Metafunction::MFRK_metaInfo, 1, 1, value_of },
  { Metafunction::MFRK_metaInfo, 1, 1, template_of },
  { Metafunction::MFRK_bool, 3, 3, can_substitute },
  { Metafunction::MFRK_metaInfo, 3, 3, substitute },
  { Metafunction::MFRK_spliceFromArg, 2, 2, extract },
  { Metafunction::MFRK_bool, 1, 1, is_public },
  { Metafunction::MFRK_bool, 1, 1, is_protected },
  { Metafunction::MFRK_bool, 1, 1, is_private },
  { Metafunction::MFRK_bool, 1, 2, is_accessible },
  { Metafunction::MFRK_bool, 1, 1, is_virtual },
  { Metafunction::MFRK_bool, 1, 1, is_pure_virtual },
  { Metafunction::MFRK_bool, 1, 1, is_override },
  { Metafunction::MFRK_bool, 1, 1, is_deleted },
  { Metafunction::MFRK_bool, 1, 1, is_defaulted },
  { Metafunction::MFRK_bool, 1, 1, is_explicit },
  { Metafunction::MFRK_bool, 1, 1, is_noexcept },
  { Metafunction::MFRK_bool, 1, 1, is_bit_field },
  { Metafunction::MFRK_bool, 1, 1, has_static_storage_duration },
  { Metafunction::MFRK_bool, 1, 1, has_internal_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_module_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_external_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_linkage },
  { Metafunction::MFRK_bool, 1, 1, is_class_member },
  { Metafunction::MFRK_bool, 1, 1, is_namespace_member },
  { Metafunction::MFRK_bool, 1, 1, is_nonstatic_data_member },
  { Metafunction::MFRK_bool, 1, 1, is_static_member },
  { Metafunction::MFRK_bool, 1, 1, is_base },
  { Metafunction::MFRK_bool, 1, 1, is_namespace },
  { Metafunction::MFRK_bool, 1, 1, is_function },
  { Metafunction::MFRK_bool, 1, 1, is_variable },
  { Metafunction::MFRK_bool, 1, 1, is_type },
  { Metafunction::MFRK_bool, 1, 1, is_alias },
  { Metafunction::MFRK_bool, 1, 1, is_incomplete_type },
  { Metafunction::MFRK_bool, 1, 1, is_template },
  { Metafunction::MFRK_bool, 1, 1, is_function_template },
  { Metafunction::MFRK_bool, 1, 1, is_variable_template },
  { Metafunction::MFRK_bool, 1, 1, is_class_template },
  { Metafunction::MFRK_bool, 1, 1, is_alias_template },
  { Metafunction::MFRK_bool, 1, 1, is_concept },
  { Metafunction::MFRK_bool, 1, 1, is_structured_binding },
  { Metafunction::MFRK_bool, 1, 1, is_value },
  { Metafunction::MFRK_bool, 1, 1, is_object },
  { Metafunction::MFRK_bool, 1, 1, has_template_arguments },
  { Metafunction::MFRK_bool, 1, 1, is_constructor },
  { Metafunction::MFRK_bool, 1, 1, is_destructor },
  { Metafunction::MFRK_bool, 1, 1, is_special_member },
  { Metafunction::MFRK_bool, 1, 1, is_user_provided },
  { Metafunction::MFRK_metaInfo, 2, 2, reflect_result },
  { Metafunction::MFRK_metaInfo, 5, 5, reflect_invoke },
  { Metafunction::MFRK_metaInfo, 10, 10, data_member_spec },
  { Metafunction::MFRK_metaInfo, 3, 3, define_class },
  { Metafunction::MFRK_sizeT, 1, 1, offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, size_of },
  { Metafunction::MFRK_sizeT, 1, 1, bit_offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, bit_size_of },
  { Metafunction::MFRK_sizeT, 1, 1, alignment_of },

  // Proposed alternative P2996 accessibility API
  { Metafunction::MFRK_metaInfo, 0, 0, access_context },

  // P3096 metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_parameter_of },
  { Metafunction::MFRK_bool, 1, 1, has_consistent_name },
  { Metafunction::MFRK_bool, 1, 1, has_ellipsis_parameter },
  { Metafunction::MFRK_bool, 1, 1, has_default_argument },
  { Metafunction::MFRK_bool, 1, 1, is_explicit_object_parameter },
  { Metafunction::MFRK_bool, 1, 1, is_function_parameter },
  { Metafunction::MFRK_metaInfo, 1, 1, return_type_of },
};
constexpr const unsigned NumMetafunctions = sizeof(Metafunctions) /
                                            sizeof(Metafunction);


// -----------------------------------------------------------------------------
// class Metafunction implementation
// -----------------------------------------------------------------------------

bool Metafunction::evaluate(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args) const {
  return ImplFn(Result, S, Evaluator, ResultTy, Range, Args);
}

bool Metafunction::Lookup(unsigned ID, const Metafunction *&result) {
  if (ID >= NumMetafunctions)
    return true;

  result = &Metafunctions[ID];
  return false;
}


// -----------------------------------------------------------------------------
// Metafunction helper functions
// -----------------------------------------------------------------------------

static bool isBasicCharacter(uint32_t c) {
  static llvm::sys::UnicodeCharRange BasicCharRanges[] = {
    {0x0009, 0x0009}, {0x000A, 0x000C}, {0x0020, 0x007E},
  };
  static llvm::sys::UnicodeCharSet BasicCharSet(BasicCharRanges);

  return BasicCharSet.contains(c);
}

static APValue makeBool(ASTContext &C, bool B) {
  return APValue(C.MakeIntValue(B, C.BoolTy));
}

static APValue makeReflection(QualType QT) {
  return APValue(ReflectionValue::RK_type, QT.getAsOpaquePtr());
}

static APValue makeReflection(const Decl *D) {
  if (isa<NamespaceDecl>(D) || isa<NamespaceAliasDecl>(D) ||
      isa<TranslationUnitDecl>(D))
    return APValue(ReflectionValue::RK_namespace, D);

  return APValue(ReflectionValue::RK_declaration, D);
}

static APValue makeReflection(TemplateName TName) {
  return APValue(ReflectionValue::RK_template, TName.getAsVoidPointer());
}

static APValue makeReflection(CXXBaseSpecifier *Base) {
  return APValue(ReflectionValue::RK_base_specifier, Base);
}

static APValue makeReflection(TagDataMemberSpec *TDMS) {
  return APValue(ReflectionValue::RK_data_member_spec, TDMS);
}

static Expr *makeCString(StringRef Str, ASTContext &C, bool Utf8) {
  QualType ConstCharTy = (Utf8 ? C.Char8Ty : C.CharTy).withConst();

  // Get the type for 'const char[Str.size()]'.
  QualType StrLitTy =
        C.getConstantArrayType(ConstCharTy, llvm::APInt(32, Str.size() + 1),
                               nullptr, ArraySizeModifier::Normal, 0);

  // Create a string literal having type 'const char [Str.size()]'.
  StringLiteralKind SLK = Utf8 ? StringLiteralKind::UTF8 :
                                 StringLiteralKind::Ordinary;
  StringLiteral *StrLit = StringLiteral::Create(C, Str, SLK, false, StrLitTy,
                                                SourceLocation{});

  // Create an expression to implicitly cast the literal to 'const char *'.
  QualType ConstCharPtrTy = C.getPointerType(ConstCharTy);
  return ImplicitCastExpr::Create(C, ConstCharPtrTy, CK_ArrayToPointerDecay,
                                  StrLit, /*BasePath=*/nullptr, VK_PRValue,
                                  FPOptionsOverride());
}

static bool SetAndSucceed(APValue &Out, const APValue &Result) {
  Out = Result;
  return false;
}

static void encodeName(std::string &Result, StringRef In, bool BasicOnly) {
  if (!BasicOnly) {
    Result = In;
  } else {
    Result.reserve(In.size() * 8);  // more than enough for '\u{XYZ}'
    
    llvm::raw_string_ostream OS(Result);
    
    const char *InCursor = In.begin();
    while (InCursor < In.end()) {
      if (isBasicCharacter(*InCursor)) {
        OS << *(InCursor++);
      } else {
        llvm::UTF32 CodePoint;
        if (llvm::conversionOK != llvm::convertUTF8Sequence(
                reinterpret_cast<const llvm::UTF8 **>(&InCursor),
                reinterpret_cast<const llvm::UTF8 *>(In.end()),
                &CodePoint, llvm::ConversionFlags::strictConversion))
          llvm_unreachable("failed converting non-basic character to UCN");

        OS << "\\u{" << llvm::format_hex_no_prefix(CodePoint, 4, true) << "}";
      }
    }
    Result.shrink_to_fit();
  }
}

static TemplateName findTemplateOfDecl(const Decl *D) {
  TemplateDecl *TDecl = nullptr;
  if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
    if (FunctionTemplateSpecializationInfo *Info =
        FD->getTemplateSpecializationInfo())
      TDecl = Info->getTemplate();
  } else if (const auto *VD = dyn_cast<VarDecl>(D)) {
    if (const auto *P = VD->getTemplateInstantiationPattern())
      VD = P;
    TDecl = VD->getDescribedVarTemplate();
  }
  assert(!isa<ClassTemplateSpecializationDecl>(D) &&
         "use findTemplateOfType instead");
  return TDecl ? TemplateName(TDecl) : TemplateName();
}

static TemplateName findTemplateOfType(QualType QT) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  if (auto *TST = dyn_cast<TemplateSpecializationType>(QT))
    return TST->getTemplateName();

  if (auto *CXXRD = QT->getAsCXXRecordDecl())
    if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD))
      return TemplateName(CTSD->getSpecializedTemplate());

  return TemplateName();
}

static void getTemplateName(std::string &Result, ASTContext &C,
                            TemplateName TName, bool BasicOnly) {
  PrintingPolicy PP = C.getPrintingPolicy();

  std::string Name;
  {
    llvm::raw_string_ostream NameOut(Name);
    TName.print(NameOut, PP, TemplateName::Qualified::None);
  }
  encodeName(Result, Name, BasicOnly);
}

static void getTypeName(std::string &Result, ASTContext &C, QualType QT,
                        bool BasicOnly) {
  if (TemplateName TName = findTemplateOfType(QT); !TName.isNull()) {
    return getTemplateName(Result, C, TName, BasicOnly);
  }

  PrintingPolicy PP = C.getPrintingPolicy();
  PP.SuppressTagKeyword = true;
  PP.SuppressScope = true;

  encodeName(Result, QT.getAsString(PP), BasicOnly);
}

static bool parameterHasConsistentName(ParmVarDecl *PVD) {
  StringRef FirstNameSeen = PVD->getName();
  unsigned ParamIdx = PVD->getFunctionScopeIndex();

  while (PVD) {
    FunctionDecl *FD = cast<FunctionDecl>(PVD->getDeclContext());
    FD = FD->getPreviousDecl();
    if (!FD)
      return true;

    PVD = FD->getParamDecl(ParamIdx);
    assert(PVD);
    if (StringRef Name = PVD->getName();
        Name.size() > 0 && Name != FirstNameSeen)
      return false;
  }
  return true;
}

static void getDeclName(std::string &Result, ASTContext &C, Decl *D,
                         bool BasicOnly) {
  if (TemplateName TName = findTemplateOfDecl(D); !TName.isNull())
    return getTemplateName(Result, C, TName, BasicOnly);

  PrintingPolicy PP = C.getPrintingPolicy();

  std::string Name;
  {
    llvm::raw_string_ostream NameOut(Name);
    if (auto *ND = dyn_cast<NamedDecl>(D);
        ND && !isa<TemplateParamObjectDecl>(D))
      ND->printName(NameOut, PP);
  }
  encodeName(Result, Name, BasicOnly);
}

static NamedDecl *findTypeDecl(QualType QT) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  // Get the type's declaration.
  NamedDecl *D = nullptr;
  if (auto *TD = QT->getAsTagDecl())
    return TD;
  else if (auto *TT = dyn_cast<TagType>(QT))
    D = TT->getDecl();
  else if (auto *TDT = dyn_cast<TypedefType>(QT))
    D = TDT->getDecl();
  else if (auto *UT = dyn_cast<UsingType>(QT))
    D = UT->getFoundDecl();
  else if (auto *UUTD = dyn_cast<UnresolvedUsingType>(QT))
    D = UUTD->getDecl();
  else if (auto *TS = dyn_cast<TemplateSpecializationType>(QT)) {
    if (auto *CTD = dyn_cast<ClassTemplateDecl>(
          TS->getTemplateName().getAsTemplateDecl())) {
      void *InsertPos;
      D = CTD->findSpecialization(TS->template_arguments(), InsertPos);
    }
  } else if (auto *STTP = dyn_cast<SubstTemplateTypeParmType>(QT))
    D = findTypeDecl(STTP->getReplacementType());
  else if (auto *ICNT = dyn_cast<InjectedClassNameType>(QT))
    D = ICNT->getDecl();
  else if (auto *DTT = dyn_cast<DecltypeType>(QT))
    D = findTypeDecl(DTT->getUnderlyingType());

  return D;
}

static bool findTypeDeclLoc(APValue &Result, ASTContext &C, EvalFn Evaluator,
                            QualType ResultTy, QualType QT) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  // Get the type's declaration.
  NamedDecl *D = const_cast<NamedDecl *>(findTypeDecl(QT));

  SourceLocExpr *SLE =
          new (C) SourceLocExpr(C, SourceLocIdentKind::SourceLocStruct,
                                ResultTy,
                                D ? D->getLocation() : SourceLocation(),
                                SourceLocation(),
                                D ? D->getDeclContext() : nullptr);

  return !Evaluator(Result, SLE, true);
}

static bool findDeclLoc(APValue &Result, ASTContext &C, EvalFn Evaluator,
                         QualType ResultTy, Decl *D) {
  SourceLocExpr *SLE =
          new (C) SourceLocExpr(C, SourceLocIdentKind::SourceLocStruct,
                                ResultTy,
                                D ? D->getLocation() : SourceLocation(),
                                SourceLocation(),
                                D ? D->getDeclContext() : nullptr);
  return !Evaluator(Result, SLE, true);
}

static bool findBaseSpecLoc(APValue &Result, ASTContext &C, EvalFn Evaluator,
                            QualType ResultTy, CXXBaseSpecifier *B) {
  SourceLocExpr *SLE =
          new (C) SourceLocExpr(C, SourceLocIdentKind::SourceLocStruct,
                                ResultTy, B->getBeginLoc(),
                                SourceLocation(), nullptr);
  return !Evaluator(Result, SLE, true);
}

static QualType desugarType(QualType QT, bool UnwrapAliases, bool DropCV,
                            bool DropRefs) {
  bool IsConst = QT.isConstQualified();
  bool IsVolatile = QT.isVolatileQualified();

  while (true) {
    QT = QualType(QT.getTypePtr(), 0);
    if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
      QT = ET->getNamedType();
    else if (auto *TDT = dyn_cast<TypedefType>(QT); TDT && UnwrapAliases)
      QT = TDT->desugar();
    else if (auto *UT = dyn_cast<UsingType>(QT); TDT && UnwrapAliases)
      QT = UT->desugar();
    else if (auto *TST = dyn_cast<TemplateSpecializationType>(QT);
             TST && UnwrapAliases && TST->isTypeAlias())
      QT = TST->getAliasedType();
    else if (auto *AT = dyn_cast<AutoType>(QT))
      QT = AT->desugar();
    else if (auto *RT = dyn_cast<ReferenceType>(QT); RT && DropRefs)
      QT = RT->getPointeeType();
    else if (auto *STTP = dyn_cast<SubstTemplateTypeParmType>(QT))
      QT = STTP->getReplacementType();
    else
      break;
  }

  if (!DropCV) {
    if (IsConst)
      QT = QT.withConst();
    if (IsVolatile)
      QT = QT.withVolatile();
  }
  return QT;
}

static bool isTypeAlias(QualType QT) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  // If it's a TypedefType, it's an alias.
  return QT->isTypedefNameType();
}

static void expandTemplateArgPacks(ArrayRef<TemplateArgument> Args,
                                   SmallVectorImpl<TemplateArgument> &Out) {
  for (const TemplateArgument &Arg : Args)
    if (Arg.getKind() == TemplateArgument::Pack)
      for (const TemplateArgument &TA : Arg.getPackAsArray())
        Out.push_back(TA);
    else
      Out.push_back(Arg);
}

bool getTemplateArgumentsFromType(QualType QT,
                                  SmallVectorImpl<TemplateArgument> &Out) {
  // Obtain the template arguments from the Type* representation
  if (auto asTmplSpecialization = QT->getAs<TemplateSpecializationType>())
    expandTemplateArgPacks(asTmplSpecialization->template_arguments(), Out);
  else if (auto DTST = QT->getAs<DependentTemplateSpecializationType>())
    expandTemplateArgPacks(DTST->template_arguments(), Out);
  else if (auto *CTSD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
        QT->getAsRecordDecl()))
    expandTemplateArgPacks(CTSD->getTemplateArgs().asArray(), Out);
  else
    return true;

  return false;
}

bool getTemplateArgumentsFromDeclaration(Decl* D,
                                         SmallVectorImpl<TemplateArgument> &Out) {
  if (auto FD = dyn_cast<FunctionDecl>(D)) {
    if (auto templArgs = FD->getTemplateSpecializationArgs()) {
      expandTemplateArgPacks(templArgs->asArray(), Out);
      return false;
    }
  } else if (auto VTSD = dyn_cast<VarTemplateSpecializationDecl>(D)) {
    expandTemplateArgPacks(VTSD->getTemplateArgs().asArray(), Out);
    return false;
  }
  return true;
}

static APValue getNthTemplateArgument(Sema &S,
                                      ArrayRef<TemplateArgument> templateArgs,
                                      EvalFn Evaluator, APValue Sentinel,
                                      size_t Idx) {
  if (Idx >= templateArgs.size()) {
    return Sentinel;
  }

  const auto& templArgument = templateArgs[Idx];
  switch (templArgument.getKind()) {
    // Works for type parameters pack as well
    case TemplateArgument::Type:
      return makeReflection(templArgument.getAsType());
    // Works for non-template parameters and parameter packs of types:
    // int, pointers
    case TemplateArgument::Expression: {
      Expr *TExpr = templArgument.getAsExpr();

      APValue ArgResult;
      bool success = Evaluator(ArgResult, TExpr, !TExpr->isLValue());
      assert(success);

      if (ArgResult.isReflection())
        return APValue(ArgResult.getReflection().getKind(),
                       ArgResult.getReflection().getOpaqueValue());

      ConstantExpr *CE =
          ConstantExpr::CreateEmpty(S.Context,
                                    ConstantResultStorageKind::APValue);
      CE->setType(TExpr->getType());
      CE->setValueKind(TExpr->getValueKind());
      CE->SetResult(ArgResult, S.Context);

      return APValue(ReflectionValue::RK_expr_result, CE);
    }
    case TemplateArgument::Template:
      return makeReflection(templArgument.getAsTemplate());
    case TemplateArgument::Reflection: {
      const ReflectionValue& asReflection = templArgument.getAsReflection();
      return APValue(asReflection.getKind(), asReflection.getOpaqueValue());
    }
    case TemplateArgument::Declaration:
      return makeReflection(templArgument.getAsDecl());
    case TemplateArgument::Pack:
      llvm_unreachable("Packs should be expanded before calling this");

    // Could not get a test case to hit one of the below
    case TemplateArgument::Null:
      llvm_unreachable("TemplateArgument::Null not supported");
    case TemplateArgument::NullPtr:
      llvm_unreachable("TemplateArgument::NullPtr not supported");
    case TemplateArgument::StructuralValue: {
      QualType QT = templArgument.getStructuralValueType();
      ExprValueKind VK = VK_PRValue;
      if (auto *RT = dyn_cast<ReferenceType>(QT)) {
        QT = RT->getPointeeType();
        VK = VK_LValue;
      }

      ConstantExpr *CE =
          ConstantExpr::CreateEmpty(S.Context,
                                    ConstantResultStorageKind::APValue);
      CE->setType(QT);
      CE->setValueKind(VK);
      CE->SetResult(templArgument.getAsStructuralValue(), S.Context);

      return APValue(ReflectionValue::RK_expr_result, CE);
    }
    case TemplateArgument::Integral: {
      ConstantExpr *CE =
          ConstantExpr::CreateEmpty(S.Context,
                                    ConstantResultStorageKind::APValue);
      CE->setType(S.Context.IntTy);
      CE->setValueKind(VK_PRValue);
      CE->SetResult(APValue(templArgument.getAsIntegral()), S.Context);

      return APValue(ReflectionValue::RK_expr_result, CE);
    }
    case TemplateArgument::IndeterminateSplice:
      llvm_unreachable("TemplateArgument::IndeterminateSplice should have been "
                       "transformed by now");
    case TemplateArgument::TemplateExpansion:
      llvm_unreachable("TemplateArgument::TemplateExpansion not supported");
  }
  llvm_unreachable("Unknown template argument type");
}

static bool isTemplateSpecialization(QualType QT) {
  if (isa<UsingType>(QT) || isa<TypedefType>(QT))
    return false;

  return isa<TemplateSpecializationType>(QT) ||
      isa<DependentTemplateSpecializationType>(QT);
}

static size_t getBitOffsetOfField(ASTContext &C, const FieldDecl *FD) {
  const RecordDecl *Parent = FD->getParent();
  assert(Parent && "no parent for field!");

  const ASTRecordLayout &Layout = C.getASTRecordLayout(Parent);
  return Layout.getFieldOffset(FD->getFieldIndex());
}

static TemplateArgumentListInfo addLocToTemplateArgs(
        Sema &S, ArrayRef<TemplateArgument> Args, Expr *InstExpr) {
  SmallVector<TemplateArgument, 4> Expanded;
  expandTemplateArgPacks(Args, Expanded);

  TemplateArgumentListInfo Result;
  for (const TemplateArgument &Arg : Expanded)
    Result.addArgument(
          S.getTrivialTemplateArgumentLoc(Arg,
                                          Arg.getNonTypeTemplateArgumentType(),
                                          InstExpr->getExprLoc()));
  return Result;
}

static bool ensureInstantiated(Sema &S, Decl *D, SourceRange Range) {
  auto validateConstraints = [&](TemplateDecl *TDecl,
                                 ArrayRef<TemplateArgument> TArgs) {
    MultiLevelTemplateArgumentList MLTAL(TDecl, TArgs, false);
    if (S.EnsureTemplateArgumentListConstraints(TDecl, MLTAL, Range))
      return false;

    return true;
  };

  // Cover case of static variables in a specialization not yet referenced.
  if (auto *VD = dyn_cast<VarDecl>(D); VD && VD->hasGlobalStorage())
    S.MarkVariableReferenced(Range.getBegin(), VD);

  if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D);
      CTSD && !CTSD->isCompleteDefinition()) {
    if (!validateConstraints(CTSD->getSpecializedTemplate(),
                             CTSD->getTemplateArgs().asArray()))
      return true;

    if (S.InstantiateClassTemplateSpecialization(
            Range.getBegin(), CTSD, TSK_ExplicitInstantiationDefinition, false))
      return false;

    S.InstantiateClassTemplateSpecializationMembers(
            Range.getBegin(), CTSD, TSK_ExplicitInstantiationDefinition);
  } else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D);
      VTSD && !VTSD->isCompleteDefinition()) {
    if (!validateConstraints(VTSD->getSpecializedTemplate(),
                             VTSD->getTemplateArgs().asArray()))
      return true;

    S.InstantiateVariableDefinition(Range.getBegin(), VTSD, true, true);
  } else if (auto *FD = dyn_cast<FunctionDecl>(D);
             FD && FD->isTemplateInstantiation()) {
    if (FD->getTemplateSpecializationArgs())
      if (!validateConstraints(FD->getPrimaryTemplate(),
                               FD->getTemplateSpecializationArgs()->asArray()))
      return true;

    S.InstantiateFunctionDefinition(Range.getBegin(), FD, true, true);
  }
  return true;
}

static bool ensureDeclared(Sema &S, QualType QT, SourceLocation SpecLoc) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  // Get the type's declaration.
  if (auto *TS = dyn_cast<TemplateSpecializationType>(QT)) {
    if (auto *CTD = dyn_cast<ClassTemplateDecl>(
          TS->getTemplateName().getAsTemplateDecl())) {
      void *InsertPos;
      if (!CTD->findSpecialization(TS->template_arguments(), InsertPos)) {
        ClassTemplateSpecializationDecl *D =
            ClassTemplateSpecializationDecl::Create(
                S.Context, CTD->getTemplatedDecl()->getTagKind(),
                CTD->getDeclContext(), SpecLoc, SpecLoc,  CTD,
                TS->template_arguments(), nullptr);
        if (!D)
          return false;

        CTD->AddSpecialization(D, InsertPos);
      }
    }
  }
  return true;
}

static bool isReflectableDecl(ASTContext &C, const Decl *D) {
  assert(D && "null declaration");
  if (isa<AccessSpecDecl>(D) || isa<EmptyDecl>(D) || isa<FriendDecl>(D))
    return false;
  if (isa<NamespaceAliasDecl>(D))
    return true;
  if (auto *Class = dyn_cast<CXXRecordDecl>(D))
    if (Class->isInjectedClassName())
      return false;
  if (isa<StaticAssertDecl>(D))
    return false;

  return D->getCanonicalDecl() == D;
}

/// Filter non-reflectable members.
static Decl *findIterableMember(ASTContext &C, Decl *D, bool Inclusive) {
  if (!D || (Inclusive && isReflectableDecl(C, D)))
    return D;

  do {
    DeclContext *DC = D->getDeclContext();

    // Get the next declaration in the DeclContext.
    //
    // Explicit specializations of templates are created with the DeclContext of
    // the template from which they're instantiated, but they end up in the
    // DeclContext within which they're declared. We therefore skip over any
    // declarations whose DeclContext is different from the previous Decl;
    // otherwise, we may inadvertently break the chain of redeclarations in
    // difficult to predit ways.
    D = D->getNextDeclInContext();
    while (D && D->getDeclContext() != DC)
       D = D->getNextDeclInContext();

    if (auto *NSDecl = dyn_cast<NamespaceDecl>(DC)) {
      while (!D && NSDecl) {
        NSDecl = NSDecl->getPreviousDecl();
        D = NSDecl ? *NSDecl->decls_begin() : nullptr;
      }
    }
  } while (D && !isReflectableDecl(C, D));
  return D;
}

bool parentOf(APValue &Result, Decl *D) {
  if (!D)
    return true;

  auto *DC = D->getDeclContext();
  while (DC && !isa<NamespaceDecl>(DC) && !isa<RecordDecl>(DC) &&
               !isa<TranslationUnitDecl>(DC))
    DC = DC->getParent();

  if (!DC)
    return true;
  else if (auto *RD = dyn_cast<RecordDecl>(DC))
    return SetAndSucceed(Result,
                         makeReflection(QualType(RD->getTypeForDecl(), 0)));

  return SetAndSucceed(Result, makeReflection(cast<Decl>(DC)));
}

bool isSpecialMember(FunctionDecl *FD) {
  bool IsSpecial = false;
  if (const auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
    IsSpecial = (isa<CXXDestructorDecl>(MD) ||
                 MD->isCopyAssignmentOperator() ||
                 MD->isMoveAssignmentOperator());

    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(MD))
      IsSpecial = IsSpecial || (CtorD->isDefaultConstructor() ||
                                CtorD->isCopyConstructor() ||
                                CtorD->isMoveConstructor());
  }
  return IsSpecial;
}

bool isAccessible(Sema &S, DeclContext *AccessDC, NamedDecl *D) {
  bool Result = false;

  if (auto *ClsDecl = dyn_cast_or_null<CXXRecordDecl>(D->getDeclContext())) {
    CXXRecordDecl *NamingCls = ClsDecl;
    for (DeclContext *DC = AccessDC; DC; DC = DC->getParent())
      if (auto *CXXRD = dyn_cast<CXXRecordDecl>(DC)) {
        if (CXXRD->isDerivedFrom(ClsDecl)) {
          NamingCls = CXXRD;
          break;
        }
      }

    DeclContext *PreviousDC = S.CurContext;
    {
      S.CurContext = AccessDC;
      Result = S.IsSimplyAccessible(D, NamingCls, QualType());
      S.CurContext = PreviousDC;
    }
  }
  return Result;
}

static bool isFunctionOrMethodNoexcept(const QualType QT) {
  const Type* T = QT.getTypePtr();

  if (T->isFunctionProtoType()) {
    // This covers (virtual) methods & functions
    const auto *FPT = T->getAs<FunctionProtoType>();

    switch (FPT->getExceptionSpecType()) {
    case EST_BasicNoexcept:
    case EST_NoexceptTrue:
      return true;
    default:
      return false;
    }
  }

  return false;
}

// -----------------------------------------------------------------------------
// Metafunction implementations
// -----------------------------------------------------------------------------

bool get_begin_enumerator_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    Decl *D = findTypeDecl(R.getReflectedType());

    if (auto enumDecl = dyn_cast_or_null<EnumDecl>(D)) {
      if (auto itr = enumDecl->enumerator_begin();
          itr != enumDecl->enumerator_end()) {
        return SetAndSucceed(Result, makeReflection(*itr));
      }
      return SetAndSucceed(Result, Sentinel);
    }
    return true;
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec: {
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_enumerator_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_declaration: {
    Decl *currEnumConstDecl = R.getReflectedDecl();
    if(auto nextEnumConstDecl = currEnumConstDecl->getNextDeclInContext()) {
      return SetAndSucceed(Result, makeReflection(nextEnumConstDecl));
    }
    return SetAndSucceed(Result, Sentinel);
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec: {
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_base_of(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    Decl *typeDecl = findTypeDecl(R.getReflectedType());

    if (auto cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(typeDecl)) {
      ensureInstantiated(S, typeDecl, Range);
      if (R.getReflectedType()->isIncompleteType())
        return true;

      auto numBases = cxxRecordDecl->getNumBases();
      if (idx >= numBases)
        return SetAndSucceed(Result, Sentinel);

      // the unqualified base class
      CXXBaseSpecifier *baseClassItr = cxxRecordDecl->bases_begin() + idx;
      return SetAndSucceed(Result, makeReflection(baseClassItr));
    }
    return true;
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_template_argument_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromType(QT, TArgs))
      return true;

    return SetAndSucceed(Result, getNthTemplateArgument(S, TArgs, Evaluator,
                                                        Sentinel, idx));
  }
  case ReflectionValue::RK_declaration: {
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromDeclaration(R.getReflectedDecl(), TArgs))
      return true;
    return SetAndSucceed(Result, getNthTemplateArgument(S, TArgs, Evaluator,
                                                        Sentinel, idx));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_begin_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  APValue R;
  if (!Evaluator(R, Args[0], true)) {
    return true;
  }

  assert(Args[1]->getType()->isReflectionType());
  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type:
  {
    QualType QT = R.getReflectedType();
    if (isTypeAlias(QT))
      QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                       /*DropRefs=*/false);

    if (isa<EnumType>(QT))  // should use 'enumerators_of' instead.
      return true;

    ensureDeclared(S, QT, Range.getBegin());
    Decl *typeDecl = findTypeDecl(QT);
    if (!typeDecl)
      return true;

    if (!ensureInstantiated(S, typeDecl, Range))
      return true;

    if (QT->isIncompleteType())
      return true;
      // NOTE(P2996): Uncomment to allow 'members_of' within member specification.
      /*if (auto *TD = dyn_cast<TagDecl>(typeDecl); !TD || !TD->isBeingDefined())
        return true;*/

    if (auto *CXXRD = dyn_cast<CXXRecordDecl>(typeDecl))
      S.ForceDeclarationOfImplicitMembers(CXXRD);

    DeclContext *declContext = dyn_cast<DeclContext>(typeDecl);
    assert(declContext && "no DeclContext?");

    Decl* beginMember = findIterableMember(S.Context,
                                           *declContext->decls_begin(), true);
    if (!beginMember)
      return SetAndSucceed(Result, Sentinel);
    return SetAndSucceed(Result, APValue(ReflectionValue::RK_declaration,
                                         beginMember));
  }
  case ReflectionValue::RK_namespace: {
    Decl *NS = R.getReflectedNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();

    DeclContext *DC = cast<DeclContext>(NS->getMostRecentDecl());

    Decl *beginMember = findIterableMember(S.Context, *DC->decls_begin(), true);
    if (!beginMember)
      return SetAndSucceed(Result, Sentinel);
    return SetAndSucceed(Result, APValue(ReflectionValue::RK_declaration,
                                         beginMember));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  assert(Args[1]->getType()->isReflectionType());
  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  if (Decl *Next = findIterableMember(S.Context, R.getReflectedDecl(), false)) {
    return SetAndSucceed(Result, APValue(ReflectionValue::RK_declaration,
                                         Next));
  }
  return SetAndSucceed(Result, Sentinel);
}

bool map_decl_to_entity(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;
  Decl *D = R.getReflectedDecl();

  if (auto *TyDecl = dyn_cast<TypeDecl>(D)) {
    QualType QT = S.Context.getTypeDeclType(TyDecl);
    return SetAndSucceed(Result, makeReflection(QT));
  } else if (auto *TDecl = dyn_cast<TemplateDecl>(D)) {
    TemplateName TName(TDecl);
    return SetAndSucceed(Result, makeReflection(TName));
  } else {
    return SetAndSucceed(Result, makeReflection(D));
  }
  llvm_unreachable("unknown reflection kind");
}

bool name_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[1], true))
    return true;

  bool IsUtf8;
  {
    APValue Scratch;
    if (!Evaluator(Scratch, Args[2], true))
      return true;
    IsUtf8 = Scratch.getInt().getBoolValue();
  }

  bool EnforceConsistent;
  {
    APValue Scratch;
    if (!Evaluator(Scratch, Args[3], true))
      return true;
    EnforceConsistent = Scratch.getInt().getBoolValue();
  }

  std::string Name;
  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    getTypeName(Name, S.Context, R.getReflectedType(), !IsUtf8);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(R.getReflectedDecl());
        EnforceConsistent && PVD && !parameterHasConsistentName(PVD))
      return true;

    getDeclName(Name, S.Context, R.getReflectedDecl(), !IsUtf8);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_template: {
    getTemplateName(Name, S.Context, R.getReflectedTemplate(), !IsUtf8);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_expr_result: {
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_namespace: {
    getDeclName(Name, S.Context, R.getReflectedNamespace(), !IsUtf8);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_base_specifier: {
    getTypeName(Name, S.Context, R.getReflectedBaseSpecifier()->getType(),
                !IsUtf8);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool display_name_of(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[1], true))
    return true;

  bool IsUtf8;
  {
    APValue Scratch;
    if (!Evaluator(Scratch, Args[2], true))
      return true;
    IsUtf8 = Scratch.getInt().getBoolValue();
  }

  std::string Name;
  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    getTypeName(Name, S.Context, R.getReflectedType(), false);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_declaration: {
    getDeclName(Name, S.Context, R.getReflectedDecl(), false);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_template: {
    getTemplateName(Name, S.Context, R.getReflectedTemplate(), false);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_expr_result: {
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_namespace: {
    getDeclName(Name, S.Context, R.getReflectedNamespace(), false);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_base_specifier: {
    getTypeName(Name, S.Context, R.getReflectedBaseSpecifier()->getType(),
                false);
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_null: {
    Name = "<null-reflection>";
    return !Evaluator(Result, makeCString(Name, S.Context, IsUtf8), true);
  }
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool source_location_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type:
    return findTypeDeclLoc(Result, S.Context, Evaluator, ResultTy,
                           R.getReflectedType());
  case ReflectionValue::RK_declaration:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       R.getReflectedDecl());
  case ReflectionValue::RK_template: {
    TemplateName TName = R.getReflectedTemplate();
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       TName.getAsTemplateDecl());
  }
  case ReflectionValue::RK_namespace:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       R.getReflectedNamespace());
  case ReflectionValue::RK_expr_result:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy, nullptr);
  case ReflectionValue::RK_base_specifier:
    return findBaseSpecLoc(Result, S.Context, Evaluator, ResultTy, nullptr);
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool type_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_expr_result: {
    ConstantExpr *E = R.getReflectedExprResult();
    bool UnwrapAliases = E->isPRValue();
    QualType QT = desugarType(E->getType(), /*UnwrapAliases=*/UnwrapAliases,
                              /*DropCV=*/false, /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionValue::RK_declaration: {
    ValueDecl *VD = cast<ValueDecl>(R.getReflectedDecl());

    bool UnwrapAliases = isa<ParmVarDecl>(VD) || isa<BindingDecl>(VD);
    bool DropCV = isa<ParmVarDecl>(VD);
    QualType QT = desugarType(VD->getType(), UnwrapAliases, DropCV,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionValue::RK_base_specifier: {
    QualType QT = R.getReflectedBaseSpecifier()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/false, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool parent_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
               SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_data_member_spec:
  case ReflectionValue::RK_base_specifier:
    return true;
  case ReflectionValue::RK_type: {
    if (TemplateName TName = findTemplateOfType(R.getReflectedType());
        !TName.isNull())
      return parentOf(Result, TName.getAsTemplateDecl());

    return parentOf(Result, findTypeDecl(R.getReflectedType()));
  }
  case ReflectionValue::RK_declaration: {
    if (TemplateName TName = findTemplateOfDecl(R.getReflectedDecl());
        !TName.isNull())
      return parentOf(Result, TName.getAsTemplateDecl());

    return parentOf(Result, R.getReflectedDecl());
  }
  case ReflectionValue::RK_template: {
    return parentOf(Result, R.getReflectedTemplate().getAsTemplateDecl());
  }
  case ReflectionValue::RK_namespace:
    return parentOf(Result, R.getReflectedNamespace());
  }
  llvm_unreachable("unknown reflection kind");
}

bool dealias(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionValue::RK_namespace: {
    Decl *NS = R.getReflectedNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();
    return SetAndSucceed(Result, makeReflection(NS));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool value_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
              SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_expr_result: {
    ConstantExpr *E = R.getReflectedExprResult();
    if (!E->isLValue())
      return SetAndSucceed(Result, R);

    if (!E->getType()->isStructuralType())
      return true;

    Expr::EvalResult ER;
    if (!E->EvaluateAsRValue(ER, S.Context, true))
      return true;

    ConstantExpr *CE =
        ConstantExpr::CreateEmpty(S.Context,
                                  ConstantResultStorageKind::APValue);
    CE->setType(desugarType(E->getType(), /*UnwrapAliases=*/true,
                            /*DropCV=*/!E->getType()->isRecordType(),
                            /*DropRefs=*/true));
    CE->setValueKind(VK_PRValue);
    CE->SetResult(ER.Val, S.Context);

    APValue Value(ReflectionValue::RK_expr_result, CE);
    return SetAndSucceed(Result, Value);
  }
  case ReflectionValue::RK_declaration: {
    ValueDecl *Decl = R.getReflectedDecl();

    APValue Value;
    if (auto *VD = dyn_cast<VarDecl>(Decl)) {
      if (!VD->isUsableInConstantExpressions(S.Context))
        return true;

      QualType QT = VD->getType();
      if (auto *LVRT = dyn_cast<LValueReferenceType>(QT)) {
        QT = LVRT->getPointeeType();
      }

      Expr *Synthesized = DeclRefExpr::Create(S.Context,
                                              NestedNameSpecifierLoc(),
                                              SourceLocation(), VD, false,
                                              Range.getBegin(), QT,
                                              VK_LValue, Decl, nullptr);
      if (!Evaluator(Value, Synthesized, true))
        return true;
    } else if (isa<EnumConstantDecl>(Decl)) {
      Expr *Synthesized = DeclRefExpr::Create(S.Context,
                                              NestedNameSpecifierLoc(),
                                              SourceLocation(), Decl, false,
                                              Range.getBegin(), Decl->getType(),
                                              VK_PRValue, Decl, nullptr);

      Expr::EvalResult ER;
      if (!Synthesized->EvaluateAsConstantExpr(ER, S.Context))
        return true;
      Value = ER.Val;
    } else if (auto *TPOD = dyn_cast<TemplateParamObjectDecl>(Decl)) {
      Value = TPOD->getValue();
    }

    QualType QT = desugarType(Decl->getType(), /*UnwrapAliases=*/true,
                              /*DropCV=*/!Decl->getType()->isRecordType(),
                              /*DropRefs=*/true);
    ConstantExpr *CE =
        ConstantExpr::CreateEmpty(S.Context,
                                  ConstantResultStorageKind::APValue);
    CE->setType(QT);
    CE->setValueKind(VK_PRValue);
    CE->SetResult(Value, S.Context);

    APValue Final(ReflectionValue::RK_expr_result, CE);
    return SetAndSucceed(Result, Final);
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unimplemented");
}

bool template_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    TemplateName TName = findTemplateOfType(R.getReflectedType());
    if (TName.isNull())
      return true;

    return SetAndSucceed(Result, makeReflection(TName));
  }
  case ReflectionValue::RK_declaration: {
    TemplateName TName = findTemplateOfDecl(R.getReflectedDecl());
    if (TName.isNull())
      return true;

    return SetAndSucceed(Result, makeReflection(TName));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

static bool CanActAsTemplateArg(const ReflectionValue &RV) {
  switch (RV.getKind()) {
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
    return true;
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
  case ReflectionValue::RK_null:
    return false;
  }
  llvm_unreachable("unknown reflection kind");
}

static TemplateArgument TArgFromReflection(Sema &S, EvalFn Evaluator,
                                           const ReflectionValue &RV,
                                           SourceLocation Loc) {
  switch (RV.getKind()) {
  case ReflectionValue::RK_type:
    return RV.getAsType().getCanonicalType();
  case ReflectionValue::RK_expr_result: {
    ConstantExpr *E = RV.getAsExprResult();
    if (E->getType()->isIntegralOrEnumerationType() && E->isPRValue()) {
      llvm::APSInt UnwrappedIntegral = E->EvaluateKnownConstInt(S.Context);
      return TemplateArgument(S.Context, UnwrappedIntegral,
                              E->getType().getCanonicalType());
    } else if(E->getType()->isReflectionType()) {
      APValue R;
      if (!Evaluator(R, E, true))
        break;
      return TemplateArgument(S.Context, R.getReflection());
    } else {
      return TemplateArgument(RV.getAsExprResult());
    }
    break;
  }
  case ReflectionValue::RK_declaration: {
    ValueDecl *Decl = RV.getAsDecl();
    Expr *Synthesized =
        DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                            SourceLocation(), Decl, false, Loc,
                            Decl->getType(), VK_LValue, Decl, nullptr);
    APValue R;
    if (!Evaluator(R, Synthesized, true))
      break;

    if (Synthesized->getType()->isIntegralOrEnumerationType())
      return TemplateArgument(S.Context, R.getInt(),
                              Synthesized->getType().getCanonicalType());
    else if(Synthesized->getType()->isReflectionType())
      return TemplateArgument(S.Context, R.getReflection());
    else
      return TemplateArgument(Synthesized);
    break;
  }
  case ReflectionValue::RK_template:
    return TemplateArgument(RV.getAsTemplate());
    break;
  default:
    llvm_unreachable("unimplemented for template argument kind");
  }
  return TemplateArgument();
}

// TODO(P2996): Abstract this out, and use as an implementation detail of
// 'substitute'.
bool can_substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());

  APValue Template;
  if (!Evaluator(Template, Args[0], true) ||
      Template.getReflection().getKind() != ReflectionValue::RK_template)
    return true;
  TemplateDecl *TDecl = Template.getReflectedTemplate().getAsTemplateDecl();

  SmallVector<TemplateArgument, 4> TArgs;
  {
    // Evaluate how many template arguments were provided.
    APValue NumArgs;
    if (!Evaluator(NumArgs, Args[2], true))
      return true;
    size_t nArgs = NumArgs.getInt().getExtValue();
    TArgs.reserve(nArgs);

    for (uint64_t k = 0; k < nArgs; ++k) {
      llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                                 S.Context.getSizeType(),
                                                 Args[1]->getExprLoc());

      Synthesized = new (S.Context) ArraySubscriptExpr(Args[1], Synthesized,
                                                       S.Context.MetaInfoTy,
                                                       VK_LValue, OK_Ordinary,
                                                       Range.getBegin());
      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return true;

      APValue Unwrapped;
      if (!Evaluator(Unwrapped, Synthesized, true) ||
          !Unwrapped.isReflection() ||
          !CanActAsTemplateArg(Unwrapped.getReflection()))
        return true;

      TemplateArgument TArg = TArgFromReflection(S, Evaluator,
                                                 Unwrapped.getReflection(),
                                                 Range.getBegin());
      if (TArg.isNull())
        return true;
      TArgs.push_back(TArg);
    }
  }

  TemplateArgumentListInfo TAListInfo =
        addLocToTemplateArgs(S, TArgs, Args[1]);

  SmallVector<TemplateArgument, 4> IgnoredCanonical;
  SmallVector<TemplateArgument, 4> IgnoredSugared;

  {
    Sema::SuppressDiagnosticsRAII NoDiagnostics(S);
    bool CanSub = !S.CheckTemplateArgumentList(TDecl, Args[0]->getExprLoc(),
                                               TAListInfo, false,
                                               IgnoredSugared, IgnoredCanonical,
                                               true);
    return SetAndSucceed(Result, makeBool(S.Context, CanSub));
  }
}

bool substitute(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());

  APValue Template;
  if (!Evaluator(Template, Args[0], true) ||
      Template.getReflection().getKind() != ReflectionValue::RK_template)
    return true;
  TemplateDecl *TDecl = Template.getReflectedTemplate().getAsTemplateDecl();

  SmallVector<TemplateArgument, 4> TArgs;
  {
    // Evaluate how many template arguments were provided.
    APValue NumArgs;
    if (!Evaluator(NumArgs, Args[2], true))
      return true;
    size_t nArgs = NumArgs.getInt().getExtValue();
    TArgs.reserve(nArgs);

    for (uint64_t k = 0; k < nArgs; ++k) {
      llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                                 S.Context.getSizeType(),
                                                 Args[1]->getExprLoc());

      Synthesized = new (S.Context) ArraySubscriptExpr(Args[1], Synthesized,
                                                       S.Context.MetaInfoTy,
                                                       VK_LValue, OK_Ordinary,
                                                       Range.getBegin());
      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return true;

      APValue Unwrapped;
      if (!Evaluator(Unwrapped, Synthesized, true) ||
          !Unwrapped.isReflection() ||
          !CanActAsTemplateArg(Unwrapped.getReflection()))
        return true;

      TemplateArgument TArg = TArgFromReflection(S, Evaluator,
                                                 Unwrapped.getReflection(),
                                                 Range.getBegin());
      if (TArg.isNull())
        return true;
      TArgs.push_back(TArg);
    }
  }

  {
    TemplateArgumentListInfo TAListInfo =
          addLocToTemplateArgs(S, TArgs, Args[1]);

    SmallVector<TemplateArgument, 4> CanonicalTArgs;
    SmallVector<TemplateArgument, 4> IgnoredSugared;
    if (S.CheckTemplateArgumentList(TDecl, Args[0]->getExprLoc(), TAListInfo,
                                    false, IgnoredSugared, CanonicalTArgs,
                                    true))
      return true;
    TArgs = CanonicalTArgs;
  }

  if (auto *CTD = dyn_cast<ClassTemplateDecl>(TDecl)) {
    void *InsertPos;
    ClassTemplateSpecializationDecl *TSpecDecl =
          CTD->findSpecialization(TArgs, InsertPos);

    if (!TSpecDecl) {
      TSpecDecl = ClassTemplateSpecializationDecl::Create(
            S.Context, CTD->getTemplatedDecl()->getTagKind(),
            CTD->getDeclContext(), Range.getBegin(), Range.getBegin(),
            CTD, TArgs, nullptr);
      CTD->AddSpecialization(TSpecDecl, InsertPos);
    }
    assert(TSpecDecl);

    APValue Value(ReflectionValue::RK_type, TSpecDecl->getTypeForDecl());
    return SetAndSucceed(Result, Value);
  } else if (isa<TypeAliasTemplateDecl>(TDecl)) {
    TemplateArgumentListInfo TAListInfo =
          addLocToTemplateArgs(S, TArgs, Args[1]);

    // TODO(P2996): Calling 'substitute' should not instantiate the alias
    // template. Probably the logic for "substituting" the arguments into the
    // template should be abstracted to a separate function.
    QualType QT = S.CheckTemplateIdType(Template.getReflectedTemplate(),
                                        Range.getBegin(), TAListInfo);
    if (QT.isNull())
      return true;

    APValue Value(ReflectionValue::RK_type, QT.getAsOpaquePtr());
    return SetAndSucceed(Result, Value);
  } else if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl)) {
    void *InsertPos;
    FunctionDecl *TSpecDecl = FTD->findSpecialization(TArgs, InsertPos);
    if (!TSpecDecl) {
      TemplateArgumentList *TAList = TemplateArgumentList::CreateCopy(S.Context,
                                                                      TArgs);

      // TODO(P2996): Calling 'substitute' should not instantiate the function
      // template. Probably the logic for "substituting" the arguments into the
      // template should be abstracted to a separate function.
      TSpecDecl = S.InstantiateFunctionDeclaration(FTD, TAList,
                                                   Range.getBegin());
    }
    if (!TSpecDecl)
      // Could not instantiate function with the provided arguments.
      return true;

    APValue Value(ReflectionValue::RK_declaration, TSpecDecl);
    return SetAndSucceed(Result, Value);
  } else if (auto *VTD = dyn_cast<VarTemplateDecl>(TDecl)) {
    void *InsertPos;
    VarTemplateSpecializationDecl *TSpecDecl =
          VTD->findSpecialization(TArgs, InsertPos);

    if (!TSpecDecl) {
      TemplateArgumentListInfo TAListInfo = addLocToTemplateArgs(S, TArgs,
                                                                 Args[1]);

      DeclResult DR = S.CheckVarTemplateId(VTD, Range.getBegin(),
                                           Range.getBegin(), TAListInfo);
      TSpecDecl = cast<VarTemplateSpecializationDecl>(DR.get());
      if (!TSpecDecl->getTemplateSpecializationKind())
        TSpecDecl->setTemplateSpecializationKind(TSK_ImplicitInstantiation);
    }
    assert(TSpecDecl);

    APValue Value(ReflectionValue::RK_declaration, TSpecDecl);
    return SetAndSucceed(Result, Value);
  } else if (auto *CD = dyn_cast<ConceptDecl>(TDecl)) {
    TemplateArgumentListInfo TAListInfo = addLocToTemplateArgs(S, TArgs,
                                                               Args[1]);

    CXXScopeSpec SS;
    DeclarationNameInfo DNI(CD->getDeclName(), Range.getBegin());
    ExprResult ER = S.CheckConceptTemplateId(SS, Range.getBegin(), DNI, CD, CD,
                                             &TAListInfo);
    assert(ER.get());

    APValue SatisfiesConcept;
    if (!Evaluator(SatisfiesConcept, ER.get(), true))
      return true;

    ConstantExpr *CE =
        ConstantExpr::CreateEmpty(S.Context,
                                  ConstantResultStorageKind::APValue);
    CE->setType(S.Context.BoolTy);
    CE->setValueKind(VK_PRValue);
    CE->SetResult(SatisfiesConcept, S.Context);

    APValue Value(ReflectionValue::RK_expr_result, CE);
    return SetAndSucceed(Result, Value);
  }
  llvm_unreachable("unimplemented for template kind");
}


bool extract(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());

  bool ReturnsLValue = false;
  QualType RawResultTy = ResultTy;
  if (auto *LVRT = dyn_cast<LValueReferenceType>(ResultTy)) {
    ReturnsLValue = true;
    ResultTy = LVRT->getPointeeType();
  }

  APValue R;
  if (!Evaluator(R, Args[1], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_expr_result: {
    Expr *Synthesized = R.getReflectedExprResult();

    if (ReturnsLValue && !Synthesized->isLValue())
      return true;

    if (auto *RD = dyn_cast_or_null<RecordDecl>(
            Synthesized->getType()->getAsCXXRecordDecl());
        RD && RD->isLambda() && ResultTy->isPointerType()) {
      TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(ResultTy, 0);
      ExprResult ER = S.BuildCStyleCastExpr(Range.getBegin(), TSI,
                                            Range.getEnd(), Synthesized);

      if (ER.isInvalid())
        return true;
      Synthesized = ER.get();
    }

    if (Synthesized->getType().getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return true;

    return !Evaluator(Result, Synthesized, !Synthesized->isLValue());
  }
  case ReflectionValue::RK_declaration: {
    ValueDecl *Decl = dyn_cast<ValueDecl>(R.getReflectedDecl());
    if (!Decl)
      return true;
    ensureInstantiated(S, Decl, Args[1]->getSourceRange());

    bool isLambda = false;
    if (auto *RD = Decl->getType()->getAsCXXRecordDecl())
        isLambda = RD->isLambda();

    Expr *Synthesized;
    if (isa<VarDecl, TemplateParamObjectDecl>(Decl) && !isLambda) {
      if (isa<LValueReferenceType>(Decl->getType().getCanonicalType())) {
        // We have a reflection of an object with reference type.
        // Synthesize a 'DeclRefExpr' designating the object, such that constant
        // evaluation resolves the underlying referenced entity.
        ReturnsLValue = true;
        if (RawResultTy.getCanonicalType().getTypePtr() !=
            Decl->getType().getCanonicalType().getTypePtr())
          return true;

        NestedNameSpecifierLocBuilder NNSLocBuilder;
        if (auto *ParentClsDecl = dyn_cast_or_null<CXXRecordDecl>(
                Decl->getDeclContext())) {
          TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(
                  QualType(ParentClsDecl->getTypeForDecl(), 0), 0);
          NNSLocBuilder.Extend(S.Context, Range.getBegin(), TSI->getTypeLoc(),
                               Range.getBegin());
        }
        Synthesized = DeclRefExpr::Create(S.Context,
                                          NNSLocBuilder.getTemporary(),
                                          SourceLocation(), Decl, false,
                                          Range.getBegin(), ResultTy,
                                          ReturnsLValue ? VK_LValue :
                                                          VK_PRValue,
                                          Decl, nullptr);
      } else {
        // We have a reflection of a (possibly local) non-reference variable.
        // Synthesize an lvalue by reaching up the call stack.
        if (ResultTy.getCanonicalType().getTypePtr() !=
            Decl->getType().getCanonicalType().getTypePtr())
          return true;

        Synthesized = ExtractLValueExpr::Create(S.Context, Range, ResultTy,
                                                Decl);
      }
    } else if (ReturnsLValue && !isa<BindingDecl>(Decl)) {
      // Only variables and structured binding may be returned as LValues.
      return true;
    } else {
      // We have a reflection of a non-variable entity (either a field,
      // function, enumerator, structured binding, or lambda).
      NestedNameSpecifierLocBuilder NNSLocBuilder;
      if (auto *ParentClsDecl = dyn_cast_or_null<CXXRecordDecl>(
              Decl->getDeclContext())) {
        TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(
                QualType(ParentClsDecl->getTypeForDecl(), 0), 0);
        NNSLocBuilder.Extend(S.Context, Range.getBegin(), TSI->getTypeLoc(),
                             Range.getBegin());
      }

      ExprValueKind VK = VK_LValue;
      if (isa<CXXMethodDecl>(Decl))
        VK = VK_PRValue;

      Synthesized = DeclRefExpr::Create(S.Context, NNSLocBuilder.getTemporary(),
                                        SourceLocation(), Decl, false,
                                        Range.getBegin(), Decl->getType(), VK,
                                        Decl, nullptr);

      if (isa<FieldDecl>(Decl) || isa<FunctionDecl>(Decl)) {
        ExprResult ER = S.CreateBuiltinUnaryOp(Range.getBegin(), UO_AddrOf,
                                               Synthesized, true);
        if (ER.isInvalid())
          return true;
        Synthesized = ER.get();
      } else if (isa<EnumConstantDecl>(Decl)) {
        Synthesized = ImplicitCastExpr::Create(
              S.Context, Synthesized->getType(), CK_IntegralCast, Synthesized,
              /*BasePath=*/nullptr, VK_PRValue, FPOptionsOverride());
      } else if (isLambda && ResultTy->isPointerType()) {
        TypeSourceInfo *TSI = S.Context.CreateTypeSourceInfo(ResultTy, 0);
        ExprResult ER = S.BuildCStyleCastExpr(Range.getBegin(), TSI,
                                              Range.getEnd(), Synthesized);
        if (ER.isInvalid())
          return true;

        Synthesized = ER.get();
      }
    }

    if (Synthesized->getType().getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return true;

    auto result = !Evaluator(Result, Synthesized, !ReturnsLValue);
    return result;
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("invalid reflection type");
}

bool is_public(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
               SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    bool IsPublic = false;
    if (const Decl *D = findTypeDecl(R.getReflectedType()))
      IsPublic = (D->getAccess() == AS_public);

    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionValue::RK_declaration: {
    bool IsPublic = (R.getReflectedDecl()->getAccess() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionValue::RK_template: {
    const Decl *D = R.getReflectedTemplate().getAsTemplateDecl();

    bool IsPublic = (D->getAccess() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionValue::RK_base_specifier: {
    CXXBaseSpecifier *Base = R.getReflectedBaseSpecifier();
    bool IsPublic = (Base->getAccessSpecifier() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_data_member_spec:
  case ReflectionValue::RK_namespace:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_protected(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    bool IsProtected = false;
    if (const Decl *D = findTypeDecl(R.getReflectedType()))
      IsProtected = (D->getAccess() == AS_protected);

    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionValue::RK_declaration: {
    bool IsProtected = (R.getReflectedDecl()->getAccess() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionValue::RK_template: {
    const Decl *D = R.getReflectedTemplate().getAsTemplateDecl();

    bool IsProtected = (D->getAccess() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionValue::RK_base_specifier: {
    CXXBaseSpecifier *Base = R.getReflectedBaseSpecifier();
    bool IsProtected = (Base->getAccessSpecifier() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_data_member_spec:
  case ReflectionValue::RK_namespace:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_private(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    bool IsPrivate = false;
    if (const Decl *D = findTypeDecl(R.getReflectedType()))
      IsPrivate = (D->getAccess() == AS_private);

    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionValue::RK_declaration: {
    bool IsPrivate = (R.getReflectedDecl()->getAccess() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionValue::RK_template: {
    const Decl *D = R.getReflectedTemplate().getAsTemplateDecl();

    bool IsPrivate = (D->getAccess() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionValue::RK_base_specifier: {
    CXXBaseSpecifier *Base = R.getReflectedBaseSpecifier();
    bool IsPrivate = (Base->getAccessSpecifier() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

static bool findAccessContext(Sema &S, EvalFn Evaluator, APValue &Result) {
  StackLocationExpr *SLE = StackLocationExpr::Create(S.Context,
                                                     SourceRange(), 1);
  if (!Evaluator(Result, SLE, true) || !Result.isReflection())
    return false;

  if (Result.getReflectedDecl() != nullptr)
    return true;

  Result = makeReflection(dyn_cast<Decl>(S.CurContext));
  return true;
}

bool access_context(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  return !findAccessContext(S, Evaluator, Result);
}

bool is_accessible(APValue &Result, Sema &S, EvalFn Evaluator,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;
  if (!Evaluator(Scratch, Args[1], true) || !Scratch.isReflection())
    return true;

  DeclContext *AccessDC = nullptr;
  switch (Scratch.getReflection().getKind()) {
  case ReflectionValue::RK_type:
    AccessDC = dyn_cast<DeclContext>(findTypeDecl(Scratch.getReflectedType()));
    if (!AccessDC)
      return true;
    break;
  case ReflectionValue::RK_namespace:
    AccessDC = dyn_cast<DeclContext>(Scratch.getReflectedNamespace());
    break;
  case ReflectionValue::RK_declaration:
    AccessDC = dyn_cast<DeclContext>(Scratch.getReflectedDecl());
    break;
  default:
    return true;
  }

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    NamedDecl *D = findTypeDecl(R.getReflectedType());
    if (!D)
      return true;
    bool Accessible = isAccessible(S, AccessDC, D);
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionValue::RK_declaration: {
    bool Accessible = isAccessible(S, AccessDC, R.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionValue::RK_template: {
    TemplateName TName = R.getReflectedTemplate();
    bool Accessible = isAccessible(S, AccessDC, TName.getAsTemplateDecl());
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionValue::RK_base_specifier: {
    CXXBaseSpecifier *BaseSpec = R.getReflectedBaseSpecifier();

    auto *Base = findTypeDecl(BaseSpec->getType());
    if (!Base)
      return true;

    CXXBasePathElement bpe = { BaseSpec, BaseSpec->getDerived(), 0 };
    CXXBasePath path;
    path.push_back(bpe);
    path.Access = BaseSpec->getAccessSpecifier();

    Sema::AccessResult AR;
    DeclContext *PreviousDC = S.CurContext;
    {
      S.CurContext = AccessDC;
      AR = S.CheckBaseClassAccess(
            Range.getBegin(), BaseSpec->getType(),
            QualType(BaseSpec->getDerived()->getTypeForDecl(), 0), path, 0,
            /*ForceCheck=*/true, /*ForceUnprivileged=*/false);
      S.CurContext = PreviousDC;
    }
    bool Accessible = (AR == Sema::AR_accessible);
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
  return true;
}

bool is_virtual(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsVirtual = false;
  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_declaration: {
    if (const auto *MD = dyn_cast<CXXMethodDecl>(R.getReflectedDecl()))
      IsVirtual = MD->isVirtual();
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  case ReflectionValue::RK_base_specifier: {
    IsVirtual = R.getReflectedBaseSpecifier()->isVirtual();
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_pure_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool IsPureVirtual = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl()))
      IsPureVirtual = FD->isPureVirtual();

    return SetAndSucceed(Result, makeBool(S.Context, IsPureVirtual));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_override(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsOverride = false;
  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    if (const auto *MD = dyn_cast<CXXMethodDecl>(R.getReflectedDecl()))
      IsOverride = MD->size_overridden_methods() > 0;
    return SetAndSucceed(Result, makeBool(S.Context, IsOverride));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_deleted(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool IsDeleted = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl()))
      IsDeleted = FD->isDeleted();
    return SetAndSucceed(Result, makeBool(S.Context, IsDeleted));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_defaulted(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool IsDefaulted = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl()))
      IsDefaulted = FD->isDefaulted();

    return SetAndSucceed(Result, makeBool(S.Context, IsDefaulted));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_explicit(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
  case ReflectionValue::RK_template:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    ValueDecl *D = R.getReflectedDecl();

    bool result = false;
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(D))
      result = CtorD->getExplicitSpecifier().isExplicit();
    else if (auto *ConvD = dyn_cast<CXXConversionDecl>(D))
      result = ConvD->getExplicitSpecifier().isExplicit();
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_noexcept(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_type: {
    const QualType QT = R.getReflectedType();
    const auto result = isFunctionOrMethodNoexcept(QT);

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_declaration: {
    const ValueDecl *D = R.getReflectedDecl();
    const auto result = isFunctionOrMethodNoexcept(D->getType());
    
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_bit_field(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *FD = dyn_cast<FieldDecl>(R.getReflectedDecl()))
      result = FD->isBitField();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_static_storage_duration(APValue &Result, Sema &S, EvalFn Evaluator,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *VD = dyn_cast<VarDecl>(R.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Static;
    else if (isa<TemplateParamObjectDecl>(R.getReflectedDecl()))
      result = true;
  } else if (R.getReflection().getKind() == ReflectionValue::RK_expr_result) {
    result = R.getReflectedExprResult()->isLValue();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_internal_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_type) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(R.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::Internal);
  } else if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *ND = dyn_cast<NamedDecl>(R.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::Internal);
  } else if (R.getReflection().getKind() == ReflectionValue::RK_expr_result &&
             R.getReflectedExprResult()->isLValue()) {
    Expr *CE = R.getReflectedExprResult();
    if (!Evaluator(R, CE, false))
      return true;

    if (!CE->getType()->isPointerType() && R.isLValue())
      if (APValue::LValueBase LVBase = R.getLValueBase();
          LVBase.is<const ValueDecl *>()) {
        const ValueDecl *VD = LVBase.get<const ValueDecl *>();
        result = (VD->getFormalLinkage() == Linkage::Internal);
      }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_module_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_type) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(R.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::Module);
  } else  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *ND = dyn_cast<NamedDecl>(R.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::Module);
  } else if (R.getReflection().getKind() == ReflectionValue::RK_expr_result &&
             R.getReflectedExprResult()->isLValue()) {
    Expr *CE = R.getReflectedExprResult();
    if (!Evaluator(R, CE, false))
      return true;

    if (!CE->getType()->isPointerType() && R.isLValue())
      if (APValue::LValueBase LVBase = R.getLValueBase();
          LVBase.is<const ValueDecl *>()) {
        const ValueDecl *VD = LVBase.get<const ValueDecl *>();
        result = (VD->getFormalLinkage() == Linkage::Module);
      }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_external_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_type) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(R.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::External ||
                typeDecl->getFormalLinkage() == Linkage::UniqueExternal);
  } else if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *ND = dyn_cast<NamedDecl>(R.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::External ||
                ND->getFormalLinkage() == Linkage::UniqueExternal);
  } else if (R.getReflection().getKind() == ReflectionValue::RK_expr_result &&
             R.getReflectedExprResult()->isLValue()) {
    Expr *CE = R.getReflectedExprResult();
    if (!Evaluator(R, CE, false))
      return true;

    if (!CE->getType()->isPointerType() && R.isLValue())
      if (APValue::LValueBase LVBase = R.getLValueBase();
          LVBase.is<const ValueDecl *>()) {
        const ValueDecl *VD = LVBase.get<const ValueDecl *>();
        result = (VD->getFormalLinkage() == Linkage::External ||
                  VD->getFormalLinkage() == Linkage::UniqueExternal);
      }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_linkage(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_type) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(R.getReflectedType())))
      result = typeDecl->hasLinkage();
  } else if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (const auto *ND = dyn_cast<NamedDecl>(R.getReflectedDecl()))
      result = ND->hasLinkage();
  } else if (R.getReflection().getKind() == ReflectionValue::RK_expr_result &&
             R.getReflectedExprResult()->isLValue()) {
    Expr *CE = R.getReflectedExprResult();
    if (!Evaluator(R, CE, false))
      return true;

    if (!CE->getType()->isPointerType() && R.isLValue())
      if (APValue::LValueBase LVBase = R.getLValueBase();
          LVBase.is<const ValueDecl *>()) {
        const ValueDecl *VD = LVBase.get<const ValueDecl *>();
        result = (VD->hasLinkage());
      }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_class_member(APValue &Result, Sema &S, EvalFn Evaluator,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;

  bool result = false;
  if (!parent_of(Scratch, S, Evaluator, S.Context.MetaInfoTy, Range, Args)) {
    assert(Scratch.isReflection());
    result = (Scratch.getReflection().getKind() == ReflectionValue::RK_type);
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_namespace_member(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;

  bool result = false;
  if (!parent_of(Scratch, S, Evaluator, S.Context.MetaInfoTy, Range, Args)) {
    assert(Scratch.isReflection());
    result = (Scratch.getReflection().getKind() == ReflectionValue::RK_namespace);
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_nonstatic_data_member(APValue &Result, Sema &S, EvalFn Evaluator,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    result = isa<const FieldDecl>(R.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_static_member(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_declaration: {
    const ValueDecl *D = cast<ValueDecl>(R.getReflectedDecl());
    if (const auto *MD = dyn_cast<CXXMethodDecl>(D))
      result = MD->isStatic();
    else if (const auto *VD = dyn_cast<VarDecl>(D))
      result = VD->isStaticDataMember();
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_template: {
    const Decl *D = R.getReflectedTemplate().getAsTemplateDecl();
    if (const auto *FTD = dyn_cast<FunctionTemplateDecl>(D)) {
      if (const auto *MD = dyn_cast<CXXMethodDecl>(FTD->getTemplatedDecl()))
        result = MD->isStatic();
    } else if (const auto *VTD = dyn_cast<VarTemplateDecl>(D)) {
      if (const auto *VD = dyn_cast<VarDecl>(VTD->getTemplatedDecl()))
        result = VD->isStaticDataMember();
    }
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_base(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  return SetAndSucceed(Result,
                       makeBool(S.Context,
                                R.getReflection().getKind() ==
                                      ReflectionValue::RK_base_specifier));
}

bool is_namespace(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context,
                                        R.getReflection().getKind() ==
                                              ReflectionValue::RK_namespace));
}

bool is_function(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    result = isa<const FunctionDecl>(R.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_variable(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    result = isa<const VarDecl>(R.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_type(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context,
                                        R.getReflection().getKind() ==
                                              ReflectionValue::RK_type));
}

bool is_alias(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
              SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    bool result = isTypeAlias(R.getReflectedType());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_namespace: {
    bool result = isa<NamespaceAliasDecl>(R.getReflectedNamespace());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_template: {
    TemplateDecl *TDecl = R.getReflectedTemplate().getAsTemplateDecl();
    bool result = isa<TypeAliasTemplateDecl>(TDecl);
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_declaration:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_incomplete_type(APValue &Result, Sema &S, EvalFn Evaluator,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_type) {
    // If this is a declared type with a reachable definition, ensure that the
    // type is instantiated.
    if (Decl *typeDecl = findTypeDecl(R.getReflectedType()))
      (void) ensureInstantiated(S, typeDecl, Range);

    result = R.getReflectedType()->isIncompleteType();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_template(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context,
                                        R.getReflection().getKind() ==
                                              ReflectionValue::RK_template));
}

bool is_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsFnTemplate = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_template) {
    const TemplateDecl *TD = R.getReflectedTemplate().getAsTemplateDecl();
    IsFnTemplate = isa<FunctionTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsFnTemplate));
}

bool is_variable_template(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsVarTemplate = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_template) {
    const TemplateDecl *TD = R.getReflectedTemplate().getAsTemplateDecl();
    IsVarTemplate = isa<VarTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsVarTemplate));
}

bool is_class_template(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsClsTemplate = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_template) {
    const TemplateDecl *TD = R.getReflectedTemplate().getAsTemplateDecl();
    IsClsTemplate = isa<ClassTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsClsTemplate));
}

bool is_alias_template(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsAliasTemplate = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_template) {
    const TemplateDecl *TD = R.getReflectedTemplate().getAsTemplateDecl();
    IsAliasTemplate = TD->isTypeAlias();
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsAliasTemplate));
}

bool is_concept(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsConcept = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_template)
    IsConcept = isa<ConceptDecl>(R.getReflectedTemplate().getAsTemplateDecl());

  return SetAndSucceed(Result, makeBool(S.Context, IsConcept));
}

bool is_structured_binding(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    result = isa<const BindingDecl>(R.getReflectedDecl());
  }

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_value(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
              SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsValue = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_expr_result)
    IsValue = R.getReflectedExprResult()->isPRValue();

  return SetAndSucceed(Result, makeBool(S.Context, IsValue));
}

bool is_object(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
               SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool IsObject = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_expr_result) {
    IsObject = R.getReflectedExprResult()->isLValue();
  } else if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    Decl *D = R.getReflectedDecl();
    if (isa<TemplateParamObjectDecl>(D))
      IsObject = true;
    else if (auto *VD = dyn_cast<VarDecl>(D))
      IsObject = !VD->getType()->isReferenceType();
  }

  return SetAndSucceed(Result, makeBool(S.Context, IsObject));
}

bool has_template_arguments(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    bool result = isTemplateSpecialization(QT);
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_declaration: {
    bool result = false;

    Decl *D = R.getReflectedDecl();
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      result = (FD->getTemplateSpecializationArgs() != nullptr);
    else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D))
      result = VTSD->getTemplateArgs().size() > 0;

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool result = isa<CXXConstructorDecl>(R.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionValue::RK_template: {
    bool result = false;
    TemplateDecl *TDecl = R.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl))
      result = isa<CXXConstructorDecl>(FTD->getTemplatedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_destructor(APValue &Result, Sema &S, EvalFn Evaluator,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool result = isa<CXXDestructorDecl>(R.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_special_member(APValue &Result, Sema &S, EvalFn Evaluator,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionValue::RK_declaration: {
    bool IsSpecial = false;
    if (auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl()))
      IsSpecial = isSpecialMember(FD);

    return SetAndSucceed(Result, makeBool(S.Context, IsSpecial));
  }
  case ReflectionValue::RK_template: {
    bool result = false;
    TemplateDecl *TDecl = R.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl))
      result = isSpecialMember(FTD->getTemplatedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_user_provided(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true) || !R.isReflection())
    return true;

  bool IsUserProvided = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration)
    if (auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl())) {
      FD = cast<FunctionDecl>(FD->getFirstDecl());
      IsUserProvided = !(FD->isImplicit() || FD->isDeleted() ||
                         FD->isDefaulted());
    }

  return SetAndSucceed(Result, makeBool(S.Context, IsUserProvided));
}

bool reflect_result(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue ArgTy;
  if (!Evaluator(ArgTy, Args[0], true))
    return true;
  assert(ArgTy.getReflection().getKind() == ReflectionValue::RK_type);
  bool IsLValue = isa<ReferenceType>(ArgTy.getReflectedType());

  if (!IsLValue && !ArgTy.getReflectedType()->isStructuralType())
    return true;

  APValue Arg;
  if (!Evaluator(Arg, Args[1], !IsLValue))
    return true;

  ConstantExpr *E =
        ConstantExpr::CreateEmpty(S.Context,
                                  ConstantResultStorageKind::APValue);
  E->setType(Args[1]->getType());
  E->setValueKind(IsLValue ? VK_LValue : VK_PRValue);
  E->SetResult(Arg, S.Context);
  {
    Expr::EvalResult Discarded;
    if (IsLValue && !E->EvaluateAsLValue(Discarded, S.Context, true))
      return true;
  }

  // If this is an lvalue to a complete object, promote the result to reflect
  // the declaration.
  if (!E->getType()->isPointerType() && Arg.isLValue() &&
      Arg.getLValueOffset().isZero())
    if (!Arg.hasLValuePath() || Arg.getLValuePath().size() == 0)
      if (APValue::LValueBase LVBase = Arg.getLValueBase();
          LVBase.is<const ValueDecl *>())
        return SetAndSucceed(Result,
                             makeReflection(LVBase.get<const ValueDecl *>()));

  APValue Value(ReflectionValue::RK_expr_result, E);
  return SetAndSucceed(Result, Value);
}

bool reflect_invoke(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());
  assert(
      Args[3]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[4]->getType()->isIntegerType());

  using ReflectionVector = SmallVector<ReflectionValue, 4>;
  auto UnpackReflectionsIntoVector = [&](ReflectionVector &Out,
                                         Expr *DataExpr, Expr *SzExpr) -> bool {
    APValue Scratch;
    if (!Evaluator(Scratch, SzExpr, true))
      return false;

    size_t nArgs = Scratch.getInt().getExtValue();
    Out.reserve(nArgs);
    for (uint64_t k = 0; k < nArgs; ++k) {
      llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                                 S.Context.getSizeType(),
                                                 DataExpr->getExprLoc());

      Synthesized = new (S.Context) ArraySubscriptExpr(DataExpr, Synthesized,
                                                       S.Context.MetaInfoTy,
                                                       VK_LValue, OK_Ordinary,
                                                       Range.getBegin());

      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return false;

      if (!Evaluator(Scratch, Synthesized, true) || !Scratch.isReflection())
        return false;
      Out.push_back(Scratch.getReflection());
    }

    return true;
  };

  TemplateArgumentListInfo ExplicitTAListInfo;
  SmallVector<TemplateArgument, 4> ExplicitTArgs;
  {
    SmallVector<ReflectionValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[1], Args[2]))
      return true;

    SmallVector<TemplateArgument, 4> ExplicitTArgs;
    for (ReflectionValue RV : Reflections) {
      if (!CanActAsTemplateArg(RV))
        return true;

      TemplateArgument TArg = TArgFromReflection(S, Evaluator, RV,
                                                 Range.getBegin());
      if (TArg.isNull())
        return true;
      ExplicitTArgs.push_back(TArg);
    }

    ExplicitTAListInfo = addLocToTemplateArgs(S, ExplicitTArgs, Args[1]);
  }

  SmallVector<Expr *, 4> ArgExprs;
  {
    SmallVector<ReflectionValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[3], Args[4]))
      return true;

    for (ReflectionValue RV : Reflections) {
      if (RV.getKind() == ReflectionValue::RK_expr_result) {
        ArgExprs.push_back(RV.getAsExprResult());
      } else if (RV.getKind() == ReflectionValue::RK_declaration) {
        ValueDecl *D = RV.getAsDecl();
        ArgExprs.push_back(
              DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                                  SourceLocation(), D, false, Range.getBegin(),
                                  D->getType(), VK_LValue, D, nullptr));
      } else {
        return true;
      }
    }
  }

  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true) || !Scratch.isReflection())
    return true;

  if (ExplicitTAListInfo.size() > 0 &&
      Scratch.getReflection().getKind() != ReflectionValue::RK_template)
    return true;

  Expr *FnRefExpr = nullptr;
  switch (Scratch.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_expr_result:
    FnRefExpr = Scratch.getReflectedExprResult();
    break;
  case ReflectionValue::RK_declaration: {
    ValueDecl *D = Scratch.getReflectedDecl();
    ensureInstantiated(S, D, Range);

    FnRefExpr =
          DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                              SourceLocation(), D, false, Range.getBegin(),
                              D->getType(), VK_LValue, D, nullptr);
    break;
  }
  case ReflectionValue::RK_template: {
    TemplateDecl *TDecl = Scratch.getReflectedTemplate().getAsTemplateDecl();
    auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl);
    if (!FTD) {
      return true;
    }

    FunctionDecl *Specialization;
    {
      sema::TemplateDeductionInfo Info(Args[0]->getExprLoc(),
                                       FTD->getTemplateDepth());
      TemplateDeductionResult Result = S.DeduceTemplateArguments(
            FTD, &ExplicitTAListInfo, ArgExprs, Specialization, Info, false,
            true, QualType(), Expr::Classification(),
            [](ArrayRef<QualType>) { return false; });
      if (Result != TemplateDeductionResult::Success)
        return true;
      ensureInstantiated(S, Specialization, Range);
    }
    assert(Specialization && "no specialization found?");

    FnRefExpr =
          DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                              SourceLocation(), Specialization, false,
                              Range.getBegin(), Specialization->getType(),
                              VK_LValue, Specialization, nullptr);
    break;
  }
  }

  ExprResult ER;
  if (auto *DRE = dyn_cast<DeclRefExpr>(FnRefExpr);
      DRE && dyn_cast<CXXConstructorDecl>(DRE->getDecl())) {
    auto *CtorD = cast<CXXConstructorDecl>(DRE->getDecl());
    ER = S.BuildCXXConstructExpr(
          Range.getBegin(), QualType(CtorD->getParent()->getTypeForDecl(), 0),
          CtorD, false, ArgExprs, false, false, false, false,
          CXXConstructionKind::Complete, Range);
  } else {
    ER = S.ActOnCallExpr(S.getCurScope(), FnRefExpr, Range.getBegin(), ArgExprs,
                         Range.getEnd(), /*ExecConfig=*/nullptr);
  }
  if (ER.isInvalid())
    return true;
  Expr *ResultExpr = ER.get();

  if (ResultExpr->isTypeDependent() || ResultExpr->isValueDependent())
    return true;

  if (!ResultExpr->getType()->isStructuralType())
    return true;

  APValue FnResult;
  if (!Evaluator(FnResult, ResultExpr, !ResultExpr->isLValue()))
    return true;

  // If this is an lvalue to a complete object, promote the result to reflect
  // the declaration.
  if (!ResultExpr->getType()->isPointerType() &&
      FnResult.getKind() == APValue::LValue &&
      FnResult.getLValueOffset().isZero())
    if (!FnResult.hasLValuePath() || FnResult.getLValuePath().size() == 0)
      if (APValue::LValueBase LVBase = FnResult.getLValueBase();
          LVBase.is<const ValueDecl *>())
        return SetAndSucceed(Result,
                             makeReflection(LVBase.get<const ValueDecl *>()));

  ConstantExpr *CE =
            ConstantExpr::CreateEmpty(S.Context,
                                      ConstantResultStorageKind::APValue);
  CE->setType(ResultExpr->getType());
  CE->setValueKind(ResultExpr->isLValue() ? VK_LValue : VK_PRValue);
  CE->SetResult(FnResult, S.Context);

  APValue Value(ReflectionValue::RK_expr_result, CE);
  return SetAndSucceed(Result, Value);
}

bool data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue Scratch;
  size_t ArgIdx = 0;

  // Extract the data member type.
  if (!Evaluator(Scratch, Args[ArgIdx++], true) ||
      Scratch.getReflection().getKind() != ReflectionValue::RK_type)
    return true;
  QualType MemberTy = Scratch.getReflectedType();

  // Evaluate whether the data member is static.
  if (!Evaluator(Scratch, Args[ArgIdx++], true) || !Scratch.isInt())
    return true;
  bool IsStatic = static_cast<bool>(Scratch.getInt().getExtValue());

  // Evaluate whether a member name was provided.
  std::optional<std::string> Name;
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;

  // Evaluate the given name. Miserably inefficient, but gets the job done.
  if (static_cast<bool>(Scratch.getInt().getExtValue())) {
    // Evaluate 'name' length.
    if (!Evaluator(Scratch, Args[ArgIdx++], true))
      return true;
    size_t nameLen = Scratch.getInt().getExtValue();
    Name.emplace(nameLen, '\0');

    // Evaluate the character type.
    if (!Evaluator(Scratch, Args[ArgIdx++], true))
      return true;
    QualType CharTy = Scratch.getReflectedType();

    // Evaluate the data contents.
    for (uint64_t k = 0; k < nameLen; ++k) {
      llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                                 S.Context.getSizeType(),
                                                 Args[ArgIdx]->getExprLoc());

      Synthesized = new (S.Context) ArraySubscriptExpr(Args[ArgIdx],
                                                       Synthesized, CharTy,
                                                       VK_LValue, OK_Ordinary,
                                                       Range.getBegin());
      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return true;

      if (!Evaluator(Scratch, Synthesized, true))
        return true;

      (*Name)[k] = static_cast<char>(Scratch.getInt().getExtValue());
    }
    ArgIdx++;
  } else {
    ArgIdx += 3;
  }

  // Validate the name as an identifier.
  if (Name) {
    Lexer Lex(Range.getBegin(), S.getLangOpts(), Name->data(), Name->data(),
              Name->data() + Name->size(), false);
    if (!Lex.validateAndRewriteIdentifier(*Name))
      return true;
  }

  // Evaluate whether an alignment was provided.
  std::optional<size_t> Alignment;
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;

  if (static_cast<bool>(Scratch.getInt().getExtValue())) {
    // Evaluate 'alignment' value.
    if (!Evaluator(Scratch, Args[ArgIdx], true))
      return true;
    int alignment = Scratch.getInt().getExtValue();

    if (alignment < 0)
      return true;
    Alignment = static_cast<size_t>(alignment);
  }
  ArgIdx++;

  // Evaluate whether a bit width was provided.
  std::optional<size_t> BitWidth;
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;

  if (static_cast<bool>(Scratch.getInt().getExtValue())) {
    // Evaluate 'width' value.
    if (!Evaluator(Scratch, Args[ArgIdx], true))
      return true;
    int width = Scratch.getInt().getExtValue();

    if (width < 0)
      return true;
    BitWidth = static_cast<size_t>(width);
  }
  ArgIdx++;

  TagDataMemberSpec *TDMS = new (S.Context) TagDataMemberSpec {
    MemberTy, Name, IsStatic, Alignment, BitWidth
  };
  return SetAndSucceed(Result, makeReflection(TDMS));
}

bool define_class(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true) ||
      Scratch.getReflection().getKind() != ReflectionValue::RK_type)
    return true;
  QualType ToComplete = Scratch.getReflectedType();
  TagDecl *OGTag, *Tag;
  {
    NamedDecl *ND;
    if (!ToComplete->isIncompleteType(&ND))
      return true;
    OGTag = Tag = cast<TagDecl>(ND);
  }

  unsigned TypeSpec = TST_struct;
  if (Tag->getTagKind() == TagTypeKind::Class)
    TypeSpec = TST_class;
  else if (Tag->getTagKind() == TagTypeKind::Union)
    TypeSpec = TST_union;

  TypeResult TR;
  SmallVector<TemplateParameterList *, 1> MTP;

  DeclContext *OGDC = S.CurContext;
  Scope ClsScope(S.getCurScope(), Scope::ClassScope | Scope::DeclScope,
                 S.Diags);
  ClsScope.setEntity(OGTag->getDeclContext());

  S.CurContext = OGTag->getDeclContext();
  auto RestoreDC = [&]() { S.CurContext = OGDC; };

  CXXScopeSpec SS;

  bool OwnedDecl = true, IsDependent = false;
  DeclResult DR;
  if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(OGTag)) {
    SmallVector<TemplateIdAnnotation *, 1> CleanupList;

    TemplateName TDecl(CTSD->getSpecializedTemplate());
    ParsedTemplateTy ParsedTemplate = ParsedTemplateTy::make(TDecl);


    SmallVector<TemplateArgument, 4> TArgs;
    expandTemplateArgPacks(CTSD->getTemplateArgs().asArray(), TArgs);

    SmallVector<ParsedTemplateArgument, 2> ParsedArgs;
    for (const TemplateArgument &TArg : TArgs) {
      switch (TArg.getKind()) {
      case TemplateArgument::Type:
        ParsedArgs.emplace_back(ParsedTemplateArgument::Type,
                                TArg.getAsType().getAsOpaquePtr(),
                                SourceLocation());
        break;
      case TemplateArgument::Integral: {
        ConstantExpr *CE =
            ConstantExpr::CreateEmpty(S.Context,
                                      ConstantResultStorageKind::APValue);
        CE->setType(TArg.getIntegralType());
        CE->setValueKind(VK_PRValue);
        CE->SetResult(APValue(TArg.getAsIntegral()), S.Context);

        ParsedArgs.emplace_back(ParsedTemplateArgument::NonType, CE,
                                SourceLocation());
        break;
      }
      case TemplateArgument::Template: {
        ParsedTemplateTy Parsed = ParsedTemplateTy::make(TArg.getAsTemplate());
        ParsedArgs.emplace_back(SS, Parsed, SourceLocation());
        break;
      }
      default:
        llvm_unreachable("unimplemented");
      }
    }

    TemplateIdAnnotation *TAnnot = TemplateIdAnnotation::Create(
          SourceLocation(), SourceLocation(), OGTag->getIdentifier(), OO_None,
          ParsedTemplate, TNK_Type_template, SourceLocation(), SourceLocation(),
          ParsedArgs, false, CleanupList);

    MTP.push_back(S.ActOnTemplateParameterList(0, SourceLocation(),
                  SourceLocation(), SourceLocation(), std::nullopt,
                  SourceLocation(), nullptr));
    DR = S.ActOnClassTemplateSpecialization(
          &ClsScope, TypeSpec, TagUseKind::Definition, Args[0]->getBeginLoc(),
          SourceLocation(), SS, *TAnnot, ParsedAttributesView::none(), MTP,
          nullptr);
    MTP.clear();

    for (auto *TAnnot : CleanupList)
      TAnnot->Destroy();
  } else {
    // If necessary, inject the tag declaration that is to be completed into the
    // current scope. This is needed to ensure that the created Decl is
    // constructed as a redeclaration of the provided incomplete Decl.
    //
    // A more robust design might allow 'ActOnTag' to take a 'PrevDecl' as an
    // input, rather than require that it be found by name lookup.
    bool InjectDecl = true;
    for (Scope *Sc = S.getCurScope(); Sc; Sc = Sc->getParent())
      if (Sc->isDeclScope(OGTag)) {
        InjectDecl = false;
        break;
      }
    if (InjectDecl) {
      S.getCurScope()->AddDecl(OGTag);
      S.IdResolver.AddDecl(OGTag);
    }

    // Create the new tag in the current scope.
    DR = S.ActOnTag(S.getCurScope(), TypeSpec, TagUseKind::Definition,
                    Args[0]->getBeginLoc(), SS, Tag->getIdentifier(),
                    Tag->getBeginLoc(), ParsedAttributesView::none(),
                    AS_none, SourceLocation(), MTP, OwnedDecl, IsDependent,
                    SourceLocation(), false, TR, false, false,
                    Sema::OOK_Outside, nullptr);

    // The new tag -should- declare the same entity as the original tag.
    assert(declaresSameEntity(OGTag, Tag) &&
         "New tag should declare same entity as original tag (scope problem?)");
  }
  if (DR.isInvalid()) {
    RestoreDC();
    return true;
  }
  Tag = cast<TagDecl>(DR.get());

  S.ActOnTagStartDefinition(&ClsScope, Tag);
  S.ActOnStartCXXMemberDeclarations(&ClsScope, Tag, SourceLocation(),
                                    false, false, SourceLocation());

  // Evaluate the number of members provided.
  if (!Evaluator(Scratch, Args[1], true)) {
    RestoreDC();
    return true;
  }
  size_t NumMembers = static_cast<size_t>(Scratch.getInt().getExtValue());

  AccessSpecifier MemberAS = (Tag->isClass() ? AS_private : AS_public);

  // Iterate over members, and add them to the class definition.
  size_t anonMemCtr = 0;
  for (size_t k = 0; k < NumMembers; ++k) {
    // Extract the reflection from the list of member specs.
    llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
    Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                               S.Context.getSizeType(),
                                               Args[2]->getExprLoc());

    Synthesized = new (S.Context) ArraySubscriptExpr(Args[2], Synthesized,
                                                     S.Context.MetaInfoTy,
                                                     VK_LValue, OK_Ordinary,
                                                     Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent()) {
      RestoreDC();
      return true;
    }

    if (!Evaluator(Scratch, Synthesized, true) ||
        Scratch.getReflection().getKind() !=
              ReflectionValue::RK_data_member_spec) {
      RestoreDC();
      return true;
    }
    TagDataMemberSpec *TDMS = Scratch.getReflectedDataMemberSpec();


    // Build the member declaration from the extracted TagDataMemberSpec.
    unsigned DiagID;
    const char *PrevSpec;
    AttributeFactory AttrFactory;
    AttributePool AttrPool(AttrFactory);
    DeclSpec DS(AttrFactory);
    DS.SetStorageClassSpec(S, TDMS->IsStatic ? DeclSpec::SCS_static :
                                               DeclSpec::SCS_unspecified,
                           Args[0]->getBeginLoc(),
                           PrevSpec, DiagID, S.Context.getPrintingPolicy());

    ParsedType MemberTy = ParsedType::make(TDMS->Ty);
    DS.SetTypeSpecType(TST_typename, Args[0]->getBeginLoc(), PrevSpec, DiagID,
                       MemberTy, S.Context.getPrintingPolicy());

    // Add any attributes (i.e., 'AlignAttr' if alignment is specified).
    ParsedAttributesView MemberAttrs;
    if (TDMS->Alignment) {
      IdentifierInfo &AttrII = S.PP.getIdentifierTable().get("alignas");

      ConstantExpr *CE =
          ConstantExpr::CreateEmpty(S.Context,
                                    ConstantResultStorageKind::APValue);
      CE->setType(S.Context.getSizeType());
      CE->setValueKind(VK_PRValue);
      CE->SetResult(APValue(llvm::APSInt::getUnsigned(TDMS->Alignment.value())),
                    S.Context);

      ArgsUnion Args(CE);
      ParsedAttr::Form Form(tok::kw_alignas);
      ParsedAttr *Attr = AttrPool.create(&AttrII, Range, nullptr,
                                         SourceLocation(), &Args, 1, Form);
      MemberAttrs.addAtEnd(Attr);
    }

    Declarator MemberDeclarator(DS, MemberAttrs, DeclaratorContext::Member);

    std::string MemberName = TDMS->Name.value_or(
            "__" + llvm::toString(llvm::APSInt::get(anonMemCtr++), 10));
    IdentifierInfo &II = S.PP.getIdentifierTable().get(MemberName);
    MemberDeclarator.SetIdentifier(&II, Tag->getBeginLoc());

    // Create an expression for bit width, if specified.
    ConstantExpr *BitWidthCE = nullptr;
    if (TDMS->BitWidth) {
      BitWidthCE =
          ConstantExpr::CreateEmpty(S.Context,
                                    ConstantResultStorageKind::APValue);
      BitWidthCE->setType(S.Context.getSizeType());
      BitWidthCE->setValueKind(VK_PRValue);
      BitWidthCE->SetResult(
            APValue(llvm::APSInt::getUnsigned(TDMS->BitWidth.value())),
            S.Context);
    }

    VirtSpecifiers VS;
    S.ActOnCXXMemberDeclarator(&ClsScope, MemberAS, MemberDeclarator, MTP,
                               BitWidthCE, VS, ICIS_NoInit);
  }

  S.ActOnFinishCXXMemberSpecification(&ClsScope, Tag->getBeginLoc(), Tag,
                                      SourceLocation(), SourceLocation(),
                                      ParsedAttributesView::none());

  S.ActOnTagFinishDefinition(&ClsScope, Tag, Args[0]->getSourceRange());
  S.ActOnPopScope(Range.getEnd(), &ClsScope);

  RestoreDC();
  return SetAndSucceed(Result, makeReflection(ToComplete));
}

bool offset_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
               SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_declaration: {
    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(R.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(S.Context, FD) /
                      S.Context.getTypeSize(S.Context.CharTy);
      return SetAndSucceed(
              Result,
              APValue(S.Context.MakeIntValue(Offset, S.Context.getSizeType())));
    }
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool size_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    size_t Sz = S.Context.getTypeSizeInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_expr_result: {
    const Expr *E = R.getReflectedExprResult();
    size_t Sz = S.Context.getTypeSizeInChars(E->getType()).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_declaration: {
    const ValueDecl *VD = cast<ValueDecl>(R.getReflectedDecl());
    size_t Sz = S.Context.getTypeSizeInChars(VD->getType()).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_offset_of(APValue &Result, Sema &S, EvalFn Evaluator,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_declaration: {
    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(R.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(S.Context, FD) %
                      S.Context.getTypeSize(S.Context.CharTy);
      return SetAndSucceed(
              Result,
              APValue(S.Context.MakeIntValue(Offset, S.Context.getSizeType())));
    }
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_size_of(APValue &Result, Sema &S, EvalFn Evaluator, QualType ResultTy,
                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    size_t Sz = S.Context.getTypeSize(QT);
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_expr_result: {
    const Expr *E = R.getReflectedExprResult();
    size_t Sz = S.Context.getTypeSize(E->getType());
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_declaration: {
    const ValueDecl *VD = cast<ValueDecl>(R.getReflectedDecl());
    size_t Sz = S.Context.getTypeSize(VD->getType());

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD))
      if (FD->isBitField())
        Sz = FD->getBitWidthValue(S.Context);

    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool alignment_of(APValue &Result, Sema &S, EvalFn Evaluator,
                  QualType ResultTy, SourceRange Range,
                  ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    QualType QT = R.getReflectedType();
    size_t Align = S.Context.getTypeAlignInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_expr_result: {
    const Expr *E = R.getReflectedExprResult();
    size_t Align = S.Context.getTypeAlignInChars(E->getType()).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_declaration: {
    const ValueDecl *VD = cast<ValueDecl>(R.getReflectedDecl());
    size_t Align = S.Context.getTypeAlignInChars(VD->getType()).getQuantity();

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD))
      Align = S.Context.getDeclAlign(FD, true).getQuantity();

    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_parameter_of(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.getReflection().getKind() == ReflectionValue::RK_type);

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_type: {
    if (auto FT = dyn_cast<FunctionProtoType>(R.getReflectedType())) {
      unsigned numParams = FT->getNumParams();
      if (idx >= numParams)
        return SetAndSucceed(Result, Sentinel);

      return SetAndSucceed(Result, makeReflection(FT->getParamType(idx)));
    }
    return true;
  }
  case ReflectionValue::RK_declaration: {
    if (auto FD = dyn_cast<FunctionDecl>(R.getReflectedDecl())) {
      unsigned numParams = FD->getNumParams();
      if (idx >= numParams)
        return SetAndSucceed(Result, Sentinel);

      return SetAndSucceed(Result, makeReflection(FD->getParamDecl(idx)));
    }
    return true;
  }
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_consistent_name(APValue &Result, Sema &S, EvalFn Evaluator,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(R.getReflectedDecl())) {
      bool Consistent = parameterHasConsistentName(PVD);
      return SetAndSucceed(Result, makeBool(S.Context, Consistent));
    }
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_ellipsis_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_type:
    if (auto *FPT = dyn_cast<FunctionProtoType>(R.getReflectedType())) {
      bool HasEllipsis = FPT->isVariadic();
      return SetAndSucceed(Result, makeBool(S.Context, HasEllipsis));
    }
    return true;
  case ReflectionValue::RK_declaration: {
    if (auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl())) {
      bool HasEllipsis = FD->getEllipsisLoc().isValid();
      return SetAndSucceed(Result, makeBool(S.Context, HasEllipsis));
    }
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_default_argument(APValue &Result, Sema &S, EvalFn Evaluator,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_type:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(R.getReflectedDecl())) {
      return SetAndSucceed(Result, makeBool(S.Context, PVD->hasDefaultArg()));
    }
    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_explicit_object_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(R.getReflectedDecl()))
      result = PVD->isExplicitObjectParameter();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_function_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                           QualType ResultTy, SourceRange Range,
                           ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  bool result = false;
  if (R.getReflection().getKind() == ReflectionValue::RK_declaration) {
    result = isa<const ParmVarDecl>(R.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool return_type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue R;
  if (!Evaluator(R, Args[0], true))
    return true;

  switch (R.getReflection().getKind()) {
  case ReflectionValue::RK_null:
  case ReflectionValue::RK_expr_result:
  case ReflectionValue::RK_template:
  case ReflectionValue::RK_namespace:
  case ReflectionValue::RK_base_specifier:
  case ReflectionValue::RK_data_member_spec:
    return true;
  case ReflectionValue::RK_type:
    if (auto *FPT = dyn_cast<FunctionProtoType>(R.getReflectedType()))
      return SetAndSucceed(Result, makeReflection(FPT->getReturnType()));

    return true;
  case ReflectionValue::RK_declaration: {
    if (auto *FD = dyn_cast<FunctionDecl>(R.getReflectedDecl());
        FD && !isa<CXXConstructorDecl>(FD) && !isa<CXXDestructorDecl>(FD))
      return SetAndSucceed(Result, makeReflection(FD->getReturnType()));

    return true;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

}  // end namespace clang
