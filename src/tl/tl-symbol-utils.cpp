/*--------------------------------------------------------------------
  (C) Copyright 2006-2013 Barcelona Supercomputing Center
                          Centro Nacional de Supercomputacion

  This file is part of Mercurium C/C++ source-to-source compiler.

  See AUTHORS file in the top level directory for information
  regarding developers and contributors.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  Mercurium C/C++ source-to-source compiler is distributed in the hope
  that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public
  License along with Mercurium C/C++ source-to-source compiler; if
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/

#include "cxx-utils.h"
#include "fortran03-scope.h"

#include "tl-symbol-utils.hpp"
#include "tl-nodecl-visitor.hpp"
#include "tl-scope.hpp"

namespace SymbolUtils
{
    TL::Symbol new_function_symbol_for_deep_copy(TL::Symbol source, std::string name)
    {
        decl_context_t decl_context = source.get_scope().get_decl_context();

        TL::Symbol dest = TL::Scope(decl_context).new_symbol(name);
        dest.get_internal_symbol()->kind = SK_FUNCTION;
        symbol_entity_specs_set_is_user_declared(dest.get_internal_symbol(), 1);
        return dest;
    }

    TL::Symbol new_function_symbol(
            TL::Symbol function,
            const std::string& name)
    {
        ERROR_CONDITION(name == function.get_name(), "The name of the new function cannot be the name of the original function", 0);
        TL::ObjectList<TL::Type> parameter_types = function.get_type().parameters();

        TL::ObjectList<std::string> parameter_names;
        TL::ObjectList<TL::Symbol> function_related_symbols = function.get_related_symbols();
        for (TL::ObjectList<TL::Symbol>::iterator it = function_related_symbols.begin();
                it != function_related_symbols.end();
                it++)
        {
            parameter_names.append(it->get_name());
        }

        TL::Symbol new_function = SymbolUtils::new_function_symbol(
                function,
                name,
                function.get_type().returns(),
                parameter_names,
                parameter_types);

        return new_function;
    }

    TL::Symbol new_function_symbol(
            TL::Symbol current_function,
            const std::string& name,
            TL::Type return_type,
            TL::ObjectList<std::string> parameter_names,
            TL::ObjectList<TL::Type> parameter_types)
    {
        if (IS_FORTRAN_LANGUAGE && current_function.is_nested_function())
        {
            // Get the enclosing function
            current_function = current_function.get_scope().get_related_symbol();
        }

        decl_context_t decl_context = current_function.get_scope().get_decl_context();

        if (decl_context.template_parameters != NULL
                && decl_context.template_parameters->is_explicit_specialization)
        {
            decl_context.template_parameters = decl_context.template_parameters->enclosing;
        }

        ERROR_CONDITION(parameter_names.size() != parameter_types.size(), "Mismatch between names and types", 0);

        decl_context_t function_context;
        if (IS_FORTRAN_LANGUAGE)
        {
            function_context = new_program_unit_context(decl_context);
        }
        else
        {
            function_context = new_function_context(decl_context);
            function_context = new_block_context(function_context);
        }

        // Build the function type
        int num_parameters = 0;
        scope_entry_t** parameter_list = NULL;

        parameter_info_t* p_types = new parameter_info_t[parameter_types.size()];
        parameter_info_t* it_ptypes = &(p_types[0]);
        TL::ObjectList<TL::Type>::iterator type_it = parameter_types.begin();
        for (TL::ObjectList<std::string>::iterator it = parameter_names.begin();
                it != parameter_names.end();
                it++, it_ptypes++, type_it++)
        {
            scope_entry_t* param = new_symbol(function_context, function_context.current_scope, uniquestr(it->c_str()));
            symbol_entity_specs_set_is_user_declared(param, 1);
            param->kind = SK_VARIABLE;
            param->locus = make_locus("", 0, 0);

            param->defined = 1;

            param->type_information = get_unqualified_type(type_it->get_internal_type());
            if (type_it->is_restrict())
            {
                // Keep restrict
                param->type_information = get_restrict_qualified_type(param->type_information);
            }

            P_LIST_ADD(parameter_list, num_parameters, param);

            it_ptypes->is_ellipsis = 0;
            it_ptypes->nonadjusted_type_info = NULL;
            it_ptypes->type_info = get_indirect_type(param);
        }

        type_t *function_type = get_new_function_type(
                return_type.get_internal_type(),
                p_types,
                parameter_types.size(), REF_QUALIFIER_NONE);

        delete[] p_types;

        // Now, we can create the new function symbol
        scope_entry_t* new_function_sym = NULL;
        if (!current_function.get_type().is_template_specialized_type()
                || current_function.get_scope().get_template_parameters()->is_explicit_specialization)
        {
            new_function_sym = new_symbol(decl_context, decl_context.current_scope, uniquestr(name.c_str()));
            symbol_entity_specs_set_is_user_declared(new_function_sym, 1);
            new_function_sym->kind = SK_FUNCTION;
            new_function_sym->locus = make_locus("", 0, 0);
            new_function_sym->type_information = function_type;
        }
        else
        {
            scope_entry_t* new_template_sym = new_symbol(
                    decl_context, decl_context.current_scope, uniquestr(name.c_str()));
            new_template_sym->kind = SK_TEMPLATE;
            new_template_sym->locus = make_locus("", 0, 0);

            new_template_sym->type_information = get_new_template_type(
                    decl_context.template_parameters,
                    function_type,
                    uniquestr(name.c_str()),
                    decl_context, make_locus("", 0, 0));

            template_type_set_related_symbol(new_template_sym->type_information, new_template_sym);

            if (current_function.is_member())
            {
                symbol_entity_specs_set_is_member(new_template_sym, 1);
                symbol_entity_specs_set_class_type(new_template_sym, current_function.get_class_type().get_internal_type());
            }

            // The new function is the primary template specialization
            new_function_sym = named_type_get_symbol(
                    template_type_get_primary_type(
                        new_template_sym->type_information));
        }

        function_context.function_scope->related_entry = new_function_sym;
        function_context.block_scope->related_entry = new_function_sym;

        new_function_sym->related_decl_context = function_context;

        for (int i = 0; i < num_parameters; i++)
        {
            symbol_entity_specs_add_related_symbols(new_function_sym, parameter_list[i]);
            symbol_set_as_parameter_of_function(
                    parameter_list[i], new_function_sym,
                    /* parameter nesting */ 0,
                    /* parameter position */ i);
        }
        xfree(parameter_list); parameter_list = NULL;

        // Make it static
        symbol_entity_specs_set_is_static(new_function_sym, 1);

        // Make it member if the enclosing function is member
        if (current_function.is_member())
        {
            symbol_entity_specs_set_is_member(new_function_sym, 1);
            symbol_entity_specs_set_class_type(new_function_sym, current_function.get_class_type().get_internal_type());

            symbol_entity_specs_set_access(new_function_sym, AS_PUBLIC);

            // We make it as a declaration because we do not expect it to be defined inside class
            ::class_type_add_member(symbol_entity_specs_get_class_type(new_function_sym), new_function_sym,
                    /* is_declaration */ 0);
        }

        if (current_function.is_inline())
            symbol_entity_specs_set_is_inline(new_function_sym, 1);

        // symbol_entity_specs_set_is_defined_inside_class_specifier(new_function_sym,
        //     symbol_entity_specs_get_is_defined_inside_class_specifier(current_function.get_internal_symbol()));

        if (IS_FORTRAN_LANGUAGE && current_function.is_in_module())
        {
            scope_entry_t* module_sym = current_function.in_module().get_internal_symbol();
            symbol_entity_specs_set_in_module(new_function_sym, module_sym);
            symbol_entity_specs_add_related_symbols(module_sym,
                    new_function_sym);
            symbol_entity_specs_set_is_module_procedure(new_function_sym, 1);
        }

        // Result symbol
        if (function_type_get_return_type(new_function_sym->type_information) != NULL
                && !is_void_type(function_type_get_return_type(new_function_sym->type_information)))
        {
            const char* result_name = ".result";
            if (IS_FORTRAN_LANGUAGE)
            {
                result_name = new_function_sym->symbol_name;
            }
            scope_entry_t* result_sym = new_symbol(function_context, function_context.current_scope, uniquestr(result_name));
            result_sym->kind = SK_VARIABLE;
            result_sym->type_information = function_type_get_return_type(new_function_sym->type_information);
            symbol_entity_specs_set_is_result_var(result_sym, 1);

            symbol_entity_specs_set_result_var(new_function_sym, result_sym);
        }

        return new_function_sym;
    }

    TL::Symbol new_function_symbol(
            TL::Scope sc,
            const std::string& name,
            bool has_return,
            const std::string& return_symbol_name,
            TL::Type return_type,
            TL::ObjectList<std::string> parameter_names,
            TL::ObjectList<TL::Type> parameter_types)
    {
        decl_context_t decl_context = sc.get_decl_context();

        if (decl_context.template_parameters != NULL
                && decl_context.template_parameters->is_explicit_specialization)
        {
            decl_context.template_parameters = decl_context.template_parameters->enclosing;
        }

        scope_entry_t* entry = new_symbol(decl_context, decl_context.current_scope, uniquestr(name.c_str()));
        symbol_entity_specs_set_is_user_declared(entry, 1);

        entry->kind = SK_FUNCTION;
        entry->locus = make_locus("", 0, 0);

        ERROR_CONDITION(parameter_names.size() != parameter_types.size(), "Mismatch between names and types", 0);

        decl_context_t function_context ;
        if (IS_FORTRAN_LANGUAGE)
        {
            function_context = new_program_unit_context(decl_context);
        }
        else
        {
            function_context = new_function_context(decl_context);
            function_context = new_block_context(function_context);
        }
        function_context.function_scope->related_entry = entry;
        function_context.block_scope->related_entry = entry;

        entry->related_decl_context = function_context;

        parameter_info_t* p_types = new parameter_info_t[parameter_types.size()];

        parameter_info_t* it_ptypes = &(p_types[0]);
        TL::ObjectList<TL::Type>::iterator type_it = parameter_types.begin();
        for (TL::ObjectList<std::string>::iterator it = parameter_names.begin();
                it != parameter_names.end();
                it++, it_ptypes++, type_it++)
        {
            scope_entry_t* param = new_symbol(function_context, function_context.current_scope, uniquestr(it->c_str()));
            symbol_entity_specs_set_is_user_declared(param, 1);
            param->kind = SK_VARIABLE;
            param->locus = make_locus("", 0, 0);

            param->defined = 1;

            symbol_set_as_parameter_of_function(param, entry,
                    /* nesting */ 0,
                    /* position */ symbol_entity_specs_get_num_related_symbols(entry));

            param->type_information = get_unqualified_type(type_it->get_internal_type());
            if (type_it->is_restrict())
            {
                // Keep restrict
                param->type_information = get_restrict_qualified_type(param->type_information);
            }

            symbol_entity_specs_add_related_symbols(entry, param);

            it_ptypes->is_ellipsis = 0;
            it_ptypes->nonadjusted_type_info = NULL;
            it_ptypes->type_info = get_indirect_type(param);
        }

        if (has_return)
        {
            // Return symbol
            scope_entry_t* return_sym = new_symbol(function_context, function_context.current_scope, uniquestr(return_symbol_name.c_str()));
            symbol_entity_specs_set_is_user_declared(return_sym, 1);
            return_sym->kind = SK_VARIABLE;
            return_sym->locus = make_locus("", 0, 0);

            return_sym->defined = 1;

            symbol_entity_specs_set_is_result_var(return_sym, 1);

            return_sym->type_information = get_unqualified_type(return_type.get_internal_type());

            symbol_entity_specs_set_result_var(entry, return_sym);
        }

        // Type of the function
        type_t *function_type = get_new_function_type(
                return_type.get_internal_type(),
                p_types, parameter_types.size(),
                REF_QUALIFIER_NONE);

        entry->type_information = function_type;

        delete[] p_types;

        return entry;
    }

    void build_empty_body_for_function(
            TL::Symbol function_symbol,
            Nodecl::NodeclBase &function_code,
            Nodecl::NodeclBase &empty_stmt)
    {
        empty_stmt = Nodecl::EmptyStatement::make(make_locus("", 0, 0));
        Nodecl::List stmt_list = Nodecl::List::make(empty_stmt);

        if (IS_C_LANGUAGE || IS_CXX_LANGUAGE)
        {
            Nodecl::CompoundStatement compound_statement =
                Nodecl::CompoundStatement::make(stmt_list,
                        /* destructors */ Nodecl::NodeclBase::null(),
                        make_locus("", 0, 0));
            stmt_list = Nodecl::List::make(compound_statement);
        }

        Nodecl::NodeclBase context = Nodecl::Context::make(
                stmt_list,
                function_symbol.get_related_scope(), make_locus("", 0, 0));

        function_symbol.get_internal_symbol()->defined = 1;

        if (function_symbol.is_dependent_function())
        {
            function_code = Nodecl::TemplateFunctionCode::make(context,
                    // Initializers
                    Nodecl::NodeclBase::null(),
                    function_symbol,
                    make_locus("", 0, 0));
        }
        else
        {
            function_code = Nodecl::FunctionCode::make(context,
                    // Initializers
                    Nodecl::NodeclBase::null(),
                    function_symbol,
                    make_locus("", 0, 0));
        }

        symbol_entity_specs_set_function_code(
                function_symbol.get_internal_symbol(),
                function_code.get_internal_nodecl());
    }
}
