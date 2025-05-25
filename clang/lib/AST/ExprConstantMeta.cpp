//===-- ExprConstantMeta.cpp - Functions targeting reflections --*- C++ -*-===//
//
// Copyright 2025 Bloomberg Finance L.P.
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
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Metafunction.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Reflection.h"
#include "clang/AST/Type.h"
#include "clang/Basic/AttributeCommonInfo.h"
#include "clang/Basic/DiagnosticMetafn.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/Lexer.h"
#include "clang/Sema/ParsedAttr.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {

using EvalFn = Metafunction::EvaluateFn;
using DiagFn = Metafunction::DiagnoseFn;

// -----------------------------------------------------------------------------
// P2996 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_begin_enumerator_decl_of(APValue &Result, ASTContext &C,
                                         MetaActions &Meta, EvalFn Evaluator,
                                         DiagFn Diagnoser, bool AllowInjection,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args,
                                         Decl *ContainingDecl);

static bool get_next_enumerator_decl_of(APValue &Result, ASTContext &C,
                                        MetaActions &Meta, EvalFn Evaluator,
                                        DiagFn Diagnoser, bool AllowInjection,
                                        QualType ResultTy, SourceRange Range,
                                        ArrayRef<Expr *> Args,
                                        Decl *ContainingDecl);

static bool get_ith_base_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl);

static bool get_ith_template_argument_of(APValue &Result, ASTContext &C,
                                         MetaActions &Meta, EvalFn Evaluator,
                                         DiagFn Diagnoser, bool AllowInjection,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args,
                                         Decl *ContainingDecl);

static bool get_begin_member_decl_of(APValue &Result, ASTContext &C,
                                     MetaActions &Meta, EvalFn Evaluator,
                                     DiagFn Diagnoser, bool AllowInjection,
                                     QualType ResultTy, SourceRange Range,
                                     ArrayRef<Expr *> Args,
                                     Decl *ContainingDecl);

static bool get_next_member_decl_of(APValue &Result, ASTContext &C,
                                    MetaActions &Meta, EvalFn Evaluator,
                                    DiagFn Diagnoser, bool AllowInjection,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args,
                                    Decl *ContainingDecl);

static bool is_structural_type(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool map_decl_to_entity(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool identifier_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool has_identifier(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl);

static bool operator_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool source_location_of(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool type_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool parent_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool underlying_entity_of(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool proxied_entity_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl);

static bool constant_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool object_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool template_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool substitute(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool extract(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_public(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_protected(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl);

static bool is_private(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_virtual(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_pure_virtual(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl);

static bool is_override(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_deleted(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_defaulted(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl);

static bool is_explicit(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_noexcept(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_bit_field(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl);

static bool is_enumerator(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool is_const(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_volatile(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_mutable_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl);

static bool is_lvalue_reference_qualified(APValue &Result, ASTContext &C,
                                          MetaActions &Meta, EvalFn Evaluator,
                                          DiagFn Diagnoser, bool AllowInjection,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args,
                                          Decl *ContainingDecl);

static bool is_rvalue_reference_qualified(APValue &Result, ASTContext &C,
                                          MetaActions &Meta, EvalFn Evaluator,
                                          DiagFn Diagnoser, bool AllowInjection,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args,
                                          Decl *ContainingDecl);

static bool has_static_storage_duration(APValue &Result, ASTContext &C,
                                        MetaActions &Meta, EvalFn Evaluator,
                                        DiagFn Diagnoser, bool AllowInjection,
                                        QualType ResultTy, SourceRange Range,
                                        ArrayRef<Expr *> Args,
                                        Decl *ContainingDecl);

static bool has_thread_storage_duration(APValue &Result, ASTContext &C,
                                        MetaActions &Meta, EvalFn Evaluator,
                                        DiagFn Diagnoser, bool AllowInjection,
                                        QualType ResultTy, SourceRange Range,
                                        ArrayRef<Expr *> Args,
                                        Decl *ContainingDecl);

static bool has_automatic_storage_duration(APValue &Result, ASTContext &C,
                                           MetaActions &Meta, EvalFn Evaluator,
                                           DiagFn Diagnoser,
                                           bool AllowInjection,
                                           QualType ResultTy, SourceRange Range,
                                           ArrayRef<Expr *> Args,
                                           Decl *ContainingDecl);

static bool has_internal_linkage(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_module_linkage(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_external_linkage(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_class_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl);

static bool is_namespace_member(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_nonstatic_data_member(APValue &Result, ASTContext &C,
                                     MetaActions &Meta, EvalFn Evaluator,
                                     DiagFn Diagnoser, bool AllowInjection,
                                     QualType ResultTy, SourceRange Range,
                                     ArrayRef<Expr *> Args,
                                     Decl *ContainingDecl);

static bool is_static_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool is_base(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_data_member_spec(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_namespace(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl);

static bool is_function(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_variable(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_type(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_alias(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_entity_proxy(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl);

static bool is_complete_type(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool has_complete_definition(APValue &Result, ASTContext &C,
                                    MetaActions &Meta, EvalFn Evaluator,
                                    DiagFn Diagnoser, bool AllowInjection,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args,
                                    Decl *ContainingDecl);

static bool is_enumerable_type(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_function_template(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_variable_template(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_class_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl);

static bool is_alias_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl);

static bool is_conversion_function_template(APValue &Result, ASTContext &C,
                                            MetaActions &Meta, EvalFn Evaluator,
                                            DiagFn Diagnoser,
                                            bool AllowInjection,
                                            QualType ResultTy,
                                            SourceRange Range,
                                            ArrayRef<Expr *> Args,
                                            Decl *ContainingDecl);

static bool is_operator_function_template(APValue &Result, ASTContext &C,
                                          MetaActions &Meta, EvalFn Evaluator,
                                          DiagFn Diagnoser, bool AllowInjection,
                                          QualType ResultTy, SourceRange Range,
                                          ArrayRef<Expr *> Args,
                                          Decl *ContainingDecl);

static bool is_literal_operator_template(APValue &Result, ASTContext &C,
                                         MetaActions &Meta, EvalFn Evaluator,
                                         DiagFn Diagnoser, bool AllowInjection,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args,
                                         Decl *ContainingDecl);

static bool is_constructor_template(APValue &Result, ASTContext &C,
                                    MetaActions &Meta, EvalFn Evaluator,
                                    DiagFn Diagnoser, bool AllowInjection,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args,
                                    Decl *ContainingDecl);

static bool is_concept(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_structured_binding(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_value(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_object(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_template_arguments(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_default_member_initializer(APValue &Result, ASTContext &C,
                                           MetaActions &Meta, EvalFn Evaluator,
                                           DiagFn Diagnoser,
                                           bool AllowInjection,
                                           QualType ResultTy, SourceRange Range,
                                           ArrayRef<Expr *> Args,
                                           Decl *ContainingDecl);

static bool is_conversion_function(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_operator_function(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_literal_operator(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_constructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl);

static bool is_default_constructor(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_copy_constructor(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_move_constructor(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_assignment(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool is_copy_assignment(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_move_assignment(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_destructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool is_special_member_function(APValue &Result, ASTContext &C,
                                       MetaActions &Meta, EvalFn Evaluator,
                                       DiagFn Diagnoser, bool AllowInjection,
                                       QualType ResultTy, SourceRange Range,
                                       ArrayRef<Expr *> Args,
                                       Decl *ContainingDecl);

static bool is_user_provided(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool is_user_declared(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool reflect_result(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl);

static bool data_member_spec(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool define_aggregate(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool offset_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool size_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool bit_offset_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool bit_size_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser,
                        bool AllowInjection, QualType ResultTy,
                        SourceRange Range, ArrayRef<Expr *> Args,
                        Decl *ContainingDecl);

static bool alignment_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl);

// -----------------------------------------------------------------------------
// P3096 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_ith_parameter_of(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_consistent_identifier(APValue &Result, ASTContext &C,
                                      MetaActions &Meta, EvalFn Evaluator,
                                      DiagFn Diagnoser, bool AllowInjection,
                                      QualType ResultTy, SourceRange Range,
                                      ArrayRef<Expr *> Args,
                                      Decl *ContainingDecl);

static bool has_ellipsis_parameter(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_default_argument(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_explicit_object_parameter(APValue &Result, ASTContext &C,
                                         MetaActions &Meta, EvalFn Evaluator,
                                         DiagFn Diagnoser, bool AllowInjection,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args,
                                         Decl *ContainingDecl);

static bool is_function_parameter(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool return_type_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl);

static bool get_ith_annotation_of(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_annotation(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);

static bool annotate(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl);

// -----------------------------------------------------------------------------
// P3385 Metafunction declarations
// -----------------------------------------------------------------------------

static bool get_ith_attribute_of(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_attribute(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool appertain(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

                          // =========================
                          // Accessibility API (P3493)
                          // =========================

static bool current_access_context(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool is_accessible(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl);


             // ===================================================
             // Other bespoke functions (not proposed at this time)
             // ===================================================

static bool is_access_specified(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool reflect_invoke(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl);

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
  { Metafunction::MFRK_bool, 1, 1, is_structural_type },
  { Metafunction::MFRK_metaInfo, 1, 1, map_decl_to_entity },

  // exposed metafunctions
  { Metafunction::MFRK_spliceFromArg, 4, 4, identifier_of },
  { Metafunction::MFRK_bool, 1, 1, has_identifier },
  { Metafunction::MFRK_sizeT, 1, 1, operator_of },
  { Metafunction::MFRK_sourceLoc, 1, 1, source_location_of },
  { Metafunction::MFRK_metaInfo, 1, 1, type_of },
  { Metafunction::MFRK_metaInfo, 1, 1, parent_of },
  { Metafunction::MFRK_metaInfo, 1, 1, underlying_entity_of },
  { Metafunction::MFRK_metaInfo, 1, 1, proxied_entity_of },
  { Metafunction::MFRK_metaInfo, 1, 1, object_of },
  { Metafunction::MFRK_metaInfo, 1, 1, constant_of },
  { Metafunction::MFRK_metaInfo, 1, 1, template_of },
  { Metafunction::MFRK_metaInfo, 4, 4, substitute },
  { Metafunction::MFRK_spliceFromArg, 2, 2, extract },
  { Metafunction::MFRK_bool, 1, 1, is_public },
  { Metafunction::MFRK_bool, 1, 1, is_protected },
  { Metafunction::MFRK_bool, 1, 1, is_private },
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
  { Metafunction::MFRK_bool, 1, 1, is_mutable_member },
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
  { Metafunction::MFRK_bool, 1, 1, is_entity_proxy },
  { Metafunction::MFRK_bool, 1, 1, is_complete_type },
  { Metafunction::MFRK_bool, 1, 1, has_complete_definition },
  { Metafunction::MFRK_bool, 1, 1, is_enumerable_type },
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
  { Metafunction::MFRK_bool, 1, 1, is_user_declared },
  { Metafunction::MFRK_metaInfo, 2, 2, reflect_result },
  { Metafunction::MFRK_metaInfo, 10, 10, data_member_spec },
  { Metafunction::MFRK_metaInfo, 3, 3, define_aggregate },
  { Metafunction::MFRK_spliceFromArg, 2, 2, offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, size_of },
  { Metafunction::MFRK_spliceFromArg, 2, 2, bit_offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, bit_size_of },
  { Metafunction::MFRK_sizeT, 1, 1, alignment_of },

  // P3096 metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_parameter_of },
  { Metafunction::MFRK_bool, 1, 1, has_consistent_identifier },
  { Metafunction::MFRK_bool, 1, 1, has_ellipsis_parameter },
  { Metafunction::MFRK_bool, 1, 1, has_default_argument },
  { Metafunction::MFRK_bool, 1, 1, is_explicit_object_parameter },
  { Metafunction::MFRK_bool, 1, 1, is_function_parameter },
  { Metafunction::MFRK_metaInfo, 1, 1, return_type_of },

  // P3394 annotation metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_annotation_of },
  { Metafunction::MFRK_bool, 1, 1, is_annotation },
  { Metafunction::MFRK_metaInfo, 2, 2, annotate },

  // P3385 attributes reflection
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_attribute_of },
  { Metafunction::MFRK_bool, 1, 1, is_attribute },
  { Metafunction::MFRK_metaInfo, 2, 2, appertain },

  // P3493 accessibility extensions
  { Metafunction::MFRK_metaInfo, 0, 0, current_access_context },
  { Metafunction::MFRK_bool, 3, 3, is_accessible },

  // Other bespoke functions (not proposed at this time)
  { Metafunction::MFRK_bool, 1, 1, is_access_specified },
  { Metafunction::MFRK_metaInfo, 5, 5, reflect_invoke },
};
constexpr const unsigned NumMetafunctions = sizeof(Metafunctions) /
                                            sizeof(Metafunction);


// -----------------------------------------------------------------------------
// class Metafunction implementation
// -----------------------------------------------------------------------------

bool Metafunction::evaluate(APValue &Result, ASTContext &C,
                            MetaActions &Meta, EvalFn Evaluator,
                            DiagFn Diagnoser, bool AllowInjection,
                            QualType ResultTy, SourceRange Range,
                            ArrayRef<Expr *> Args, Decl *ContainingDecl) const {
  return ImplFn(Result, C, Meta, Evaluator, Diagnoser, AllowInjection, ResultTy,
                Range, Args, ContainingDecl);
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

static APValue makeReflection(std::nullptr_t) {
  return APValue(ReflectionKind::Null, nullptr);
}

static APValue makeReflection(QualType QT) {
  return APValue(ReflectionKind::Type, QT.getAsOpaquePtr());
}

static APValue makeReflection(Decl *D) {
  if (isa<NamespaceDecl>(D) || isa<NamespaceAliasDecl>(D) ||
      isa<TranslationUnitDecl>(D))
    return APValue(ReflectionKind::Namespace, D);
  else if (isa<TemplateDecl>(D))
    return APValue(ReflectionKind::Template, D);
  else if (isa<UsingShadowDecl>(D))
    return APValue(ReflectionKind::EntityProxy, D);

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

static APValue makeReflection(CXX26AnnotationAttr *A) {
  return APValue(ReflectionKind::Annotation, A);
}

static APValue makeReflection(const ParsedAttr * Attr) {
  return APValue(ReflectionKind::Attribute, Attr);
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
  // Parameters instantiated from function parameter packs are not considered
  // to have identifiers.
  if (auto STTPT = dyn_cast<SubstTemplateTypeParmType>(PVD->getType());
      STTPT && STTPT->getPackIndex())
    return true;

  unsigned ParamIdx = PVD->getFunctionScopeIndex();

  // TODO(P2996): This will crash if we're in the trailing requires-clause of
  // a function declaration, since the DeclContext is not the function but the
  // TranslationUnitDecl.
  FunctionDecl *FD = cast<FunctionDecl>(PVD->getDeclContext());
  FD = FD->getMostRecentDecl();
  PVD = FD->getParamDecl(ParamIdx);

  bool Consistent = true;
  StringRef FirstNameSeen = PVD->getName();

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

static ParmVarDecl *getMostRecentParmVarDecl(ParmVarDecl *PVD) {
  // TODO(P2996): This will crash if we're in the trailing requires-clause of
  // a function declaration, since the DeclContext is not the function but the
  // TranslationUnitDecl.
  FunctionDecl *FD = cast<FunctionDecl>(PVD->getDeclContext());
  FD = FD->getMostRecentDecl();
  return FD->getParamDecl(PVD->getFunctionScopeIndex());
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
                                ResultTy, B->getBeginLoc(), SourceLocation(),
                                B->getDerived());
  return !Evaluator(Result, SLE, true);
}

static bool findAnnotLoc(APValue &Result, ASTContext &C, EvalFn Evaluator,
                         QualType ResultTy, CXX26AnnotationAttr *A) {
  SourceLocExpr *SLE =
          new (C) SourceLocExpr(C, SourceLocIdentKind::SourceLocStruct,
                                ResultTy, A->getEqLoc(), SourceLocation(),
                                nullptr);
  return !Evaluator(Result, SLE, true);
}

static bool findAttrLoc(APValue &Result, ASTContext &C, EvalFn Evaluator,
                         QualType ResultTy, AttributeCommonInfo *A) {
  SourceLocExpr *SLE =
          new (C) SourceLocExpr(C, SourceLocIdentKind::SourceLocStruct,
                                ResultTy, A->getLoc(), SourceLocation(),
                                nullptr);
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

static APValue getNthTemplateArgument(ASTContext &C,
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
                           CharUnits::fromQuantity(C.getTargetNullPointerValue(
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
    case TemplateArgument::Splice:
      llvm_unreachable("TemplateArgument::Splice should have been transformed "
                       "by now");
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

static size_t getOffsetOfBase(ASTContext &C, const CXXBaseSpecifier *Base) {
  const CXXRecordDecl *Derived = Base->getDerived();
  assert(Derived && "no parent for field!");

  const ASTRecordLayout &Layout = C.getASTRecordLayout(Derived);

  QualType BaseQT = Base->getType();
  BaseQT = desugarType(BaseQT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                       /*DropRefs=*/false);
  CXXRecordDecl *RD = BaseQT->getAsCXXRecordDecl();
  assert(RD && "base isn't a record type?");

  if (Base->isVirtual())
    return Layout.getVBaseClassOffset(RD).getQuantity();
  else
    return Layout.getBaseClassOffset(RD).getQuantity();
}

static bool ensureDeclared(ASTContext &C, QualType QT, SourceLocation SpecLoc) {
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
                C, CTD->getTemplatedDecl()->getTagKind(),
                CTD->getDeclContext(), SpecLoc, SpecLoc,  CTD,
                TS->template_arguments(), false, nullptr);
        if (!D)
          return false;

        CTD->AddSpecialization(D, InsertPos);
      }
    }
  }
  return true;
}

static bool isReflectableDecl(MetaActions &Meta, ASTContext &C, Decl *D) {
  assert(D && "null declaration");

  if (isa<NamespaceAliasDecl>(D))
    return true;

  if (!isa<VarDecl, FunctionDecl, TypeDecl, FieldDecl, TemplateDecl,
           NamespaceDecl, NamespaceAliasDecl, TranslationUnitDecl,
           UsingShadowDecl>(D))
    return false;

  if (isa<UsingShadowDecl>(D) && !C.getLangOpts().EntityProxyReflection)
    return false;

  if (auto *Class = dyn_cast<CXXRecordDecl>(D))
    if (Class->isInjectedClassName() || Class->isLambda())
      return false;

  if (auto *FD = dyn_cast<FunctionDecl>(D)) {
    for (auto *R = FD->getMostRecentDecl(); R; R = R->getPreviousDecl()) {
      if (!R->getDeclaredReturnType()->isUndeducedType() &&
          R->getDeclContext() == R->getLexicalDeclContext() &&
          Meta.HasSatisfiedConstraints(R))
        return true;
    }
    return false;
  }

  if (isa<ClassTemplateSpecializationDecl, VarTemplateSpecializationDecl>(D))
    return false;

  return D->getCanonicalDecl() == D;
}

/// Filter non-reflectable members.
static Decl *findIterableMember(MetaActions &Meta, ASTContext &C, Decl *D,
                                bool Inclusive) {
  if (!D)
    return D;

  if (Inclusive) {
    if (isReflectableDecl(Meta, C, D))
      return D;

    // Handle the case where the first Decl is a LinkageSpecDecl.
    if (auto *LSDecl = dyn_cast_or_null<LinkageSpecDecl>(D)) {
      Decl *RecD = findIterableMember(Meta, C, *LSDecl->decls_begin(), true);
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
      Decl *RecD = findIterableMember(Meta, C, *LSDecl->decls_begin(), true);
      if (RecD) return RecD;
    }

    // Pop back out of a recursively entered LinkageSpecDecl.
    if (!D && isa<LinkageSpecDecl>(DC))
      return findIterableMember(Meta, C, cast<Decl>(DC), false);
  } while (D && !isReflectableDecl(Meta, C, D));

  return D;
}

unsigned parentOf(APValue &Result, Decl *D) {
  if (!D)
    return diag::metafn_parent_of_undeclared;

  if (auto *FD = dyn_cast<FunctionDecl>(D); FD && FD->isExternC())
    return diag::metafn_parent_of_extern_c;
  else if (auto *VD = dyn_cast<VarDecl>(D); VD && VD->isExternC())
    return diag::metafn_parent_of_extern_c;

  auto *DC = D->getDeclContext();
  while (DC && !isa<NamespaceDecl>(DC) && !isa<RecordDecl>(DC) &&
               !isa<FunctionDecl>(DC) && !isa<TranslationUnitDecl>(DC) &&
               !isa<EnumDecl>(DC))
    DC = DC->getParent();

  assert(DC);
  if (auto *RD = dyn_cast<TagDecl>(DC))
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

QualType ComputeResultType(QualType ExprTy, const APValue &V) {
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

static APValue MaybeUnproxy(ASTContext &C, APValue RV, bool Dealias = true) {
  assert(RV.isReflection());

  if (!RV.isReflectedEntityProxy())
    return RV;

  NamedDecl *ND = RV.getReflectedEntityProxy()->getTargetDecl();
  if (auto *T = dyn_cast<TypeDecl>(ND)) {
    QualType QT = C.getTypeDeclType(T);
    if (Dealias)
      QT = desugarType(QT, /*UnwrapAlias=*/true, /*DropCV=*/false,
                       /*DropRefs=*/false);

    return APValue(ReflectionKind::Type, QT.getAsOpaquePtr());
  } else if (auto *T = dyn_cast<TemplateDecl>(ND)) {
    return APValue(ReflectionKind::Template, T);
  }

  return APValue(ReflectionKind::Declaration, ND);
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
  case ReflectionKind::EntityProxy: {
    return "an entity proxy";
  }
  case ReflectionKind::BaseSpecifier: {
    return "a base class specifier";
  }
  case ReflectionKind::DataMemberSpec: {
    return "a description of a non-static data member";
  }
  case ReflectionKind::Annotation: {
    return "an annotation";
  }
  case ReflectionKind::Attribute: {
    return "an attribute";
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

struct AttributeScratchpad {
  AttributeFactory factory;
  ParsedAttributes attributes;
  ArgsVector ArgExprs;
  bool argFound;
  AttributeScratchpad() : factory(), attributes(factory), ArgExprs(), argFound(false) {}
};

llvm::SmallVector<const Attr*, 8> static collectUniqueCxx11Attrs(const Decl *D) {
  llvm::SmallVector<const Attr*, 8> Result;
  llvm::SmallSet<attr::Kind, 8> SeenKinds;

  for (const Decl *RD : D->redecls()) {
    for (const Attr *A : RD->getAttrs()) {
      if (A->isCXX11Attribute() && SeenKinds.insert(A->getKind()).second) {
        Result.push_back(A);
      }
    }
  }

  return Result;
}

// -----------------------------------------------------------------------------
// Metafunction implementations
// -----------------------------------------------------------------------------

bool get_ith_attribute_of(APValue &Result, ASTContext &C,
                          MetaActions &Meta, EvalFn Evaluator,
                          DiagFn Diagnoser, bool AllowInjection,
                          QualType ResultTy, SourceRange Range,
                          ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
    case ReflectionKind::Attribute: {
      if (idx != 0) {
        return SetAndSucceed(Result, Sentinel);
      }
      ParsedAttr *attr = RV.getReflectedAttribute();
      if (attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11) {
        return SetAndSucceed(Result, makeReflection(attr));
      }
      // Non standard
      return Diagnoser(Range.getBegin(), diag::metafn_p3385_non_standard_attribute)
        << attr->getAttrName();
    }
    case ReflectionKind::Type: {
      QualType qType = RV.getReflectedType();
      Decl *D = findTypeDecl(qType)->getMostRecentDecl();
      if (!D) {
        // FIXME how would we end up here ?
        return DiagnoseReflectionKind(Diagnoser, Range, "attribute, type or declaration",
                                    DescriptionOf(RV));
      }

      if (!D->hasAttrs()) {
        return SetAndSucceed(Result, Sentinel);
      }

      auto cxx11Attrs = collectUniqueCxx11Attrs(D);

      if (idx >= cxx11Attrs.size()) {
        return SetAndSucceed(Result, Sentinel);
      }

      // Attr -> ParsedAttr
      const Attr * const val = cxx11Attrs[idx];
      assert(val);
      static AttributeScratchpad scratchpad;

      const ParsedAttr * parsedAttr = val->fromParsedAttr();
      assert(parsedAttr && "no backlink from semantic attribute");

      if (scratchpad.argFound = parsedAttr->getNumArgs() != 0; scratchpad.argFound) {
        scratchpad.ArgExprs.push_back(parsedAttr->getArg(0));
      } else {
        scratchpad.ArgExprs.clear();
      }
      auto * fetchedAttribute = scratchpad.attributes.addNew(
        const_cast<IdentifierInfo*>(parsedAttr->getAttrName()),
        val->getRange(),
        nullptr,
        val->getLoc(),
        scratchpad.ArgExprs.data(), scratchpad.argFound,
        parsedAttr->getForm()
      );
      return SetAndSucceed(Result, makeReflection(fetchedAttribute));
    }
    case ReflectionKind::Declaration: {
      ValueDecl *D = RV.getReflectedDecl();

      if (!D) {
        // FIXME how would we end up here ?
        return DiagnoseReflectionKind(Diagnoser, Range, "attribute, type or declaration",
                                    DescriptionOf(RV));
      }

      auto attrs = D->attrs();
      if (attrs.empty()) {
        return SetAndSucceed(Result, Sentinel);
      }

      std::vector<Attr * const *> cxx11Attrs;
      // poor man ::filter, copy_if, etc....
      for (Attr *const *attr = attrs.begin(); attr != attrs.end(); ++attr) {
        if ((*attr)->isCXX11Attribute()) {
          cxx11Attrs.push_back(attr);
        }
      }

      if (idx >= cxx11Attrs.size()) {
        return SetAndSucceed(Result, Sentinel);
      }

      // Attr -> ParsedAttr
      Attr * const val = *cxx11Attrs[idx];
      assert(val);
      static AttributeScratchpad scratchpad;

      const ParsedAttr * parsedAttr = val->fromParsedAttr();
      assert(parsedAttr && "no backlink from semantic attribute");

      scratchpad.ArgExprs.clear();
      if (scratchpad.argFound = parsedAttr->getNumArgs() != 0; scratchpad.argFound) {
        scratchpad.ArgExprs.push_back(parsedAttr->getArg(0));
      }

      auto * fetchedAttribute = scratchpad.attributes.addNew(
        const_cast<IdentifierInfo*>(parsedAttr->getAttrName()),
        val->getRange(),
        nullptr,
        val->getLoc(),
        scratchpad.ArgExprs.data(), scratchpad.argFound,
        parsedAttr->getForm()
      );
      return SetAndSucceed(Result, makeReflection(fetchedAttribute));

    }
    case ReflectionKind::Null:
    case ReflectionKind::Template:
    case ReflectionKind::Object:
    case ReflectionKind::Value:
    case ReflectionKind::Namespace:
    case ReflectionKind::BaseSpecifier:
    case ReflectionKind::DataMemberSpec:
    case ReflectionKind::Annotation:
      return DiagnoseReflectionKind(Diagnoser, Range, "declaration or attribute",
                                    DescriptionOf(RV));
  }
  llvm_unreachable("unknown reflection kind");
  return false;
}

bool appertain(APValue &Result, ASTContext &C,
               MetaActions &Meta, EvalFn Evaluator,
               DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range,
               ArrayRef<Expr *> Args, Decl *ContainingDecl) {

  assert(ResultTy == C.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());

  APValue Appertainee;
  if (!Evaluator(Appertainee, Args[0], true))
    return true;

  APValue Value;
  if (!Evaluator(Value, Args[1], true))
    return true;

  if (!AllowInjection)
    return Diagnoser(Range.getBegin(),
                      diag::metafn_injected_decl_non_plainly_consteval);


  if (!Value.isReflectedAttribute()) {
    Diagnoser(Range.getBegin(), diag::metafn_p3385_appertain)
      << DescriptionOf(Value) << Range;
    return true;
  }

  ArgsVector ArgExprs;
  switch (Appertainee.getReflectionKind()) {
    case ReflectionKind::Type: {
      Decl *D = findTypeDecl(Appertainee.getReflectedType());
      ParsedAttr* recoveredAttr = Value.getReflectedAttribute();
      Meta.Appertain(D->getMostRecentDecl(), recoveredAttr, Range.getBegin());

      return SetAndSucceed(Result, makeReflection(D));
    }
    case ReflectionKind::Declaration: {
      Decl *D = Appertainee.getReflectedDecl();
      ParsedAttr* recoveredAttr = Value.getReflectedAttribute();
      Meta.Appertain(D->getMostRecentDecl(), recoveredAttr, Range.getBegin());

      return SetAndSucceed(Result, makeReflection(D));
    default:
      Diagnoser(Range.getBegin(), diag::metafn_p3385_appertain)
          << DescriptionOf(Appertainee) << Range;
      return false;
  }
  llvm_unreachable("unknown reflection kind");
  return true;
}

bool get_begin_enumerator_decl_of(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation: {
    return DiagnoseReflectionKind(Diagnoser, Range, "an enum type",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_enumerator_decl_of(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation: {
    llvm_unreachable("should have failed in 'get_begin_enumerator_decl_of'");
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_base_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);

    Decl *typeDecl = findTypeDecl(QT);

    if (auto cxxRecordDecl = dyn_cast_or_null<CXXRecordDecl>(typeDecl)) {
      Meta.EnsureInstantiated(typeDecl, Range);
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
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "a class type",
                                  DescriptionOf(RV));
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_template_argument_of(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromType(RV.getReflectedType(), TArgs))
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");

    APValue R = getNthTemplateArgument(C, TArgs, Evaluator, Sentinel, idx);
    if (R.isReflectedDecl())
      R = APValue(APValue::LValueBase{R.getReflectedDecl()}, CharUnits::Zero(),
                  {}, false, false).Lift(QualType{});
    return SetAndSucceed(Result, R);
  }
  case ReflectionKind::Declaration: {
    SmallVector<TemplateArgument, 4> TArgs;
    if (getTemplateArgumentsFromDecl(RV.getReflectedDecl(), TArgs))
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a template specialization");
    APValue R = getNthTemplateArgument(C, TArgs, Evaluator, Sentinel, idx);
    if (R.isReflectedDecl())
      R = APValue(APValue::LValueBase{R.getReflectedDecl()}, CharUnits::Zero(),
                  {}, false, false).Lift(QualType{});
    return SetAndSucceed(Result, R);
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "a template specialization",
                                  DescriptionOf(RV));
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_begin_member_decl_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl) {
  assert(ResultTy == C.MetaInfoTy);

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

    ensureDeclared(C, QT, Range.getBegin());
    Decl *typeDecl = findTypeDecl(QT);
    if (!typeDecl)
      return true;

    if (!Meta.EnsureInstantiated(typeDecl, Range))
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
      Meta.EnsureDeclarationOfImplicitMembers(CXXRD);

    DeclContext *declContext = dyn_cast<DeclContext>(typeDecl);
    assert(declContext && "no DeclContext?");

    Decl* beginMember = findIterableMember(Meta, C, *declContext->decls_begin(),
                                           true);
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

    Decl *beginMember = findIterableMember(Meta, C, *DC->decls_begin(), true);
    if (!beginMember)
      return SetAndSucceed(Result, Sentinel);
    return SetAndSucceed(Result,
                         APValue(ReflectionKind::Declaration, beginMember));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Declaration:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Template:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_next_member_decl_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl) {
  assert(ResultTy == C.MetaInfoTy);

  assert(Args[0]->getType()->isReflectionType());
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  assert(Args[1]->getType()->isReflectionType());

  APValue Sentinel;
  if (!Evaluator(Sentinel, Args[1], true))
    return true;
  assert(Sentinel.isReflectedType());

  if (Decl *Next = findIterableMember(Meta, C, RV.getReflectedDecl(), false))
    return SetAndSucceed(Result, APValue(ReflectionKind::Declaration, Next));
  return SetAndSucceed(Result, Sentinel);
}

bool is_structural_type(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  auto result = false;
  if (RV.isReflectedType()) {
    const QualType QT = RV.getReflectedType();
    const Type* T = QT.getTypePtr();

    result = T->isStructuralType();
  }

  return SetAndSucceed(Result, makeBool(C, result));
}

bool map_decl_to_entity(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(ResultTy == C.MetaInfoTy);
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  Decl *D = RV.getReflectedDecl();

  if (auto *TyDecl = dyn_cast<TypeDecl>(D)) {
    QualType QT = C.getTypeDeclType(TyDecl);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  return SetAndSucceed(Result, makeReflection(D));
}

bool identifier_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
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

  RV = MaybeUnproxy(C, RV, /*Dealias=*/false);

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
    getDeclName(Name, C, RV.getReflectedNamespace());
    break;
  }
  case ReflectionKind::Attribute: {
    AttributeCommonInfo* attr = RV.getReflectedAttribute();
    Name = attr->getAttrName()->getName();
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
  case ReflectionKind::Annotation:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_have_name)
        << DescriptionOf(RV) << Range;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  if (Name.empty())
    return Diagnoser(Range.getBegin(), diag::metafn_anonymous_entity)
        << DescriptionOf(RV) << Range;

  Expr *StrLit = makeStrLiteral(Name, C, IsUtf8);

  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result,
                       APValue(StrLit, CharUnits::Zero(), Path, false));
}

bool has_identifier(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  RV = MaybeUnproxy(C, RV, /*Dealias=*/false);

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
    else if (isa<VarTemplateSpecializationDecl>(D))
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
  case ReflectionKind::Attribute: {
    // FIXME deal with ^^ [[ ]]
    HasIdentifier = true;
    break;
  }
  case ReflectionKind::Null:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Annotation:
    break;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }

  return SetAndSucceed(Result, makeBool(C, HasIdentifier));
}

bool operator_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.getSizeType());

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

  RV = MaybeUnproxy(C, RV);

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

  return SetAndSucceed(Result,
                       APValue(C.MakeIntValue(OperatorId, C.getSizeType())));
}

bool source_location_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    return findTypeDeclLoc(Result, C, Evaluator, ResultTy,
                           RV.getReflectedType());
  case ReflectionKind::Declaration:
    return findDeclLoc(Result, C, Evaluator, ResultTy, RV.getReflectedDecl());
  case ReflectionKind::Template: {
    TemplateName TName = RV.getReflectedTemplate();
    return findDeclLoc(Result, C, Evaluator, ResultTy,
                       TName.getAsTemplateDecl());
  }
  case ReflectionKind::Namespace:
    return findDeclLoc(Result, C, Evaluator, ResultTy,
                       RV.getReflectedNamespace());
  case ReflectionKind::EntityProxy:
    return findDeclLoc(Result, C, Evaluator, ResultTy,
                       RV.getReflectedEntityProxy());
  case ReflectionKind::BaseSpecifier:
    return findBaseSpecLoc(Result, C, Evaluator, ResultTy,
                           RV.getReflectedBaseSpecifier());
  case ReflectionKind::Annotation:
    return findAnnotLoc(Result, C, Evaluator, ResultTy,
                        RV.getReflectedAnnotation());
  case ReflectionKind::Attribute:
    return findAttrLoc(Result, C, Evaluator, ResultTy,
                        RV.getReflectedAttribute());
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Null:
  case ReflectionKind::DataMemberSpec:
    return findDeclLoc(Result, C, Evaluator, ResultTy, nullptr);
  }
  llvm_unreachable("unknown reflection kind");
}

bool type_of(APValue &Result, ASTContext &C, MetaActions &Meta,
             EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Attribute:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
    return Diagnoser(Range.getBegin(), diag::metafn_no_associated_property)
        << DescriptionOf(RV) << 0 << Range;
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    QualType QT = desugarType(RV.getTypeOfReflectedResult(C),
                              /*UnwrapAliases=*/true, /*DropCV=*/false,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    if (isa<CXXConstructorDecl, CXXDestructorDecl, BindingDecl>(VD))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 0 << DescriptionOf(RV) << Range;

    bool DropCV = isa<ParmVarDecl>(VD);
    QualType QT = desugarType(VD->getType(),
                              /*UnwrapAliases=*/ true, DropCV,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::BaseSpecifier: {
    QualType QT = RV.getReflectedBaseSpecifier()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::DataMemberSpec:
  {
    QualType QT = RV.getReflectedDataMemberSpec()->Ty;
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Annotation: {
    QualType QT = RV.getReflectedAnnotation()->getArg()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/true,
                     /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool parent_of(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
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
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
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
  case ReflectionKind::EntityProxy:
    return DiagWrapper(parentOf(Result, RV.getReflectedEntityProxy()));
  case ReflectionKind::BaseSpecifier: {
    CXXRecordDecl *RD = RV.getReflectedBaseSpecifier()->getDerived();
    QualType QT = desugarType(QualType(RD->getTypeForDecl(), 0),
                              /*UnwrapAliases=*/true, /*DropCV=*/false,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool underlying_entity_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
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
  case ReflectionKind::EntityProxy:
    return SetAndSucceed(Result, MaybeUnproxy(C, RV));
  }
  llvm_unreachable("unknown reflection kind");
}

bool proxied_entity_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser,bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::Namespace:
  case ReflectionKind::Template:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
    return DiagnoseReflectionKind(Diagnoser, Range, "an entity proxy");
  case ReflectionKind::EntityProxy:
    return SetAndSucceed(Result, MaybeUnproxy(C, RV, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool object_of(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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

    Meta.EnsureInstantiated(VD, Args[0]->getSourceRange());

    QualType QT = VD->getType();
    if (auto *LVRT = dyn_cast<LValueReferenceType>(QT)) {
      QT = LVRT->getPointeeType();
    }

    Expr *Synthesized = DeclRefExpr::Create(C,
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
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Template:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 1 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}


bool constant_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Value:
    return SetAndSucceed(Result, RV);
  case ReflectionKind::Object: {
    if (!RV.getTypeOfReflectedResult(C)->isStructuralType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object of non-structural type" << Range;

    QualType ObjectTy = RV.getTypeOfReflectedResult(C);
    Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(), ObjectTy, VK_LValue);
    Expr *CE = ConstantExpr::Create(C, OVE, RV.getReflectedObject());

    Expr::EvalResult ER;
    if (!CE->EvaluateAsRValue(ER, C, true))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object not usable in constant expressions" << Range;

    APValue Constant = ER.Val;
    QualType ConstantTy = ComputeResultType(RV.getTypeOfReflectedResult(C),
                                            Constant);
    if (ConstantTy->isRecordType()) {
      auto *TPO = C.getTemplateParamObjectDecl(ConstantTy, Constant);
      Constant = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                    false);
      ConstantTy = QualType{};
    }
    return SetAndSucceed(Result, Constant.Lift(ConstantTy));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();

    APValue Constant;
    QualType QT;
    if (auto *VD = dyn_cast<VarDecl>(Decl)) {
      if (!VD->isUsableInConstantExpressions(C))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "a variable not usable in constant expressions" << Range;

      QT = VD->getType();
      if (auto *LVRT = dyn_cast<LValueReferenceType>(QT))
        QT = LVRT->getPointeeType();

      Expr *Synthesized = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                              SourceLocation(), VD, false,
                                              Range.getBegin(), QT,
                                              VK_LValue, Decl, nullptr);
      if (!Evaluator(Constant, Synthesized, true))
        llvm_unreachable("failed to evaluate variable usable in constant "
                         "expressions");
    } else if (isa<EnumConstantDecl>(Decl)) {
      Expr *Synthesized = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                              SourceLocation(), Decl, false,
                                              Range.getBegin(), Decl->getType(),
                                              VK_PRValue, Decl, nullptr);
      QT = Synthesized->getType();

      Expr::EvalResult ER;
      if (!Synthesized->EvaluateAsConstantExpr(ER, C))
        llvm_unreachable("failed to evaluate enumerator constant");
      Constant = ER.Val;
    } else if (auto *TPOD = dyn_cast<TemplateParamObjectDecl>(Decl)) {
      Constant = TPOD->getValue();
      QT = TPOD->getType();
    } else {
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << DescriptionOf(RV) << Range;
    }

    QualType ConstantTy = ComputeResultType(QT, Constant);
    if (ConstantTy->isRecordType()) {
      auto *TPO = C.getTemplateParamObjectDecl(ConstantTy, Constant);
      Constant = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                    false);
      ConstantTy = QualType{};
    }

    return SetAndSucceed(Result, Constant.Lift(ConstantTy));
  }
  case ReflectionKind::Annotation: {
    CXX26AnnotationAttr *A = RV.getReflectedAnnotation();
    APValue Value = RV.getReflectedAnnotation()->getValue();

    QualType Ty = desugarType(A->getArg()->getType(), /*UnwrapAliases=*/true,
                              /*DropCV=*/true, /*DropRefs=*/false);
    return SetAndSucceed(Result, A->getValue().Lift(Ty));
  }
  case ReflectionKind::Attribute: // TODO P3385 anything to do ?
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 2 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool template_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "a template specialization",
                                  DescriptionOf(RV));
    return true;
  }
  llvm_unreachable("unknown reflection kind");
}

static bool CanActAsTemplateArg(const APValue &RV) {
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
    return true;
  case ReflectionKind::Declaration:
    return (!isa<FieldDecl>(RV.getReflectedDecl()));
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    return isa<ClassTemplateDecl, TypeAliasTemplateDecl>(TDecl);
  }
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
  case ReflectionKind::Null:
    return false;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("expected proxies to have been unwrapped before calling");
  }
  llvm_unreachable("unknown reflection kind");
}

static TemplateArgument TArgFromReflection(ASTContext &C, EvalFn Evaluator,
                                           const APValue &RV,
                                           SourceLocation Loc) {
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    return RV.getReflectedType().getCanonicalType();
  case ReflectionKind::Object: {
    QualType RefTy = C.getLValueReferenceType(RV.getTypeOfReflectedResult(C));
    return TemplateArgument(C, RefTy, RV.getReflectedObject(), false);
  }
  case ReflectionKind::Value: {
    APValue Lowered = RV.getReflectedValue();
    QualType ResultTy = RV.getTypeOfReflectedResult(C);
    if (Lowered.isInt()) {
      return TemplateArgument(C, Lowered.getInt(), ResultTy.getCanonicalType());
    }
    TemplateArgument TArg(C, ResultTy, Lowered, false);
    return TArg;
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();
    if (Decl->isInvalidDecl())
      break;

    QualType QT = desugarType(Decl->getType(), /*UnwrapAliases=*/ false,
                              /*DropCV=*/false, /*DropRefs=*/true);

    // Don't worry about the cost of creating an expression here: The template
    // substitution machinery will otherwise create one from the argument
    // anyway, so we aren't really losing any efficiency here.
    Expr *Synthesized =
        DeclRefExpr::Create(C, NestedNameSpecifierLoc(), SourceLocation(), Decl,
                            false, Loc, QT, VK_LValue, Decl, nullptr);

    return TemplateArgument(Synthesized);
  }
  case ReflectionKind::Template:
    return TemplateArgument(RV.getReflectedTemplate());
    break;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("expected proxies to have been unwrapped before calling");
  default:
    llvm_unreachable("unimplemented for template argument kind");
  }
  return TemplateArgument();
}

bool substitute(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
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

  APValue DiagnoseAPV;
  if (!Evaluator(DiagnoseAPV, Args[3], true))
    return true;
  bool NoDiagnose = !DiagnoseAPV.getInt().getBoolValue();
  auto ElideDiagnosis = [&] {
    return SetAndSucceed(Result, makeReflection(nullptr));
  };

  SmallVector<TemplateArgument, 4> TArgs;
  {
    // Evaluate how many template arguments were provided.
    APValue NumArgs;
    if (!Evaluator(NumArgs, Args[2], true))
      return true;
    size_t nArgs = NumArgs.getInt().getExtValue();
    TArgs.reserve(nArgs);

    for (uint64_t k = 0; k < nArgs; ++k) {
      llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(),
                                                 Args[1]->getExprLoc());

      Synthesized = new (C) ArraySubscriptExpr(Args[1], Synthesized,
                                               C.MetaInfoTy, VK_LValue,
                                               OK_Ordinary, Range.getBegin());
      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return true;

      APValue Unwrapped;
      if (!Evaluator(Unwrapped, Synthesized, true) ||
          !Unwrapped.isReflection())
        return true;
      Unwrapped = MaybeUnproxy(C, Unwrapped);
      if (!CanActAsTemplateArg(Unwrapped))
        return NoDiagnose ? ElideDiagnosis() :
               Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
                 << DescriptionOf(Unwrapped) << 1 << Range;

      TemplateArgument TArg = TArgFromReflection(C, Evaluator, Unwrapped,
                                                 Range.getBegin());
      if (TArg.isNull())
        return true;
      TArgs.push_back(TArg);
    }
  }

  SmallVector<TemplateArgument, 4> ExpandedTArgs;
  expandTemplateArgPacks(TArgs, ExpandedTArgs);

  // Lookup cached specialization; if found, return it.
  llvm::FoldingSetNodeID ID;
  {
    ID.AddPointer(TDecl);
    for (const TemplateArgument &TArg : ExpandedTArgs)
      TArg.Profile(ID, C);
  }
  unsigned SubstitutionHash = ID.ComputeHash();
  if (C.checkCachedSubstitution(SubstitutionHash, &Result))
    return false;

  if (!Meta.CheckTemplateArgumentList(TDecl, ExpandedTArgs, NoDiagnose,
                                      Args[0]->getExprLoc()))
    return NoDiagnose ? ElideDiagnosis() : true;
  for (const auto &TArg : ExpandedTArgs)
    if (TArg.getKind() == TemplateArgument::Expression &&
        TArg.getAsExpr()->containsErrors())
      return true;

  if (auto *CTD = dyn_cast<ClassTemplateDecl>(TDecl)) {
    void *InsertPos;
    ClassTemplateSpecializationDecl *TSpecDecl =
          CTD->findSpecialization(ExpandedTArgs, InsertPos);

    if (!TSpecDecl) {
      TSpecDecl = ClassTemplateSpecializationDecl::Create(
            C, CTD->getTemplatedDecl()->getTagKind(),
            CTD->getDeclContext(), Range.getBegin(), Range.getBegin(),
            CTD, ExpandedTArgs, false, nullptr);
      CTD->AddSpecialization(TSpecDecl, InsertPos);
    }
    assert(TSpecDecl);

    APValue RV(ReflectionKind::Type,
               const_cast<Type *>(TSpecDecl->getTypeForDecl()));
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, RV);
  } else if (auto *TATD = dyn_cast<TypeAliasTemplateDecl>(TDecl)) {
    TArgs.clear();
    expandTemplateArgPacks(ExpandedTArgs, TArgs);

    QualType QT = Meta.Substitute(TATD, TArgs, Range.getBegin());
    assert(!QT.isNull() && "substitution failed after validating arguments?");

    APValue RV = makeReflection(QT);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, makeReflection(QT));
  } else if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl)) {
    FunctionDecl *Spec = Meta.Substitute(FTD, ExpandedTArgs, Range.getBegin());
    assert(Spec && "substitution failed after validating arguments?");

    if (Spec->getReturnType()->isUndeducedType())
      return NoDiagnose ? ElideDiagnosis() :
             Diagnoser(Range.getBegin(), diag::metafn_undeduced_placeholder)
               << Spec << Spec->getType() << Range;

    APValue RV = makeReflection(Spec);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, RV);
  } else if (auto *VTD = dyn_cast<VarTemplateDecl>(TDecl)) {
    TArgs.clear();
    expandTemplateArgPacks(ExpandedTArgs, TArgs);

    VarDecl *Spec = Meta.Substitute(VTD, TArgs, Range.getBegin());
    assert(Spec && "substitution failed after validating arguments?");

    APValue RV = makeReflection(Spec);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, makeReflection(Spec));
  } else if (auto *CD = dyn_cast<ConceptDecl>(TDecl)) {
    TArgs.clear();
    expandTemplateArgPacks(ExpandedTArgs, TArgs);

    Expr *Spec = Meta.Substitute(CD, TArgs, Range.getBegin());
    assert(Spec && "substitution failed after validating arguments?");

    APValue SatisfiesConcept;
    if (!Evaluator(SatisfiesConcept, Spec, true))
      llvm_unreachable("failed to evaluate substituted concept");

    APValue RV = SatisfiesConcept.Lift(C.BoolTy);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, SatisfiesConcept.Lift(C.BoolTy));
  }
  llvm_unreachable("unimplemented for template kind");
}


bool extract(APValue &Result, ASTContext &C, MetaActions &Meta,
             EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());

  bool ReturnsLValue = false;
  QualType RawResultTy = ResultTy;
  if (auto *LVRT = dyn_cast<LValueReferenceType>(ResultTy)) {
    ReturnsLValue = true;
    ResultTy = LVRT->getPointeeType();
  }

  auto extractLambda = [&](APValue &Out, CXXRecordDecl *RD) -> bool {
    if (!RD->isCapturelessLambda())
      return true;

    CXXMethodDecl *CallOp = RD->getLambdaStaticInvoker();
    QualType LambdaPtrTy = C.getPointerType(CallOp->getType());

    if (LambdaPtrTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 0 << QualType(RD->getTypeForDecl(), 0) << 0 << ResultTy << Range;

    // If not already done, generate a fake body for the call-operator.
    // The real body is generated during CodeGen.
    if (!CallOp->hasBody()) {
      CallOp->markUsed(C);
      CallOp->setReferenced();
      CallOp->setBody(new (C) CompoundStmt(Range.getBegin()));
    }

    APValue CallOpLV(CallOp, CharUnits::Zero(), {}, false, false);
    return SetAndSucceed(Out, CallOpLV);
  };

  APValue RV;
  if (!Evaluator(RV, Args[1], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Object: {
    QualType ObjectTy = RV.getTypeOfReflectedResult(C);

    if (auto *RD = ObjectTy->getAsCXXRecordDecl();
        RD && RD->isLambda() && ResultTy->isPointerType())
      return extractLambda(Result, RD);

    if (ObjectTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 1 << ObjectTy << ReturnsLValue << ResultTy << Range;

    Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(), ObjectTy, VK_LValue);
    Expr *CE = ConstantExpr::Create(C, OVE, RV.getReflectedObject());

    if (!Evaluator(RV, CE, !ReturnsLValue))
      return true;

    return SetAndSucceed(Result, RV);
  }
  case ReflectionKind::Value: {
    QualType ValueTy = RV.getTypeOfReflectedResult(C);
    if (ReturnsLValue)
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
          << 1 << DescriptionOf(RV) << Range;

    if (ValueTy.getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 0 << ValueTy << ReturnsLValue << ResultTy << Range;

    return SetAndSucceed(Result, RV.getReflectedValue());
  }
  case ReflectionKind::Annotation: {
    if (ReturnsLValue)
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
          << 1 << DescriptionOf(RV) << Range;

    CXX26AnnotationAttr *A = RV.getReflectedAnnotation();
    if (auto *RD = A->getArg()->getType()->getAsCXXRecordDecl();
        RD && RD->isLambda() && ResultTy->isPointerType())
      return extractLambda(Result, RD);

    if (A->getArg()->getType().getCanonicalType().getTypePtr() !=
        ResultTy.getCanonicalType().getTypePtr())
      return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
          << 3 << A->getArg()->getType() << ReturnsLValue << ResultTy << Range;

    return SetAndSucceed(Result, A->getValue());
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();
    Meta.EnsureInstantiated(Decl, Args[1]->getSourceRange());

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
          TypeSourceInfo *TSI = C.CreateTypeSourceInfo(
                  QualType(ParentClsDecl->getTypeForDecl(), 0), 0);
          NNSLocBuilder.Extend(C, Range.getBegin(), TSI->getTypeLoc(),
                               Range.getBegin());
        }
        Synthesized = DeclRefExpr::Create(C, NNSLocBuilder.getTemporary(),
                                          SourceLocation(), Decl, false,
                                          Range.getBegin(), ResultTy, VK_LValue,
                                          Decl, nullptr);
      } else if (auto *ArrTy = dyn_cast<ArrayType>(Decl->getType())) {
        QualType Elt = ArrTy->getElementType();
        if (auto *VD = dyn_cast<VarDecl>(Decl)) {
          if (VD->isConstexpr()) {
            Elt.addConst();
          }
        }

        ReturnsLValue = true;
        if (!RawResultTy->isPointerType() || !RawResultTy->getPointeeType().isAtLeastAsQualifiedAs(Elt, C))
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 1 << C.getPointerType(Elt) << 1 << ResultTy << Range;

        NestedNameSpecifierLocBuilder NNSLocBuilder;
        if (auto *ParentClsDecl = dyn_cast_or_null<CXXRecordDecl>(
                Decl->getDeclContext())) {
          TypeSourceInfo *TSI = C.CreateTypeSourceInfo(
                  QualType(ParentClsDecl->getTypeForDecl(), 0), 0);
          NNSLocBuilder.Extend(C, Range.getBegin(), TSI->getTypeLoc(),
                               Range.getBegin());
        }

        APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
        return SetAndSucceed(Result,
                             APValue(Decl, CharUnits::Zero(), Path, false));
      } else {
        // We have a reflection of a (possibly local) non-reference variable.
        // Synthesize an lvalue by reaching up the call stack.
        if (ResultTy.getCanonicalType().getTypePtr() !=
            Decl->getType().getCanonicalType().getTypePtr())
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;

        Synthesized = ExtractLValueExpr::Create(C, Range, ResultTy, Decl);
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

      QualType MemPtrTy = C.getMemberPointerType(
              Decl->getType(), nullptr,
              cast<CXXRecordDecl>(Decl->getDeclContext()));
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
      QualType FnPtrTy = C.getPointerType(Decl->getType());
      if (FnPtrTy.getCanonicalType().getTypePtr() !=
          ResultTy.getCanonicalType().getTypePtr())
        return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
            << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;

      return SetAndSucceed(Result, APValue(Decl, CharUnits::Zero(),
                           {}, false, false));
    }
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Attribute:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
        << (ReturnsLValue ? 1 : 0) << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("invalid reflection type");
}

template <AccessSpecifier Specifier>
bool is_ACCESS(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool HasTargetAccess = false;
    if (const Decl *D = findTypeDecl(RV.getReflectedType()))
      HasTargetAccess = (D->getAccess() == Specifier);

    return SetAndSucceed(Result, makeBool(C, HasTargetAccess));
  }
  case ReflectionKind::Declaration: {
    bool HasTargetAccess = (RV.getReflectedDecl()->getAccess() == Specifier);
    return SetAndSucceed(Result, makeBool(C, HasTargetAccess));
  }
  case ReflectionKind::EntityProxy: {
    bool HasTargetAccess = (RV.getReflectedEntityProxy()->getAccess() ==
                            Specifier);
    return SetAndSucceed(Result, makeBool(C, HasTargetAccess));
  }
  case ReflectionKind::Template: {
    const Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool HasTargetAccess = (D->getAccess() == Specifier);
    return SetAndSucceed(Result, makeBool(C, HasTargetAccess));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool HasTargetAccess = (Base->getAccessSpecifier() == Specifier);
    return SetAndSucceed(Result, makeBool(C, HasTargetAccess));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Namespace:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_public(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  return is_ACCESS<AS_public>(Result, C, Meta, Evaluator, Diagnoser,
                              AllowInjection, ResultTy, Range, Args,
                              ContainingDecl);
}

bool is_protected(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  return is_ACCESS<AS_protected>(Result, C, Meta, Evaluator, Diagnoser,
                                 AllowInjection, ResultTy, Range, Args,
                                 ContainingDecl);
}

bool is_private(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  return is_ACCESS<AS_private>(Result, C, Meta, Evaluator, Diagnoser,
                               AllowInjection, ResultTy, Range, Args,
                               ContainingDecl);
}

bool is_virtual(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsVirtual = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      IsVirtual = MD->isVirtual();
    return SetAndSucceed(Result, makeBool(C, IsVirtual));
  }
  case ReflectionKind::BaseSpecifier: {
    IsVirtual = RV.getReflectedBaseSpecifier()->isVirtual();
    return SetAndSucceed(Result, makeBool(C, IsVirtual));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, IsVirtual));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_pure_virtual(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsPureVirtual = false;
  if (RV.isReflectedDecl())
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsPureVirtual = FD->isPureVirtual();

  return SetAndSucceed(Result, makeBool(C, IsPureVirtual));
}

bool is_override(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsOverride = false;
  if (RV.isReflectedDecl())
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      IsOverride = MD->size_overridden_methods() > 0;

  return SetAndSucceed(Result, makeBool(C, IsOverride));
}

bool is_deleted(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsDeleted = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsDeleted = FD->isDeleted();

  return SetAndSucceed(Result, makeBool(C, IsDeleted));
}

bool is_defaulted(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsDefaulted = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsDefaulted = FD->getMostRecentDecl()->isDefaulted();

  return SetAndSucceed(Result, makeBool(C, IsDefaulted));
}

bool is_explicit(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsExplicit = false;
  if (RV.isReflectedDecl()) {
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      IsExplicit = CtorD->getExplicitSpecifier().isExplicit();
    else if (auto *ConvD = dyn_cast<CXXConversionDecl>(RV.getReflectedDecl()))
      IsExplicit = ConvD->getExplicitSpecifier().isExplicit();
  }

  return SetAndSucceed(Result, makeBool(C, IsExplicit));
}

bool is_noexcept(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsNoexcept = false;
  if (RV.isReflectedType())
    IsNoexcept = isFunctionOrMethodNoexcept(RV.getReflectedType());
  else if (RV.isReflectedDecl())
    IsNoexcept = isFunctionOrMethodNoexcept(RV.getReflectedDecl()->getType());

  return SetAndSucceed(Result, makeBool(C, IsNoexcept));
}

bool is_bit_field(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl()))
      result = FD->isBitField();
    else if (const auto *BD = dyn_cast<BindingDecl>(RV.getReflectedDecl()))
      result = BD->getBinding()->refersToBitField();
  } else if (RV.isReflectedDataMemberSpec()) {
    result = RV.getReflectedDataMemberSpec()->BitWidth.has_value();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_enumerator(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<EnumConstantDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_const(APValue &Result, ASTContext &C, MetaActions &Meta,
              EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
              Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  case ReflectionKind::Type: {
    bool result = isConstQualifiedType(RV.getReflectedType());

    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;
    if (!isa<ParmVarDecl>(RV.getReflectedDecl()))
      result = isConstQualifiedType(RV.getReflectedDecl()->getType());

    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    bool result = isConstQualifiedType(RV.getTypeOfReflectedResult(C));

    return SetAndSucceed(Result, makeBool(C, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_volatile(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
    return SetAndSucceed(Result, makeBool(C, false));
  case ReflectionKind::Type: {
    bool result = isVolatileQualifiedType(RV.getReflectedType());

    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;
    if (!isa<ParmVarDecl>(RV.getReflectedDecl()))
      result = isVolatileQualifiedType(RV.getReflectedDecl()->getType());

    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    bool result = isVolatileQualifiedType(RV.getTypeOfReflectedResult(C));

    return SetAndSucceed(Result, makeBool(C, result));
  }
  }
  llvm_unreachable("invalid reflection type");
}

bool is_mutable_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsMutableMember = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl()))
      IsMutableMember = FD->isMutable();

  return SetAndSucceed(Result, makeBool(C, IsMutableMember));
}

bool is_lvalue_reference_qualified(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args,
                                   Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_rvalue_reference_qualified(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args,
                                   Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_static_storage_duration(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_thread_storage_duration(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, bool AllowInjection,
                                 QualType ResultTy, SourceRange Range,
                                 ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (const auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Thread;

  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_automatic_storage_duration(APValue &Result, ASTContext &C,
                                    MetaActions &Meta, EvalFn Evaluator,
                                    DiagFn Diagnoser, bool AllowInjection,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args,
                                    Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (const auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
      result = VD->getStorageDuration() == SD_Automatic;

  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_internal_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_module_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_external_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_class_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue Scratch;
  bool result = false;

  decltype(Diagnoser) SwallowDiags {};
  if (!parent_of(Scratch, C, Meta, Evaluator, SwallowDiags, AllowInjection,
                 C.MetaInfoTy, Range, Args, ContainingDecl)) {
    assert(Scratch.isReflection());
    result = Scratch.isReflectedType() &&
             Scratch.getReflectedType()->isRecordType();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_namespace_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue Scratch;
  bool result = false;

  decltype(Diagnoser) SwallowDiags {};
  if (!parent_of(Scratch, C, Meta, Evaluator, SwallowDiags, AllowInjection,
                 C.MetaInfoTy, Range, Args, ContainingDecl)) {
    assert(Scratch.isReflection());
    result = Scratch.isReflectedNamespace();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_nonstatic_data_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                              EvalFn Evaluator, DiagFn Diagnoser,
                              bool AllowInjection, QualType ResultTy,
                              SourceRange Range, ArrayRef<Expr *> Args,
                              Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_static_member(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
    return SetAndSucceed(Result, makeBool(C, result));
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
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, result));
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_base(APValue &Result, ASTContext &C, MetaActions &Meta,
             EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedBaseSpecifier()));
}

bool is_data_member_spec(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedDataMemberSpec()));
}

bool is_namespace(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedNamespace()));
}

bool is_attribute(APValue &Result, ASTContext &C,
                  MetaActions &Meta, EvalFn Evaluator,
                  DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range,
                  ArrayRef<Expr *> Args, Decl *ContainingDecl){
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
  return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedAttribute()));
}

bool is_function(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<const FunctionDecl>(RV.getReflectedDecl());
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_variable(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<const VarDecl>(RV.getReflectedDecl());
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_type(APValue &Result, ASTContext &C, MetaActions &Meta,
             EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedType()));
}

bool is_alias(APValue &Result, ASTContext &C, MetaActions &Meta,
              EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
              Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    bool result = isTypeAlias(RV.getReflectedType());
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Namespace: {
    bool result = isa<NamespaceAliasDecl>(RV.getReflectedNamespace());
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    bool result = isa<TypeAliasTemplateDecl>(TDecl);
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_entity_proxy(APValue &Result, ASTContext &C, MetaActions &Meta,
                     EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                     QualType ResultTy, SourceRange Range,
                     ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedEntityProxy()));
}

bool is_complete_type(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedType()) {
    // If this is a declared type with a reachable definition, ensure that the
    // type is instantiated.
    if (Decl *typeDecl = findTypeDecl(RV.getReflectedType()))
      (void) Meta.EnsureInstantiated(typeDecl, Range);

    result = !RV.getReflectedType()->isIncompleteType();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_complete_definition(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    if (Decl *typeDecl = findTypeDecl(RV.getReflectedType())) {
      if (auto *TD = dyn_cast<TagDecl>(typeDecl))
        result = (TD->getDefinition() != nullptr &&
                  !TD->getDefinition()->isBeingDefined());
    }
    break;
  case ReflectionKind::Declaration: {
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      result = (FD->getDefinition() != nullptr &&
                FD->getDefinition()->hasBody());
    break;
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    break;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_enumerable_type(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type:
    if (Decl *typeDecl = findTypeDecl(RV.getReflectedType())) {
      if (auto *TD = dyn_cast<TagDecl>(typeDecl)) {
        (void) Meta.EnsureInstantiated(TD, Range);
        result = (TD->getDefinition() != nullptr &&
                  !TD->getDefinition()->isBeingDefined());
      }
    }
    break;
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Declaration:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    break;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range,
                 ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedTemplate()));
}

bool is_function_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsFnTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsFnTemplate = isa<FunctionTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(C, IsFnTemplate));
}

bool is_variable_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsVarTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsVarTemplate = isa<VarTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(C, IsVarTemplate));
}

bool is_class_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsClsTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsClsTemplate = isa<ClassTemplateDecl>(TD);
  }
  return SetAndSucceed(Result, makeBool(C, IsClsTemplate));
}

bool is_alias_template(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsAliasTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    IsAliasTemplate = TD->isTypeAlias();
  }
  return SetAndSucceed(Result, makeBool(C, IsAliasTemplate));
}

bool is_conversion_function_template(APValue &Result, ASTContext &C,
                                     MetaActions &Meta, EvalFn Evaluator,
                                     DiagFn Diagnoser, bool AllowInjection,
                                     QualType ResultTy, SourceRange Range,
                                     ArrayRef<Expr *> Args,
                                     Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConversionTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsConversionTemplate = isa<CXXConversionDecl>(FTD->getTemplatedDecl());
  }
  return SetAndSucceed(Result, makeBool(C, IsConversionTemplate));
}

bool is_operator_function_template(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args,
                                   Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, IsOperatorTemplate));
}

bool is_literal_operator_template(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  return SetAndSucceed(Result, makeBool(C, IsLiteralOperator));
}

bool is_constructor_template(APValue &Result, ASTContext &C,
                             MetaActions &Meta, EvalFn Evaluator,
                             DiagFn Diagnoser, bool AllowInjection,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsCtorTemplate = false;
  if (RV.isReflectedTemplate()) {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      IsCtorTemplate = isa<CXXConstructorDecl>(FTD->getTemplatedDecl());
  }
  return SetAndSucceed(Result, makeBool(C, IsCtorTemplate));
}

bool is_concept(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConcept = false;
  if (RV.isReflectedTemplate())
    IsConcept = isa<ConceptDecl>(RV.getReflectedTemplate().getAsTemplateDecl());

  return SetAndSucceed(Result, makeBool(C, IsConcept));
}

bool is_structured_binding(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    result = isa<BindingDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_value(APValue &Result, ASTContext &C, MetaActions &Meta,
              EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
              Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedValue()));
}

bool is_object(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsObject = RV.isReflectedObject();
  if (RV.isReflectedDecl())
    IsObject = isa<TemplateParamObjectDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(C, IsObject));
}

bool has_template_arguments(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    bool result = isTemplateSpecialization(QT);
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Declaration: {
    bool result = false;

    Decl *D = RV.getReflectedDecl();
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      result = (FD->getTemplateSpecializationArgs() != nullptr);
    else if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D))
      result = VTSD->getTemplateArgs().size() > 0;

    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_default_member_initializer(APValue &Result, ASTContext &C,
                                    MetaActions &Meta, EvalFn Evaluator,
                                    DiagFn Diagnoser, bool AllowInjection,
                                    QualType ResultTy, SourceRange Range,
                                    ArrayRef<Expr *> Args,
                                    Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool HasInitializer = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl()))
      HasInitializer = FD->hasInClassInitializer();

  return SetAndSucceed(Result, makeBool(C, HasInitializer));
}

bool is_conversion_function(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsConversion = false;
  if (RV.isReflectedDecl())
    IsConversion = isa<CXXConversionDecl>(RV.getReflectedDecl());

  return SetAndSucceed(Result, makeBool(C, IsConversion));
}

bool is_operator_function(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsOperator = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsOperator = (FD->getOverloadedOperator() != OO_None);

  return SetAndSucceed(Result, makeBool(C, IsOperator));
}

bool is_literal_operator(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsLiteralOperator = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsLiteralOperator = FD->getDeclName().getNameKind() ==
                          DeclarationName::CXXLiteralOperatorName;

  return SetAndSucceed(Result, makeBool(C, IsLiteralOperator));
}

bool is_constructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
    return SetAndSucceed(Result, makeBool(C, false));
  case ReflectionKind::Declaration: {
    bool result = isa<CXXConstructorDecl>(RV.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("invalid reflection type");
}

bool is_default_constructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isDefaultConstructor();

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_copy_constructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isCopyConstructor();

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_move_constructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *CtorD = dyn_cast<CXXConstructorDecl>(RV.getReflectedDecl()))
      result = CtorD->isMoveConstructor();

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_assignment(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      result = (FD->getOverloadedOperator() == OO_Equal);

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_copy_assignment(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      result = MD->isCopyAssignmentOperator();

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_move_assignment(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl())
    if (auto *MD = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl()))
      result = MD->isMoveAssignmentOperator();

  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_destructor(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
    return SetAndSucceed(Result, makeBool(C, false));
  case ReflectionKind::Declaration: {
    bool result = isa<CXXDestructorDecl>(RV.getReflectedDecl());
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("invalid reflection type");
}

bool is_special_member_function(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
    return SetAndSucceed(Result, makeBool(C, false));
  case ReflectionKind::Declaration: {
    bool IsSpecial = false;
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      IsSpecial = isSpecialMember(FD);

    return SetAndSucceed(Result, makeBool(C, IsSpecial));
  }
  case ReflectionKind::Template: {
    bool result = false;
    TemplateDecl *TDecl = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl))
      result = isSpecialMember(FTD->getTemplatedDecl());
    return SetAndSucceed(Result, makeBool(C, result));
  }
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("invalid reflection type");
}

bool is_user_provided(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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

  return SetAndSucceed(Result, makeBool(C, IsUserProvided));
}

bool is_user_declared(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool IsUserDeclared = false;
  if (RV.isReflectedDecl())
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl())) {
      FD = cast<FunctionDecl>(FD->getFirstDecl());
      IsUserDeclared = !(FD->isImplicit());
    }

  return SetAndSucceed(Result, makeBool(C, IsUserDeclared));
}

bool reflect_result(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
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

  // Construct an expression whose result is 'Arg', and evaluate it to check if
  // it's an allowed result of a constant template argument.
  //
  // This is just a hack to get 'CheckConstantExpression' in ExprConstant.cpp
  // called on 'Arg', to diagnose cases like string literals and temporaries
  // that aren't allowed in template arguments.
  //
  // The expression is constructed in three layers:
  // - A ConstantExpr to hold 'Arg'
  // - An OpaqueValueExpr to act as the ConstantExpr's subexpression (we can
  //   otherwise ICE when e.g., checking source location of the ConstantExpr)
  // - An OpaqueValueExpr wrapper around the ConstantExpr to prevent
  //   EvaluateAsConstantExpr from grabbing 'Arg' and short-circuiting the
  //   evaluation (and, more imporantly, the result validation).
  Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(), Args[1]->getType(),
                                      IsLValue ? VK_LValue : VK_PRValue);
  {
    Expr *CE = ConstantExpr::Create(C, OVE, Arg);
    OVE = new (C) OpaqueValueExpr(Range.getBegin(), Args[1]->getType(),
                                  CE->getValueKind(), OK_Ordinary, CE);
  }
  {
    Expr::EvalResult Discarded;

    ConstantExprKind CEKind = (OVE->getType()->isRecordType() && !IsLValue) ?
                              ConstantExprKind::ClassTemplateArgument :
                              ConstantExprKind::NonClassTemplateArgument;
    if (!OVE->EvaluateAsConstantExpr(Discarded, C, CEKind))
      return Diagnoser(Range.getBegin(), diag::metafn_result_not_representable)
          << (IsLValue ? 1 : 0) << Range;
  }

  // If this is an lvalue to a function, promote the result to reflect
  // the declaration.
  if (OVE->getType()->isFunctionType() && Arg.isLValue() &&
      Arg.getLValueOffset().isZero())
    if (!Arg.hasLValuePath() || Arg.getLValuePath().size() == 0)
      if (APValue::LValueBase LVBase = Arg.getLValueBase();
          LVBase.is<const ValueDecl *>())
        return SetAndSucceed(
            Result,
            makeReflection(
                const_cast<ValueDecl *>(LVBase.get<const ValueDecl *>())));

  QualType ReflTy = ArgTy.getReflectedType();
  if (!IsLValue && ReflTy->isRecordType()) {
    auto *TPO = C.getTemplateParamObjectDecl(ReflTy, Arg);
    Arg = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                  false);
    ReflTy = QualType{};
  }

  return SetAndSucceed(Result, Arg.Lift(ReflTy));
}

bool data_member_spec(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
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
      llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(),
                                                 Args[ArgIdx]->getExprLoc());

      Synthesized = new (C) ArraySubscriptExpr(Args[ArgIdx], Synthesized,
                                               CharTy, VK_LValue, OK_Ordinary,
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
    Lexer Lex(Range.getBegin(), C.getLangOpts(), Name->data(), Name->data(),
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

  TagDataMemberSpec *TDMS = new (C) TagDataMemberSpec {
    MemberTy, Name, Alignment, BitWidth, NoUniqueAddress
  };
  return SetAndSucceed(Result, makeReflection(TDMS));
}

bool define_aggregate(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true))
    return true;
  if (!Scratch.isReflectedType())
    return DiagnoseReflectionKind(Diagnoser, Range, "a class type",
                                  DescriptionOf(Scratch));

  QualType ToComplete = Scratch.getReflectedType();
  if (!ToComplete->isRecordType())
    return DiagnoseReflectionKind(Diagnoser, Range, "a class type",
                                  DescriptionOf(Scratch));

  // Evaluate the number of members provided.
  if (!Evaluator(Scratch, Args[1], true))
    return true;
  size_t NumMembers = static_cast<size_t>(Scratch.getInt().getExtValue());

  SmallVector<TagDataMemberSpec *, 4> MemberSpecs;
  llvm::FoldingSetNodeID ID;
  llvm::StringSet<> MemberNames;
  for (size_t k = 0; k < NumMembers; ++k) {
    // Extract the reflection from the list of member specs.
    llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
    Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(),
                                               Args[2]->getExprLoc());

    Synthesized = new (C) ArraySubscriptExpr(Args[2], Synthesized, C.MetaInfoTy,
                                             VK_LValue, OK_Ordinary,
                                             Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
      return true;

    if (!Evaluator(Scratch, Synthesized, true))
      return true;
    if (!Scratch.isReflectedDataMemberSpec())
      return DiagnoseReflectionKind(Diagnoser, Range,
                                    "a description of a data member",
                                    DescriptionOf(Scratch));
    MemberSpecs.push_back(Scratch.getReflectedDataMemberSpec());
    Scratch.Profile(ID);

    if (MemberSpecs.back()->Name &&
        !MemberNames.insert(*MemberSpecs.back()->Name).second)
      return Diagnoser(Range.getBegin(), diag::metafn_duplicate_member_names)
          << *MemberSpecs.back()->Name << Range;
  }
  unsigned MemberSpecHash = ID.ComputeHash();

  CXXRecordDecl *IncompleteDecl;
  {
    NamedDecl *ND;
    if (!ToComplete->isIncompleteType(&ND)) {
      // NOTE: Uncomment following lines for 'define_aggregate' idempotency.
      /*unsigned PriorHash;
      if (C.checkClassMemberSpecHash(ToComplete, PriorHash) &&
          MemberSpecHash == PriorHash)
        return SetAndSucceed(Result, makeReflection(ToComplete));
      else*/
        return Diagnoser(Range.getBegin(), diag::metafn_already_complete_type)
          << ToComplete << Range;
    }
    IncompleteDecl = cast<CXXRecordDecl>(ND);
  }

  if (!AllowInjection)
    return Diagnoser(Range.getBegin(),
                     diag::metafn_injected_decl_non_plainly_consteval);

  CXXRecordDecl *Definition = Meta.DefineAggregate(IncompleteDecl, MemberSpecs,
                                                   ContainingDecl,
                                                   Range.getBegin());
  if (!Definition)
    return true;

  C.recordClassMemberSpecHash(ToComplete, MemberSpecHash);
  return SetAndSucceed(Result, makeReflection(ToComplete));
}

bool offset_of(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[1], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  case ReflectionKind::Declaration: {
    if (const FieldDecl *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(C, FD) / C.getTypeSize(C.CharTy);
      return SetAndSucceed(Result, APValue(C.MakeIntValue(Offset, ResultTy)));
    }
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    if (Base->isVirtual() && Base->getDerived()->isAbstract())
      return Diagnoser(Range.getBegin(),
                       diag::metafn_offset_virtual_base_of_abstract)
          << Range;

    size_t Offset = getOffsetOfBase(C, Base);
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Offset, ResultTy)));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool size_of(APValue &Result, ASTContext &C, MetaActions &Meta,
             EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
             QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
             Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();

    NamedDecl *typeDecl = findTypeDecl(RV.getReflectedType());
    if (typeDecl)
      Meta.EnsureInstantiated(typeDecl, Range);

    if (QT->isIncompleteType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
          << 4 << 0 << Range;

    size_t Sz = C.getTypeSizeInChars(QT).getQuantity();
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    QualType QT = RV.getTypeOfReflectedResult(C);
    size_t Sz = C.getTypeSizeInChars(QT).getQuantity();
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *VD = RV.getReflectedDecl();
    size_t Sz = C.getTypeSizeInChars(VD->getType()).getQuantity();
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    size_t Sz = C.getTypeSizeInChars(TDMS->Ty).getQuantity();
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 3 << DescriptionOf(RV);
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_offset_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[1], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  case ReflectionKind::Declaration: {
    if (FieldDecl *FD = dyn_cast<FieldDecl>(RV.getReflectedDecl())) {
      size_t Offset = getBitOffsetOfField(C, FD) % C.getTypeSize(C.CharTy);
      return SetAndSucceed(Result, APValue(C.MakeIntValue(Offset, ResultTy)));
    }
    return DiagnoseReflectionKind(Diagnoser, Range, "a non-static data member",
                                  DescriptionOf(RV));
  }
  case ReflectionKind::BaseSpecifier:
    return SetAndSucceed(Result, APValue(C.MakeIntValue(0, ResultTy)));
  }
  llvm_unreachable("unknown reflection kind");
}

bool bit_size_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();

    NamedDecl *typeDecl = findTypeDecl(RV.getReflectedType());
    if (typeDecl)
      Meta.EnsureInstantiated(typeDecl, Range);

    if (QT->isIncompleteType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
          << 4 << 0 << Range;

    size_t Sz = C.getTypeSize(QT);
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    size_t Sz = C.getTypeSize(RV.getTypeOfReflectedResult(C));
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    const ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    size_t Sz = C.getTypeSize(VD->getType());

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD))
      if (FD->isBitField())
        Sz = FD->getBitWidthValue();

    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();

    size_t Sz = TDMS->BitWidth.value_or(C.getTypeSize(TDMS->Ty));
    return SetAndSucceed(Result, APValue(C.MakeIntValue(Sz, C.getSizeType())));
  }

  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 3 << DescriptionOf(RV);
  }
  llvm_unreachable("unknown reflection kind");
}

bool alignment_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.getSizeType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    QualType QT = RV.getReflectedType();
    if (QT->isIncompleteType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
          << 3 << 0 << Range;

    size_t Align = C.getTypeAlignInChars(QT).getQuantity();
    return SetAndSucceed(Result,
                         APValue(C.MakeIntValue(Align, C.getSizeType())));
  }
  case ReflectionKind::Object:
  case ReflectionKind::Value: {
    QualType QT = RV.getTypeOfReflectedResult(C);
    size_t Align = C.getTypeAlignInChars(QT).getQuantity();
    return SetAndSucceed(Result,
                         APValue(C.MakeIntValue(Align, C.getSizeType())));
  }
  case ReflectionKind::Declaration: {
    const ValueDecl *VD = cast<ValueDecl>(RV.getReflectedDecl());
    size_t Align = C.getTypeAlignInChars(VD->getType()).getQuantity();

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD)) {
      if (FD->isBitField())
        return true;
    }
    Align = C.getDeclAlign(VD, true).getQuantity();

    return SetAndSucceed(Result,
                         APValue(C.MakeIntValue(Align, C.getSizeType())));
  }
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    if (TDMS->BitWidth)
      return true;

    size_t Align = TDMS->Alignment.value_or(
          C.getTypeAlignInChars(TDMS->Ty).getQuantity());

    return SetAndSucceed(Result,
                         APValue(C.MakeIntValue(Align, C.getSizeType())));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 4 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_parameter_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

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
        << 2 << 2 << Range;
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
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return true;
  }
  return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
}

bool has_consistent_identifier(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration:
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl())) {
      [[maybe_unused]] std::string Unused;
      bool Consistent = getParameterName(PVD, Unused);

      return SetAndSucceed(Result, makeBool(C, Consistent));
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
  case ReflectionKind::Annotation:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Attribute:
    return has_identifier(Result, C, Meta, Evaluator, Diagnoser, AllowInjection,
                          ResultTy, Range, Args, ContainingDecl);
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_ellipsis_parameter(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
  case ReflectionKind::Type:
    if (auto *FPT = dyn_cast<FunctionProtoType>(RV.getReflectedType())) {
      bool HasEllipsis = FPT->isVariadic();
      return SetAndSucceed(Result, makeBool(C, HasEllipsis));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 2 << 2;
  case ReflectionKind::Declaration: {
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl())) {
      bool HasEllipsis = FD->getEllipsisLoc().isValid();
      return SetAndSucceed(Result, makeBool(C, HasEllipsis));
    }
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool has_default_argument(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Declaration: {
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl())) {
      PVD = getMostRecentParmVarDecl(PVD);
      return SetAndSucceed(Result, makeBool(C, PVD->hasDefaultArg()));
    }
    [[fallthrough]];
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "a function parameter",
                                  DescriptionOf(RV));
  }
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_explicit_object_parameter(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(RV.getReflectedDecl()))
      result = PVD->isExplicitObjectParameter();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool is_function_parameter(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  bool result = false;
  if (RV.isReflectedDecl()) {
    result = isa<const ParmVarDecl>(RV.getReflectedDecl());
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

bool return_type_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    if (auto *FPT = dyn_cast<FunctionProtoType>(RV.getReflectedType())) {
      QualType QT =
          desugarType(FPT->getReturnType(), /*UnwrapAliases=*/ true,
                      /*DropCV=*/false, /*DropRefs=*/false);
      return SetAndSucceed(Result, makeReflection(QT));
    }

    return Diagnoser(Range.getBegin(), diag::metafn_cannot_introspect_type)
        << 3 << 2 << Range;
  }
  case ReflectionKind::Declaration:
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl());
        FD && !isa<CXXConstructorDecl>(FD) && !isa<CXXDestructorDecl>(FD)) {
      QualType QT =
          desugarType(FD->getReturnType(), /*UnwrapAliases=*/ true,
                      /*DropCV=*/false, /*DropRefs=*/false);
      return SetAndSucceed(Result, makeReflection(QT));
    }
    [[fallthrough]];
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 6 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool get_ith_annotation_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, QualType ResultTy,
                           SourceRange Range, ArrayRef<Expr *> Args,
                           Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  auto findAnnotation = [&](Decl *D, size_t idx, APValue Sentinel) {
    D = D ? D->getMostRecentDecl() : D;

    while (D) {
      auto Annots = D->attrs();
      for (auto It = Annots.begin(); It != Annots.end(); ++It)
        if (isa<CXX26AnnotationAttr>(*It))
          if (idx-- == 0)
            return makeReflection(dyn_cast<CXX26AnnotationAttr>(*It));
      D = D->getPreviousDecl();
    }
    return Sentinel;
  };

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
    NamedDecl *typeDecl = findTypeDecl(RV.getReflectedType());
    if (typeDecl)
      Meta.EnsureInstantiated(typeDecl, Range);

    return SetAndSucceed(Result, findAnnotation(typeDecl, idx, Sentinel));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *VD = RV.getReflectedDecl();

    return SetAndSucceed(Result, findAnnotation(VD, idx, Sentinel));
  }
  case ReflectionKind::Namespace: {
    Decl *D = RV.getReflectedNamespace();

    return SetAndSucceed(Result, findAnnotation(D, idx, Sentinel));
  }
  case ReflectionKind::EntityProxy: {
    Decl *D = RV.getReflectedEntityProxy()->getIntroducer();

    return SetAndSucceed(Result, findAnnotation(D, idx, Sentinel));
  }
  // Disallow reflecting annotations of unspecialized templates, as they might
  // contain a dependent name.
  case ReflectionKind::Template: /*{
    Decl *D = RV.getReflectedTemplate().getAsTemplateDecl()->getTemplatedDecl();

    return SetAndSucceed(Result, findAnnotation(D, idx, Sentinel));
  }*/
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 7 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool is_annotation(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedAnnotation()));
}

bool annotate(APValue &Result, ASTContext &C, MetaActions &Meta,
              EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
              QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
              Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  APValue Appertainee;
  if (!Evaluator(Appertainee, Args[0], true))
    return true;

  APValue Value;
  if (!Evaluator(Value, Args[1], true) || !Value.isReflectedValue())
    return true;

  if (!AllowInjection)
    return Diagnoser(Range.getBegin(),
                     diag::metafn_injected_decl_non_plainly_consteval);

  switch (Appertainee.getReflectionKind()) {
  case ReflectionKind::Type: {
    Decl *D = findTypeDecl(Appertainee.getReflectedType());
    if (auto *Annot = Meta.Annotate(D->getMostRecentDecl(), Value,
                                    ContainingDecl, Range.getBegin()))
      return SetAndSucceed(Result, makeReflection(Annot));
    return true;
  }
  case ReflectionKind::Declaration: {
    Decl *D = Appertainee.getReflectedDecl();
    if (!isa<VarDecl, FunctionDecl>(D))
      return true;

    if (auto *Annot = Meta.Annotate(D->getMostRecentDecl(), Value,
                                    ContainingDecl, Range.getBegin()))
      return SetAndSucceed(Result, makeReflection(Annot));
    return true;
  }
  case ReflectionKind::Namespace: {
    Decl *D = Appertainee.getReflectedNamespace();
    if (auto *Annot = Meta.Annotate(D->getMostRecentDecl(), Value,
                                    ContainingDecl, Range.getBegin()))
      return SetAndSucceed(Result, makeReflection(Annot));
    return true;
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Template:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_annotate)
        << DescriptionOf(Appertainee) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool current_access_context(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(ResultTy == C.MetaInfoTy);
  Decl *Ctx = nullptr;

  StackLocationExpr *SLE = StackLocationExpr::Create(C, SourceRange(), 1);
  if (!Evaluator(Result, SLE, true) || !Result.isReflectedDecl())
    return true;
  else if (Ctx = Result.getReflectedDecl(); !Ctx)
    Ctx = Meta.CurrentCtx();

  if (auto *Ctor = dyn_cast<CXXConstructorDecl>(Ctx);
      Ctor && Ctor->isInheritingConstructor())
    Ctx = cast<Decl>(Ctor->getDeclContext());

  if (auto *RD = dyn_cast<CXXRecordDecl>(Ctx))
    return SetAndSucceed(Result,
                         makeReflection(QualType(RD->getTypeForDecl(), 0)));
  return SetAndSucceed(Result, makeReflection(Ctx));
}

bool is_accessible(APValue &Result, ASTContext &C, MetaActions &Meta,
                   EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                   QualType ResultTy, SourceRange Range,
                   ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(Args[1]->getType()->isReflectionType());
  assert(Args[2]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue Scratch;
  if (!Evaluator(Scratch, Args[1], true) || !Scratch.isReflection())
    return true;

  bool UnconditionalAccess = false;

  DeclContext *AccessDC = nullptr;
  switch (Scratch.getReflectionKind()) {
  case ReflectionKind::Null:
    UnconditionalAccess = true;
    break;
  case ReflectionKind::Type:
    AccessDC = dyn_cast_or_null<DeclContext>(
        findTypeDecl(Scratch.getReflectedType()));
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

  CXXRecordDecl *NamingCls = nullptr;
  if (!Evaluator(Scratch, Args[2], true) || !Scratch.isReflection())
    return true;
  Scratch = MaybeUnproxy(C, Scratch);
  assert(Scratch.isNullReflection() || Scratch.isReflectedType());
  if (Scratch.isReflectedType()) {
    NamingCls = cast<CXXRecordDecl>(findTypeDecl(Scratch.getReflectedType()));

    Meta.EnsureInstantiated(NamingCls, Range);
    NamingCls = NamingCls->getDefinition();

    if (!NamingCls)
      return true;  // TODO(P2996): Diagnostic for naming class.
  }

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  auto validate = [&](Decl *D, CXXRecordDecl *&NamingCls) -> bool {
    auto *DC = dyn_cast<CXXRecordDecl>(D->getNonTransparentDeclContext());
    if (!NamingCls)
      NamingCls = DC;

    if (DC && DC->isBeingDefined())
      return Diagnoser(Range.getBegin(),
                       diag::metafn_access_query_class_being_defined)
          << DC << Range;
    return false;
  };

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    NamedDecl *D = findTypeDecl(RV.getReflectedType());
    if (validate(D, NamingCls))
      return true;
    else if (!NamingCls)
      return SetAndSucceed(Result, makeBool(C, true));

    bool Accessible = UnconditionalAccess ||
                      Meta.IsAccessible(D, AccessDC, NamingCls);
    return SetAndSucceed(Result, makeBool(C, Accessible));
  }
  case ReflectionKind::Declaration: {
    ValueDecl *D = RV.getReflectedDecl();
    if (validate(D, NamingCls))
      return true;
    else if (!NamingCls)
      return SetAndSucceed(Result, makeBool(C, true));

    bool Accessible = UnconditionalAccess ||
                      Meta.IsAccessible(RV.getReflectedDecl(), AccessDC,
                                        NamingCls);
    return SetAndSucceed(Result, makeBool(C, Accessible));
  }
  case ReflectionKind::Template: {
    TemplateDecl *D = RV.getReflectedTemplate().getAsTemplateDecl();
    if (validate(D, NamingCls))
      return true;
    else if (!NamingCls)
      return SetAndSucceed(Result, makeBool(C, true));

    bool Accessible = UnconditionalAccess ||
                      Meta.IsAccessible(D, AccessDC, NamingCls);
    return SetAndSucceed(Result, makeBool(C, Accessible));
  }
  case ReflectionKind::EntityProxy: {
    UsingShadowDecl *USD = RV.getReflectedEntityProxy();
    if (validate(USD, NamingCls))
      return true;
    else if (!NamingCls)
      return SetAndSucceed(Result, makeBool(C, true));

    bool Accessible = UnconditionalAccess ||
                      Meta.IsAccessible(USD, AccessDC, NamingCls);
    return SetAndSucceed(Result, makeBool(C, Accessible));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *BaseSpec = RV.getReflectedBaseSpecifier();

    auto *Base = findTypeDecl(BaseSpec->getType());
    assert(Base && "base class has no type declaration?");

    QualType BaseTy = BaseSpec->getType();

    CXXRecordDecl *DerivedDecl = BaseSpec->getDerived();
    if (DerivedDecl->isBeingDefined())
      return Diagnoser(Range.getBegin(),
                       diag::metafn_access_query_class_being_defined)
          << DerivedDecl << Range;
    QualType DerivedTy(BaseSpec->getDerived()->getTypeForDecl(), 0);

    CXXBasePathElement bpe = { BaseSpec, BaseSpec->getDerived(), 0 };
    CXXBasePath path;
    path.push_back(bpe);
    path.Access = BaseSpec->getAccessSpecifier();

    bool Accessible = UnconditionalAccess ||
                      Meta.IsAccessibleBase(BaseTy, DerivedTy, path, AccessDC,
                                            Range.getBegin());
    return SetAndSucceed(Result, makeBool(C, Accessible));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("invalid reflection type");
}


bool is_access_specified(APValue &Result, ASTContext &C, MetaActions &Meta,
                         EvalFn Evaluator, DiagFn Diagnoser,
                         bool AllowInjection, QualType ResultTy,
                         SourceRange Range, ArrayRef<Expr *> Args,
                         Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

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

    return SetAndSucceed(Result, makeBool(C, IsSpecified));
  }
  case ReflectionKind::Declaration: {
    bool IsSpecified = findAccessSpec(RV.getReflectedDecl()) != AS_none;
    return SetAndSucceed(Result, makeBool(C, IsSpecified));
  }
  case ReflectionKind::Template: {
    Decl *D = RV.getReflectedTemplate().getAsTemplateDecl();

    bool IsSpecified = findAccessSpec(D) != AS_none;
    return SetAndSucceed(Result, makeBool(C, IsSpecified));
  }
  case ReflectionKind::EntityProxy: {
    Decl *D = RV.getReflectedEntityProxy()->getIntroducer();

    bool IsSpecified = findAccessSpec(D) != AS_none;
    return SetAndSucceed(Result, makeBool(C, IsSpecified));
  }
  case ReflectionKind::BaseSpecifier: {
    CXXBaseSpecifier *Base = RV.getReflectedBaseSpecifier();
    bool IsSpecified = (Base->getAccessSpecifierAsWritten() != AS_none);
    return SetAndSucceed(Result, makeBool(C, IsSpecified));
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Namespace:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("invalid reflection type");
}

bool is_nonstatic_member_function(ValueDecl *FD) {
  if (!FD) {
    return false;
  }

  if (dyn_cast<CXXConstructorDecl>(FD)) {
    return false;
  }

  auto *MD = dyn_cast<CXXMethodDecl>(FD);
  if (!MD) {
    // might be a pointer to member function
    QualType QT = FD->getType();
    // check if the type is a pointer to a member
    if (const MemberPointerType *MPT = QT->getAs<MemberPointerType>()) {
      QualType PT = MPT->getPointeeType();
      // check if the pointee type is a function type
      if (PT->getAs<FunctionProtoType>()) {
        return true;
      }
    }
  } else {
    return !MD->isStatic();
  }

  return false;
}

CXXMethodDecl *getCXXMethodDeclFromDeclRefExpr(DeclRefExpr *DRE,
                                               ASTContext &C) {
  ValueDecl *VD = DRE->getDecl();

  if (auto *MD = dyn_cast<CXXMethodDecl>(VD)) {
    // method declaration
    return MD;
  } else {
    // pointer to non-static method
    // validation was done in is_nonstatic_member_function
    Expr::EvalResult ER;
    if (!DRE->EvaluateAsRValue(ER, C)) {
      return nullptr;
    }

    APValue Result = ER.Val;
    if (!Result.isMemberPointer()) {
      return nullptr;
    }

    const ValueDecl *MemberDecl = Result.getMemberPointerDecl();
    if (const CXXMethodDecl *MethodDecl = dyn_cast<CXXMethodDecl>(MemberDecl)) {
      // get non-const version
      return const_cast<CXXMethodDecl *>(MethodDecl);
    }
  }

  return nullptr;
}

bool reflect_invoke(APValue &Result, ASTContext &C, MetaActions &Meta,
                    EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
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
      llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
      Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(),
                                                 DataExpr->getExprLoc());

      Synthesized = new (C) ArraySubscriptExpr(DataExpr, Synthesized,
                                               C.MetaInfoTy, VK_LValue,
                                               OK_Ordinary, Range.getBegin());

      if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
        return false;

      if (!Evaluator(Scratch, Synthesized, true) || !Scratch.isReflection())
        return false;
      Scratch = MaybeUnproxy(C, Scratch);
      Out.push_back(Scratch);
    }

    return true;
  };

  APValue FnRefl;
  if (!Evaluator(FnRefl, Args[0], true))
    return true;
  FnRefl = MaybeUnproxy(C, FnRefl);

  SmallVector<TemplateArgument, 4> ExplicitTArgs;
  {
    SmallVector<APValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[1], Args[2]))
      llvm_unreachable("failed to unpack template arguments from vector?");

    if (Reflections.size() > 0 && !FnRefl.isReflectedTemplate())
      return DiagnoseReflectionKind(Diagnoser, Range, "a template",
                                    DescriptionOf(FnRefl));

    SmallVector<TemplateArgument, 4> TArgs;
    for (APValue RV : Reflections) {
      if (!CanActAsTemplateArg(RV))
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
            << DescriptionOf(RV) << 1 << Range;

      TemplateArgument TArg = TArgFromReflection(C, Evaluator, RV,
                                                 Range.getBegin());
      if (TArg.isNull())
        return true;
      TArgs.push_back(TArg);
    }

    expandTemplateArgPacks(TArgs, ExplicitTArgs);
  }

  SmallVector<Expr *, 4> ArgExprs;
  {
    SmallVector<APValue, 4> Reflections;
    if (!UnpackReflectionsIntoVector(Reflections, Args[3], Args[4]))
      llvm_unreachable("failed to unpack function arguments from vector?");

    for (APValue RV : Reflections) {
      if (RV.isReflectedObject()) {
        Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(),
                                            RV.getTypeOfReflectedResult(C),
                                            VK_LValue);
        Expr *CE = ConstantExpr::Create(C, OVE, RV.getReflectedObject());
        ArgExprs.push_back(CE);
      } else if (RV.isReflectedValue()) {
        Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(),
                                            RV.getTypeOfReflectedResult(C),
                                            VK_PRValue);
        Expr *CE = ConstantExpr::Create(C, OVE, RV.getReflectedValue());
        ArgExprs.push_back(CE);
      } else if (RV.isReflectedDecl()) {
        ValueDecl *D = RV.getReflectedDecl();
        ArgExprs.push_back(
              DeclRefExpr::Create(C, NestedNameSpecifierLoc(), SourceLocation(),
                                  D, false, Range.getBegin(), D->getType(),
                                  VK_LValue, D, nullptr));
      } else {
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_be_arg)
            << DescriptionOf(RV) << 0 << Range;
      }
    }
  }

  Expr *FnRefExpr = nullptr;
  switch (FnRefl.getReflectionKind()) {
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Namespace:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_invoke)
        << DescriptionOf(FnRefl) << Range;
  case ReflectionKind::Object: {
    Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(),
                                        FnRefl.getTypeOfReflectedResult(C),
                                        VK_LValue);
    FnRefExpr = ConstantExpr::Create(C, OVE, FnRefl.getReflectedObject());
    break;
  }
  case ReflectionKind::Value: {
    Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(),
                                        FnRefl.getTypeOfReflectedResult(C),
                                        VK_PRValue);
    FnRefExpr = ConstantExpr::Create(C, OVE, FnRefl.getReflectedValue());
    break;
  }
  case ReflectionKind::Declaration: {
    ValueDecl *D = FnRefl.getReflectedDecl();
    Meta.EnsureInstantiated(D, Range);

    FnRefExpr =
          DeclRefExpr::Create(C, NestedNameSpecifierLoc(), SourceLocation(), D,
                              false, Range.getBegin(), D->getType(), VK_LValue,
                              D, nullptr);
    break;
  }
  case ReflectionKind::Template: {
    TemplateDecl *TDecl = FnRefl.getReflectedTemplate().getAsTemplateDecl();
    auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl);
    if (!FTD) {
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_invoke)
          << DescriptionOf(FnRefl) << Range;
    }

    FunctionDecl *Spec;
    {
      bool exclude_first_arg =
          is_nonstatic_member_function(FTD->getTemplatedDecl()) &&
          ArgExprs.size() > 0;

      SmallVector<TemplateArgument, 4> ExpandedTArgs;
      expandTemplateArgPacks(ExplicitTArgs, ExpandedTArgs);

      ArrayRef ArgView(ArgExprs.begin() + (exclude_first_arg ? 1 : 0),
                       ArgExprs.end());

      Spec = Meta.DeduceSpecialization(FTD, ExpandedTArgs, ArgView,
                                       Range.getBegin());
      if (!Spec)
        return Diagnoser(Range.getBegin(), diag::metafn_no_specialization_found)
            << FTD << Range;

      Meta.EnsureInstantiated(Spec, Range);
    }

    FnRefExpr = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                    SourceLocation(), Spec, false,
                                    Range.getBegin(), Spec->getType(),
                                    VK_LValue, Spec, nullptr);
    break;
  }
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }

  Expr* CallExpr;
  {
    auto *DRE = dyn_cast<DeclRefExpr>(FnRefExpr);
    if (DRE && dyn_cast<CXXConstructorDecl>(DRE->getDecl())) {
      CallExpr = Meta.SynthesizeCallExpr(DRE, ArgExprs);
    } else {
      Expr *FnExpr = FnRefExpr;
      bool handle_member_func =
          DRE && is_nonstatic_member_function(DRE->getDecl());

      if (handle_member_func) {
        if (ArgExprs.size() < 1)
          // need to have object as a first argument
          return Diagnoser(Range.getBegin(),
                           diag::metafn_first_argument_is_not_object)
                 << Range;

        Expr *ObjExpr = ArgExprs[0];
        QualType ObjType = ObjExpr->getType();

        if (ObjType->isPointerType()) {
          ObjType = ObjType->getPointeeType();
          // Convert pointer to rvalue (if needed).
          APValue Val;
          if (!Evaluator(Val, ObjExpr, true))
            return true;

          ObjExpr = new (C) OpaqueValueExpr(Range.getBegin(),
                                            ObjExpr->getType(), VK_PRValue);
          ObjExpr = ConstantExpr::Create(C, ObjExpr, Val);
        }

        if (!ObjType->getAsCXXRecordDecl()) {
          // first argument is not an object
          return Diagnoser(Range.getBegin(),
                           diag::metafn_first_argument_is_not_object)
                 << Range;
        }

        CXXMethodDecl *MD = getCXXMethodDeclFromDeclRefExpr(DRE, C);
        if (!MD) {
          // most likely, non-constexpr pointer to method was passed
          return true;
        }

        APValue ReflMD = makeReflection(MD);
        CXXReflectExpr *ReflMDExpr =
            CXXReflectExpr::Create(C, Range.getBegin(), Range, ReflMD);

        auto ObjClass = ObjType->getAsCXXRecordDecl();
        // check that method belongs to class
        bool IsMethodFromClassOrParent = (MD->getParent() == ObjClass) ||
                                       ObjClass->isDerivedFrom(MD->getParent());
        if (!IsMethodFromClassOrParent) {
          return Diagnoser(Range.getBegin(),
                           diag::metafn_function_is_not_member_of_object)
                 << Range;
        }

        if (MD->getReturnType()->isVoidType()) {
          // void return type is not supported
          return Diagnoser(Range.getBegin(), diag::metafn_function_returns_void)
                 << Range;
        }

        FnExpr = Meta.SynthesizeDirectMemberAccess(ObjExpr, ReflMDExpr,
                                                   Range.getBegin());
        if (!FnExpr)
          return true;
      }

      MutableArrayRef<Expr *> ArgView(
            ArgExprs.begin() + (handle_member_func ? 1 : 0), ArgExprs.end());
      CallExpr = Meta.SynthesizeCallExpr(FnExpr, ArgView);
    }
  }

  if (!CallExpr)
    return Diagnoser(Range.getBegin(), diag::metafn_invalid_call_expr) << Range;

  if (CallExpr->isTypeDependent() || CallExpr->isValueDependent())
    return true;

  if (!CallExpr->getType()->isStructuralType() && !CallExpr->isLValue())
    return Diagnoser(Range.getBegin(), diag::metafn_returns_non_structural_type)
        << CallExpr->getType() << Range;

  Expr::EvalResult EvalResult;
  if (!CallExpr->EvaluateAsConstantExpr(EvalResult, C))
    return Diagnoser(Range.getBegin(),
                     diag::metafn_invocation_not_constant_expr)
        << Range;

  // If this is an lvalue to a function, promote the result to reflect
  // the declaration.
  if (CallExpr->getType()->isFunctionType() &&
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

  return SetAndSucceed(Result, EvalResult.Val.Lift(CallExpr->getType()));
}

}  // end namespace clang
