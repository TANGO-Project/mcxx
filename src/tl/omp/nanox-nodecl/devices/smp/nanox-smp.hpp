/*--------------------------------------------------------------------
  (C) Copyright 2006-2012 Barcelona Supercomputing Center
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



#ifndef NANOX_SMP_HPP
#define NANOX_SMP_HPP

#include "tl-compilerphase.hpp"
#include "tl-devices.hpp"

namespace TL
{
    namespace Nanox
    {
        class DeviceSMP : public DeviceProvider
        {
            public:

                virtual void run(DTO& dto);
                virtual void pre_run(DTO& dto);

                DeviceSMP();

                virtual ~DeviceSMP() { }

                virtual void phase_cleanup(DTO& data_flow);

                virtual void create_outline(
                        CreateOutlineInfo &info,
                        Nodecl::NodeclBase &outline_placeholder,
                        Nodecl::Utils::SymbolMap* &symbol_map);

                virtual void get_device_descriptor(
                        DeviceDescriptorInfo& info,
                        Source &ancillary_device_description,
                        Source &device_descriptor,
                        Source &fortran_dynamic_init);

            private:

                Nodecl::List _extra_c_code;

                Source emit_allocate_statement(
                        TL::Symbol sym,
                        int &lower_bound_index,
                        int &upper_bound_index);

                void add_forward_code_to_extra_c_code(
                        const std::string& outline_name,
                        TL::ObjectList<OutlineDataItem*> data_items,
                        Nodecl::NodeclBase parse_context);
        };
    }
}

#endif // NANOX_SMP_HPP