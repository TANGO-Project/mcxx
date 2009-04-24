/*
    Mercurium C/C++ Compiler
    Copyright (C) 2006-2009 - Roger Ferrer Ibanez <roger.ferrer@bsc.es>
    Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
    Universitat Politecnica de Catalunya

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "cxx-process.h"
#include "cxx-utils.h"
#include "uniquestr.h"

// Compilation options
compilation_process_t compilation_process = { 0 };

void add_new_file_to_compilation_process(const char* file_path, const char* output_file, 
        compilation_configuration_t* configuration)
{
    translation_unit_t* translation_unit = (translation_unit_t*)calloc(1, sizeof(*translation_unit));
    // Initialize with the translation unit root tree
    translation_unit->input_filename = uniquestr(file_path);

    compilation_file_process_t *new_compiled_file = (compilation_file_process_t*) calloc(1, sizeof(*new_compiled_file));

    configuration->verbose = CURRENT_CONFIGURATION->verbose;
    configuration->do_not_link = CURRENT_CONFIGURATION->do_not_link;
    configuration->do_not_compile = CURRENT_CONFIGURATION->do_not_compile;
    configuration->do_not_prettyprint = CURRENT_CONFIGURATION->do_not_prettyprint;

    new_compiled_file->translation_unit = translation_unit;
    new_compiled_file->compilation_configuration = configuration;

    if ((configuration->do_not_link
            || configuration->do_not_compile)
            && output_file != NULL)
    {
        translation_unit->output_filename = uniquestr(output_file);
    }

    P_LIST_ADD(compilation_process.translation_units, 
            compilation_process.num_translation_units, 
            new_compiled_file);
}

unsigned long long int _bytes_dynamic_lists = 0;

unsigned long long dynamic_lists_used_memory(void)
{
    return _bytes_dynamic_lists;
}
