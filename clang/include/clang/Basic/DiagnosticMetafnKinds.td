//==--- DiagnosticSemaKinds.td - P2996 Metafunction diagnostics -----------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let Component = "Metafn" in {

// General diagnostics.
def metafn_expected_reflection_of : Note<"expected a reflection of %0">;
def metafn_expected_reflection_of_but_got : Note<
  "expected a reflection of %0, but got %1">;
def metafn_no_associated_property : Note<
  "%0 has no %select{type|parent}1">;
def metafn_cannot_query_property : Note<"cannot query the "
  "%select{type|object|value|size|alignment|parameters|return type|"
           "annotations|attributes}0 of %1">;

// Ranges of entities.
def metafn_cannot_introspect_type : Note<
  "cannot introspect the %select{bases|members|parameters|alignment|size}0 of "
  "%select{an|a|a}1 %select{incomplete|non-class|non-function}1 type">;
def metafn_members_of_enum : Note<"did you mean to use 'enumerators_of'?">;

// Names.
def metafn_name_is_not_identifier : Note<
  "names of "
  "%select{template specializations|constructors|destructors|operators|"
           "conversion functions|constructor templates|operator templates|"
           "conversion function templates}0 are not identifiers">;
def metafn_inconsistent_name : Note<
  "reflected %0 has inconsistent identifiers between declarations">;
def metafn_name_of_unnamed_singleton: Note<
  "the %select{null reflection|global namespace}0 has no associated identifier">;
def metafn_cannot_have_name : Note<
  "reflection of %0 cannot have an associated identifier">;
def metafn_anonymous_entity : Note<
  "reflected %0 is anonymous and has no associated identifier">;
def metafn_not_an_operator : Note<"%0 is not an operator or operator template">;

// Parents.
def metafn_parent_of_undeclared : Note<
  "cannot query the parent of %0 with no declaration">;

// Substitution and invokation.
def metafn_cannot_be_arg : Note<
  "a reflection of %0 cannot represent a %select{function|template}1 argument">;
def metafn_cannot_invoke : Note<"cannot invoke %0">;
def metafn_no_specialization_found : Note<
  "no specialization of the function template %0 matched the provided "
  "arguments">;
def metafn_invalid_call_expr : Note<
  "invocation cannot be formed with the provided arguments">;
def metafn_returns_non_structural_type : Note<
  "invocation evaluates to a value of non-structural type %0, which cannot be "
  "represented as a reflection">;
def metafn_invocation_not_constant_expr : Note<
  "invocation is not a constant expression">;
def metafn_first_argument_is_not_object : Note<
  "expected related object reflection as a first argument for invoking "
  "non-static member function">;
def metafn_function_is_not_member_of_object : Note<
   "method is not a member of "
   "given object reflection">;
def metafn_function_returns_void : Note<
   "cannot invoke reflection of void-returning function">;


// Extraction.
def metafn_cannot_extract : Note<
  "cannot extract a %select{value|reference|pointer to member}0 from a "
  "reflection of %1">;
def metafn_extract_type_mismatch : Note<
  "reflected %select{value|object|enum constant|annotation}0 of type %1 cannot "
  "be extracted as a %select{value of|reference to}2 type %3">;
def metafn_extract_entity_type_mismatch : Note<
  "a pointer of type %0 cannot be extracted from a reflection of %1 whose "
  "pointer has type %2">;
def metafn_extract_structured_binding : Note<
  "extraction from a reflection of a structured binding is not allowed">;

// Reflection of expression results.
def metafn_value_not_structural_type : Note<
  "values of non-structural type %0 cannot be represented as reflections">;
def metafn_result_not_representable : Note<
  "provided %select{value|object}0 cannot be represented by a reflection">;
def metafn_p3385_trace_execution_checkpoint : Note<
  "(p3385-metafn) trace_execution_checkpoint %0">;
def metafn_p3385_non_standard_attribute : Warning<
  "(p3385-metafn) Non standard attribute discovered %0">;

// Class definition.
def metafn_name_invalid_identifier : Note<
  "provided name '%0' is not a valid identifier">;
def metafn_already_complete_type : Note<
  "%0 is already a complete type">;

// Annotation injection.
def metafn_cannot_annotate : Note<"cannot attach an annotation to %1">;

}  // end of metafn component
