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
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Reflection.h"
#include "clang/Basic/DiagnosticMetafn.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
#include "clang/Sema/Metafunction.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/TemplateDeduction.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"


namespace clang {

using EvalFn = Metafunction::EvaluateFn;
using DiagFn = Metafunction::DiagnoseFn;

// -----------------------------------------------------------------------------
// P2996 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_begin_enumerator_decl_of(APValue &Result, Sema &S,
                                         EvalFn Evaluator, DiagFn Diagnoser,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool get_next_enumerator_decl_of(APValue &Result, Sema &S,
                                        EvalFn Evaluator, DiagFn Diagnoser,
                                        QualType ResultTy,
                                        SourceRange Range,
                                        ArrayRef<Expr *> Args);

static bool get_ith_base_of(APValue &Result, Sema &S,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args);

static bool get_ith_template_argument_of(APValue &Result, Sema &S,
                                         EvalFn Evaluator, DiagFn Diagnoser,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool get_begin_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                     DiagFn Diagnoser, QualType ResultTy,
                                     SourceRange Range, ArrayRef<Expr *> Args);

static bool get_next_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                    DiagFn Diagnoser, QualType ResultTy,
                                    SourceRange Range, ArrayRef<Expr *> Args);

static bool map_decl_to_entity(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args);

static bool identifier_of(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool has_identifier(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool operator_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool source_location_of(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args);

static bool type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool parent_of(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool dealias(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool value_of(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool object_of(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool template_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool can_substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool extract(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_public(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool is_protected(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_private(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_access_specified(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool access_context(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool is_accessible(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool is_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_pure_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args);

static bool is_override(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_deleted(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_defaulted(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_explicit(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_noexcept(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_bit_field(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_enumerator(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool is_const(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool is_volatile(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_lvalue_reference_qualified(APValue &Result, Sema &S,
                                          EvalFn Evaluator, DiagFn Diagnoser,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args);

static bool is_rvalue_reference_qualified(APValue &Result, Sema &S,
                                          EvalFn Evaluator, DiagFn Diagnoser,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args);

static bool has_static_storage_duration(APValue &Result, Sema &S,
                                        EvalFn Evaluator, DiagFn Diagnoser,
                                        QualType ResultTy, SourceRange Range,
                                        ArrayRef<Expr *> Args);

static bool has_thread_storage_duration(APValue &Result, Sema &S,
                                        EvalFn Evaluator, DiagFn Diagnoser,
                                        QualType ResultTy, SourceRange Range,
                                        ArrayRef<Expr *> Args);

static bool has_automatic_storage_duration(APValue &Result, Sema &S,
                                           EvalFn Evaluator, DiagFn Diagnoser,
                                           QualType ResultTy, SourceRange Range,
                                           ArrayRef<Expr *> Args);

static bool has_internal_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool has_module_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args);

static bool has_external_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool has_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_class_member(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args);

static bool is_namespace_member(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool is_nonstatic_data_member(APValue &Result, Sema &S,
                                     EvalFn Evaluator, DiagFn Diagnoser,
                                     QualType ResultTy, SourceRange Range,
                                     ArrayRef<Expr *> Args);

static bool is_static_member(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args);

static bool is_base(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool is_namespace(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool is_function(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_variable(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_type(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool is_alias(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool is_complete_type(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args);

static bool is_template(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool is_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool is_variable_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool is_class_template(APValue &Result, Sema &S, EvalFn Evaluator,
                              DiagFn Diagnoser, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args);

static bool is_alias_template(APValue &Result, Sema &S, EvalFn Evaluator,
                              DiagFn Diagnoser, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args);

static bool is_conversion_function_template(APValue &Result, Sema &S,
                                            EvalFn Evaluator, DiagFn Diagnoser,
                                            QualType ResultTy,
                                            SourceRange Range,
                                            ArrayRef<Expr *> Args);

static bool is_operator_function_template(APValue &Result, Sema &S,
                                          EvalFn Evaluator, DiagFn Diagnoser,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args);

static bool is_literal_operator_template(APValue &Result, Sema &S,
                                         EvalFn Evaluator, DiagFn Diagnoser,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool is_constructor_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                    DiagFn Diagnoser, QualType ResultTy,
                                    SourceRange Range, ArrayRef<Expr *> Args);

static bool is_concept(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args);

static bool is_structured_binding(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args);

static bool is_value(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args);

static bool is_object(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool has_template_arguments(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args);

static bool has_default_member_initializer(APValue &Result, Sema &S,
                                           EvalFn Evaluator, DiagFn Diagnoser,
                                           QualType ResultTy, SourceRange Range,
                                           ArrayRef<Expr *> Args);

static bool is_conversion_function(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args);

static bool is_operator_function(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool is_literal_operator(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool is_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool is_default_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args);

static bool is_copy_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool is_move_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args);

static bool is_assignment(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool is_copy_assignment(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args);

static bool is_move_assignment(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args);

static bool is_destructor(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool is_special_member_function(APValue &Result, Sema &S,
                                       EvalFn Evaluator, DiagFn Diagnoser,
                                       QualType ResultTy, SourceRange Range,
                                       ArrayRef<Expr *> Args);

static bool is_user_provided(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args);

static bool reflect_result(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool reflect_invoke(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

static bool data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args);

static bool define_class(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool offset_of(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args);

static bool size_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args);

static bool bit_offset_of(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args);

static bool bit_size_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args);

static bool alignment_of(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args);

static bool define_static_string(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

// -----------------------------------------------------------------------------
// P3096 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_ith_parameter_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool has_consistent_identifier(APValue &Result, Sema &S,
                                      EvalFn Evaluator, DiagFn Diagnoser,
                                      QualType ResultTy, SourceRange Range,
                                      ArrayRef<Expr *> Args);

static bool has_ellipsis_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args);

static bool has_default_argument(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args);

static bool is_explicit_object_parameter(APValue &Result, Sema &S,
                                         EvalFn Evaluator, DiagFn Diagnoser,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args);

static bool is_function_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args);

static bool return_type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args);

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
  { Metafunction::MFRK_spliceFromArg, 4, 4, identifier_of },
  { Metafunction::MFRK_bool, 1, 1, has_identifier },
  { Metafunction::MFRK_sizeT, 1, 1, operator_of },
  { Metafunction::MFRK_sourceLoc, 1, 1, source_location_of },
  { Metafunction::MFRK_metaInfo, 1, 1, type_of },
  { Metafunction::MFRK_metaInfo, 1, 1, parent_of },
  { Metafunction::MFRK_metaInfo, 1, 1, dealias },
  { Metafunction::MFRK_metaInfo, 1, 1, object_of },
  { Metafunction::MFRK_metaInfo, 1, 1, value_of },
  { Metafunction::MFRK_metaInfo, 1, 1, template_of },
  { Metafunction::MFRK_bool, 3, 3, can_substitute },
  { Metafunction::MFRK_metaInfo, 3, 3, substitute },
  { Metafunction::MFRK_spliceFromArg, 2, 2, extract },
  { Metafunction::MFRK_bool, 1, 1, is_public },
  { Metafunction::MFRK_bool, 1, 1, is_protected },
  { Metafunction::MFRK_bool, 1, 1, is_private },
  { Metafunction::MFRK_bool, 1, 1, is_access_specified },
  { Metafunction::MFRK_metaInfo, 0, 0, access_context },
  { Metafunction::MFRK_bool, 1, 2, is_accessible },
  { Metafunction::MFRK_bool, 1, 1, is_virtual },
  { Metafunction::MFRK_bool, 1, 1, is_pure_virtual },
  { Metafunction::MFRK_bool, 1, 1, is_override },
  { Metafunction::MFRK_bool, 1, 1, is_deleted },
  { Metafunction::MFRK_bool, 1, 1, is_defaulted },
  { Metafunction::MFRK_bool, 1, 1, is_explicit },
  { Metafunction::MFRK_bool, 1, 1, is_noexcept },
  { Metafunction::MFRK_bool, 1, 1, is_bit_field },
  { Metafunction::MFRK_bool, 1, 1, is_enumerator },
  { Metafunction::MFRK_bool, 1, 1, is_const },
  { Metafunction::MFRK_bool, 1, 1, is_volatile },
  { Metafunction::MFRK_bool, 1, 1, is_lvalue_reference_qualified },
  { Metafunction::MFRK_bool, 1, 1, is_rvalue_reference_qualified },
  { Metafunction::MFRK_bool, 1, 1, has_static_storage_duration },
  { Metafunction::MFRK_bool, 1, 1, has_thread_storage_duration },
  { Metafunction::MFRK_bool, 1, 1, has_automatic_storage_duration },
  { Metafunction::MFRK_bool, 1, 1, has_internal_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_module_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_external_linkage },
  { Metafunction::MFRK_bool, 1, 1, has_linkage },
  { Metafunction::MFRK_bool, 1, 1, is_class_member },
  { Metafunction::MFRK_bool, 1, 1, is_namespace_member },
  { Metafunction::MFRK_bool, 1, 1, is_nonstatic_data_member },
  { Metafunction::MFRK_bool, 1, 1, is_static_member },
  { Metafunction::MFRK_bool, 1, 1, is_base },
  { Metafunction::MFRK_bool, 1, 1, is_data_member_spec },
  { Metafunction::MFRK_bool, 1, 1, is_namespace },
  { Metafunction::MFRK_bool, 1, 1, is_function },
  { Metafunction::MFRK_bool, 1, 1, is_variable },
  { Metafunction::MFRK_bool, 1, 1, is_type },
  { Metafunction::MFRK_bool, 1, 1, is_alias },
  { Metafunction::MFRK_bool, 1, 1, is_complete_type },
  { Metafunction::MFRK_bool, 1, 1, is_template },
  { Metafunction::MFRK_bool, 1, 1, is_function_template },
  { Metafunction::MFRK_bool, 1, 1, is_variable_template },
  { Metafunction::MFRK_bool, 1, 1, is_class_template },
  { Metafunction::MFRK_bool, 1, 1, is_alias_template },
  { Metafunction::MFRK_bool, 1, 1, is_conversion_function_template },
  { Metafunction::MFRK_bool, 1, 1, is_operator_function_template },
  { Metafunction::MFRK_bool, 1, 1, is_literal_operator_template },
  { Metafunction::MFRK_bool, 1, 1, is_constructor_template },
  { Metafunction::MFRK_bool, 1, 1, is_concept },
  { Metafunction::MFRK_bool, 1, 1, is_structured_binding },
  { Metafunction::MFRK_bool, 1, 1, is_value },
  { Metafunction::MFRK_bool, 1, 1, is_object },
  { Metafunction::MFRK_bool, 1, 1, has_template_arguments },
  { Metafunction::MFRK_bool, 1, 1, has_default_member_initializer },
  { Metafunction::MFRK_bool, 1, 1, is_conversion_function },
  { Metafunction::MFRK_bool, 1, 1, is_operator_function },
  { Metafunction::MFRK_bool, 1, 1, is_literal_operator },
  { Metafunction::MFRK_bool, 1, 1, is_constructor },
  { Metafunction::MFRK_bool, 1, 1, is_default_constructor },
  { Metafunction::MFRK_bool, 1, 1, is_copy_constructor },
  { Metafunction::MFRK_bool, 1, 1, is_move_constructor },
  { Metafunction::MFRK_bool, 1, 1, is_assignment },
  { Metafunction::MFRK_bool, 1, 1, is_copy_assignment },
  { Metafunction::MFRK_bool, 1, 1, is_move_assignment },
  { Metafunction::MFRK_bool, 1, 1, is_destructor },
  { Metafunction::MFRK_bool, 1, 1, is_special_member_function },
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
  { Metafunction::MFRK_spliceFromArg, 5, 5, define_static_string },

  // P3096 metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_parameter_of },
  { Metafunction::MFRK_bool, 1, 1, has_consistent_identifier },
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
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args) const {
  return ImplFn(Result, S, Evaluator, Diagnoser, ResultTy, Range, Args);
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

static APValue makeBool(ASTContext &C, bool B) {
  return APValue(C.MakeIntValue(B, C.BoolTy));
}

static APValue makeReflection(QualType QT) {
  return APValue(ReflectionKind::Type, QT.getAsOpaquePtr());
}

static APValue makeReflection(Decl *D) {
  if (isa<NamespaceDecl>(D) || isa<NamespaceAliasDecl>(D) ||
      isa<TranslationUnitDecl>(D))
    return APValue(ReflectionKind::Namespace, D);

  return APValue(ReflectionKind::Declaration, D);
}

static APValue makeReflection(TemplateName TName) {
  return APValue(ReflectionKind::Template, TName.getAsVoidPointer());
}

static APValue makeReflection(CXXBaseSpecifier *Base) {
  return APValue(ReflectionKind::BaseSpecifier, Base);
}

static APValue makeReflection(TagDataMemberSpec *TDMS) {
  return APValue(ReflectionKind::DataMemberSpec, TDMS);
}

static Expr *makeStrLiteral(StringRef Str, ASTContext &C, bool Utf8) {
  QualType ConstCharTy = (Utf8 ? C.Char8Ty : C.CharTy).withConst();

  // Get the type for 'const char[Str.size()]'.
  QualType StrLitTy =
        C.getConstantArrayType(ConstCharTy, llvm::APInt(32, Str.size() + 1),
                               nullptr, ArraySizeModifier::Normal, 0);

  // Create a string literal having type 'const char [Str.size()]'.
  StringLiteralKind SLK = Utf8 ? StringLiteralKind::UTF8 :
                                 StringLiteralKind::Ordinary;
  return StringLiteral::Create(C, Str, SLK, false, StrLitTy, SourceLocation{});
}

static bool SetAndSucceed(APValue &Out, const APValue &Result) {
  Out = Result;
  return false;
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

  if (auto *TST = dyn_cast<TemplateSpecializationType>(QT)) {
    TemplateName TName = TST->getTemplateName();
    if (TName.getKind() == TemplateName::QualifiedTemplate)
      TName = TName.getAsQualifiedTemplateName()->getUnderlyingTemplate();
    return TName;
  }

  if (auto *CXXRD = QT->getAsCXXRecordDecl())
    if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD))
      return TemplateName(CTSD->getSpecializedTemplate());

  return TemplateName();
}

static void getTemplateName(std::string &Result, ASTContext &C,
                            TemplateName TName) {
  PrintingPolicy PP = C.getPrintingPolicy();
  {
    llvm::raw_string_ostream NameOut(Result);
    TName.print(NameOut, PP, TemplateName::Qualified::None);
  }
}

static void getDeclName(std::string &Result, ASTContext &C, Decl *D) {
  if (TemplateName TName = findTemplateOfDecl(D); !TName.isNull())
    return getTemplateName(Result, C, TName);

  PrintingPolicy PP = C.getPrintingPolicy();
  {
    llvm::raw_string_ostream NameOut(Result);
    if (auto *ND = dyn_cast<NamedDecl>(D);
        ND && !isa<TemplateParamObjectDecl>(D))
      ND->printName(NameOut, PP);
  }
}

static bool getParameterName(ParmVarDecl *PVD, std::string &Out) {
  StringRef FirstNameSeen = PVD->getName();
  unsigned ParamIdx = PVD->getFunctionScopeIndex();

  FunctionDecl *FD = cast<FunctionDecl>(PVD->getDeclContext());
  FD = FD->getMostRecentDecl();

  bool Consistent = true;

  PVD = FD->getParamDecl(ParamIdx);
  while (PVD) {
    FD = cast<FunctionDecl>(PVD->getDeclContext());
    FD = FD->getPreviousDecl();
    if (!FD) {
      Out = FirstNameSeen;
      return true;
    }

    PVD = FD->getParamDecl(ParamIdx);
    assert(PVD);
    if (IdentifierInfo *II = PVD->getIdentifier()) {
      if (FirstNameSeen.empty()) {
        FirstNameSeen = II->getName();
      } else if (II->getName() != FirstNameSeen) {
        Consistent = false;
        break;
      }
    }
  }
  Out = FirstNameSeen;
  return Consistent;
}

static NamedDecl *findTypeDecl(QualType QT) {
  // If it's an ElaboratedType, get the underlying NamedType.
  if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(QT))
    QT = ET->getNamedType();

  // Get the type's declaration.
  NamedDecl *D = nullptr;
  if (auto *TDT = dyn_cast<TypedefType>(QT))
    D = TDT->getDecl();
  else if (auto *UT = dyn_cast<UsingType>(QT))
    D = UT->getFoundDecl();
  else if (auto *TD = QT->getAsTagDecl())
    return TD;
  else if (auto *TT = dyn_cast<TagType>(QT))
    D = TT->getDecl();
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
    else if (auto *RST = dyn_cast<ReflectionSpliceType>(QT))
      QT = RST->desugar();
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

bool getTemplateArgumentsFromDecl(Decl* D,
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
  if (Idx >= templateArgs.size())
    return Sentinel;

  const auto& templArgument = templateArgs[Idx];
  switch (templArgument.getKind()) {
    case TemplateArgument::Type:
      return makeReflection(templArgument.getAsType());
    case TemplateArgument::Expression: {
      Expr *TExpr = templArgument.getAsExpr();

      APValue ArgResult;
      bool success = Evaluator(ArgResult, TExpr, !TExpr->isLValue());
      assert(success);

      return ArgResult.Lift(TExpr->getType());
    }
    case TemplateArgument::Template: {
      TemplateName TName = templArgument.getAsTemplate();
      if (TName.getKind() == TemplateName::QualifiedTemplate)
        TName = TName.getAsQualifiedTemplateName()->getUnderlyingTemplate();
      return makeReflection(TName);
    } case TemplateArgument::Declaration:
      return makeReflection(templArgument.getAsDecl());
    case TemplateArgument::NullPtr: {
      APValue NullPtrValue((ValueDecl *)nullptr,
                           CharUnits::fromQuantity(
                               S.Context.getTargetNullPointerValue(
                                   templArgument.getNullPtrType())),
                           APValue::NoLValuePath(),
                           /*IsNullPtr=*/true);
      return NullPtrValue.Lift(templArgument.getNullPtrType());
    }
    case TemplateArgument::StructuralValue: {
      APValue SV = templArgument.getAsStructuralValue();
      return SV.Lift(templArgument.getStructuralValueType());
    }
    case TemplateArgument::Integral: {
      APValue IV(templArgument.getAsIntegral());
      return IV.Lift(templArgument.getIntegralType());
    }
    case TemplateArgument::SpliceSpecifier:
      llvm_unreachable("TemplateArgument::SpliceSpecifier should have been "
                       "transformed by now");
    case TemplateArgument::Pack:
      llvm_unreachable("Packs should be expanded before calling this");

    // Could not get a test case to hit one of the below
    case TemplateArgument::Null:
      llvm_unreachable("TemplateArgument::Null not supported");
    case TemplateArgument::TemplateExpansion:
      llvm_unreachable("TemplateArgument::TemplateExpansion not supported");
  }
  llvm_unreachable("Unknown template argument type");
}

static bool isTemplateSpecialization(QualType QT) {
  if (isa<UsingType>(QT) || isa<TypedefType>(QT))
    return false;

  return isa<TemplateSpecializationType>(QT) ||
      isa<DependentTemplateSpecializationType>(QT) ||
      isa_and_nonnull<ClassTemplateSpecializationDecl>(
          QT->getAsCXXRecordDecl());
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

  if (isa<NamespaceAliasDecl>(D))
    return true;

  if (!isa<VarDecl, FunctionDecl, TypeDecl, FieldDecl, TemplateDecl,
           NamespaceDecl, NamespaceAliasDecl, TranslationUnitDecl>(D))
    return false;

  if (auto *Class = dyn_cast<CXXRecordDecl>(D))
    if (Class->isInjectedClassName())
      return false;

  if (isa<ClassTemplatePartialSpecializationDecl,
          VarTemplatePartialSpecializationDecl>(D))
    return false;

  return D->getCanonicalDecl() == D;
}

/// Filter non-reflectable members.
static Decl *findIterableMember(ASTContext &C, Decl *D, bool Inclusive) {
  if (!D)
    return D;

  if (Inclusive) {
    if (isReflectableDecl(C, D))
      return D;

    // Handle the case where the first Decl is a LinkageSpecDecl.
    if (auto *LSDecl = dyn_cast_or_null<LinkageSpecDecl>(D)) {
      Decl *RecD = findIterableMember(C, *LSDecl->decls_begin(), true);
      if (RecD) return RecD;
    }
  }

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
    do {
      D = D->getNextDeclInContext();
    } while (D && D->getDeclContext() != DC);

    // In the case of namespaces, walk the redeclaration chain.
    if (auto *NSDecl = dyn_cast<NamespaceDecl>(DC)) {
      while (!D && NSDecl) {
        NSDecl = NSDecl->getPreviousDecl();
        D = NSDecl ? *NSDecl->decls_begin() : nullptr;
      }
    }

    // We need to recursively descend into LinkageSpecDecls to iterate over the
    // members declared therein (e.g., `extern "C"` blocks).
    if (auto *LSDecl = dyn_cast_or_null<LinkageSpecDecl>(D)) {
      Decl *RecD = findIterableMember(C, *LSDecl->decls_begin(), true);
      if (RecD) return RecD;
    }

    // Pop back out of a recursively entered LinkageSpecDecl.
    if (!D && isa<LinkageSpecDecl>(DC))
      return findIterableMember(C, cast<Decl>(DC), false);
  } while (D && !isReflectableDecl(C, D));

  return D;
}

unsigned parentOf(APValue &Result, Decl *D) {
  if (!D)
    return diag::metafn_parent_of_undeclared;

  auto *DC = D->getDeclContext();
  while (DC && !isa<NamespaceDecl>(DC) && !isa<RecordDecl>(DC) &&
               !isa<FunctionDecl>(DC) && !isa<TranslationUnitDecl>(DC))
    DC = DC->getParent();

  assert(DC);
  if (auto *RD = dyn_cast<RecordDecl>(DC))
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

static bool isConstQualifiedType(QualType QT) {
  bool result = QT.isConstQualified();
  if (auto *FPT = dyn_cast<FunctionProtoType>(QT))
    result |= FPT->isConst();

  return result;
}

static bool isVolatileQualifiedType(QualType QT) {
  bool result = QT.isVolatileQualified();
  if (auto *FPT = dyn_cast<FunctionProtoType>(QT))
    result |= FPT->isVolatile();

  return result;
}

QualType Sema::ComputeResultType(QualType ExprTy, const APValue &V) {
  SplitQualType SQT;

  if (V.isLValue() && !ExprTy->isPointerType() &&
      !V.getLValueBase().isNull()) {
    SQT = V.getLValueBase().getType().split();

    for (auto p = V.getLValuePath().begin();
         p != V.getLValuePath().end(); ++p) {
      const Decl *D = V.getLValuePath().back().getAsBaseOrMember().getPointer();
      if (D) {  // base or member case
        if (auto *VD = dyn_cast<FieldDecl>(D)) {
          QualType QT = VD->getType();
          SQT.Ty = QT.getTypePtr();

          if (QT.isConstQualified()) SQT.Quals.addConst();
          if (QT.isVolatileQualified()) SQT.Quals.addVolatile();

          continue;
        } else if (auto *TD = dyn_cast<CXXRecordDecl>(D)) {
          SQT.Ty = TD->getTypeForDecl();
          continue;
        }

        llvm_unreachable("unknown lvalue path kind");
      } else { // array case
        QualType QT = cast<ArrayType>(SQT.Ty)->getElementType();
        SQT.Ty = QT.getTypePtr();
        if (QT.isConstQualified()) SQT.Quals.addConst();
        if (QT.isVolatileQualified()) SQT.Quals.addVolatile();
      }
    }
    return QualType(SQT.Ty, SQT.Quals.getAsOpaqueValue());
  }
  return desugarType(ExprTy, /*UnwrapAliases=*/true,
                     /*DropCV=*/!ExprTy->isRecordType(),
                     /*DropRefs=*/true);
}


// -----------------------------------------------------------------------------
// Diagnostic helper function
// -----------------------------------------------------------------------------

StringRef DescriptionOf(APValue RV, bool Granular = true) {
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
    return "a null reflection";
  case ReflectionKind::Type:
    if (isTypeAlias(RV.getReflectedType())) return "type alias";
    else return "a type";
  case ReflectionKind::Object:
    return "an object";
  case ReflectionKind::Value:
    return "a value";
  case ReflectionKind::Declaration: {
    ValueDecl *D = RV.getReflectedDecl();

    switch (D->getDeclName().getNameKind()) {
    case DeclarationName::CXXConstructorName:
      return "a constructor";
    case DeclarationName::CXXDestructorName:
      return "a destuctor";
    case DeclarationName::CXXConversionFunctionName:
      return "a conversion function";
    case DeclarationName::CXXOperatorName:
      return "an operator function";
    case DeclarationName::CXXLiteralOperatorName:
      return "a literal operator";
    default:
      break;
    }
    if (auto *FD = dyn_cast<FieldDecl>(D)) {
      if (FD->isUnnamedBitField()) return "an unnamed bit-field";
      else if (FD->isBitField()) return "a bit-field";
      return "a non-static data member";
    }
    else if (isa<ParmVarDecl>(D)) return "function parameter";
    else if (isa<VarDecl>(D)) return "a variable";
    else if (isa<BindingDecl>(D)) return "a structured binding";
    else if (isa<FunctionDecl>(D)) return "a function";
    else if (isa<EnumConstantDecl>(D)) return "a enumerator";
    llvm_unreachable("unhandled declaration kind");
  }
  case ReflectionKind::Template: {
    TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();

    switch (TD->getDeclName().getNameKind()) {
    case DeclarationName::CXXConstructorName:
      return "a constructor template";
    case DeclarationName::CXXDestructorName:
      return "a destuctor template";
    case DeclarationName::CXXConversionFunctionName:
      return "a conversion function template";
    case DeclarationName::CXXOperatorName:
      return "an operator function template";
    case DeclarationName::CXXLiteralOperatorName:
      return "a literal operator template";
    default:
      break;
    }
    if (isa<FunctionTemplateDecl>(TD)) return "a function template";
    else if (isa<ClassTemplateDecl>(TD)) return "a class template";
    else if (isa<TypeAliasTemplateDecl>(TD)) return "an alias template";
    else if (isa<VarTemplateDecl>(TD)) return "a variable template";
    else if (isa<ConceptDecl>(TD)) return "a concept";
    llvm_unreachable("unhandled template kind");
  }
  case ReflectionKind::Namespace: {
    Decl *D = RV.getReflectedNamespace();
    if (isa<TranslationUnitDecl>(D)) return "the global namespace";
    else if (isa<NamespaceAliasDecl>(D)) return "a namespace alias";
    else if (isa<NamespaceDecl>(D)) return "a namespace";
    llvm_unreachable("unhandled namespace kind");
  }
  case ReflectionKind::BaseSpecifier: {
    return "a base class specifier";
  }
  case ReflectionKind::DataMemberSpec: {
    return "a description of a non-static data member";
  }
  }
}

bool DiagnoseReflectionKind(DiagFn Diagnoser, SourceRange Range,
                            StringRef Expected, StringRef Instead = "") {
  if (!Instead.empty())
    Diagnoser(Range.getBegin(),
              diag::metafn_expected_reflection_of_but_got)
        << Expected << Instead << Range;
  else
    Diagnoser(Range.getBegin(), diag::metafn_expected_reflection_of)
        << Expected << Range;

  return true;
}

// -----------------------------------------------------------------------------
// Metafunction implementations
// -----------------------------------------------------------------------------

bool get_begin_enumerator_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    Decl *D = findTypeDecl(RV.getReflectedType());

    if (auto enumDecl = dyn_cast_or_null<EnumDecl>(D)) {
      if (auto itr = enumDecl->enumerator_begin();
          itr != enumDecl->enumerator_end()) {
        return SetAndSucceed(Result, makeReflection(*itr));
      }
      return SetAndSucceed(Result, Sentinel);
    }
    return DiagnoseReflectionKind(Diagnoser, Range, "an enum type");
  }
  case ReflectionKind::Null:
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec: {
    return DiagnoseReflectionKind(Diagnoser, Range, "an enum type",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_enumerator_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    Decl *currEnumConstDecl = RV.getReflectedDecl();
    if(auto nextEnumConstDecl = currEnumConstDecl->getNextDeclInContext()) {
      return SetAndSucceed(Result, makeReflection(nextEnumConstDecl));
    }
    return SetAndSucceed(Result, Sentinel);
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec: {
    llvm_unreachable("should have failed in 'get_begin_enumerator_decl_of'");
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_base_of(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    Decl *typeDecl = findTypeDecl(RV.getReflectedType());

    if (auto cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(typeDecl)) {
      ensureInstantiated(S, typeDecl, Range);
      if (RV.getReflectedType()->isIncompleteType())
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
            << 0 << 0 << Range;

      auto numBases = cxxRecordDecl->getNumBases();
      if (idx >= numBases)
        return SetAndSucceed(Result, Sentinel);

      // the unqualified base class
      CXXBaseSpecifier *baseClassItr = cxxRecordDecl->bases_begin() + idx;
      return SetAndSucceed(Result, makeReflection(baseClassItr));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 0 << 1 << Range;
  }
  case ReflectionKind::Null:
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a class type",
                                  DescriptionOf(RV));
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_template_argument_of(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromType(QT, TArgs))
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");

    APValue R = getNthTemplateArgument(S, TArgs, Evaluator, Sentinel, idx);
    return SetAndSucceed(Result, R);
  }
  case ReflectionKind::Declaration: {
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromDecl(RV.getReflectedDecl(), TArgs))
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");
    return SetAndSucceed(Result, getNthTemplateArgument(S, TArgs, Evaluator,
                                                        Sentinel, idx));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a template specialization",
                                  DescriptionOf(RV));
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_begin_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                              DiagFn Diagnoser, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  APValue RV;
  if (!Evaluator(RV, Args[0], true)) {
    return true;
  }

  assert(Args[1]->getType()->isReflectionType());
  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
  {
    QualType QT = RV.getReflectedType();
    if (isTypeAlias(QT))
      QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                       /*DropRefs=*/false);

    if (isa<EnumType>(QT)) {
      Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
            << 1 << 1 << Range;
      return Diagnoser(Range.getBegin(), diag::metafn_members_of_enum) << Range;
    }

    ensureDeclared(S, QT, Range.getBegin());
    Decl *typeDecl = findTypeDecl(QT);
    if (!typeDecl)
      return true;

    if (!ensureInstantiated(S, typeDecl, Range))
      return true;

    if (QT->isIncompleteType())
      return true;
      // NOTE(P2996): Uncomment to allow 'members_of' within member
      // specification.
      /*
      if (auto *TD = dyn_cast<TagDecl>(typeDecl); !TD || !TD->isBeingDefined())
        return true;
      */

    if (auto *CXXRD = dyn_cast<CXXRecordDecl>(typeDecl))
      S.ForceDeclarationOfImplicitMembers(CXXRD);

    DeclContext *declContext = dyn_cast<DeclContext>(typeDecl);
    assert(declContext && "no DeclContext?");

    Decl* beginMember = findIterableMember(S.Context,
                                           *declContext->decls_begin(), true);
    if (!beginMember)
      return SetAndSucceed(Result, Sentinel);
    return SetAndSucceed(Result,
                         APValue(ReflectionKind::Declaration, beginMember));
  }
  case ReflectionKind::Namespace: {
    Decl *NS = RV.getReflectedNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();

    DeclContext *DC = cast<DeclContext>(NS->getMostRecentDecl());

    Decl *beginMember = findIterableMember(S.Context, *DC->decls_begin(), true);
    if (!beginMember)
      return SetAndSucceed(Result, Sentinel);
    return SetAndSucceed(Result,
                         APValue(ReflectionKind::Declaration, beginMember));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_member_decl_of(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  assert(Args[1]->getType()->isReflectionType());
  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  if (Decl *Next = findIterableMember(S.Context, RV.getReflectedDecl(), false))
    return SetAndSucceed(Result, APValue(ReflectionKind::Declaration, Next));
  return SetAndSucceed(Result, Sentinel);
}

bool map_decl_to_entity(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  Decl *D = RV.getReflectedDecl();

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

bool identifier_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[1], true))
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
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    if (isTemplateSpecialization(QT))
      return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
          << 0 << Range;

    if (auto *D = findTypeDecl(QT))
      if (auto *ND = dyn_cast<NamedDecl>(D); ND && ND->getIdentifier())
        Name = ND->getIdentifier()->getName();

    break;
  }
  case ReflectionKind::Declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl())) {
      bool ConsistentName = getParameterName(PVD, Name);
      if (EnforceConsistent && !ConsistentName) {
        return Diagnoser(Range.getBegin(), diag::metafn_inconsistent_name)
            << DescriptionOf(RV) << Range;
      }
      break;
    }

    if (auto *ND = dyn_cast<NamedDecl>(RV.getReflectedDecl())) {
      if (!findTemplateOfDecl(ND).isNull())
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 0 << Range;
      else if (isa<CXXConstructorDecl>(ND))
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 1 << Range;
      else if (isa<CXXDestructorDecl>(ND))
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 2 << Range;
      else if (ND->getDeclName().getNameKind() ==
               DeclarationName::CXXOperatorName)
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 3 << Range;
      else if (ND->getDeclName().getNameKind() ==
               DeclarationName::CXXConversionFunctionName)
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 4 << Range;

      if (auto *II = ND->getIdentifier())
        Name = II->getName();
      else if (auto *II = ND->getDeclName().getCXXLiteralIdentifier())
        Name = II->getName();
    }

    break;
  }
  case ReflectionKind::Template: {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD)) {
      if (isa<CXXConstructorDecl>(FTD->getTemplatedDecl()))
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 5 << Range;
      else if (FTD->getDeclName().getNameKind() ==
               DeclarationName::CXXOperatorName)
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 6 << Range;
      else if (FTD->getDeclName().getNameKind() ==
               DeclarationName::CXXConversionFunctionName)
        return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
            << 7 << Range;
    }


    if (auto *II = TD->getIdentifier())
      Name = II->getName();
    else if (auto *II = TD->getDeclName().getCXXLiteralIdentifier())
      Name = II->getName();

    break;
  }
  case ReflectionKind::Namespace: {
    if (isa<TranslationUnitDecl>(RV.getReflectedNamespace()))
      return Diagnoser(Range.getBegin(),
                       diag::metafn_name_of_unnamed_singleton) << 1 << Range;
    getDeclName(Name, S.Context, RV.getReflectedNamespace());
    break;
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    if (TDMS->Name)
      Name = *TDMS->Name;
    break;
  }
  case ReflectionKind::Null:
    return Diagnoser(Range.getBegin(),
                     diag::metafn_name_of_unnamed_singleton) << 0 << Range;
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::BaseSpecifier:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_have_name)
        << DescriptionOf(RV) << Range;
  }
  if (Name.empty())
    return Diagnoser(Range.getBegin(), diag::metafn_anonymous_entity)
        << DescriptionOf(RV) << Range;

  Expr *StrLit = makeStrLiteral(Name, S.Context, IsUtf8);

  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result,
                       APValue(StrLit, CharUnits::Zero(), Path, false));
}

bool has_identifier(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool HasIdentifier = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    if (isTemplateSpecialization(QT))
      break;

    if (auto *D = findTypeDecl(QT))
      if (auto *ND = dyn_cast<NamedDecl>(D); ND && ND->getIdentifier())
        HasIdentifier = (ND->getIdentifier() != nullptr);

    break;
  }
  case ReflectionKind::Declaration: {
    auto *D = RV.getReflectedDecl();
    if (auto *PVD = dyn_cast<ParmVarDecl>(D)) {
      std::string Name;
      (void) getParameterName(PVD, Name);

      HasIdentifier = !Name.empty();
      break;
    } else if (auto *FD = dyn_cast<FunctionDecl>(D);
               FD && FD->getTemplateSpecializationArgs())
      break;
    else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D))
      break;
    else if (auto *ND = dyn_cast<NamedDecl>(D))
      HasIdentifier = (ND->getIdentifier() != nullptr);

    break;
  }
  case ReflectionKind::Template: {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      if (isa<CXXConstructorDecl>(FTD->getTemplatedDecl()))
        break;

    HasIdentifier = (TD->getIdentifier() != nullptr);
    break;
  }
  case ReflectionKind::Namespace: {
    if (auto *ND = dyn_cast<NamedDecl>(RV.getReflectedNamespace()))
      HasIdentifier = (ND->getIdentifier() != nullptr);
    break;
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    HasIdentifier = TDMS->Name && !TDMS->Name->empty();
    break;
  }
  case ReflectionKind::Null:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
    break;
  }

  return SetAndSucceed(Result, makeBool(S.Context, HasIdentifier));
}

bool operator_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  static constexpr OverloadedOperatorKind OperatorIndices[] = {
    OO_None, OO_New, OO_Delete, OO_Array_New, OO_Array_Delete, OO_Coawait,
    OO_Call, OO_Subscript, OO_Arrow, OO_ArrowStar, OO_Tilde, OO_Exclaim,
    OO_Plus, OO_Minus, OO_Star, OO_Slash, OO_Percent, OO_Caret, OO_Amp, OO_Pipe,
    OO_Equal, OO_PlusEqual, OO_MinusEqual, OO_StarEqual, OO_SlashEqual,
    OO_PercentEqual, OO_CaretEqual, OO_AmpEqual, OO_PipeEqual, OO_EqualEqual,
    OO_ExclaimEqual, OO_Less, OO_Greater, OO_LessEqual, OO_GreaterEqual,
    OO_Spaceship, OO_AmpAmp, OO_PipePipe, OO_LessLess, OO_GreaterGreater,
    OO_LessLessEqual, OO_GreaterGreaterEqual, OO_PlusPlus, OO_MinusMinus,
    OO_Comma,
  };

  auto findOperatorOf = [](FunctionDecl *FD) -> size_t {
    OverloadedOperatorKind OO = FD->getOverloadedOperator();
    if (OO == OO_None)
      return 0;

    auto *OpPtr = std::find(std::begin(OperatorIndices),
                            std::end(OperatorIndices), OO);
    assert(OpPtr < std::end(OperatorIndices));

    return (OpPtr - OperatorIndices);
  };

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  size_t OperatorId = 0;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      OperatorId = findOperatorOf(FTD->getTemplatedDecl());
  } else if (RV.isReflectedDecl()) {
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      OperatorId = findOperatorOf(FD);
  }

  if (OperatorId == 0)
    return Diagnoser(Range.getBegin(), diag::metafn_not_an_operator)
        << DescriptionOf(RV) << Range;

  return SetAndSucceed(
          Result,
          APValue(S.Context.MakeIntValue(OperatorId, S.Context.getSizeType())));
}

bool source_location_of(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    return findTypeDeclLoc(Result, S.Context, Evaluator, ResultTy,
                           RV.getReflectedType());
  case ReflectionKind::Declaration:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       RV.getReflectedDecl());
  case ReflectionKind::Template: {
    TemplateName TName = RV.getReflectedTemplate();
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       TName.getAsTemplateDecl());
  }
  case ReflectionKind::Namespace:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy,
                       RV.getReflectedNamespace());
  case ReflectionKind::BaseSpecifier:
    return findBaseSpecLoc(Result, S.Context, Evaluator, ResultTy,
                           RV.getReflectedBaseSpecifier());
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Null:
  case ReflectionKind::DataMemberSpec:
    return findDeclLoc(Result, S.Context, Evaluator, ResultTy, nullptr);
  }
  llvm_unreachable("unknown reflection kind");
}

bool type_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
    return Diagnoser(Range.getBegin(), diag::metafn_no_associated_property)
        << DescriptionOf(RV) << 0 << Range;
  case ReflectionKind::Object: {
    QualType QT = desugarType(RV.getTypeOfReflectedResult(S.Context),
                              /*UnwrapAliases=*/false, /*DropCV=*/false,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Value: {
    QualType QT = desugarType(RV.getTypeOfReflectedResult(S.Context),
                              /*UnwrapAliases=*/true, /*DropCV=*/false,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    if (isa<CXXConstructorDecl, CXXDestructorDecl, BindingDecl>(VD))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 0 << DescriptionOf(RV) << Range;

    bool UnwrapAliases = isa<ParmVarDecl>(VD) || isa<BindingDecl>(VD);
    bool DropCV = isa<ParmVarDecl>(VD);
    QualType QT = desugarType(VD->getType(), UnwrapAliases, DropCV,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::BaseSpecifier: {
    QualType QT = RV.getReflectedBaseSpecifier()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/false, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::DataMemberSpec:
  {
    QualType QT = RV.getReflectedDataMemberSpec()->Ty;
    QT = desugarType(QT, /*UnwrapAliases=*/false, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool parent_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  auto DiagWrapper = [&](unsigned DiagId) {
    if (DiagId && Diagnoser)
      return bool(Diagnoser(Range.getBegin(), DiagId)
          << DescriptionOf(RV) << Range);

    return DiagId > 0;
  };

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::BaseSpecifier:
    if (Diagnoser)
      return Diagnoser(Range.getBegin(), diag::metafn_no_associated_property)
          << DescriptionOf(RV) << 1 << Range;
    return true;
  case ReflectionKind::Type: {
    if (TemplateName TName = findTemplateOfType(RV.getReflectedType());
        !TName.isNull())
      return DiagWrapper(parentOf(Result, TName.getAsTemplateDecl()));

    return DiagWrapper(parentOf(Result, findTypeDecl(RV.getReflectedType())));
  }
  case ReflectionKind::Declaration: {
    if (TemplateName TName = findTemplateOfDecl(RV.getReflectedDecl());
        !TName.isNull())
      return DiagWrapper(parentOf(Result, TName.getAsTemplateDecl()));

    return DiagWrapper(parentOf(Result, RV.getReflectedDecl()));
  }
  case ReflectionKind::Template: {
    return DiagWrapper(parentOf(Result,
                                RV.getReflectedTemplate().getAsTemplateDecl()));
  }
  case ReflectionKind::Namespace:
    if (isa<TranslationUnitDecl>(RV.getReflectedNamespace())) {
      if (Diagnoser)
        return Diagnoser(Range.getBegin(), diag::metafn_no_associated_property)
            << DescriptionOf(RV) << 1 << Range;
      return true;
    }
    return DiagWrapper(parentOf(Result, RV.getReflectedNamespace()));
  }
  llvm_unreachable("unknown reflection kind");
}

bool dealias(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, RV);
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Namespace: {
    Decl *NS = RV.getReflectedNamespace();
    if (auto *A = dyn_cast<NamespaceAliasDecl>(NS))
      NS = A->getNamespace();
    return SetAndSucceed(Result, makeReflection(NS));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool object_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Object:
    return SetAndSucceed(Result, RV);
  case ReflectionKind::Declaration: {
    VarDecl *VD = dyn_cast<VarDecl>(RV.getReflectedDecl());
    if (!VD)
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 1 << DescriptionOf(RV) << Range;

    QualType QT = VD->getType();
    if (auto *LVRT = dyn_cast<LValueReferenceType>(QT)) {
      QT = LVRT->getPointeeType();
    }

    Expr *Synthesized = DeclRefExpr::Create(S.Context,
                                            NestedNameSpecifierLoc(),
                                            SourceLocation(), VD, false,
                                            Range.getBegin(), QT,
                                            VK_LValue, VD, nullptr);
    APValue Value;
    if (!Evaluator(Value, Synthesized, false) || !Value.isLValue())
      return true;

    APValue OV = Value.Lift(QualType{});
    return SetAndSucceed(Result, OV);
  }
  case ReflectionKind::Null:
  case ReflectionKind::Value:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 1 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unimplemented");
}


bool value_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Value:
    return SetAndSucceed(Result, RV);
  case ReflectionKind::Object: {
    if (!RV.getTypeOfReflectedResult(S.Context)->isStructuralType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object of non-structural type" << Range;

    QualType ObjectTy = RV.getTypeOfReflectedResult(S.Context);
    Expr *OVE = new (S.Context) OpaqueValueExpr(Range.getBegin(),
                                                ObjectTy,
                                                VK_LValue);
    Expr *CE = ConstantExpr::Create(S.Context, OVE, RV.getReflectedObject());

    Expr::EvalResult ER;
    if (!CE->EvaluateAsRValue(ER, S.Context, true))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object not usable in constant expressions" << Range;

    QualType ValueTy =
          S.ComputeResultType(RV.getTypeOfReflectedResult(S.Context), ER.Val);
    return SetAndSucceed(Result, ER.Val.Lift(ValueTy));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();

    APValue Value;
    QualType QT;
    if (auto *VD = dyn_cast<VarDecl>(Decl)) {
      if (!VD->isUsableInConstantExpressions(S.Context))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "a variable not usable in constant expressions" << Range;

      QT = VD->getType();
      if (auto *LVRT = dyn_cast<LValueReferenceType>(QT))
        QT = LVRT->getPointeeType();

      Expr *Synthesized = DeclRefExpr::Create(S.Context,
                                              NestedNameSpecifierLoc(),
                                              SourceLocation(), VD, false,
                                              Range.getBegin(), QT,
                                              VK_LValue, Decl, nullptr);
      if (!Evaluator(Value, Synthesized, true))
        llvm_unreachable("failed to evaluate variable usable in constant "
                         "expressions");
    } else if (isa<EnumConstantDecl>(Decl)) {
      Expr *Synthesized = DeclRefExpr::Create(S.Context,
                                              NestedNameSpecifierLoc(),
                                              SourceLocation(), Decl, false,
                                              Range.getBegin(), Decl->getType(),
                                              VK_PRValue, Decl, nullptr);
      QT = Synthesized->getType();

      Expr::EvalResult ER;
      if (!Synthesized->EvaluateAsConstantExpr(ER, S.Context))
        llvm_unreachable("failed to evaluate enumerator constant");
      Value = ER.Val;
    } else if (auto *TPOD = dyn_cast<TemplateParamObjectDecl>(Decl)) {
      Value = TPOD->getValue();
      QT = TPOD->getType();
    } else {
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << DescriptionOf(RV) << Range;
    }

    QualType ValueTy = S.ComputeResultType(QT, Value);
    return SetAndSucceed(Result, Value.Lift(ValueTy));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 2 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unimplemented");
}

bool template_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    TemplateName TName = findTemplateOfType(RV.getReflectedType());
    if (TName.isNull())
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");

    return SetAndSucceed(Result, makeReflection(TName));
  }
  case ReflectionKind::Declaration: {
    TemplateName TName = findTemplateOfDecl(RV.getReflectedDecl());
    if (TName.isNull())
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");

    return SetAndSucceed(Result, makeReflection(TName));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a template specialization",
                                  DescriptionOf(RV));
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

static bool CanActAsTemplateArg(const APValue &RV) {
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
  case ReflectionKind::Declaration:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
    return true;
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    return isa<ClassTemplateDecl, TypeAliasTemplateDecl>(TDecl);
  }
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Null:
    return false;
  }
  llvm_unreachable("unknown reflection kind");
}

static TemplateArgument TArgFromReflection(Sema &S, EvalFn Evaluator,
                                           const APValue &RV,
                                           SourceLocation Loc) {
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    return RV.getReflectedType().getCanonicalType();
  case ReflectionKind::Object: {
    QualType RefTy = S.Context.getLValueReferenceType(
            RV.getTypeOfReflectedResult(S.Context));
    return TemplateArgument(S.Context, RefTy, RV.getReflectedObject(), false);
  }
  case ReflectionKind::Value: {
    APValue Lowered = RV.getReflectedValue();
    QualType ResultTy = RV.getTypeOfReflectedResult(S.Context);
    if (Lowered.isInt()) {
      return TemplateArgument(S.Context, Lowered.getInt(),
                              ResultTy.getCanonicalType());
    }
    TemplateArgument TArg(S.Context, ResultTy, Lowered, false);
    return TArg;
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();

    // Don't worry about the cost of creating an expression here: The template
    // substitution machinery will otherwise create one from the argument
    // anyway, so we aren't really losing any efficiency here.
    Expr *Synthesized =
        DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                            SourceLocation(), Decl, false, Loc,
                            Decl->getType(), VK_LValue, Decl, nullptr);

    if (Decl->getType()->isIntegralOrEnumerationType()) {
      APValue R;
      if (!Evaluator(R, Synthesized, true))
        break;

      return TemplateArgument(S.Context, R.getInt(),
                              Synthesized->getType().getCanonicalType());
    }

    return TemplateArgument(Synthesized);
  }
  case ReflectionKind::Template:
    return TemplateArgument(RV.getReflectedTemplate());
    break;
  default:
    llvm_unreachable("unimplemented for template argument kind");
  }
  return TemplateArgument();
}

// TODO(P2996): Abstract this out, and use as an implementation detail of
// 'substitute'.
bool can_substitute(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());

  APValue Template;
  if (!Evaluator(Template, Args[0], true))
    return true;

  if (!Template.isReflectedTemplate())
    return DiagnoseReflectionKind(Diagnoser, Range, "a template",
                                  DescriptionOf(Template));
  TemplateDecl *TDecl = Template.getReflectedTemplate().getAsTemplateDecl();
  if (TDecl->isInvalidDecl())
    return true;

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
          !Unwrapped.isReflection())
        return true;
      if (!CanActAsTemplateArg(Unwrapped))
        return SetAndSucceed(Result, makeBool(S.Context, false));

      TemplateArgument TArg = TArgFromReflection(S, Evaluator,
                                                 Unwrapped, Range.getBegin());
      if (TArg.isNull())
        llvm_unreachable("could not form template argument?");
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

bool substitute(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());

  APValue Template;
  if (!Evaluator(Template, Args[0], true))
    return true;

  if (!Template.isReflectedTemplate())
    return DiagnoseReflectionKind(Diagnoser, Range, "a template",
                                  DescriptionOf(Template));

  TemplateDecl *TDecl = Template.getReflectedTemplate().getAsTemplateDecl();
  if (TDecl->isInvalidDecl())
    return true;

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
          !Unwrapped.isReflection())
        return true;
      if (!CanActAsTemplateArg(Unwrapped))
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
            << DescriptionOf(Unwrapped) << 1 << Range;

      TemplateArgument TArg = TArgFromReflection(S, Evaluator,
                                                 Unwrapped, Range.getBegin());
      if (TArg.isNull())
        llvm_unreachable("could not form template argument?");
      TArgs.push_back(TArg);
    }
  }

  {
    TemplateArgumentListInfo TAListInfo =
          addLocToTemplateArgs(S, TArgs, Args[1]);

    SmallVector<TemplateArgument, 4> CanonicalTArgs;
    SmallVector<TemplateArgument, 4> IgnoredSugared;
    if (S.CheckTemplateArgumentList(TDecl, Range.getBegin(), TAListInfo, false,
                                    IgnoredSugared, CanonicalTArgs, true))
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

    APValue RV(ReflectionKind::Type,
               const_cast<Type *>(TSpecDecl->getTypeForDecl()));
    return SetAndSucceed(Result, RV);
  } else if (isa<TypeAliasTemplateDecl>(TDecl)) {
    TemplateArgumentListInfo TAListInfo =
          addLocToTemplateArgs(S, TArgs, Args[1]);

    // TODO(P2996): Calling 'substitute' should not instantiate the alias
    // template. Probably the logic for "substituting" the arguments into the
    // template should be abstracted to a separate function.
    QualType QT = S.CheckTemplateIdType(Template.getReflectedTemplate(),
                                        Range.getBegin(), TAListInfo);
    if (QT.isNull())
      llvm_unreachable("Substitution failed after validating arguments?");

    return SetAndSucceed(Result, makeReflection(QT));
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
      llvm_unreachable("Substitution failed after validating arguments?");

    return SetAndSucceed(Result,
                         APValue(ReflectionKind::Declaration, TSpecDecl));
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

    return SetAndSucceed(Result, makeReflection(TSpecDecl));
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
      llvm_unreachable("Substitution failed after validating arguments?");

    return SetAndSucceed(Result, SatisfiesConcept.Lift(S.Context.BoolTy));
  }
  llvm_unreachable("unimplemented for template kind");
}


bool extract(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());

  bool ReturnsLValue = false;
  QualType RawResultTy = ResultTy;
  if (auto *LVRT = dyn_cast<LValueReferenceType>(ResultTy)) {
    ReturnsLValue = true;
    ResultTy = LVRT->getPointeeType();
  }

  auto extractLambda = [&](APValue &Out, CXXRecordDecl *RD) -> bool {
    // Lambdas with captures are not structural types; should not be possible
    // to get a reflection to a value of such a type.
    assert(RD->isCapturelessLambda());

    CXXMethodDecl *CallOp = RD->getLambdaStaticInvoker();
    QualType LambdaPtrTy = S.Context.getPointerType(CallOp->getType());

    if (LambdaPtrTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 0 << QualType(RD->getTypeForDecl(), 0) << 0 << ResultTy << Range;

    // If not already done, generate a fake body for the call-operator.
    // The real body is generated during CodeGen.
    if (!CallOp->hasBody()) {
      CallOp->markUsed(S.Context);
      CallOp->setReferenced();
      CallOp->setBody(new (S.Context) CompoundStmt(Range.getBegin()));
    }

    APValue CallOpLV(CallOp, CharUnits::Zero(), APValue::NoLValuePath());
    return SetAndSucceed(Out, CallOpLV);
  };

  APValue RV;
  if (!Evaluator(RV, Args[1], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Object: {
    QualType ObjectTy = RV.getTypeOfReflectedResult(S.Context);
    if (ObjectTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 1 << ObjectTy << ReturnsLValue << ResultTy << Range;

    return SetAndSucceed(Result, RV.getReflectedObject());
  }
  case ReflectionKind::Value: {
    if (ReturnsLValue)
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
          << 1 << DescriptionOf(RV) << Range;

    QualType ValueTy = RV.getTypeOfReflectedResult(S.Context);
    if (auto *RD = ValueTy->getAsCXXRecordDecl();
        RD && RD->isLambda() && ResultTy->isPointerType())
      return extractLambda(Result, RD);

    if (ValueTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 0 << ValueTy << ReturnsLValue << ResultTy << Range;

    return SetAndSucceed(Result, RV.getReflectedValue());
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();
    ensureInstantiated(S, Decl, Args[1]->getSourceRange());

    if (auto *RD = Decl->getType()->getAsCXXRecordDecl();
        RD && RD->isLambda() && ResultTy->isPointerType())
      return extractLambda(Result, RD);

    if (isa<VarDecl, TemplateParamObjectDecl>(Decl)) {
      Expr *Synthesized;
      if (isa<LValueReferenceType>(Decl->getType().getCanonicalType())) {
        // We have a reflection of an object with reference type.
        // Synthesize a 'DeclRefExpr' designating the object, such that constant
        // evaluation resolves the underlying referenced entity.
        ReturnsLValue = true;
        if (RawResultTy.getCanonicalType().getTypePtr() !=
            Decl->getType().getCanonicalType().getTypePtr())
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 1 << Decl->getType() << 1 << ResultTy << Range;

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
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;

        Synthesized = ExtractLValueExpr::Create(S.Context, Range, ResultTy,
                                                Decl);
      }

      if (Synthesized->getType().getCanonicalType().getTypePtr() !=
          ResultTy.getCanonicalType().getTypePtr())
        return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
            << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;
      return !Evaluator(Result, Synthesized, !ReturnsLValue);
    } else if (isa<BindingDecl>(Decl)) {
      return Diagnoser(Range.getBegin(),
                       diag::metafn_extract_structured_binding) << Range;

    } else if (ReturnsLValue) {
      // Only variables may be returned as LValues.
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
          << 1 << DescriptionOf(RV);
    } else if (isa<FieldDecl, CXXMethodDecl>(Decl)) {
      // Extracting a non-static member as a pointer.
      if (auto *FD = dyn_cast<FieldDecl>(Decl); FD && FD->isBitField())
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract) << 2
            << DescriptionOf(RV) << Range;

      auto *ParentTy = cast<RecordDecl>(
              Decl->getDeclContext())->getTypeForDecl();
      QualType MemPtrTy = S.Context.getMemberPointerType(Decl->getType(),
                                                         ParentTy);
      if (MemPtrTy.getCanonicalType().getTypePtr() !=
          ResultTy.getCanonicalType().getTypePtr())
        return Diagnoser(Range.getBegin(),
                         diag::metafn_extract_entity_type_mismatch)
            << ResultTy << DescriptionOf(RV) << MemPtrTy << Range;

      APValue MemPtrLV(Decl, false, ArrayRef<const CXXRecordDecl *> {});
      return SetAndSucceed(Result, MemPtrLV);
    } else if (auto *ECD = dyn_cast<EnumConstantDecl>(Decl)) {
      if (ECD->getType().getCanonicalType().getTypePtr() !=
          ResultTy.getCanonicalType().getTypePtr())
        return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
            << 2 << Decl->getType() << 0 << ResultTy << Range;

      return SetAndSucceed(Result, APValue(ECD->getInitVal()));
    } else {
      QualType FnPtrTy = S.Context.getPointerType(Decl->getType());
      if (FnPtrTy.getCanonicalType().getTypePtr() !=
          ResultTy.getCanonicalType().getTypePtr())
        return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
            << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;

      return SetAndSucceed(Result, APValue(Decl, CharUnits::Zero(),
                           APValue::NoLValuePath()));
    }
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
        << (ReturnsLValue ? 1 : 0) << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("invalid reflection type");
}

bool is_public(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool IsPublic = false;
    if (const Decl *D = findTypeDecl(RV.getReflectedType()))
      IsPublic = (D->getAccess() == AS_public);

    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionKind::Declaration: {
    bool IsPublic = (RV.getReflectedDecl()->getAccess() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionKind::Template: {
    const Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool IsPublic = (D->getAccess() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool IsPublic = (Base->getAccessSpecifier() == AS_public);
    return SetAndSucceed(Result, makeBool(S.Context, IsPublic));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Namespace:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_protected(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool IsProtected = false;
    if (const Decl *D = findTypeDecl(RV.getReflectedType()))
      IsProtected = (D->getAccess() == AS_protected);

    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionKind::Declaration: {
    bool IsProtected = (RV.getReflectedDecl()->getAccess() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionKind::Template: {
    const Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool IsProtected = (D->getAccess() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool IsProtected = (Base->getAccessSpecifier() == AS_protected);
    return SetAndSucceed(Result, makeBool(S.Context, IsProtected));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Namespace:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_private(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool IsPrivate = false;
    if (const Decl *D = findTypeDecl(RV.getReflectedType()))
      IsPrivate = (D->getAccess() == AS_private);

    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionKind::Declaration: {
    bool IsPrivate = (RV.getReflectedDecl()->getAccess() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionKind::Template: {
    const Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool IsPrivate = (D->getAccess() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool IsPrivate = (Base->getAccessSpecifier() == AS_private);
    return SetAndSucceed(Result, makeBool(S.Context, IsPrivate));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_access_specified(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  auto findAccessSpec = [](Decl *D) -> AccessSpecifier {
    DeclContext *DC = D->getDeclContext();
    for (auto I = DC->decls_begin(); *I != D; ++I) {
      assert(I != DC->decls_end());
      if (auto *ASD = dyn_cast<AccessSpecDecl>(*I))
        return ASD->getAccess();
    }
    return AS_none;
  };

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool IsSpecified = false;
    if (Decl *D = findTypeDecl(RV.getReflectedType()))
      IsSpecified = findAccessSpec(D) != AS_none;

    return SetAndSucceed(Result, makeBool(S.Context, IsSpecified));
  }
  case ReflectionKind::Declaration: {
    bool IsSpecified = findAccessSpec(RV.getReflectedDecl()) != AS_none;
    return SetAndSucceed(Result, makeBool(S.Context, IsSpecified));
  }
  case ReflectionKind::Template: {
    Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool IsSpecified = findAccessSpec(D) != AS_none;
    return SetAndSucceed(Result, makeBool(S.Context, IsSpecified));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool IsSpecified = (Base->getAccessSpecifierAsWritten() != AS_none);
    return SetAndSucceed(Result, makeBool(S.Context, IsSpecified));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
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
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(ResultTy == S.Context.MetaInfoTy);

  return !findAccessContext(S, Evaluator, Result);
}

bool is_accessible(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;
  if (!Evaluator(Scratch, Args[1], true) || !Scratch.isReflection())
    return true;

  DeclContext *AccessDC = nullptr;
  switch (Scratch.getReflectionKind()) {
  case ReflectionKind::Null:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Type:
    AccessDC = dyn_cast<DeclContext>(findTypeDecl(Scratch.getReflectedType()));
    if (!AccessDC)
      return true;
    break;
  case ReflectionKind::Namespace:
    AccessDC = dyn_cast<DeclContext>(Scratch.getReflectedNamespace());
    break;
  case ReflectionKind::Declaration:
    AccessDC = dyn_cast<DeclContext>(Scratch.getReflectedDecl());
    break;
  default:
    llvm_unreachable("invalid access context");
  }

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    NamedDecl *D = findTypeDecl(RV.getReflectedType());
    if (!D || !D->getDeclContext() || !isa<CXXRecordDecl>(D->getDeclContext()))
      return DiagnoseReflectionKind(Diagnoser, Range, "a class member");

    bool Accessible = isAccessible(S, AccessDC, D);
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *D = RV.getReflectedDecl();
    if (!D->getDeclContext() || !isa<CXXRecordDecl>(D->getDeclContext()))
      return DiagnoseReflectionKind(Diagnoser, Range, "a class member");

    bool Accessible = isAccessible(S, AccessDC, RV.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionKind::Template: {
    TemplateDecl *D = RV.getReflectedTemplate().getAsTemplateDecl();
    if (!D->getDeclContext() || !isa<CXXRecordDecl>(D->getDeclContext()))
      return DiagnoseReflectionKind(Diagnoser, Range, "a class member");

    bool Accessible = isAccessible(S, AccessDC, D);
    return SetAndSucceed(Result, makeBool(S.Context, Accessible));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *BaseSpec = RV.getReflectedBaseSpecifier();

    auto *Base = findTypeDecl(BaseSpec->getType());
    assert(Base && "base class has no type declaration?");

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
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a class member",
                                  DescriptionOf(RV));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_virtual(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsVirtual = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    if (const auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      IsVirtual = MD->isVirtual();
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  case ReflectionKind::BaseSpecifier: {
    IsVirtual = RV.getReflectedBaseSpecifier()->isVirtual();
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, IsVirtual));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_pure_virtual(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool IsPureVirtual = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsPureVirtual = FD->isPureVirtual();

    return SetAndSucceed(Result, makeBool(S.Context, IsPureVirtual));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_override(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsOverride = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    if (const auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      IsOverride = MD->size_overridden_methods() > 0;
    return SetAndSucceed(Result, makeBool(S.Context, IsOverride));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_deleted(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool IsDeleted = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsDeleted = FD->isDeleted();
    return SetAndSucceed(Result, makeBool(S.Context, IsDeleted));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_defaulted(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool IsDefaulted = false;
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsDefaulted = FD->isDefaulted();

    return SetAndSucceed(Result, makeBool(S.Context, IsDefaulted));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_explicit(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Template:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    ValueDecl *D = RV.getReflectedDecl();

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

bool is_noexcept(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Type: {
    const QualType QT = RV.getReflectedType();
    const auto result = isFunctionOrMethodNoexcept(QT);

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Declaration: {
    const ValueDecl *D = RV.getReflectedDecl();
    const auto result = isFunctionOrMethodNoexcept(D->getType());

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_bit_field(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl()))
      result = FD->isBitField();
  } else if (RV.isReflectedDataMemberSpec()) {
    result = RV.getReflectedDataMemberSpec()->BitWidth.has_value();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_enumerator(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<EnumConstantDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_const(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Type: {
    bool result = isConstQualifiedType(RV.getReflectedType());

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;
    if (!isa<ParmVarDecl>(RV.getReflectedDecl()))
      result = isConstQualifiedType(RV.getReflectedDecl()->getType());

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    bool result = isConstQualifiedType(RV.getTypeOfReflectedResult(S.Context));

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_volatile(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Type: {
    bool result = isVolatileQualifiedType(RV.getReflectedType());

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;
    if (!isa<ParmVarDecl>(RV.getReflectedDecl()))
      result = isVolatileQualifiedType(RV.getReflectedDecl()->getType());

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    bool result = isVolatileQualifiedType(
            RV.getTypeOfReflectedResult(S.Context));

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_lvalue_reference_qualified(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (auto FT = dyn_cast<FunctionProtoType>(RV.getReflectedType()))
      result = (FT->getRefQualifier() == RQ_LValue);
  } else if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      if (auto FT = dyn_cast<FunctionProtoType>(FD->getType()))
        result = (FT->getRefQualifier() == RQ_LValue);
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_rvalue_reference_qualified(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (auto FT = dyn_cast<FunctionProtoType>(RV.getReflectedType()))
      result = (FT->getRefQualifier() == RQ_RValue);
  } else if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      if (auto FT = dyn_cast<FunctionProtoType>(FD->getType()))
        result = (FT->getRefQualifier() == RQ_RValue);
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_static_storage_duration(APValue &Result, Sema &S, EvalFn Evaluator,
                                 DiagFn Diagnoser, QualType ResultTy,
                                 SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (const auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Static;
    else if (isa<TemplateParamObjectDecl>(RV.getReflectedDecl()))
      result = true;
  } else if (RV.isReflectedObject()) {
    result = true;
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_thread_storage_duration(APValue &Result, Sema &S,
                                 EvalFn Evaluator, DiagFn Diagnoser,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (const auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Thread;
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_automatic_storage_duration(APValue &Result, Sema &S,
                                    EvalFn Evaluator, DiagFn Diagnoser,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (const auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Automatic;
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_internal_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(RV.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::Internal);
  } else if (RV.isReflectedDecl()) {
    if (const auto *ND = dyn_cast<NamedDecl>(RV.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::Internal);
  } else if (RV.isReflectedObject()) {
    if (APValue::LValueBase LVBase = RV.getReflectedObject().getLValueBase();
        LVBase.is<const ValueDecl *>()) {
      const ValueDecl *VD = LVBase.get<const ValueDecl *>();
      result = (VD->getFormalLinkage() == Linkage::Internal);
    }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_module_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(RV.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::Module);
  } else  if (RV.isReflectedDecl()) {
    if (const auto *ND = dyn_cast<NamedDecl>(RV.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::Module);
  } else if (RV.isReflectedObject()) {
    if (APValue::LValueBase LVBase = RV.getReflectedObject().getLValueBase();
        LVBase.is<const ValueDecl *>()) {
      const ValueDecl *VD = LVBase.get<const ValueDecl *>();
      result = (VD->getFormalLinkage() == Linkage::Module);
    }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_external_linkage(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(RV.getReflectedType())))
      result = (typeDecl->getFormalLinkage() == Linkage::External ||
                typeDecl->getFormalLinkage() == Linkage::UniqueExternal);
  } else if (RV.isReflectedDecl()) {
    if (const auto *ND = dyn_cast<NamedDecl>(RV.getReflectedDecl()))
      result = (ND->getFormalLinkage() == Linkage::External ||
                ND->getFormalLinkage() == Linkage::UniqueExternal);
  } else if (RV.isReflectedObject()) {
    if (APValue::LValueBase LVBase = RV.getReflectedObject().getLValueBase();
        LVBase.is<const ValueDecl *>()) {
      const ValueDecl *VD = LVBase.get<const ValueDecl *>();
      result = (VD->getFormalLinkage() == Linkage::External ||
                VD->getFormalLinkage() == Linkage::UniqueExternal);
    }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool has_linkage(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    if (NamedDecl *typeDecl =
            dyn_cast_or_null<NamedDecl>(findTypeDecl(RV.getReflectedType())))
      result = typeDecl->hasLinkage();
  } else if (RV.isReflectedDecl()) {
    if (const auto *ND = dyn_cast<NamedDecl>(RV.getReflectedDecl()))
      result = ND->hasLinkage();
  } else if (RV.isReflectedObject()) {
    if (APValue::LValueBase LVBase = RV.getReflectedObject().getLValueBase();
        LVBase.is<const ValueDecl *>()) {
      const ValueDecl *VD = LVBase.get<const ValueDecl *>();
      result = (VD->hasLinkage());
    }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_class_member(APValue &Result, Sema &S, EvalFn Evaluator,
                     DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;
  bool result = false;

  decltype(Diagnoser) SwallowDiags {};
  if (!parent_of(Scratch, S, Evaluator, SwallowDiags, S.Context.MetaInfoTy,
                 Range, Args)) {
    assert(Scratch.isReflection());
    result = Scratch.isReflectedType();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_namespace_member(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue Scratch;
  bool result = false;

  decltype(Diagnoser) SwallowDiags {};
  if (!parent_of(Scratch, S, Evaluator, SwallowDiags, S.Context.MetaInfoTy,
                 Range, Args)) {
    assert(Scratch.isReflection());
    result = Scratch.isReflectedNamespace();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_nonstatic_data_member(APValue &Result, Sema &S, EvalFn Evaluator,
                              DiagFn Diagnoser, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl())) {
      // Unnamed bit-fields are not members, but just about every other field
      // should be a nonstatic data member.
      result = (!FD->isBitField() || FD->getIdentifier());
    }
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_static_member(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    const ValueDecl *D = cast<ValueDecl>(RV.getReflectedDecl());
    if (const auto *MD = dyn_cast<CXXMethodDecl>(D))
      result = MD->isStatic();
    else if (const auto *VD = dyn_cast<VarDecl>(D))
      result = VD->isStaticDataMember();
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Template: {
    const Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();
    if (const auto *FTD = dyn_cast<FunctionTemplateDecl>(D)) {
      if (const auto *MD = dyn_cast<CXXMethodDecl>(FTD->getTemplatedDecl()))
        result = MD->isStatic();
    } else if (const auto *VTD = dyn_cast<VarTemplateDecl>(D)) {
      if (const auto *VD = dyn_cast<VarDecl>(VTD->getTemplatedDecl()))
        result = VD->isStaticDataMember();
    }
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_base(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result,
                       makeBool(S.Context, RV.isReflectedBaseSpecifier()));
}

bool is_data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result,
                       makeBool(S.Context, RV.isReflectedDataMemberSpec()));
}

bool is_namespace(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context, RV.isReflectedNamespace()));
}

bool is_function(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<const FunctionDecl>(RV.getReflectedDecl());
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_variable(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<const VarDecl>(RV.getReflectedDecl());
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_type(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context, RV.isReflectedType()));
}

bool is_alias(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool result = isTypeAlias(RV.getReflectedType());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Namespace: {
    bool result = isa<NamespaceAliasDecl>(RV.getReflectedNamespace());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    bool result = isa<TypeAliasTemplateDecl>(TDecl);
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_complete_type(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    // If this is a declared type with a reachable definition, ensure that the
    // type is instantiated.
    if (Decl *typeDecl = findTypeDecl(RV.getReflectedType()))
      (void) ensureInstantiated(S, typeDecl, Range);

    result = !RV.getReflectedType()->isIncompleteType();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_template(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context, RV.isReflectedTemplate()));
}

bool is_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsFnTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsFnTemplate = isa<FunctionTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsFnTemplate));
}

bool is_variable_template(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsVarTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsVarTemplate = isa<VarTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsVarTemplate));
}

bool is_class_template(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsClsTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsClsTemplate = isa<ClassTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsClsTemplate));
}

bool is_alias_template(APValue &Result, Sema &S, EvalFn Evaluator,
                       DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsAliasTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsAliasTemplate = TD->isTypeAlias();
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsAliasTemplate));
}

bool is_conversion_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                     DiagFn Diagnoser, QualType ResultTy,
                                     SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConversionTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsConversionTemplate = isa<CXXConversionDecl>(FTD->getTemplatedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsConversionTemplate));
}

bool is_operator_function_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                   DiagFn Diagnoser, QualType ResultTy,
                                   SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsOperatorTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsOperatorTemplate = (FTD->getTemplatedDecl()->getOverloadedOperator() !=
                            OO_None);
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsOperatorTemplate));
}

bool is_literal_operator_template(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsLiteralOperator = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsLiteralOperator = FTD->getDeclName().getNameKind() ==
                          DeclarationName::CXXLiteralOperatorName;
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsLiteralOperator));
}

bool is_constructor_template(APValue &Result, Sema &S, EvalFn Evaluator,
                             DiagFn Diagnoser, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsCtorTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsCtorTemplate = isa<CXXConstructorDecl>(FTD->getTemplatedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, IsCtorTemplate));
}

bool is_concept(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConcept = false;
  if (RV.isReflectedTemplate())
    IsConcept = isa<ConceptDecl>(RV.getReflectedTemplate().getAsTemplateDecl());

  return SetAndSucceed(Result, makeBool(S.Context, IsConcept));
}

bool is_structured_binding(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<BindingDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_value(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(S.Context, RV.isReflectedValue()));
}

bool is_object(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsObject = RV.isReflectedObject();
  if (RV.isReflectedDecl())
    IsObject = isa<TemplateParamObjectDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(S.Context, IsObject));
}

bool has_template_arguments(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    bool result = isTemplateSpecialization(QT);
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;

    Decl *D = RV.getReflectedDecl();
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      result = (FD->getTemplateSpecializationArgs() != nullptr);
    else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D))
      result = VTSD->getTemplateArgs().size() > 0;

    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_default_member_initializer(APValue &Result, Sema &S, EvalFn Evaluator,
                                    DiagFn Diagnoser, QualType ResultTy,
                                    SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool HasInitializer = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl()))
      HasInitializer = FD->hasInClassInitializer();

  return SetAndSucceed(Result, makeBool(S.Context, HasInitializer));
}

bool is_conversion_function(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConversion = false;
  if (RV.isReflectedDecl())
    IsConversion = isa<CXXConversionDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(S.Context, IsConversion));
}

bool is_operator_function(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsOperator = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsOperator = (FD->getOverloadedOperator() != OO_None);

  return SetAndSucceed(Result, makeBool(S.Context, IsOperator));
}

bool is_literal_operator(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsLiteralOperator = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsLiteralOperator = FD->getDeclName().getNameKind() ==
                          DeclarationName::CXXLiteralOperatorName;

  return SetAndSucceed(Result, makeBool(S.Context, IsLiteralOperator));
}

bool is_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::Template:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool result = isa<CXXConstructorDecl>(RV.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_default_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isDefaultConstructor();

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_copy_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isCopyConstructor();

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_move_constructor(APValue &Result, Sema &S, EvalFn Evaluator,
                         DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isMoveConstructor();

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_assignment(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      result = (FD->getOverloadedOperator() == OO_Equal);

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_copy_assignment(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      result = MD->isCopyAssignmentOperator();

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_move_assignment(APValue &Result, Sema &S, EvalFn Evaluator,
                        DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      result = MD->isMoveAssignmentOperator();

  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_destructor(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool result = isa<CXXDestructorDecl>(RV.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_special_member_function(APValue &Result, Sema &S, EvalFn Evaluator,
                                DiagFn Diagnoser, QualType ResultTy,
                                SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return SetAndSucceed(Result, makeBool(S.Context, false));
  case ReflectionKind::Declaration: {
    bool IsSpecial = false;
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsSpecial = isSpecialMember(FD);

    return SetAndSucceed(Result, makeBool(S.Context, IsSpecial));
  }
  case ReflectionKind::Template: {
    bool result = false;
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl))
      result = isSpecialMember(FTD->getTemplatedDecl());
    return SetAndSucceed(Result, makeBool(S.Context, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_user_provided(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsUserProvided = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl())) {
      FD = cast<FunctionDecl>(FD->getFirstDecl());
      IsUserProvided = !(FD->isImplicit() || FD->isDeleted() ||
                         FD->isDefaulted());
    }

  return SetAndSucceed(Result, makeBool(S.Context, IsUserProvided));
}

bool reflect_result(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue ArgTy;
  if (!Evaluator(ArgTy, Args[0], true))
    return true;
  assert(ArgTy.isReflectedType());
  bool IsLValue = isa<ReferenceType>(ArgTy.getReflectedType());

  if (!IsLValue && !ArgTy.getReflectedType()->isStructuralType())
    return Diagnoser(Range.getBegin(), diag::metafn_value_not_structural_type)
        << ArgTy.getReflectedType() << Range;

  APValue Arg;
  if (!Evaluator(Arg, Args[1], !IsLValue))
    return true;

  Expr *OVE = new (S.Context) OpaqueValueExpr(Range.getBegin(),
                                              Args[1]->getType(),
                                              IsLValue ? VK_LValue :
                                                         VK_PRValue);
  Expr *CE = ConstantExpr::Create(S.Context, OVE, Arg);
  {
    Expr::EvalResult Discarded;
    if (IsLValue && !CE->EvaluateAsLValue(Discarded, S.Context, true))
      return Diagnoser(Range.getBegin(),
                       diag::metafn_object_not_permitted_result);
  }

  // If this is an lvalue to a function, promote the result to reflect
  // the declaration.
  if (CE->getType()->isFunctionType() && Arg.isLValue() &&
      Arg.getLValueOffset().isZero())
    if (!Arg.hasLValuePath() || Arg.getLValuePath().size() == 0)
      if (APValue::LValueBase LVBase = Arg.getLValueBase();
          LVBase.is<const ValueDecl *>())
        return SetAndSucceed(
            Result,
            makeReflection(
                const_cast<ValueDecl *>(LVBase.get<const ValueDecl *>())));

  return SetAndSucceed(Result, Arg.Lift(Args[1]->getType()));
}

bool reflect_invoke(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(
      Args[1]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[2]->getType()->isIntegerType());
  assert(
      Args[3]->getType()->getPointeeOrArrayElementType()->isReflectionType());
  assert(Args[4]->getType()->isIntegerType());

  using ReflectionVector = SmallVector<APValue, 4>;
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
      Out.push_back(Scratch);
    }

    return true;
  };

  TemplateArgumentListInfo ExplicitTAListInfo;
  SmallVector<TemplateArgument, 4> ExplicitTArgs;
  {
    SmallVector<APValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[1], Args[2]))
      llvm_unreachable("failed to unpack template arguments from vector?");

    SmallVector<TemplateArgument, 4> ExplicitTArgs;
    for (APValue RV : Reflections) {
      if (!CanActAsTemplateArg(RV))
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
            << DescriptionOf(RV) << 1 << Range;

      TemplateArgument TArg = TArgFromReflection(S, Evaluator, RV,
                                                 Range.getBegin());
      if (TArg.isNull())
        llvm_unreachable("could not form template argument?");
      ExplicitTArgs.push_back(TArg);
    }

    ExplicitTAListInfo = addLocToTemplateArgs(S, ExplicitTArgs, Args[1]);
  }

  SmallVector<Expr *, 4> ArgExprs;
  {
    SmallVector<APValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[3], Args[4]))
      llvm_unreachable("failed to unpack function arguments from vector?");

    for (APValue RV : Reflections) {
      if (RV.isReflectedObject()) {
        Expr *OVE = new (S.Context) OpaqueValueExpr(
                Range.getBegin(), RV.getTypeOfReflectedResult(S.Context),
                VK_LValue);
        Expr *CE = ConstantExpr::Create(S.Context, OVE,
                                        RV.getReflectedObject());
        ArgExprs.push_back(CE);
      } else if (RV.isReflectedValue()) {
        Expr *OVE = new (S.Context) OpaqueValueExpr(
                Range.getBegin(), RV.getTypeOfReflectedResult(S.Context),
                VK_PRValue);
        Expr *CE = ConstantExpr::Create(S.Context, OVE, RV.getReflectedValue());
        ArgExprs.push_back(CE);
      } else if (RV.isReflectedDecl()) {
        ValueDecl *D = RV.getReflectedDecl();
        ArgExprs.push_back(
              DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                                  SourceLocation(), D, false, Range.getBegin(),
                                  D->getType(), VK_LValue, D, nullptr));
      } else {
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
            << DescriptionOf(RV) << 0 << Range;
      }
    }
  }

  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true))
    return true;

  if (ExplicitTAListInfo.size() > 0 && !Scratch.isReflectedTemplate())
    return DiagnoseReflectionKind(Diagnoser, Range, "a template",
                                  DescriptionOf(Scratch));

  Expr *FnRefExpr = nullptr;
  switch (Scratch.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_invoke)
        << DescriptionOf(Scratch) << Range;
  case ReflectionKind::Object: {
    Expr *OVE = new (S.Context) OpaqueValueExpr(
          Range.getBegin(), Scratch.getTypeOfReflectedResult(S.Context),
          VK_LValue);
    FnRefExpr = ConstantExpr::Create(S.Context, OVE,
                                     Scratch.getReflectedObject());
    break;
  }
  case ReflectionKind::Value: {
    Expr *OVE = new (S.Context) OpaqueValueExpr(
          Range.getBegin(), Scratch.getTypeOfReflectedResult(S.Context),
          VK_PRValue);
    FnRefExpr = ConstantExpr::Create(S.Context, OVE,
                                     Scratch.getReflectedValue());
    break;
  }
  case ReflectionKind::Declaration: {
    ValueDecl *D = Scratch.getReflectedDecl();
    ensureInstantiated(S, D, Range);

    FnRefExpr =
          DeclRefExpr::Create(S.Context, NestedNameSpecifierLoc(),
                              SourceLocation(), D, false, Range.getBegin(),
                              D->getType(), VK_LValue, D, nullptr);
    break;
  }
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = Scratch.getReflectedTemplate().getAsTemplateDecl();
    auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl);
    if (!FTD) {
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_invoke)
          << DescriptionOf(Scratch) << Range;
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
        return Diagnoser(Range.getBegin(), diag::metafn_no_specialization_found)
            << FTD << Range;
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
  {
    EnterExpressionEvaluationContext Context(
            S, Sema::ExpressionEvaluationContext::ConstantEvaluated);
    if (auto *DRE = dyn_cast<DeclRefExpr>(FnRefExpr);
        DRE && dyn_cast<CXXConstructorDecl>(DRE->getDecl())) {
      auto *CtorD = cast<CXXConstructorDecl>(DRE->getDecl());

      ER = S.BuildCXXConstructExpr(
            Range.getBegin(), QualType(CtorD->getParent()->getTypeForDecl(), 0),
            CtorD, false, ArgExprs, false, false, false, false,
            CXXConstructionKind::Complete, Range);
    } else {
      ER = S.ActOnCallExpr(S.getCurScope(), FnRefExpr, Range.getBegin(),
                           ArgExprs, Range.getEnd(), /*ExecConfig=*/nullptr);
    }
  }
  if (ER.isInvalid())
    return Diagnoser(Range.getBegin(), diag::metafn_invalid_call_expr) << Range;
  Expr *ResultExpr = ER.get();

  if (ResultExpr->isTypeDependent() || ResultExpr->isValueDependent())
    return true;

  if (!ResultExpr->getType()->isStructuralType() && !ResultExpr->isLValue())
    return Diagnoser(Range.getBegin(), diag::metafn_returns_non_structural_type)
        << ResultExpr->getType() << Range;

  Expr::EvalResult EvalResult;
  if (!ResultExpr->EvaluateAsConstantExpr(EvalResult, S.Context))
    return Diagnoser(Range.getBegin(),
                     diag::metafn_invocation_not_constant_expr)
        << Range;

  // If this is an lvalue to a function, promote the result to reflect
  // the declaration.
  if (ResultExpr->getType()->isFunctionType() &&
      EvalResult.Val.getKind() == APValue::LValue &&
      EvalResult.Val.getLValueOffset().isZero())
    if (!EvalResult.Val.hasLValuePath() ||
         EvalResult.Val.getLValuePath().size() == 0)
      if (APValue::LValueBase LVBase = EvalResult.Val.getLValueBase();
          LVBase.is<const ValueDecl *>())
        return SetAndSucceed(
              Result,
              makeReflection(
                  const_cast<ValueDecl *>(LVBase.get<const ValueDecl *>())));

  return SetAndSucceed(Result, EvalResult.Val.Lift(ResultExpr->getType()));
}

bool data_member_spec(APValue &Result, Sema &S, EvalFn Evaluator,
                      DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue Scratch;
  size_t ArgIdx = 0;

  // Extract the data member type.
  if (!Evaluator(Scratch, Args[ArgIdx++], true) || !Scratch.isReflectedType())
    return true;
  QualType MemberTy = Scratch.getReflectedType();

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
    if (!Lex.validateIdentifier(*Name))
      return Diagnoser(Range.getBegin(), diag::metafn_name_invalid_identifier)
          << *Name << Range;
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

  // Evaluate whether the "no_unique_address" attribute should apply.
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  bool NoUniqueAddress = Scratch.getInt().getBoolValue();
  ArgIdx++;

  TagDataMemberSpec *TDMS = new (S.Context) TagDataMemberSpec {
    MemberTy, Name, Alignment, BitWidth, NoUniqueAddress
  };
  return SetAndSucceed(Result, makeReflection(TDMS));
}

bool define_class(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true))
    return true;
  if (!Scratch.isReflectedType())
    return DiagnoseReflectionKind(Diagnoser, Range, "an incomplete class type",
                                  DescriptionOf(Scratch));

  QualType ToComplete = Scratch.getReflectedType();
  TagDecl *OGTag, *Tag;
  {
    NamedDecl *ND;
    if (!ToComplete->isIncompleteType(&ND))
      return Diagnoser(Range.getBegin(), diag::metafn_already_complete_type)
        << ToComplete << Range;
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
        IntegerLiteral *IL = IntegerLiteral::Create(S.Context,
                                                    TArg.getAsIntegral(),
                                                    TArg.getIntegralType(),
                                                    Range.getBegin());
        ParsedArgs.emplace_back(ParsedTemplateArgument::NonType, IL,
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

    if (!Evaluator(Scratch, Synthesized, true))
      return true;
    if (!Scratch.isReflectedDataMemberSpec()) {
      RestoreDC();
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a description of a data member",
                                    DescriptionOf(Scratch));
    }
    TagDataMemberSpec *TDMS = Scratch.getReflectedDataMemberSpec();


    // Build the member declaration from the extracted TagDataMemberSpec.
    unsigned DiagID;
    const char *PrevSpec;
    AttributeFactory AttrFactory;
    AttributePool AttrPool(AttrFactory);
    DeclSpec DS(AttrFactory);
    DS.SetStorageClassSpec(S, DeclSpec::SCS_unspecified,
                           Args[0]->getBeginLoc(),
                           PrevSpec, DiagID, S.Context.getPrintingPolicy());

    ParsedType MemberTy = ParsedType::make(TDMS->Ty);
    DS.SetTypeSpecType(TST_typename, Args[0]->getBeginLoc(), PrevSpec, DiagID,
                       MemberTy, S.Context.getPrintingPolicy());

    // Add any attributes (i.e., 'AlignAttr' if alignment is specified).
    ParsedAttributesView MemberAttrs;
    if (TDMS->Alignment) {
      IdentifierInfo &AttrII = S.PP.getIdentifierTable().get("alignas");

      IntegerLiteral *IL = IntegerLiteral::Create(
              S.Context,
              llvm::APSInt::getUnsigned(TDMS->Alignment.value()),
              S.Context.getSizeType(),
              Range.getBegin());

      ArgsUnion Args(IL);
      ParsedAttr::Form Form(tok::kw_alignas);
      ParsedAttr *Attr = AttrPool.create(&AttrII, Range, nullptr,
                                         SourceLocation(), &Args, 1, Form);
      MemberAttrs.addAtEnd(Attr);
    }
    if (TDMS->NoUniqueAddress) {
      IdentifierInfo &AttrII =
          S.PP.getIdentifierTable().get("no_unique_address");
      ParsedAttr *Attr = AttrPool.create(&AttrII, Range, nullptr,
                                         SourceLocation(), nullptr, 0,
                                         ParsedAttr::Form::CXX11());
      MemberAttrs.addAtEnd(Attr);
    }

    Declarator MemberDeclarator(DS, MemberAttrs, DeclaratorContext::Member);

    if (!TDMS->BitWidth || *TDMS->BitWidth > 0) {
      std::string MemberName = TDMS->Name.value_or(
              "__" + llvm::toString(llvm::APSInt::get(anonMemCtr++), 10));
      IdentifierInfo &II = S.PP.getIdentifierTable().get(MemberName);
      MemberDeclarator.SetIdentifier(&II, Tag->getBeginLoc());
    }

    // Create an expression for bit width, if specified.
    Expr *BitWidthCE = nullptr;
    if (TDMS->BitWidth) {
      BitWidthCE = IntegerLiteral::Create(
              S.Context,
              llvm::APSInt::getUnsigned(TDMS->BitWidth.value()),
              S.Context.getSizeType(),
              Range.getBegin());
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

bool offset_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  case ReflectionKind::Declaration: {
    if (const FieldDecl *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(S.Context, FD) /
                      S.Context.getTypeSize(S.Context.CharTy);
      return SetAndSucceed(
              Result,
              APValue(S.Context.MakeIntValue(Offset, S.Context.getSizeType())));
    }
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool size_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    size_t Sz = S.Context.getTypeSizeInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    QualType QT = RV.getTypeOfReflectedResult(S.Context);
    size_t Sz = S.Context.getTypeSizeInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *VD = RV.getReflectedDecl();
    size_t Sz = S.Context.getTypeSizeInChars(VD->getType()).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    size_t Sz = S.Context.getTypeSizeInChars(TDMS->Ty).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 3 << DescriptionOf(RV);
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_offset_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  case ReflectionKind::Declaration: {
    if (FieldDecl *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(S.Context, FD) %
                      S.Context.getTypeSize(S.Context.CharTy);
      return SetAndSucceed(
              Result,
              APValue(S.Context.MakeIntValue(Offset, S.Context.getSizeType())));
    }
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_size_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    size_t Sz = S.Context.getTypeSize(QT);
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    size_t Sz = S.Context.getTypeSize(RV.getTypeOfReflectedResult(S.Context));
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    const ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    size_t Sz = S.Context.getTypeSize(VD->getType());

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD))
      if (FD->isBitField())
        Sz = FD->getBitWidthValue(S.Context);

    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();

    size_t Sz = TDMS->BitWidth.value_or(S.Context.getTypeSize(TDMS->Ty));
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Sz, S.Context.getSizeType())));
  }

  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 3 << DescriptionOf(RV);
  }
  llvm_unreachable("unknown reflection kind");
}

bool alignment_of(APValue &Result, Sema &S, EvalFn Evaluator, DiagFn Diagnoser,
                  QualType ResultTy, SourceRange Range,
                  ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    size_t Align = S.Context.getTypeAlignInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    QualType QT = RV.getTypeOfReflectedResult(S.Context);
    size_t Align = S.Context.getTypeAlignInChars(QT).getQuantity();
    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    const ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    size_t Align = S.Context.getTypeAlignInChars(VD->getType()).getQuantity();

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD)) {
      if (FD->isBitField())
        return true;
      Align = S.Context.getDeclAlign(FD, true).getQuantity();
    }

    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    if (TDMS->BitWidth)
      return true;

    size_t Align = TDMS->Alignment.value_or(
          S.Context.getTypeAlignInChars(TDMS->Ty).getQuantity());

    return SetAndSucceed(
            Result,
            APValue(S.Context.MakeIntValue(Align, S.Context.getSizeType())));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 4 << DescriptionOf(RV);
  }
  llvm_unreachable("unknown reflection kind");
}

bool define_static_string(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());

  APValue Scratch;

  // Evaluate the character type.
  if (!Evaluator(Scratch, Args[1], true))
    return true;
  QualType CharTy = Scratch.getReflectedType();

  // Evaluate the length of the string provided.
  std::string Contents;
  if (!Evaluator(Scratch, Args[2], true))
    return true;
  size_t Length = Scratch.getInt().getExtValue();
  Contents.resize(Length);

  // Evaluate the given name. Miserably inefficient, but gets the job done.
  for (uint64_t k = 0; k < Length; ++k) {
    llvm::APInt Idx(S.Context.getTypeSize(S.Context.getSizeType()), k, false);
    Expr *Synthesized = IntegerLiteral::Create(S.Context, Idx,
                                               S.Context.getSizeType(),
                                               Args[3]->getExprLoc());

    Synthesized = new (S.Context) ArraySubscriptExpr(Args[3],
                                                     Synthesized, CharTy,
                                                     VK_LValue, OK_Ordinary,
                                                     Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
      return true;

    if (!Evaluator(Scratch, Synthesized, true))
      return true;

    Contents[k] = static_cast<char>(Scratch.getInt().getExtValue());
  }

  if (!Evaluator(Scratch, Args[4], true))
    return true;
  bool IsUtf8 = Scratch.getInt().getBoolValue();

  VarDecl *AnonArr = S.Context.getGeneratedCharArray(Contents, IsUtf8);
  if (!AnonArr->hasInit()) {
    Expr *StrLit = makeStrLiteral(Contents, S.Context, IsUtf8);

    AnonArr->setConstexpr(true);
    S.AddInitializerToDecl(AnonArr, StrLit, false);

    DeclGroupRef DG(AnonArr);
    S.Consumer.HandleTopLevelDecl(DG);
  }
  assert(AnonArr->getFormalLinkage() == Linkage::Internal);

  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result,
                       APValue(AnonArr, CharUnits::Zero(), Path, false));
}

bool get_ith_parameter_of(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  APValue Idx;
  if (!Evaluator(Idx, Args[2], true))
    return true;
  size_t idx = Idx.getInt().getExtValue();

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    if (auto FT = dyn_cast<FunctionProtoType>(RV.getReflectedType())) {
      unsigned numParams = FT->getNumParams();
      if (idx >= numParams)
        return SetAndSucceed(Result, Sentinel);

      return SetAndSucceed(Result, makeReflection(FT->getParamType(idx)));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 2 << 2;
  }
  case ReflectionKind::Declaration: {
    if (auto FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl())) {
      unsigned numParams = FD->getNumParams();
      if (idx >= numParams)
        return SetAndSucceed(Result, Sentinel);

      return SetAndSucceed(Result, makeReflection(FD->getParamDecl(idx)));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 5 << DescriptionOf(RV) << Range;
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return true;
  }
  return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
}

bool has_consistent_identifier(APValue &Result, Sema &S, EvalFn Evaluator,
                               DiagFn Diagnoser, QualType ResultTy,
                               SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration:
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl())) {
      [[maybe_unused]] std::string Unused;
      bool Consistent = getParameterName(PVD, Unused);

      return SetAndSucceed(Result, makeBool(S.Context, Consistent));
    }
    [[fallthrough]];
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return has_identifier(Result, S, Evaluator, Diagnoser, ResultTy, Range,
                          Args);
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_ellipsis_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                            DiagFn Diagnoser, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
  case ReflectionKind::Type:
    if (auto *FPT = dyn_cast<FunctionProtoType>(RV.getReflectedType())) {
      bool HasEllipsis = FPT->isVariadic();
      return SetAndSucceed(Result, makeBool(S.Context, HasEllipsis));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 2 << 2;
  case ReflectionKind::Declaration: {
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl())) {
      bool HasEllipsis = FD->getEllipsisLoc().isValid();
      return SetAndSucceed(Result, makeBool(S.Context, HasEllipsis));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_default_argument(APValue &Result, Sema &S, EvalFn Evaluator,
                          DiagFn Diagnoser, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl())) {
      return SetAndSucceed(Result, makeBool(S.Context, PVD->hasDefaultArg()));
    }
    [[fallthrough]];
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return DiagnoseReflectionKind(Diagnoser, Range, "a function parameter",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_explicit_object_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                                  DiagFn Diagnoser, QualType ResultTy,
                                  SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl()))
      result = PVD->isExplicitObjectParameter();
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool is_function_parameter(APValue &Result, Sema &S, EvalFn Evaluator,
                           DiagFn Diagnoser, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    result = isa<const ParmVarDecl>(RV.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(S.Context, result));
}

bool return_type_of(APValue &Result, Sema &S, EvalFn Evaluator,
                    DiagFn Diagnoser, QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == S.Context.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    if (auto *FPT = dyn_cast<FunctionProtoType>(RV.getReflectedType()))
      return SetAndSucceed(Result, makeReflection(FPT->getReturnType()));

    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 3 << 2;
  case ReflectionKind::Declaration:
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl());
        FD && !isa<CXXConstructorDecl>(FD) && !isa<CXXDestructorDecl>(FD))
      return SetAndSucceed(Result, makeReflection(FD->getReturnType()));
    [[fallthrough]];
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 6 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

}  // end namespace clang
