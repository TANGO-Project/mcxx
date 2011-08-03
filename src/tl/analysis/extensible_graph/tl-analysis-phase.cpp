/*--------------------------------------------------------------------
(C) Copyright 2006-2009 Barcelona Supercomputing Center 
Centro Nacional de Supercomputacion

This file is part of Mercurium C/C++ source-to-source compiler.

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
#include "tl-analysis-phase.hpp"
#include "tl-cfg-visitor.hpp"
#include "tl-scopelink.hpp"


namespace TL
{
    AnalysisPhase::AnalysisPhase()
    {
        set_phase_name("Experimental phase for analysis");
        set_phase_description("This phase builds a Control Flow Graph and performs different analysis on demand. ");
    }
    
    void AnalysisPhase::run(TL::DTO& dto)
    {
        RefPtr<Nodecl::NodeclBase> nodecl = RefPtr<Nodecl::NodeclBase>::cast_dynamic(dto["nodecl"]);
        
        CfgVisitor cfg_visitor(dto["scope_link"]);
        cfg_visitor.walk(*nodecl);
    }
}

EXPORT_PHASE(TL::AnalysisPhase);