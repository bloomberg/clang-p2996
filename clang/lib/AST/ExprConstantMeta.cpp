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

#include "AttributeScratchpad.h"
#include "clang/AST/APValue.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Attrs.inc"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Metafunction.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Reflection.h"
#include "clang/AST/Type.h"
#include "clang/Basic/AttributeCommonInfo.h"
#include "clang/Basic/DiagnosticMetafn.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/Lexer.h"
#include "clang/Sema/ParsedAttr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>


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

static bool is_final(APValue &Result, ASTContext &C, MetaActions &Meta,
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

static bool enumerator_spec(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool is_enumerator_spec(APValue &Result, ASTContext &C,
                                MetaActions &Meta, EvalFn Evaluator,
                                DiagFn Diagnoser, bool AllowInjection,
                                QualType ResultTy, SourceRange Range,
                                ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool define_aggregate(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool define_enum(APValue &Result, ASTContext &C, MetaActions &Meta,
                             EvalFn Evaluator, DiagFn Diagnoser,
                             bool AllowInjection, QualType ResultTy,
                             SourceRange Range, ArrayRef<Expr *> Args,
                             Decl *ContainingDecl);

static bool define_unscoped_enum(APValue &Result, ASTContext &C,
                             MetaActions &Meta, EvalFn Evaluator,
                             DiagFn Diagnoser, bool AllowInjection,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool define_encoded_static_string(APValue &Result, ASTContext &C,
                                         MetaActions &Meta, EvalFn Evaluator,
                                         DiagFn Diagnoser, bool AllowInjection,
                                         QualType ResultTy, SourceRange Range,
                                         ArrayRef<Expr *> Args,
                                         Decl *ContainingDecl);

static bool has_parent(APValue &Result, ASTContext &C, MetaActions &Meta,
                       EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                       QualType ResultTy, SourceRange Range,
                       ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_c_language_linkage(APValue &Result, ASTContext &C,
                                   MetaActions &Meta, EvalFn Evaluator,
                                   DiagFn Diagnoser, bool AllowInjection,
                                   QualType ResultTy, SourceRange Range,
                                   ArrayRef<Expr *> Args, Decl *ContainingDecl);

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

static bool variable_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl);

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
static bool is_unscoped_attribute(APValue &Result, ASTContext &C,
                                  MetaActions &Meta, EvalFn Evaluator,
                                  DiagFn Diagnoser, bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args, Decl *ContainingDecl);
static bool is_clang_attribute(APValue &Result, ASTContext &C,
                               MetaActions &Meta, EvalFn Evaluator,
                               DiagFn Diagnoser, bool AllowInjection,
                               QualType ResultTy, SourceRange Range,
                               ArrayRef<Expr *> Args, Decl *ContainingDecl);
static bool is_gcc_attribute(APValue &Result, ASTContext &C,
                             MetaActions &Meta, EvalFn Evaluator,
                             DiagFn Diagnoser, bool AllowInjection,
                             QualType ResultTy, SourceRange Range,
                             ArrayRef<Expr *> Args, Decl *ContainingDecl);
static bool is_msvc_attribute(APValue &Result, ASTContext &C,
                              MetaActions &Meta, EvalFn Evaluator,
                              DiagFn Diagnoser, bool AllowInjection,
                              QualType ResultTy, SourceRange Range,
                              ArrayRef<Expr *> Args, Decl *ContainingDecl);
static bool is_attribute(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_attribute(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool has_attribute_namespace(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool attribute_token_of(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl);

static bool attribute_namespace_of(APValue &Result, ASTContext &C,
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
  { Metafunction::MFRK_bool, 1, 1, is_unscoped_attribute },
  { Metafunction::MFRK_bool, 1, 1, is_clang_attribute },
  { Metafunction::MFRK_bool, 1, 1, is_msvc_attribute },
  { Metafunction::MFRK_bool, 1, 1, is_gcc_attribute },

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
  { Metafunction::MFRK_metaInfo, 1, 3, constant_of },
  { Metafunction::MFRK_metaInfo, 1, 1, template_of },
  { Metafunction::MFRK_metaInfo, 4, 4, substitute },
  { Metafunction::MFRK_spliceFromArg, 2, 4, extract },
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
  { Metafunction::MFRK_bool, 1, 1, is_final },
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
  { Metafunction::MFRK_metaInfo, 12, 12, data_member_spec },
  { Metafunction::MFRK_metaInfo, 8, 8, enumerator_spec },
  { Metafunction::MFRK_bool, 1, 1, is_enumerator_spec },
  { Metafunction::MFRK_metaInfo, 3, 3, define_aggregate },
  { Metafunction::MFRK_metaInfo, 3, 3, define_enum },
  { Metafunction::MFRK_spliceFromArg, 2, 2, offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, size_of },
  { Metafunction::MFRK_spliceFromArg, 2, 2, bit_offset_of },
  { Metafunction::MFRK_sizeT, 1, 1, bit_size_of },
  { Metafunction::MFRK_sizeT, 1, 1, alignment_of },

  // P3096 metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_parameter_of },
  { Metafunction::MFRK_bool, 1, 1, has_ellipsis_parameter },
  { Metafunction::MFRK_bool, 1, 1, has_default_argument },
  { Metafunction::MFRK_bool, 1, 1, is_explicit_object_parameter },
  { Metafunction::MFRK_bool, 1, 1, is_function_parameter },
  { Metafunction::MFRK_metaInfo, 1, 1, return_type_of },
  { Metafunction::MFRK_metaInfo, 1, 1, variable_of,
    Metafunction::MFEK_Caller },

  // P3394 annotation metafunction extensions
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_annotation_of },
  { Metafunction::MFRK_bool, 1, 1, is_annotation },
  { Metafunction::MFRK_metaInfo, 2, 2, annotate },

  // P3385 attributes reflection
  { Metafunction::MFRK_metaInfo, 3, 3, get_ith_attribute_of },
  { Metafunction::MFRK_bool, 1, 1, is_attribute },
  { Metafunction::MFRK_bool, 3, 3, has_attribute },
  { Metafunction::MFRK_bool, 1, 1, has_attribute_namespace },
  { Metafunction::MFRK_spliceFromArg, 3, 3, attribute_token_of },
  { Metafunction::MFRK_spliceFromArg, 3, 3, attribute_namespace_of },

  // P3493 accessibility extensions
  { Metafunction::MFRK_metaInfo, 0, 0, current_access_context },
  { Metafunction::MFRK_bool, 3, 3, is_accessible },

  // Other bespoke functions (not proposed at this time)
  { Metafunction::MFRK_bool, 1, 1, is_access_specified },
  { Metafunction::MFRK_metaInfo, 5, 5, reflect_invoke },

  // P4033 extension: completing unscoped (C-style) enums
  { Metafunction::MFRK_metaInfo, 3, 3, define_unscoped_enum },

  // P3867: define_encoded_static_string
  { Metafunction::MFRK_spliceFromArg, 3, 3, define_encoded_static_string },

  // [meta.reflection.queries] has_parent, has_c_language_linkage
  { Metafunction::MFRK_bool, 1, 1, has_parent },
  { Metafunction::MFRK_bool, 1, 1, has_c_language_linkage },
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
  // Always write the out-parameter: IDs reach this from deserialized ASTs, and
  // a caller that forgets to check the return value must not be left holding
  // whatever happened to be on the stack.
  if (ID >= NumMetafunctions) {
    result = nullptr;
    return true;
  }

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
  else if (isa<ParmVarDecl>(D))
    return APValue(ReflectionKind::Parameter, D);

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

static APValue makeReflection(EnumeratorSpec *EMS) {
  return APValue(ReflectionKind::EnumeratorSpec, EMS);
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

/// Lifts 'V' into a reflection and stores it in 'Out'.
///
/// The reflection-depth counter is a fixed-width field, and 'std::meta::info'
/// is itself a structural type, so a metafunction handed a reflection can be
/// asked to reflect it again without bound. Diagnose at the limit rather than
/// letting 'APValue::Lift' silently produce a value of kind 'None'.
static bool SetAndSucceedWithLift(APValue &Out, DiagFn Diagnoser,
                                  SourceRange Range, const APValue &V,
                                  QualType ResultTy) {
  if (!V.canLift())
    return Diagnoser(Range.getBegin(), diag::metafn_reflection_depth_exceeded)
        << APValue::MaxReflectionDepth << Range;

  return SetAndSucceed(Out, V.Lift(ResultTy));
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

  // For an instantiated member, instantiating the out-of-line definition
  // REPLACES the parameters on the same FunctionDecl (no redeclaration is
  // added), so walking the instantiation's chain reads whichever
  // redeclaration happened to be instantiated last -- the answer would
  // depend on instantiation state and could differ between translation
  // units reflecting the same entity. The template pattern carries the
  // full declaration chain regardless of instantiation state, so walk
  // that instead. (Skipped when the pattern contains a parameter pack:
  // its parameter list does not line up index-for-index with the
  // instantiation's, and pack-substituted parameters were already
  // filtered above.)
  if (FunctionDecl *Pattern = FD->getTemplateInstantiationPattern();
      Pattern && llvm::none_of(Pattern->parameters(), [&](const ParmVarDecl *P) {
        return P->isParameterPack() && P->getFunctionScopeIndex() <= ParamIdx;
      }))
    FD = Pattern;

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
    else if (auto *AT = dyn_cast<AutoType>(QT); AT && AT->isDeduced())
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
  // Obtain the template arguments from the Type* representation. An alias
  // template specialization has only the arguments written in its sugar; a
  // class template specialization has its converted arguments on the
  // declaration, which are preferred over the (possibly unconverted)
  // arguments of the TemplateSpecializationType sugar.
  auto *TST = QT->getAs<TemplateSpecializationType>();
  if (TST && TST->isTypeAlias())
    expandTemplateArgPacks(TST->template_arguments(), Out);
  else if (auto *CTSD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
        QT->getAsRecordDecl()))
    expandTemplateArgPacks(CTSD->getTemplateArgs().asArray(), Out);
  else if (TST)
    expandTemplateArgPacks(TST->template_arguments(), Out);
  else if (auto DTST = QT->getAs<DependentTemplateSpecializationType>())
    expandTemplateArgPacks(DTST->template_arguments(), Out);
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

      // An lvalue naming a function is the argument of a parameter of
      // reference-to-function type ([meta.reflection.queries]/59.3.1).
      if (TExpr->isLValue() && TExpr->getType()->isFunctionType())
        if (auto *DRE = dyn_cast<DeclRefExpr>(TExpr->IgnoreParenImpCasts()))
          return makeReflection(DRE->getDecl());

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
    } case TemplateArgument::Declaration: {
      // [meta.reflection.queries]/59.3: if the parameter has reference type,
      // the reflection represents the function or object referred to
      // (59.3.1); otherwise the parameter has pointer or pointer-to-member
      // type and the reflection represents the value of the argument
      // (59.3.3), which is obtained by evaluating the expression that the
      // argument denotes so that it compares equal to reflect_constant of
      // the same pointer.
      ValueDecl *D = templArgument.getAsDecl();
      QualType ParamTy = templArgument.getParamTypeForDecl();

      // 59.3.2: for a parameter of class type, the template parameter object.
      if (isa<TemplateParamObjectDecl>(D) ||
          (!ParamTy.isNull() && !ParamTy->isReferenceType() &&
           !ParamTy->isPointerType() && !ParamTy->isMemberPointerType()))
        return APValue(APValue::LValueBase{D}, CharUnits::Zero(), {}, false,
                       false).Lift(QualType{});

      if (ParamTy.isNull() || ParamTy->isReferenceType()) {
        if (isa<FunctionDecl>(D))
          return makeReflection(D);

        Expr *DRE = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                        SourceLocation(), D, false,
                                        SourceLocation(),
                                        D->getType().getNonReferenceType(),
                                        VK_LValue, D, nullptr);
        APValue Object;
        if (!Evaluator(Object, DRE, false))
          return makeReflection(D);
        return Object.Lift(QualType{});
      }

      Expr *DRE = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                      SourceLocation(), D, false,
                                      SourceLocation(), D->getType(),
                                      VK_LValue, D, nullptr);
      Expr *Value;
      if (ParamTy->isPointerType() && D->getType()->isArrayType())
        Value = ImplicitCastExpr::Create(C, ParamTy, CK_ArrayToPointerDecay,
                                         DRE, nullptr, VK_PRValue,
                                         FPOptionsOverride());
      else
        Value = UnaryOperator::Create(C, DRE, UO_AddrOf, ParamTy, VK_PRValue,
                                      OK_Ordinary, SourceLocation(), false,
                                      FPOptionsOverride());
      APValue Ptr;
      if (!Evaluator(Ptr, Value, true))
        return makeReflection(D);
      return Ptr.Lift(ParamTy);
    }
    case TemplateArgument::NullPtr: {
      // Evaluate a null pointer conversion so that the representation matches
      // that of reflect_constant of a null pointer of the same type.
      QualType NullTy = templArgument.getNullPtrType();
      Expr *Null = new (C) CXXNullPtrLiteralExpr(C.NullPtrTy, SourceLocation());
      Expr *Cast = ImplicitCastExpr::Create(
          C, NullTy,
          NullTy->isMemberPointerType() ? CK_NullToMemberPointer
                                        : CK_NullToPointer,
          Null, nullptr, VK_PRValue, FPOptionsOverride());
      APValue NullPtrValue;
      if (!Evaluator(NullPtrValue, Cast, true))
        NullPtrValue = APValue((ValueDecl *)nullptr,
                               CharUnits::fromQuantity(
                                   C.getTargetNullPointerValue(NullTy)),
                               APValue::NoLValuePath(), /*IsNullPtr=*/true);
      return NullPtrValue.Lift(NullTy);
    }
    case TemplateArgument::StructuralValue: {
      APValue SV = templArgument.getAsStructuralValue();
      return SV.Lift(templArgument.getStructuralValueType());
    }
    case TemplateArgument::Integral: {
      APValue IV(templArgument.getAsIntegral());
      return IV.Lift(templArgument.getIntegralType());
    }
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

/// Whether 'QT' is a cv-qualified type, as opposed to a (possibly cv-qualified
/// in its definition) type alias. [meta.reflection.names]/1.4 gives a
/// cv-qualified class or enumeration type no identifier, while /1.3 lets an
/// alias such as 'using CI = const int;' keep its name.
static bool isCVQualifiedType(QualType QT) {
  if (QT.hasLocalQualifiers())
    return true;
  if (isTypeAlias(QT))
    return false;
  return QT.getCanonicalType().hasLocalQualifiers();
}

/// The identifier of the type 'QT' per [meta.reflection.names]/1.1, /1.3 and
/// /1.4, or nullptr if it has none. Template specializations and cv-qualified
/// types are rejected by the callers with a more specific diagnostic.
static IdentifierInfo *getTypeIdentifier(QualType QT) {
  if (isTemplateSpecialization(QT) || isCVQualifiedType(QT))
    return nullptr;

  NamedDecl *D = findTypeDecl(QT);
  if (!D)
    return nullptr;
  if (IdentifierInfo *II = D->getIdentifier())
    return II;

  // /1.1: an unnamed class or enumeration declared in a typedef declaration
  // has the typedef name for linkage purposes ([dcl.typedef]/9).
  if (auto *TD = dyn_cast<TagDecl>(D))
    if (TypedefNameDecl *TND = TD->getTypedefNameForAnonDecl())
      return TND->getIdentifier();
  return nullptr;
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

  if (D != D->getCanonicalDecl()) {
    Decl *First = nullptr;
    for (Decl *I = D->getMostRecentDecl(); I; I = I->getPreviousDecl())
      if (I->getLexicalDeclContext() == D->getLexicalDeclContext())
        First = I;
    if (D != First)
      return false;
  }

  if (D->isLocalExternDecl())
    return false;

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
      if (!R->getReturnType()->isUndeducedType() &&
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
    DeclContext *DC = D->getDeclContext();  // note: SemanticDC

    if (D->getLexicalDeclContext() == DC) {
      // Get the next declaration in the DeclContext.
      //
      // Explicit specializations of templates are created with the DeclContext
      // of the template from which they're instantiated, but they end up in the
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

        if (!D) {
          auto *Canonical = cast<NamespaceDecl>(DC->getPrimaryContext());
          D = Canonical->getLastMultDCSemaDecl();
        }
      }
    } else {
      D = D->getPrevMultDCDeclInSemaContext();
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
  while (DC) {
    // [meta.reflection.queries]/51.4.1: the function call operator of the
    // closure type of a consteval block is transparent; the parent is that of
    // the closure type.
    if (auto *MD = dyn_cast<CXXMethodDecl>(DC);
        MD && MD->getParent()->isConstevalBlockLambda()) {
      DC = MD->getParent()->getLexicalDeclContext();
      continue;
    }
    if (isa<NamespaceDecl, RecordDecl, FunctionDecl, TranslationUnitDecl,
            EnumDecl>(DC))
      break;
    DC = DC->getParent();
  }

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
  // getAs, not dyn_cast: attributes on a function declarator (e.g.
  // [[clang::lifetimebound]] on the implicit object parameter) wrap the
  // FunctionProtoType in AttributedType sugar that dyn_cast cannot see
  // through, silently dropping the method qualifiers.
  if (const auto *FPT = QT->getAs<FunctionProtoType>())
    result |= FPT->isConst();

  return result;
}

static bool isVolatileQualifiedType(QualType QT) {
  bool result = QT.isVolatileQualified();
  if (const auto *FPT = QT->getAs<FunctionProtoType>())
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
  ND = cast<NamedDecl>(ND->getCanonicalDecl());

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
      return "a destructor";
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
      return "a destructor template";
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
  case ReflectionKind::Parameter: {
    return "a parameter";
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

llvm::SmallVector<const Attr*, 8> static collectUniqueCxx11Attrs(const Decl *D) {
  llvm::SmallVector<const Attr*, 8> Result;
  llvm::SmallSet<std::string, 8> SeenKinds;

  for (const Decl *RD : D->redecls()) {
    if (!RD->hasAttrs()) {
      continue;
    }
    for (const Attr *A : RD->getAttrs()) {
      if (!isAttributeWithReflectableVariant(A->getParsedKind())) {
        continue;
      }

      if (SeenKinds.insert(std::string(A->getSpelling())).second) {
        Result.push_back(A);
      }
    }
  }

  return Result;
}

// -----------------------------------------------------------------------------
// Metafunction implementations
// -----------------------------------------------------------------------------

bool has_attribute_namespace(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  ParsedAttr *attr = RV.getReflectedAttribute();

  return SetAndSucceed(
    Result,
    makeBool(C, attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11 && attr->hasScope())
  );
}

bool attribute_namespace_of(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
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

  if (RV.getReflectionKind() != ReflectionKind::Attribute)
    return DiagnoseReflectionKind(Diagnoser, Range, "an attribute", DescriptionOf(RV));

  auto name(RV.getReflectedAttribute()->getNormalizedScopeName());
  if (name.empty())
    return Diagnoser(Range.getBegin(), diag::metafn_anonymous_entity) << DescriptionOf(RV) << Range;

  Expr *StrLit = makeStrLiteral(name, C, IsUtf8);
  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result, APValue(StrLit, CharUnits::Zero(), Path, false));
}

bool attribute_token_of(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
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

  if (RV.getReflectionKind() != ReflectionKind::Attribute)
    return DiagnoseReflectionKind(Diagnoser, Range, "an attribute", DescriptionOf(RV));

  auto name(RV.getReflectedAttribute()->getAttrName()->getName());
  if (name.empty())
    return Diagnoser(Range.getBegin(), diag::metafn_anonymous_entity) << DescriptionOf(RV) << Range;

  Expr *StrLit = makeStrLiteral(name, C, IsUtf8);
  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result, APValue(StrLit, CharUnits::Zero(), Path, false));
}

bool is_unscoped_attribute(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  ParsedAttr *attr = RV.getReflectedAttribute();
  const bool isClang = attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11
    && !attr->hasScope();
  return SetAndSucceed(Result, makeBool(C, isClang));
}

bool is_clang_attribute(APValue &Result, ASTContext &C,
                      MetaActions &Meta, EvalFn Evaluator,
                      DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  ParsedAttr *attr = RV.getReflectedAttribute();
  const bool isClang = attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11
    && attr->isClangScope();
  return SetAndSucceed(Result, makeBool(C, isClang));
}

bool is_gcc_attribute(APValue &Result, ASTContext &C,
                    MetaActions &Meta, EvalFn Evaluator,
                    DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  ParsedAttr *attr = RV.getReflectedAttribute();
  const bool isGnu = attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11
    && attr->isGNUScope();
  return SetAndSucceed(Result, makeBool(C, isGnu));
}

bool is_msvc_attribute(APValue &Result, ASTContext &C,
                    MetaActions &Meta, EvalFn Evaluator,
                    DiagFn Diagnoser, bool AllowInjection,
                    QualType ResultTy, SourceRange Range,
                    ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  ParsedAttr *attr = RV.getReflectedAttribute();
  const bool isGnu = attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11
    && attr->isMsvcScope();
  return SetAndSucceed(Result, makeBool(C, isGnu));
}

// Synthesize back a ParsedAttr from an Attr, the best I can...
// Return a nullptr if the process met an error
static const ParsedAttr* toSyntacticForm(const Attr* val, ASTContext * C) {
  // Owned by the ASTContext: the pool's allocator is not thread-safe, and the
  // ParsedAttrs built below point back into this context.
  AttributeScratchpad &scratchpad = C->getAttributeScratchpad();
  ParsedAttr * recoveredAttr = nullptr;
  auto onArgs = [&](
      IdentifierInfo * attrName,
      SmallVector<llvm::PointerUnion<Expr *, IdentifierLoc *>, 2> argExprs,
      SmallVector<void *, 2> typeArgs,
      AttributeCommonInfo::Form /* Do we need this fed back to us at all ?...*/
    ) {
      AttributeScopeInfo scope;
      if (val->hasScope())
        scope = AttributeScopeInfo(val->getScopeName(), val->getLoc());
      if (!typeArgs.empty() && argExprs.size() == 1 && argExprs[0].isNull()) {
        ParsedType pt = ParsedType::getFromOpaquePtr(typeArgs[0]);
        recoveredAttr = scratchpad.pool.createTypeAttribute(
          attrName, val->getRange(), scope, pt, val->getForm(),
          SourceLocation());
      } else {
        recoveredAttr = scratchpad.pool.create(
          attrName, val->getRange(), scope,
          argExprs.data(), argExprs.size(), val->getForm());
      }
      return recoveredAttr != nullptr;
    };
    // FIXME why is this not just returning the vector of args...
    // Did I worry about lifetime... ?
    if (!extractSyntacticArguments(val, *C, onArgs, val->getLocation())) {
      recoveredAttr = nullptr;
    }
    return recoveredAttr;
}

enum class AttributeComparison : int64_t {
  /* 0 = include all */
  IgnoreNamespace = 1 << 1, // Namespace is ignored during the comparison
  IgnoreArgument  = 1 << 2, // The argument is ignored during the comparison
};

bool has_attribute(APValue &Result, ASTContext &C,
                  MetaActions &Meta, EvalFn Evaluator,
                  DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range,
                  ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  APValue RV;

  assert(ResultTy == C.BoolTy);

  // Policy
  assert(Args[2]->getType()->isIntegralOrEnumerationType());
  if (!Evaluator(RV, Args[2], true)) {
    return true;
  }
  const int64_t policy = RV.getInt().getExtValue();
  const bool isContributingNamespace = (policy & static_cast<int64_t>(AttributeComparison::IgnoreNamespace)) == 0;
  const bool isContributingArgument = (policy & static_cast<int64_t>(AttributeComparison::IgnoreArgument)) == 0;

  // Attribute to look for
  assert(Args[1]->getType()->isReflectionType());
  if (!Evaluator(RV, Args[1], true)) {
    return true;
  }
  if (RV.getReflectionKind() != ReflectionKind::Attribute) {
    return SetAndSucceed(Result, makeBool(C, false));
  }
  const ParsedAttr* testAttr = RV.getReflectedAttribute();

  // Entity to inspect
  assert(Args[0]->getType()->isReflectionType());
  if (!Evaluator(RV, Args[0], true)) {
    return true;
  }

  auto findMatchingAttribute = [&](Decl* decl, const ParsedAttr* testAttr) -> bool {
    llvm::FoldingSetNodeID providedAttrID;
    testAttr->profile(providedAttrID, isContributingNamespace, isContributingArgument);

    auto cxx11Attrs = collectUniqueCxx11Attrs(decl);
    if (cxx11Attrs.empty()) {
      return false;
    }
    for (const auto * val : cxx11Attrs) {
      assert(val);
      const ParsedAttr * recoveredAttr = toSyntacticForm(val, &C);
      llvm::FoldingSetNodeID recoveredAttrID;
      recoveredAttr->profile(recoveredAttrID, isContributingNamespace, isContributingArgument);
      if (recoveredAttrID == providedAttrID) {
        return true;
      }
    }
    return false;
  };

  switch (RV.getReflectionKind()) {
    default: return DiagnoseReflectionKind(Diagnoser, Range, "variable, attribute, function, namespace", DescriptionOf(RV));
    case ReflectionKind::Attribute: { // Somewhat useless...
      llvm::FoldingSetNodeID testAttrID;
      testAttr->profile(testAttrID);
      const ParsedAttr* reflectedAttr = RV.getReflectedAttribute();
      llvm::FoldingSetNodeID reflectedAttrID;
      reflectedAttr->profile(reflectedAttrID);

      return SetAndSucceed(Result, makeBool(C, reflectedAttrID == testAttrID));
    }
    case ReflectionKind::Type: {
      QualType qType = RV.getReflectedType();
      Decl *D = findTypeDecl(qType)->getMostRecentDecl();
      if (!D) {
        return Diagnoser(Range.getBegin(), diag::metafn_p3385_no_declaration_for_type)
          << DescriptionOf(RV);
      }
      return SetAndSucceed(Result, makeBool(C, findMatchingAttribute(D, testAttr)));
    }
    case ReflectionKind::Declaration: {
      ValueDecl *D = RV.getReflectedDecl();
      if (!D) {
        return DiagnoseReflectionKind(
          Diagnoser, Range, "attribute, type, declaration", DescriptionOf(RV));
      }
      return SetAndSucceed(Result, makeBool(C, findMatchingAttribute(D, testAttr)));
    }
    case ReflectionKind::Namespace: {
      Decl* D = RV.getReflectedNamespace()->getMostRecentDecl();
      if (!D) {
        return DiagnoseReflectionKind(
          Diagnoser, Range, "attribute, type, declaration", DescriptionOf(RV));
      }
      return SetAndSucceed(Result, makeBool(C, findMatchingAttribute(D, testAttr)));
    }
  }
}

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

  auto buildIthParsedAttrFromDecl = [&](size_t i, Decl* decl, const ParsedAttr* &result) -> bool {
    auto cxx11Attrs = collectUniqueCxx11Attrs(decl);
    if (i + 1 > cxx11Attrs.size()) {
      result = nullptr;
      return true;
    }
    const Attr * val = cxx11Attrs[i];
    assert(val);
    result = toSyntacticForm(val, &C);
    return result != nullptr;
  };

  switch (RV.getReflectionKind()) {
    default: return DiagnoseReflectionKind(Diagnoser, Range, "variable, attribute, function, namespace", DescriptionOf(RV));
    case ReflectionKind::Attribute: {
      if (idx != 0) {
        return SetAndSucceed(Result, Sentinel);
      }
      ParsedAttr *attr = RV.getReflectedAttribute();
      if (attr->getForm().getSyntax() == AttributeCommonInfo::Syntax::AS_CXX11) {
        return SetAndSucceed(Result, makeReflection(attr));
      }
      return Diagnoser(Range.getBegin(), diag::metafn_p3385_non_standard_attribute)
        << attr->getAttrName();
    }
    case ReflectionKind::Type: {
      QualType qType = RV.getReflectedType();
      Decl *D = findTypeDecl(qType);
      if (!D) {
        return Diagnoser(Range.getBegin(), diag::metafn_p3385_no_declaration_for_type)
          << DescriptionOf(RV);
      }

      if (const ParsedAttr* fetchedAttribute{}; buildIthParsedAttrFromDecl(idx, D, fetchedAttribute)) {
        if (fetchedAttribute) {
          return SetAndSucceed(Result, makeReflection(fetchedAttribute));
        }
        // Reached the end
        return SetAndSucceed(Result, Sentinel);
      }
      return true;
    }
    case ReflectionKind::Declaration: {
      ValueDecl *D = RV.getReflectedDecl();
      if (!D) {
        return DiagnoseReflectionKind(
          Diagnoser, Range, "attribute, type, variable, namespace", DescriptionOf(RV));
      }

      if (const ParsedAttr* fetchedAttribute{}; buildIthParsedAttrFromDecl(idx, D, fetchedAttribute)) {
        if (fetchedAttribute) {
          return SetAndSucceed(Result, makeReflection(fetchedAttribute));
        }
        return SetAndSucceed(Result, Sentinel);
      }
      return true;
    }
    case ReflectionKind::Namespace: {
      Decl* D = RV.getReflectedNamespace();
      if (!D) {
        return DiagnoseReflectionKind(
          Diagnoser, Range, "attribute, type, variable, namespace", DescriptionOf(RV));
      }
      if (const ParsedAttr* fetchedAttribute{}; buildIthParsedAttrFromDecl(idx, D, fetchedAttribute)) {
        if (fetchedAttribute) {
          return SetAndSucceed(Result, makeReflection(fetchedAttribute));
        }
        return SetAndSucceed(Result, Sentinel);
      }
      return true;
    }
    case ReflectionKind::Null:
      return Diagnoser(Range.getBegin(), diag::metafn_p3385_attributes_of_null) << Range;
  }
  llvm_unreachable("unknown reflection kind");
  return false;
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
    if (R.isReflectedDecl() && !isa<FunctionDecl>(R.getReflectedDecl()))
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
    if (R.isReflectedDecl() && !isa<FunctionDecl>(R.getReflectedDecl()))
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
    // If this is a declared type with a reachable definition, ensure that the
    // type is instantiated.
    if (Decl *typeDecl = findTypeDecl(RV.getReflectedType()))
      Meta.EnsureInstantiated(typeDecl, Range);

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

  // [meta.reflection.names]/3.1, /3.4 for a type; /3.5 routes a direct base
  // class relationship through the type of its base class.
  auto identifierOfType = [&](QualType QT, std::string &Name) -> bool {
    if (isTemplateSpecialization(QT))
      return Diagnoser(Range.getBegin(), diag::metafn_name_is_not_identifier)
          << 0 << Range;
    if (isCVQualifiedType(QT))
      return Diagnoser(Range.getBegin(), diag::metafn_name_of_cv_qualified_type)
          << QT << Range;
    if (IdentifierInfo *II = getTypeIdentifier(QT))
      Name = II->getName();
    return false;
  };

  std::string Name;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    if (identifierOfType(RV.getReflectedType(), Name))
      return true;
    break;
  }
  case ReflectionKind::Declaration: {
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
  case ReflectionKind::Parameter: {
    bool ConsistentName = getParameterName(RV.getReflectedParameter(), Name);
    if (EnforceConsistent && !ConsistentName) {
      return Diagnoser(Range.getBegin(), diag::metafn_inconsistent_name)
          << DescriptionOf(RV) << Range;
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
  case ReflectionKind::DataMemberSpec: {
    TagDataMemberSpec *TDMS = RV.getReflectedDataMemberSpec();
    if (TDMS->Name)
      Name = *TDMS->Name;
    break;
  }
  case ReflectionKind::BaseSpecifier: {
    QualType QT = RV.getReflectedBaseSpecifier()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    if (identifierOfType(QT, Name))
      return true;
    break;
  }
  case ReflectionKind::Null:
    return Diagnoser(Range.getBegin(),
                     diag::metafn_name_of_unnamed_singleton) << 0 << Range;
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
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

  // [meta.reflection.names]/1.5, /1.6: a literal operator (template) is not
  // an operator function (template); its ud-suffix is its identifier (/3.2).
  auto hasIdentifier = [](const NamedDecl *ND) {
    return ND->getIdentifier() != nullptr ||
           ND->getDeclName().getNameKind() ==
               DeclarationName::CXXLiteralOperatorName;
  };

  bool HasIdentifier = false;
  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    HasIdentifier = getTypeIdentifier(RV.getReflectedType()) != nullptr;
    break;
  }
  case ReflectionKind::Parameter: {
    auto *PVD = RV.getReflectedParameter();

    std::string Name;
    bool Consistent = getParameterName(PVD, Name);

    HasIdentifier = Consistent && !Name.empty();
    break;
  }
  case ReflectionKind::Declaration: {
    auto *D = RV.getReflectedDecl();

    if (auto *FD = dyn_cast<FunctionDecl>(D);
               FD && FD->getTemplateSpecializationArgs())
      break;
    else if (isa<VarTemplateSpecializationDecl>(D))
      break;
    else if (auto *PVD = dyn_cast<ParmVarDecl>(D)) {
      std::string Name;
      (void) getParameterName(PVD, Name);
      HasIdentifier = !Name.empty();
    }
    else if (auto *ND = dyn_cast<NamedDecl>(D))
      HasIdentifier = hasIdentifier(ND);

    break;
  }
  case ReflectionKind::Template: {
    const TemplateDecl *TD = RV.getReflectedTemplate().getAsTemplateDecl();
    if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TD))
      if (isa<CXXConstructorDecl>(FTD->getTemplatedDecl()))
        break;

    HasIdentifier = hasIdentifier(TD);
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
  case ReflectionKind::BaseSpecifier: {
    // [meta.reflection.names]/1.12: has_identifier(type_of(r)).
    QualType QT = RV.getReflectedBaseSpecifier()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/false,
                     /*DropRefs=*/false);
    HasIdentifier = getTypeIdentifier(QT) != nullptr;
    break;
  }
  case ReflectionKind::Null:
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
  case ReflectionKind::Parameter:
    return findDeclLoc(Result, C, Evaluator, ResultTy,
                       RV.getReflectedParameter());
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

    if (auto *FD = dyn_cast<FunctionDecl>(VD)) {
      // A function whose type contains an undeduced placeholder type has no
      // type ([meta.reflection.queries]/1).
      if (FD->getReturnType()->isUndeducedType())
        return Diagnoser(Range.getBegin(), diag::metafn_undeduced_return_type)
            << DescriptionOf(RV) << Range;
      Meta.EnsureInstantiationOfExceptionSpec(Range.getBegin(), FD);
    }

    QualType QT = desugarType(VD->getType(),
                              /*UnwrapAliases=*/ true, /*DropCV=*/false,
                              /*DropRefs=*/false);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  case ReflectionKind::Parameter: {
    ParmVarDecl *PVD = RV.getReflectedParameter();
    QualType QT = desugarType(PVD->getType(),
                              /*UnwrapAliases=*/ true, /*DropCV=*/true,
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
    // [meta.reflection.queries]/2.3: type_of(constant_of(r)). constant_of
    // yields a value of the (cv-unqualified) type of the annotation's
    // constant, except for a class type, where it yields the corresponding
    // template parameter object, whose type is const-qualified
    // ([temp.param]/8).
    QualType QT = RV.getReflectedAnnotation()->getArg()->getType();
    QT = desugarType(QT, /*UnwrapAliases=*/true, /*DropCV=*/true,
                     /*DropRefs=*/false);
    if (QT->isRecordType())
      QT = QT.withConst();
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
  case ReflectionKind::Parameter: {
    return DiagWrapper(parentOf(Result, RV.getReflectedParameter()));
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return DiagnoseReflectionKind(Diagnoser, Range, "an entity proxy");
  case ReflectionKind::EntityProxy:
    return SetAndSucceed(Result, MaybeUnproxy(C, RV, false));
  }
  llvm_unreachable("unknown reflection kind");
}

/// Whether the object designated by 'Base' has static storage duration
/// ([basic.stc.static]): a variable with static storage duration, a template
/// parameter object, a string literal, a temporary whose lifetime was extended
/// to static storage duration, or a std::type_info object.
static bool hasStaticStorageDuration(const APValue::LValueBase &Base) {
  if (const auto *VD = Base.dyn_cast<const ValueDecl *>()) {
    if (const auto *Var = dyn_cast<VarDecl>(VD))
      return Var->getStorageDuration() == SD_Static;
    return isa<TemplateParamObjectDecl>(VD);
  }
  if (const auto *E = Base.dyn_cast<const Expr *>()) {
    if (const auto *MTE = dyn_cast<MaterializeTemporaryExpr>(E))
      return MTE->getStorageDuration() == SD_Static;
    return isa<StringLiteral, CompoundLiteralExpr, PredefinedExpr>(E);
  }
  return Base.is<TypeInfoLValue>();
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
    bool IsReference = QT->isReferenceType();
    if (IsReference)
      QT = QT.getNonReferenceType();

    // [meta.reflection.queries]/5.2: a variable must declare (or, if it is a
    // reference, refer to) an object with static storage duration
    // ([basic.stc.general]); a variable with thread or automatic storage
    // duration declares no such object.
    if (!IsReference && VD->getStorageDuration() != SD_Static)
      return Diagnoser(Range.getBegin(), diag::metafn_object_of_non_static)
          << DescriptionOf(RV)
          << (VD->getStorageDuration() == SD_Thread ? 0 : 1) << Range;

    Expr *Synthesized = DeclRefExpr::Create(C,
                                            NestedNameSpecifierLoc(),
                                            SourceLocation(), VD, false,
                                            Range.getBegin(), QT,
                                            VK_LValue, VD, nullptr);
    APValue Value;
    if (!Evaluator(Value, Synthesized, false) || !Value.isLValue())
      return true;

    if (IsReference && !hasStaticStorageDuration(Value.getLValueBase()))
      return Diagnoser(Range.getBegin(), diag::metafn_object_of_non_static)
          << DescriptionOf(RV) << 2 << Range;

    APValue OV = Value.Lift(QualType{});
    return SetAndSucceed(Result, OV);
  }
  case ReflectionKind::Null:
  case ReflectionKind::Value:
  case ReflectionKind::Type:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Parameter:
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


static TemplateArgument TArgFromReflection(ASTContext &C, MetaActions &Meta,
                                           EvalFn Evaluator, const APValue &RV,
                                           SourceLocation Loc);

/// 'reflect_constant_array' ([meta.define.static]/8-12) for an array value:
/// substitutes the element type and 'reflect_constant' of each element into
/// the variable template that the <meta> header's reflect_constant_array uses
/// ('FixedArray', or 'EmptyArray' for an empty array), so that the result
/// compares equal to what reflect_constant_array yields for the same array.
static bool reflectConstantArray(APValue &Result, ASTContext &C,
                                 MetaActions &Meta, EvalFn Evaluator,
                                 DiagFn Diagnoser, SourceRange Range,
                                 VarTemplateDecl *FixedArray,
                                 VarTemplateDecl *EmptyArray, QualType ArrTy,
                                 const APValue &ArrayVal) {
  const ConstantArrayType *CAT = C.getAsConstantArrayType(ArrTy);
  if (!CAT || !ArrayVal.isArray() || !FixedArray || !EmptyArray)
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 2 << "an array of unknown bound" << Range;

  // ranges::range_value_t strips cv-qualifiers from the element type.
  QualType ElemTy = desugarType(CAT->getElementType(), /*UnwrapAliases=*/true,
                                /*DropCV=*/true, /*DropRefs=*/false);
  if (ElemTy->isArrayType())
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 2 << "a multidimensional array" << Range;
  if (!ElemTy->isStructuralType())
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 2 << "an array of non-structural type" << Range;

  SmallVector<TemplateArgument, 8> TArgs;
  TArgs.push_back(TemplateArgument(ElemTy.getCanonicalType()));

  unsigned Size = ArrayVal.getArraySize();
  for (unsigned I = 0; I < Size; ++I) {
    APValue Elem = I < ArrayVal.getArrayInitializedElts()
                       ? ArrayVal.getArrayInitializedElt(I)
                       : ArrayVal.getArrayFiller();

    // reflect_constant of the element: an object for a class type, a value
    // otherwise ([meta.reflection.result]/2).
    APValue Refl;
    if (ElemTy->isRecordType()) {
      auto *TPO = C.getTemplateParamObjectDecl(ElemTy, Elem);
      Refl = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                     false).Lift(QualType{});
    } else {
      Refl = Elem.Lift(ElemTy);
    }

    TemplateArgument TArg = TArgFromReflection(C, Meta, Evaluator, Refl,
                                               Range.getBegin());
    if (TArg.isNull())
      return true;
    TArgs.push_back(TArg);
  }

  VarTemplateDecl *VTD = Size == 0 ? EmptyArray : FixedArray;
  SmallVector<TemplateArgument, 8> ExpandedTArgs;
  expandTemplateArgPacks(TArgs, ExpandedTArgs);
  if (!Meta.CheckTemplateArgumentList(VTD, ExpandedTArgs,
                                      /*SuppressDiagnostics=*/false,
                                      Range.getBegin()))
    return true;
  TArgs.clear();
  expandTemplateArgPacks(ExpandedTArgs, TArgs);

  VarDecl *Spec = Meta.Substitute(VTD, TArgs, Range.getBegin());
  if (!Spec)
    return true;
  return SetAndSucceed(Result, makeReflection(Spec));
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

  // The variable templates behind the header's reflect_constant_array
  // ([meta.reflection.queries]/8 makes constant_of of an array equivalent to
  // reflect_constant_array([:R:])), handed over by the <meta> wrapper.
  VarTemplateDecl *FixedArray = nullptr, *EmptyArray = nullptr;
  for (unsigned I = 1; I < Args.size() && I < 3; ++I) {
    APValue TV;
    if (!Evaluator(TV, Args[I], true) || !TV.isReflectedTemplate())
      return true;
    auto *VTD = dyn_cast<VarTemplateDecl>(
        TV.getReflectedTemplate().getAsTemplateDecl());
    (I == 1 ? FixedArray : EmptyArray) = VTD;
  }

  // reflect_constant([:R:]) for a glvalue of type 'QT' whose value is
  // 'Constant' ([meta.reflection.queries]/8, [meta.reflection.result]/2): an
  // object (template parameter object) for a class type, a value otherwise.
  auto reflectConstant = [&](QualType QT, APValue Constant) -> bool {
    QualType ConstantTy = ComputeResultType(QT, Constant);
    if (ConstantTy->isRecordType()) {
      auto *TPO = C.getTemplateParamObjectDecl(ConstantTy, Constant);
      Constant = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                         false);
      ConstantTy = QualType{};
    }
    return SetAndSucceedWithLift(Result, Diagnoser, Range, Constant,
                                 ConstantTy);
  };

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Value:
    return SetAndSucceed(Result, RV);
  case ReflectionKind::Object: {
    QualType ObjectTy = RV.getTypeOfReflectedResult(C);
    if (!ObjectTy->isArrayType() && !ObjectTy->isStructuralType())
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object of non-structural type" << Range;

    Expr *OVE = new (C) OpaqueValueExpr(Range.getBegin(), ObjectTy, VK_LValue);
    Expr *CE = ConstantExpr::Create(C, OVE, RV.getReflectedObject());

    Expr::EvalResult ER;
    if (!CE->EvaluateAsRValue(ER, C, true))
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
          << 2 << "an object not usable in constant expressions" << Range;

    if (ObjectTy->isArrayType())
      return reflectConstantArray(Result, C, Meta, Evaluator, Diagnoser, Range,
                                  FixedArray, EmptyArray, ObjectTy, ER.Val);
    return reflectConstant(ObjectTy, ER.Val);
  }
  case ReflectionKind::Declaration: {
    ValueDecl *Decl = RV.getReflectedDecl();

    APValue Constant;
    QualType QT;
    if (auto *VD = dyn_cast<VarDecl>(Decl)) {
      // An array that reflect_constant_array already promoted is its own
      // constant: promoting it again would only copy its elements once more.
      if (auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(VD);
          VTSD && (VTSD->getSpecializedTemplate() == FixedArray ||
                   VTSD->getSpecializedTemplate() == EmptyArray))
        return SetAndSucceed(Result, RV);

      // A specialization of a variable template (such as the one that
      // reflect_constant_array yields) has no initializer until instantiated.
      Meta.EnsureInstantiated(VD, Args[0]->getSourceRange());
      if (!VD->isUsableInConstantExpressions(C))
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
            << 2 << "a variable not usable in constant expressions" << Range;

      QT = VD->getType();
      if (QT->isReferenceType())
        QT = QT.getNonReferenceType();

      Expr *Synthesized = DeclRefExpr::Create(C, NestedNameSpecifierLoc(),
                                              SourceLocation(), VD, false,
                                              Range.getBegin(), QT,
                                              VK_LValue, Decl, nullptr);
      if (!Evaluator(Constant, Synthesized, !QT->isFunctionType()))
        llvm_unreachable("failed to evaluate variable usable in constant "
                         "expressions");

      // [meta.reflection.queries]/8: an array yields reflect_constant_array.
      if (QT->isArrayType())
        return reflectConstantArray(Result, C, Meta, Evaluator, Diagnoser,
                                    Range, FixedArray, EmptyArray, QT,
                                    Constant);

      // A reference to a function has reference type, so /8 reaches
      // reflect_constant([:R:]), whose by-value parameter deduces a pointer
      // to the function: the result is a value of pointer type.
      if (QT->isFunctionType())
        QT = C.getPointerType(QT);
    } else if (auto *FD = dyn_cast<FunctionDecl>(Decl)) {
      // [meta.reflection.queries]/8: reflect_function([:R:]), a reflection of
      // the function itself; [:R:] is not a valid splice-expression for a
      // non-static member function (/9).
      if (auto *MD = dyn_cast<CXXMethodDecl>(FD); MD && MD->isInstance())
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
            << 2 << DescriptionOf(RV) << Range;
      return SetAndSucceed(Result, makeReflection(FD));
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

    return reflectConstant(QT, Constant);
  }
  case ReflectionKind::Annotation: {
    CXX26AnnotationAttr *A = RV.getReflectedAnnotation();
    APValue Constant = RV.getReflectedAnnotation()->getValue();

    QualType ConstantTy = desugarType(A->getArg()->getType(),
                                      /*UnwrapAliases=*/true, /*DropCV=*/true,
                                      /*DropRefs=*/false);
    if (ConstantTy->isRecordType()) {
      auto *TPO = C.getTemplateParamObjectDecl(ConstantTy, Constant);
      Constant = APValue(APValue::LValueBase{TPO}, CharUnits::Zero(), {}, false,
                    false);
      ConstantTy = QualType{};
    }
    return SetAndSucceedWithLift(Result, Diagnoser, Range, Constant,
                                 ConstantTy);
  }
  case ReflectionKind::Attribute: // TODO P3385 anything to do ?
  case ReflectionKind::Null:
  case ReflectionKind::Type:
  case ReflectionKind::Template:
  case ReflectionKind::Namespace:
  case ReflectionKind::EntityProxy:
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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

static TemplateArgument TArgFromReflection(ASTContext &C, MetaActions &Meta,
                                           EvalFn Evaluator, const APValue &RV,
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

    if (!Meta.EnsureInstantiated(Decl, SourceRange(Loc, Loc)))
      return TemplateArgument();

    QualType QT = desugarType(Decl->getType(), /*UnwrapAliases=*/ false,
                              /*DropCV=*/false, /*DropRefs=*/true);

    // Don't worry about the cost of creating an expression here: The template
    // substitution machinery will otherwise create one from the argument
    // anyway, so we aren't really losing any efficiency here.
    Expr *Synthesized =
        DeclRefExpr::Create(C, NestedNameSpecifierLoc(), SourceLocation(), Decl,
                            false, Loc, QT, VK_LValue, Decl, nullptr);

    return TemplateArgument(Synthesized, true);
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

      TemplateArgument TArg = TArgFromReflection(C, Meta, Evaluator, Unwrapped,
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
  }
  if (auto *TATD = dyn_cast<TypeAliasTemplateDecl>(TDecl)) {
    TArgs.clear();
    expandTemplateArgPacks(ExpandedTArgs, TArgs);

    QualType QT = Meta.Substitute(TATD, TArgs, Range.getBegin());
    if(QT.isNull()) {
      // substitution failed after validating arguments
      return true;
    }
    APValue RV = makeReflection(QT);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, makeReflection(QT));
  }
  if (auto *FTD = dyn_cast<FunctionTemplateDecl>(TDecl)) {
    FunctionDecl *Spec = Meta.Substitute(FTD, ExpandedTArgs, Range.getBegin());
    assert(Spec && "substitution failed after validating arguments?");

    if (Spec->getReturnType()->isUndeducedType())
      return NoDiagnose ? ElideDiagnosis() :
             Diagnoser(Range.getBegin(), diag::metafn_undeduced_placeholder)
               << Spec << Spec->getType() << Range;

    APValue RV = makeReflection(Spec);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, RV);
  }
  if (auto *VTD = dyn_cast<VarTemplateDecl>(TDecl)) {
    TArgs.clear();
    expandTemplateArgPacks(ExpandedTArgs, TArgs);

    VarDecl *Spec = Meta.Substitute(VTD, TArgs, Range.getBegin());
    assert(Spec && "substitution failed after validating arguments?");

    APValue RV = makeReflection(Spec);
    //C.recordCachedSubstitution(SubstitutionHash, RV);
    return SetAndSucceed(Result, makeReflection(Spec));
  }
  if (auto *CD = dyn_cast<ConceptDecl>(TDecl)) {
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


/// [conv.qual]/3: whether a prvalue of type 'From' converts to 'To' by a
/// qualification conversion. Top-level qualifiers of a prvalue are ignored.
static bool isQualificationConversion(ASTContext &C, QualType From,
                                      QualType To) {
  From = From.getCanonicalType();
  To = To.getCanonicalType();

  // Whether 'To' is const at every level 1..i-1 seen so far ([conv.qual]/3.2
  // and /3.3 require that wherever the decompositions differ).
  bool ConstAbove = true;
  for (unsigned Level = 0;; ++Level) {
    Qualifiers FromQ, ToQ;
    From = C.getUnqualifiedArrayType(From, FromQ);
    To = C.getUnqualifiedArrayType(To, ToQ);

    if (Level > 0) {
      if (!ToQ.compatiblyIncludes(FromQ, C))
        return false;
      if (FromQ != ToQ && !ConstAbove)
        return false;
    }

    const auto *FromPtr = From->getAs<PointerType>();
    const auto *ToPtr = To->getAs<PointerType>();
    const auto *FromMemPtr = From->getAs<MemberPointerType>();
    const auto *ToMemPtr = To->getAs<MemberPointerType>();
    const ArrayType *FromArr = C.getAsArrayType(From);
    const ArrayType *ToArr = C.getAsArrayType(To);

    if (FromPtr && ToPtr) {
      From = FromPtr->getPointeeType();
      To = ToPtr->getPointeeType();
    } else if (FromMemPtr && ToMemPtr) {
      if (!declaresSameEntity(FromMemPtr->getMostRecentCXXRecordDecl(),
                              ToMemPtr->getMostRecentCXXRecordDecl()))
        return false;
      From = FromMemPtr->getPointeeType();
      To = ToMemPtr->getPointeeType();
    } else if (FromArr && ToArr) {
      const auto *FromCAT = dyn_cast<ConstantArrayType>(FromArr);
      const auto *ToCAT = dyn_cast<ConstantArrayType>(ToArr);
      if (FromCAT && ToCAT) {
        if (FromCAT->getSize() != ToCAT->getSize())
          return false;
      } else if (FromCAT && isa<IncompleteArrayType>(ToArr)) {
        // /3.3: array of known bound to array of unknown bound needs const
        // at every level 1..i.
        if (Level == 0 || !ConstAbove || !ToQ.hasConst())
          return false;
      } else if (!isa<IncompleteArrayType>(FromArr) ||
                 !isa<IncompleteArrayType>(ToArr)) {
        return false;
      }
      From = FromArr->getElementType();
      To = ToArr->getElementType();
    } else {
      return C.hasSameType(From, To);
    }

    if (Level > 0)
      ConstAbove = ConstAbove && ToQ.hasConst();
  }
}

/// [meta.reflection.extract]/5.2: a reference of type 'T&' may be bound to a
/// variable or object of type 'U' only through a qualification conversion,
/// expressed as is_convertible_v<U(*)[], T(*)[]>.
static bool isReferenceCompatible(ASTContext &C, QualType U, QualType T) {
  QualType UArr = C.getPointerType(C.getIncompleteArrayType(
      U.getNonReferenceType(), ArraySizeModifier::Normal, 0));
  QualType TArr = C.getPointerType(C.getIncompleteArrayType(
      T.getNonReferenceType(), ArraySizeModifier::Normal, 0));
  return isQualificationConversion(C, UArr, TArr);
}

/// The function type 'FnTy' without a non-throwing exception specification:
/// [meta.reflection.extract]/7.2 and /7.3 accept a function "of type F or
/// F noexcept" for a pointer (to member) of type F.
static QualType withoutNoexcept(ASTContext &C, QualType FnTy) {
  if (const auto *FPT = FnTy->getAs<FunctionProtoType>();
      FPT && FPT->isNothrow())
    return C.getFunctionTypeWithExceptionSpec(FnTy, EST_None);
  return FnTy;
}

static bool isSameFunctionTypeAllowingNoexcept(ASTContext &C, QualType FnTy,
                                               QualType T) {
  return C.hasSameType(FnTy, T) || C.hasSameType(withoutNoexcept(C, FnTy), T);
}

/// [meta.reflection.extract]/10.1, /10.2: whether a value of type 'U' can be
/// extracted as a value of type 'T': for a pointer, through a qualification
/// conversion or a function pointer conversion ([conv.fctptr]); otherwise the
/// cv-unqualified types must be the same.
static bool isValueExtractableAs(ASTContext &C, QualType U, QualType T) {
  if (C.hasSameUnqualifiedType(U, T))
    return true;
  if (!U->isPointerType() || !T->isPointerType())
    return false;

  QualType UP = U->getPointeeType(), TP = T->getPointeeType();
  if (UP->isFunctionType() && TP->isFunctionType())
    return C.hasSameType(withoutNoexcept(C, UP), TP);
  return isQualificationConversion(C, U, T);
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

    if (!isValueExtractableAs(C, LambdaPtrTy, ResultTy))
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

  // [meta.reflection.extract]/12: a non-reference extraction from a variable
  // or object is extract-value(constant_of(r)); for an array, constant_of is
  // reflect_constant_array, so the pointer obtained under /10.3 designates
  // the promoted copy rather than the original array. The wrapper passes the
  // templates that constant_of needs for that after the reflection.
  if (!ReturnsLValue) {
    QualType Ty;
    if (RV.isReflectedObject())
      Ty = RV.getTypeOfReflectedResult(C);
    else if (RV.isReflectedDecl())
      if (auto *VD = dyn_cast<VarDecl>(RV.getReflectedDecl()))
        Ty = VD->getType().getNonReferenceType();

    if (!Ty.isNull() && Ty->isArrayType()) {
      APValue Promoted;
      if (constant_of(Promoted, C, Meta, Evaluator, Diagnoser, AllowInjection,
                      C.MetaInfoTy, Range, Args.slice(1), ContainingDecl))
        return true;
      RV = Promoted;
    }
  }

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Object: {
    QualType ObjectTy = RV.getTypeOfReflectedResult(C);

    if (auto *RD = ObjectTy->getAsCXXRecordDecl();
        RD && RD->isLambda() && ResultTy->isPointerType())
      return extractLambda(Result, RD);

    if (ReturnsLValue ? !isReferenceCompatible(C, ObjectTy, ResultTy)
                      : !isValueExtractableAs(C, ObjectTy, ResultTy))
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

    if (!isValueExtractableAs(C, ValueTy, ResultTy))
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

    if (!isValueExtractableAs(C, A->getArg()->getType(), ResultTy))
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
        if (!RawResultTy->isReferenceType() ||
            !isReferenceCompatible(C, Decl->getType(), RawResultTy))
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 1 << Decl->getType() << 1 << ResultTy << Range;

        NestedNameSpecifierLocBuilder NNSLocBuilder;
        if (auto *ParentClsDecl = dyn_cast_or_null<CXXRecordDecl>(
                Decl->getDeclContext())) {
          TypeSourceInfo *TSI = C.CreateTypeSourceInfo(
                  QualType(ParentClsDecl->getTypeForDecl(), 0), 0);
          NNSLocBuilder.Extend(C, TSI->getTypeLoc(), Range.getBegin());
        }
        Synthesized = DeclRefExpr::Create(C, NNSLocBuilder.getTemporary(),
                                          SourceLocation(), Decl, false,
                                          Range.getBegin(), ResultTy, VK_LValue,
                                          Decl, nullptr);
      } else if (auto *ArrTy = dyn_cast<ArrayType>(Decl->getType());
                 ArrTy && !ReturnsLValue) {
        // [meta.reflection.extract]/10.3: an array (by now the promoted copy
        // from reflect_constant_array) is extracted as a pointer to its first
        // element, 'remove_extent_t<U>*' and T being similar and convertible.
        QualType Elt = ArrTy->getElementType();
        if (auto *VD = dyn_cast<VarDecl>(Decl)) {
          if (VD->isConstexpr()) {
            Elt.addConst();
          }
        }

        if (!RawResultTy->isPointerType() ||
            !isQualificationConversion(C, C.getPointerType(Elt), RawResultTy))
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 1 << C.getPointerType(Elt) << 1 << ResultTy << Range;

        APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
        return SetAndSucceed(Result,
                             APValue(Decl, CharUnits::Zero(), Path, false));
      } else if (ReturnsLValue) {
        // [meta.reflection.extract]/5: a reference to the object declared by
        // the (possibly local) variable; only a qualification conversion from
        // its type to T is allowed (/5.2).
        if (!isReferenceCompatible(C, Decl->getType(), ResultTy))
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 1 << Decl->getType() << 1 << ResultTy << Range;

        Synthesized = ExtractLValueExpr::Create(C, Range, ResultTy, Decl);
      } else {
        // We have a reflection of a (possibly local) non-reference variable.
        // Synthesize an lvalue by reaching up the call stack.
        if (!isValueExtractableAs(C, Decl->getType(), ResultTy))
          return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
              << 0 << Decl->getType() << ReturnsLValue << ResultTy << Range;

        Synthesized = ExtractLValueExpr::Create(C, Range, ResultTy, Decl);
      }

      return !Evaluator(Result, Synthesized, !ReturnsLValue);
    } else if (isa<BindingDecl>(Decl)) {
      return Diagnoser(Range.getBegin(),
                       diag::metafn_extract_structured_binding) << Range;

    } else if (ReturnsLValue) {
      // Only variables may be returned as LValues.
      return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract)
          << 1 << DescriptionOf(RV);
    } else if (isa<FieldDecl, CXXMethodDecl>(Decl)) { // Extracting a non-static member as a pointer.
      // [meta.reflection.extract]/7.3: a static or explicit object member
      // function is extracted as a pointer to function.
      if (auto *MD = dyn_cast<CXXMethodDecl>(Decl);
          MD && (MD->isStatic() || MD->isExplicitObjectMemberFunction())) {
        if (!ResultTy->isPointerType() ||
            !isSameFunctionTypeAllowingNoexcept(C, MD->getType(),
                                                ResultTy->getPointeeType()))
          return Diagnoser(Range.getBegin(),
                           diag::metafn_extract_entity_type_mismatch)
              << ResultTy << DescriptionOf(RV)
              << C.getPointerType(MD->getType()) << Range;
        APValue StaticFuncPtrLV(Decl, CharUnits::Zero(), {}, false, false);
        return SetAndSucceed(Result, StaticFuncPtrLV);
      }

      if (auto *FD = dyn_cast<FieldDecl>(Decl); FD && FD->isBitField())
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract) << 2
            << DescriptionOf(RV) << Range;

      DeclContext *ObjDC = Decl->getDeclContext();
      while (ObjDC &&
             [](DeclContext *DC) {
               if (auto *RD = dyn_cast<CXXRecordDecl>(DC))
                 return RD->isAnonymousStructOrUnion();
               else return DC->isTransparentContext();
             }(ObjDC))
      if (isa<TranslationUnitDecl>(ObjDC))
        // Can happen if Target was a member of a static anonymous union at
        // namespace scope.
        return Diagnoser(Range.getBegin(), diag::metafn_cannot_extract) << 2
            << "a field that is not a member of a class";
      else
        ObjDC = ObjDC->getParent();

      // [meta.reflection.extract]/7.1: for a data member of type X, T and
      // 'X C::*' must be similar and 'X C::*' convertible to T (a
      // qualification conversion); /7.2: for a member function of type F or
      // F noexcept, T must be 'F C::*'.
      QualType MemPtrTy = C.getMemberPointerType(Decl->getType(), nullptr,
                                                 cast<CXXRecordDecl>(ObjDC));
      bool Matches;
      if (isa<FieldDecl>(Decl)) {
        Matches = isQualificationConversion(C, MemPtrTy, ResultTy);
      } else {
        const auto *RMP = ResultTy->getAs<MemberPointerType>();
        Matches = RMP &&
                  declaresSameEntity(RMP->getMostRecentCXXRecordDecl(),
                                     cast<CXXRecordDecl>(ObjDC)) &&
                  isSameFunctionTypeAllowingNoexcept(C, Decl->getType(),
                                                     RMP->getPointeeType());
      }
      if (!Matches)
        return Diagnoser(Range.getBegin(),
                         diag::metafn_extract_entity_type_mismatch)
            << ResultTy << DescriptionOf(RV) << MemPtrTy << Range;

      APValue MemPtrLV(Decl, false, ArrayRef<const CXXRecordDecl *> {});
      return SetAndSucceed(Result, MemPtrLV);
    } else if (auto *ECD = dyn_cast<EnumConstantDecl>(Decl)) {
      if (!C.hasSameUnqualifiedType(ECD->getType(), ResultTy))
        return Diagnoser(Range.getBegin(), diag::metafn_extract_type_mismatch)
            << 2 << Decl->getType() << 0 << ResultTy << Range;

      return SetAndSucceed(Result, APValue(ECD->getInitVal()));
    } else {
      // [meta.reflection.extract]/7.3: a non-member function of type F or
      // F noexcept is extracted as a pointer of type F*.
      if (!ResultTy->isPointerType() ||
          !isSameFunctionTypeAllowingNoexcept(C, Decl->getType(),
                                              ResultTy->getPointeeType()))
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
  case ReflectionKind::Annotation:
  case ReflectionKind::Namespace:
  case ReflectionKind::Attribute:
    return SetAndSucceed(Result, makeBool(C, false));
  }
  llvm_unreachable("invalid reflection type");
}

template <AccessSpecifier AS>
static inline
bool is_ClassMember_ACCESS(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
  [[maybe_unused]] bool scratch
    = is_class_member(Result, C, Meta, Evaluator, Diagnoser,
                      AllowInjection, ResultTy, Range, Args,
                      ContainingDecl);

  if (const bool isClassMember = Result.getInt().getBoolValue();isClassMember) {
    return is_ACCESS<AS>(Result, C, Meta, Evaluator, Diagnoser,
                                AllowInjection, ResultTy, Range, Args,
                                ContainingDecl);
  }
  // fallthrough: base-class relationship
  scratch = is_base(Result, C, Meta, Evaluator, Diagnoser,
                    AllowInjection, ResultTy, Range, Args,
                    ContainingDecl);
  if (const bool isBaseClass = Result.getInt().getBoolValue();isBaseClass) {
    return is_ACCESS<AS>(Result, C, Meta, Evaluator, Diagnoser,
                                AllowInjection, ResultTy, Range, Args,
                                ContainingDecl);
  }
  return false;
}

bool is_public(APValue &Result, ASTContext &C, MetaActions &Meta,
               EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
               QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
               Decl *ContainingDecl) {
    return is_ClassMember_ACCESS<AS_public>(
      Result, C, Meta, Evaluator, Diagnoser,
      AllowInjection, ResultTy, Range, Args,
      ContainingDecl);
}

bool is_protected(APValue &Result, ASTContext &C, MetaActions &Meta,
                  EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                  QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                  Decl *ContainingDecl) {
  return is_ClassMember_ACCESS<AS_protected>(
    Result, C, Meta, Evaluator, Diagnoser,
    AllowInjection, ResultTy, Range, Args,
    ContainingDecl);
}

bool is_private(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  return is_ClassMember_ACCESS<AS_private>(
    Result, C, Meta, Evaluator, Diagnoser,
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
  case ReflectionKind::Parameter:
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
  else if (RV.isReflectedDecl()) {
    if (auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      Meta.EnsureInstantiationOfExceptionSpec(Range.getBegin(), FD);

    IsNoexcept = isFunctionOrMethodNoexcept(RV.getReflectedDecl()->getType());
  }

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

bool is_final(APValue &Result, ASTContext &C, MetaActions &Meta,
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
    if (auto * recordDecl = dyn_cast<CXXRecordDecl>(RV.getReflectedType()->getAsCXXRecordDecl())) {
      result = recordDecl->hasAttr<FinalAttr>();
    }
  } else if (RV.isReflectedDecl()) {
    if (auto * funcDecl = dyn_cast<CXXMethodDecl>(RV.getReflectedDecl())){
      result = funcDecl->hasAttr<FinalAttr>();
    }
    else if (auto * recordDecl = dyn_cast<CXXRecordDecl>(RV.getReflectedDecl())) {
      result = recordDecl->hasAttr<FinalAttr>();
    }
  }
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
  case ReflectionKind::Parameter:
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
    bool result = isConstQualifiedType(RV.getReflectedDecl()->getType());
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
  case ReflectionKind::Parameter:
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
    bool result = isVolatileQualifiedType(RV.getReflectedDecl()->getType());
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
    if (const auto *FT = RV.getReflectedType()->getAs<FunctionProtoType>())
      result = (FT->getRefQualifier() == RQ_LValue);
  } else if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      if (const auto *FT = FD->getType()->getAs<FunctionProtoType>())
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
    if (const auto *FT = RV.getReflectedType()->getAs<FunctionProtoType>())
      result = (FT->getRefQualifier() == RQ_RValue);
  } else if (RV.isReflectedDecl()) {
    if (const auto *FD = dyn_cast<FunctionDecl>(RV.getReflectedDecl()))
      if (const auto *FT = FD->getType()->getAs<FunctionProtoType>())
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

/// [meta.reflection.queries]/27: the linkage of the name of the variable,
/// function, type, template or namespace that 'RV' represents, or
/// std::nullopt if 'RV' represents nothing whose name has linkage
/// ([basic.link]/4): a non-static data member, enumerator, structured
/// binding, object, value, type alias, and so on.
static std::optional<Linkage> linkageOf(ASTContext &C, APValue RV) {
  RV = MaybeUnproxy(C, RV, /*Dealias=*/false);

  switch (RV.getReflectionKind()) {
  case ReflectionKind::Type: {
    // A type alias is not a type with a name of its own; a class or
    // enumeration has the linkage of its name (including a typedef name for
    // linkage purposes), regardless of cv-qualification.
    QualType QT = RV.getReflectedType();
    if (isTypeAlias(QT))
      return std::nullopt;
    if (NamedDecl *D = findTypeDecl(QT))
      return D->getFormalLinkage();
    return std::nullopt;
  }
  case ReflectionKind::Declaration: {
    Decl *D = RV.getReflectedDecl();
    if (!isa<VarDecl, FunctionDecl>(D))
      return std::nullopt;
    return cast<NamedDecl>(D)->getFormalLinkage();
  }
  case ReflectionKind::Template:
    return RV.getReflectedTemplate().getAsTemplateDecl()->getFormalLinkage();
  case ReflectionKind::Namespace: {
    // [basic.link]/4: an unnamed namespace, or a namespace declared within
    // one, has internal linkage; all other namespaces (the global namespace
    // included) have external linkage.
    Decl *D = RV.getReflectedNamespace();
    if (isa<TranslationUnitDecl>(D))
      return Linkage::External;
    if (auto *A = dyn_cast<NamespaceAliasDecl>(D))
      D = A->getNamespace();
    return cast<NamedDecl>(D)->getFormalLinkage();
  }
  case ReflectionKind::Null:
  case ReflectionKind::Object:
  case ReflectionKind::Value:
  case ReflectionKind::Parameter:
  case ReflectionKind::BaseSpecifier:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return std::nullopt;
  case ReflectionKind::EntityProxy:
    llvm_unreachable("proxies should already have been unwrapped");
  }
  llvm_unreachable("unknown reflection kind");
}

/// 'Linkage::None' selects "any linkage" ([meta.reflection.queries]/27,
/// has_linkage).
template <Linkage L>
static bool has_LINKAGE(APValue &Result, ASTContext &C, EvalFn Evaluator,
                        ArrayRef<Expr *> Args) {
  assert(Args[0]->getType()->isReflectionType());

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  std::optional<Linkage> Found = linkageOf(C, RV);
  bool result = Found && (L == Linkage::None ? *Found != Linkage::None
                                             : *Found == L);
  return SetAndSucceed(Result, makeBool(C, result));
}

bool has_internal_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(ResultTy == C.BoolTy);
  return has_LINKAGE<Linkage::Internal>(Result, C, Evaluator, Args);
}

bool has_module_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                        EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                        QualType ResultTy, SourceRange Range,
                        ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  assert(ResultTy == C.BoolTy);
  return has_LINKAGE<Linkage::Module>(Result, C, Evaluator, Args);
}

bool has_external_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  assert(ResultTy == C.BoolTy);
  return has_LINKAGE<Linkage::External>(Result, C, Evaluator, Args);
}

bool has_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(ResultTy == C.BoolTy);
  return has_LINKAGE<Linkage::None>(Result, C, Evaluator, Args);
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
    // For unscoped enumerators, parent_of will return its enumeration type
    // We need now to lookup context on that type
    if (Scratch.isReflectedType() && Scratch.getReflectedType()->isUnscopedEnumerationType()) {
      Decl *D = findTypeDecl(Scratch.getReflectedType());
      result = D && D->getDeclContext() && D->getDeclContext()->isRecord();
    } else {
      result = Scratch.isReflectedType() &&
              Scratch.getReflectedType()->isRecordType();
    }
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
    // Desugar aliases first (exactly as the members_of family does): on a
    // TypedefType, findTypeDecl returns the alias declaration, for which
    // EnsureInstantiated is a no-op -- a never-yet-instantiated (but
    // perfectly instantiable) specialization named through an alias then
    // wrongly reported as incomplete.
    QualType QT = desugarType(RV.getReflectedType(), /*UnwrapAliases=*/true,
                              /*DropCV=*/false, /*DropRefs=*/false);

    // If this is a declared type with a reachable definition, ensure that the
    // type is instantiated.
    if (Decl *typeDecl = findTypeDecl(QT))
      (void) Meta.EnsureInstantiated(typeDecl, Range);

    result = !QT->isIncompleteType();
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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

  // 'std::meta::info' is a structural type, so 'reflect_constant' accepts a
  // reflection and yields a reflection of it. Nothing else caps how many times
  // that can be repeated, so check here before the depth counter would
  // overflow.
  if (!Arg.canLift())
    return Diagnoser(Range.getBegin(), diag::metafn_reflection_depth_exceeded)
        << APValue::MaxReflectionDepth << Range;

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

  // Next is `N` and then { attr_i, ..., attr_N }
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  llvm::SmallVector<ParsedAttr *, 2> Attributes;
  if (int64_t N = Scratch.getInt().getExtValue(); N > 0) {
    for (int64_t i = 0; i < N; ++i) {
      llvm::APInt Idx(C.getTypeSize(C.getSizeType()), i, false);
      Expr *indexExpr = IntegerLiteral::Create(C, Idx, C.getSizeType(),
                                               Args[ArgIdx]->getExprLoc());
      Expr *arraySubExpr =
          new (C) ArraySubscriptExpr(Args[ArgIdx], indexExpr, C.getUnsignedWCharType(), VK_LValue,
                                     OK_Ordinary, Range.getBegin());
      if (!Evaluator(Scratch, arraySubExpr, true))
        return true;
      if (!Scratch.isReflectedAttribute()) {
        return DiagnoseReflectionKind(Diagnoser, Range, "a reflection of an attribute", DescriptionOf(Scratch));
      }
      Attributes.push_back(Scratch.getReflectedAttribute());
    }
  }

  TagDataMemberSpec *TDMS = new (C) TagDataMemberSpec {
    MemberTy, Name, Alignment, BitWidth, NoUniqueAddress, Attributes
  };
  return SetAndSucceed(Result, makeReflection(TDMS));
}

bool enumerator_spec(APValue &Result, ASTContext &C, MetaActions &Meta,
                      EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                      QualType ResultTy, SourceRange Range,
                      ArrayRef<Expr *> Args, Decl *ContainingDecl) {
  // ARGS: {
  //   ^^const char, s.size(), s.data(),
  //   val,
  //   annotations.size(), annotations.begin(),
  //   attributes.size(), attributes.begin());
  // }
  APValue Scratch;
  int ArgIdx = 0;

  // Evaluate the character type.
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  QualType CharTy = Scratch.getReflectedType();

  std::string Name;
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  size_t nameLen = Scratch.getInt().getExtValue();
  Name.resize(nameLen);
  Name[nameLen]='\0';
  // Why cant i make EvaluateCharRangeAsString work ?...
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

    Name[k] = static_cast<char>(Scratch.getInt().getExtValue());
  }
  ArgIdx++;
  // Value of the enumerator
  APValue Val;
  if (!Evaluator(Val, Args[ArgIdx++], true))
    return true;

  // annotations.size(), annotations.begin()
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  size_t nbAnnotReflections = Scratch.getInt().getExtValue();
  SmallVector<APValue*, 2> annotations;
  for (uint64_t k = 0; k < nbAnnotReflections; ++k) {
    llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
    Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(), Args[ArgIdx]->getExprLoc());

    Synthesized = new (C) ArraySubscriptExpr(Args[ArgIdx], Synthesized,
                                              C.getUnsignedWCharType(), VK_LValue, OK_Ordinary,
                                              Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
      return true;
    if (!Evaluator(Scratch, Synthesized, true))
      return true;
    if (!Scratch.isReflectedValue() && !Scratch.isReflectedObject()) {
      return DiagnoseReflectionKind(Diagnoser, Range, "a reflected constant", DescriptionOf(Scratch));
    }
    annotations.push_back(new (C) APValue(Scratch));
  }
  ArgIdx++;

  // attributes.size(), // attributes.begin()
  if (!Evaluator(Scratch, Args[ArgIdx++], true))
    return true;
  size_t nbAttrReflections = Scratch.getInt().getExtValue();
  SmallVector<APValue*, 2> attributes;
  for (uint64_t k = 0; k < nbAttrReflections; ++k) {
    llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
    Expr *Synthesized = IntegerLiteral::Create(C, Idx, C.getSizeType(), Args[ArgIdx]->getExprLoc());

    Synthesized = new (C) ArraySubscriptExpr(Args[ArgIdx], Synthesized,
                                              C.MetaInfoTy, VK_LValue, OK_Ordinary,
                                              Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
      return true;
    if (!Evaluator(Scratch, Synthesized, true))
      return true;
    if (!Scratch.isReflectedAttribute()) {
      return DiagnoseReflectionKind(Diagnoser, Range, "a reflection of an attribute", DescriptionOf(Scratch));
    }
    attributes.push_back(new (C) APValue(ReflectionKind::Attribute, Scratch.getReflectedAttribute()));
  }
  ArgIdx++; // = end()

  EnumeratorSpec * ES = new (C) EnumeratorSpec{
    Name,
    !Val.isNullReflection(),
    Val.isNullReflection() ? 0: Val.getInt().getExtValue(),
    annotations,
    attributes
  };
  return SetAndSucceed(Result, makeReflection(ES));
}

bool is_enumerator_spec(APValue &Result, ASTContext &C,
                         MetaActions &Meta, EvalFn Evaluator,
                         DiagFn Diagnoser, bool AllowInjection,
                         QualType ResultTy, SourceRange Range,
                         ArrayRef<Expr *> Args, Decl *ContainingDecl)
{
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedEnumMemberSpec()));
}

static bool defineEnumImpl(APValue &Result, ASTContext &C, MetaActions &Meta,
                           EvalFn Evaluator, DiagFn Diagnoser,
                           bool AllowInjection, SourceRange Range,
                           ArrayRef<Expr *> Args, Decl *ContainingDecl,
                           bool RequireScoped) {
  if (!AllowInjection) {
    return Diagnoser(Range.getBegin(),
                     diag::metafn_injected_decl_non_plainly_consteval);
  }
  assert(Args[0]->getType()->isReflectionType());
  APValue Scratch;
  if (!Evaluator(Scratch, Args[0], true)) {
    return true;
  }

  // Checking we have the reflection of an enum type as first arg.
  if (!Scratch.isReflectedType()) {
    return DiagnoseReflectionKind(Diagnoser, Range, "an enum type",
                                  DescriptionOf(Scratch));
  }
  QualType TargetEnum = Scratch.getReflectedType();
  EnumDecl *foundDecl = llvm::dyn_cast<EnumDecl>(findTypeDecl(TargetEnum));
  if (!foundDecl) {
    return DiagnoseReflectionKind(Diagnoser, Range, "an enum type",
                                  DescriptionOf(Scratch));
  }

  // define_enum only completes scoped enums; define_unscoped_enum only
  // completes unscoped (C-style) enums.
  if (foundDecl->isScoped() != RequireScoped) {
    return Diagnoser(Range.getBegin(),
                     RequireScoped ? diag::metafn_enum_not_scoped
                                   : diag::metafn_enum_not_unscoped)
           << TargetEnum.getAsString();
  }

  // Need to check we only have a fwd declare enum
  if (!foundDecl->enumerators().empty()) {
    // Diagnostic on found enumerators
    return Diagnoser(Range.getBegin(), diag::metafn_enum_already_complete)
           << TargetEnum.getAsString();
  }

  // Get nb of enumerators spec.
  if (!Evaluator(Scratch, Args[1], true))
    return true;
  size_t NumEnumerators = static_cast<size_t>(Scratch.getInt().getExtValue());
  SmallVector<EnumeratorSpec *, 8> EnumSpecs;
  llvm::FoldingSetNodeID ID;
  llvm::StringSet<> MemberNames;
  for (size_t k = 0; k < NumEnumerators; ++k) {
    // Extract the reflection from the list of member specs.
    llvm::APInt Idx(C.getTypeSize(C.getSizeType()), k, false);
    Expr *Synthesized =
        IntegerLiteral::Create(C, Idx, C.getSizeType(), Args[2]->getExprLoc());

    Synthesized =
        new (C) ArraySubscriptExpr(Args[2], Synthesized, C.MetaInfoTy,
                                   VK_LValue, OK_Ordinary, Range.getBegin());
    if (Synthesized->isValueDependent() || Synthesized->isTypeDependent())
      return true;

    if (!Evaluator(Scratch, Synthesized, true))
      return true;
    if (!Scratch.isReflectedEnumMemberSpec())
      return DiagnoseReflectionKind(
          Diagnoser, Range, "a description of an enumerator for 'define_enum'",
          DescriptionOf(Scratch));
    EnumSpecs.push_back(Scratch.getReflectedEnumeratorSpec());
  }

  EnumDecl *completedEnum =
      Meta.DefineEnum(foundDecl, EnumSpecs, ContainingDecl,
                                TargetEnum, nullptr, Args[0]->getExprLoc());
  if (!completedEnum) {
    return true;
  }
  return SetAndSucceed(Result, makeReflection(completedEnum));
}

bool define_enum(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  return defineEnumImpl(Result, C, Meta, Evaluator, Diagnoser, AllowInjection,
                        Range, Args, ContainingDecl, /*RequireScoped=*/true);
}

bool define_unscoped_enum(APValue &Result, ASTContext &C, MetaActions &Meta,
                          EvalFn Evaluator, DiagFn Diagnoser,
                          bool AllowInjection, QualType ResultTy,
                          SourceRange Range, ArrayRef<Expr *> Args,
                          Decl *ContainingDecl) {
  return defineEnumImpl(Result, C, Meta, Evaluator, Diagnoser, AllowInjection,
                        Range, Args, ContainingDecl, /*RequireScoped=*/false);
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
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
  case ReflectionKind::Parameter:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
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
  case ReflectionKind::Parameter:
  case ReflectionKind::Attribute:
  case ReflectionKind::Annotation:
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

    if (const FieldDecl *FD = dyn_cast<const FieldDecl>(VD)) {
      if (FD->isBitField())
        return true;
    }
    size_t Align = C.getDeclAlign(VD, false).getQuantity();

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
  case ReflectionKind::Parameter:
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
    if (const auto *FT = RV.getReflectedType()->getAs<FunctionProtoType>()) {
      unsigned numParams = FT->getNumParams();
      if (idx >= numParams)
        return SetAndSucceed(Result, Sentinel);

      // [meta.reflection.queries]/62.2: the types in the parameter-type-list
      // of the function type, which [dcl.fct]/5 forms by deleting top-level
      // cv-qualifiers (array and function types are already adjusted to
      // pointers in the FunctionProtoType). The list holds types, not type
      // aliases, so an alias used in the declarator is looked through.
      QualType ParamTy = desugarType(FT->getParamType(idx),
                                     /*UnwrapAliases=*/true, /*DropCV=*/true,
                                     /*DropRefs=*/false);
      return SetAndSucceed(Result, makeReflection(ParamTy));
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
  case ReflectionKind::Parameter:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return true;
  }
  return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
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
  case ReflectionKind::Parameter:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
      << 5 << DescriptionOf(RV) << Range;
  case ReflectionKind::Type:
    if (const auto *FPT = RV.getReflectedType()->getAs<FunctionProtoType>()) {
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
  case ReflectionKind::Parameter: {
    ParmVarDecl *PVD = getMostRecentParmVarDecl(RV.getReflectedParameter());
    return SetAndSucceed(Result, makeBool(C, PVD->hasDefaultArg()));
  }
  case ReflectionKind::Declaration:
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
  if (RV.isReflectedParameter())
    result = RV.getReflectedParameter()->isExplicitObjectParameter();
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

  return SetAndSucceed(Result, makeBool(C, RV.isReflectedParameter()));
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
    if (const auto *FPT = RV.getReflectedType()->getAs<FunctionProtoType>()) {
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
      // A function whose type contains an undeduced placeholder type has no
      // type ([meta.reflection.queries]/1), and so no return type.
      if (FD->getReturnType()->isUndeducedType())
        return Diagnoser(Range.getBegin(), diag::metafn_undeduced_return_type)
            << DescriptionOf(RV) << Range;

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
  case ReflectionKind::Parameter:
  case ReflectionKind::DataMemberSpec:
  case ReflectionKind::Annotation:
  case ReflectionKind::Attribute:
    return Diagnoser(Range.getBegin(), diag::metafn_cannot_query_property)
        << 6 << DescriptionOf(RV) << Range;
  }
  llvm_unreachable("unknown reflection kind");
}

bool variable_of(APValue &Result, ASTContext &C, MetaActions &Meta,
                 EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                 QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                 Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.MetaInfoTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  if (!RV.isReflectedParameter())
    return DiagnoseReflectionKind(Diagnoser, Range, "a function parameter",
                                  DescriptionOf(RV));

  ParmVarDecl *PVD = RV.getReflectedParameter();
  FunctionDecl *FD = cast<FunctionDecl>(PVD->getDeclContext());

  Decl *EvaluationContext = ContainingDecl ? ContainingDecl : Meta.CurrentCtx();
  FunctionDecl *EvaluationFunction = dyn_cast<FunctionDecl>(EvaluationContext);
  if (!EvaluationFunction)
    EvaluationFunction =
        dyn_cast_or_null<FunctionDecl>(EvaluationContext->getDeclContext());
  if (!EvaluationFunction || EvaluationFunction->getCanonicalDecl() !=
                                 FD->getCanonicalDecl())
    return true;
  assert(FD->getDefinition());
  PVD = FD->getDefinition()->getParamDecl(PVD->getFunctionScopeIndex());

  APValue Var(ReflectionKind::Declaration, PVD);
  return SetAndSucceed(Result, Var);
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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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

  // [meta.reflection.scope]/3.5: a point in a consteval block is evaluated at
  // the point inhabited by the outermost enclosing consteval block, so the
  // function call operator of the closure type of a consteval block is
  // transparent.
  while (auto *MD = dyn_cast<CXXMethodDecl>(Ctx)) {
    if (!MD->getParent()->isConstevalBlockLambda())
      break;
    Ctx = cast<Decl>(
        MD->getParent()->getLexicalDeclContext()->getNonTransparentContext());
  }

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
  case ReflectionKind::Parameter:
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
  case ReflectionKind::Parameter:
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

      TemplateArgument TArg = TArgFromReflection(C, Meta, Evaluator, RV,
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
  case ReflectionKind::Parameter:
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

  return SetAndSucceedWithLift(Result, Diagnoser, Range, EvalResult.Val,
                               CallExpr->getType());
}

// -----------------------------------------------------------------------------
// P3867: define_encoded_static_string
// -----------------------------------------------------------------------------

static std::optional<StringLiteralKind>
stringLiteralKindForCharType(ASTContext &C, QualType CharTy) {
  if (CharTy->isWideCharType())
    return StringLiteralKind::Wide;
  if (CharTy->isChar8Type())
    return StringLiteralKind::UTF8;
  if (CharTy->isChar16Type())
    return StringLiteralKind::UTF16;
  if (CharTy->isChar32Type())
    return StringLiteralKind::UTF32;
  // `isCharType()` also matches signed/unsigned char; P3867 only allows `char`.
  if (C.hasSameUnqualifiedType(CharTy, C.CharTy))
    return StringLiteralKind::Ordinary;
  return std::nullopt;
}

bool define_encoded_static_string(APValue &Result, ASTContext &C,
                                  [[maybe_unused]] MetaActions &Meta,
                                  EvalFn Evaluator, DiagFn Diagnoser,
                                  [[maybe_unused]] bool AllowInjection,
                                  QualType ResultTy, SourceRange Range,
                                  ArrayRef<Expr *> Args,
                                  [[maybe_unused]] Decl *ContainingDecl) {
  // ResultTy is `const CharT *`, spliced from the first argument.
  if (ResultTy.isNull() || !ResultTy->isPointerType())
    return Diagnoser(Range.getBegin(),
                     diag::metafn_encoded_string_invalid_char_type)
           << ResultTy << Range;

  QualType CharTy = ResultTy->getPointeeType().getUnqualifiedType();
  std::optional<StringLiteralKind> Kind =
      stringLiteralKindForCharType(C, CharTy);
  if (!Kind)
    return Diagnoser(Range.getBegin(),
                     diag::metafn_encoded_string_invalid_char_type)
           << CharTy << Range;

  APValue SizeV;
  if (!Evaluator(SizeV, Args[2], /*ConvertToRValue=*/true))
    return true;
  uint64_t Len = SizeV.getInt().getZExtValue();

  APValue DataV;
  if (!Evaluator(DataV, Args[1], /*ConvertToRValue=*/false))
    return true;

  std::string Utf8;
  Expr::EvalResult Status;
  if (!Args[1]->EvaluateCharRangeAsString(Utf8, Len, DataV, C, Status))
    return true;

  unsigned CharByteWidth = C.getTypeSizeInChars(CharTy).getQuantity();
  assert(CharByteWidth == 1 || CharByteWidth == 2 || CharByteWidth == 4);

  // ConvertUTF8toWide requires WideCharWidth * (Source.size() + 1) bytes.
  llvm::SmallVector<char, 32> Encoded;
  Encoded.resize((Utf8.size() + 1) * CharByteWidth);
  char *Ptr = Encoded.data();
  const llvm::UTF8 *ErrorPtr = nullptr;
  if (!llvm::ConvertUTF8toWide(CharByteWidth, Utf8, Ptr, ErrorPtr))
    return Diagnoser(Range.getBegin(),
                     diag::metafn_encoded_string_conversion_failed)
           << CharTy << Range;

  unsigned ByteLength = static_cast<unsigned>(Ptr - Encoded.data());
  assert(ByteLength % CharByteWidth == 0 && "partial output code unit");
  // ConvertUTF8toWide does not write a terminator; include the extra zeroed
  // code unit reserved by the resize above, matching Sema string literals.
  ByteLength += CharByteWidth;
  unsigned NumChars = ByteLength / CharByteWidth;

  // getStringLiteralArrayType adds a trailing NUL to the array type.
  QualType StrLitTy = C.getStringLiteralArrayType(CharTy, NumChars - 1);
  Expr *StrLit =
      StringLiteral::Create(C, StringRef(Encoded.data(), ByteLength), *Kind,
                            /*Pascal=*/false, StrLitTy, SourceLocation{});

  APValue::LValuePathEntry Path[1] = {APValue::LValuePathEntry::ArrayIndex(0)};
  return SetAndSucceed(Result, APValue(StrLit, CharUnits::Zero(), Path, false));
}

// -----------------------------------------------------------------------------
// [meta.reflection.queries] has_parent, has_c_language_linkage
// -----------------------------------------------------------------------------

// [meta.reflection.queries]/50: 'has_parent(r)' is true exactly when
// 'parent_of(r)' would not throw ([meta.reflection.queries]/52), so reuse
// 'parent_of' itself with a null diagnoser rather than duplicate its rules.
bool has_parent(APValue &Result, ASTContext &C, MetaActions &Meta,
                EvalFn Evaluator, DiagFn Diagnoser, bool AllowInjection,
                QualType ResultTy, SourceRange Range, ArrayRef<Expr *> Args,
                Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  // Evaluate the argument first so that a non-constant argument fails the
  // call instead of being reported as "no parent".
  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;

  APValue Scratch;
  bool Failed = parent_of(Scratch, C, Meta, Evaluator, /*Diagnoser=*/nullptr,
                          AllowInjection, C.MetaInfoTy, Range, Args,
                          ContainingDecl);
  return SetAndSucceed(Result, makeBool(C, !Failed));
}

// [meta.reflection.queries]/28: true if 'r' represents a variable, function,
// or function type with C language linkage. Clang does not distinguish
// function types by language linkage (an 'extern "C"' function type is the
// same type as the C++ one), so a function type never reports C linkage.
bool has_c_language_linkage(APValue &Result, ASTContext &C, MetaActions &Meta,
                            EvalFn Evaluator, DiagFn Diagnoser,
                            bool AllowInjection, QualType ResultTy,
                            SourceRange Range, ArrayRef<Expr *> Args,
                            Decl *ContainingDecl) {
  assert(Args[0]->getType()->isReflectionType());
  assert(ResultTy == C.BoolTy);

  APValue RV;
  if (!Evaluator(RV, Args[0], true))
    return true;
  RV = MaybeUnproxy(C, RV);

  bool result = false;
  if (RV.isReflectedDecl()) {
    Decl *D = RV.getReflectedDecl();
    if (auto *FD = dyn_cast<FunctionDecl>(D))
      result = FD->isExternC();
    else if (auto *VD = dyn_cast<VarDecl>(D))
      result = VD->isExternC();
  }
  return SetAndSucceed(Result, makeBool(C, result));
}

}  // end namespace clang
