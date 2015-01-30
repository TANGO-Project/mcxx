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

#ifndef TL_VECTORIZER_PREFETCHER_HPP
#define TL_VECTORIZER_PREFETCHER_HPP


#include "tl-vectorization-prefetcher-common.hpp"
#include "tl-vectorization-common.hpp"
#include "tl-nodecl.hpp"
#include "tl-nodecl-visitor.hpp"


namespace TL
{
    namespace Vectorization
    {
        typedef struct prefetch_info
        {
            int L1_distance;
            int L2_distance;
            bool enabled;

            prefetch_info() : L1_distance(0), L2_distance(0), enabled(false) {}
        } prefetch_info_t;        


        class Prefetcher : 
            public Nodecl::ExhaustiveVisitor<void>
        {
            private:
                const int _L1_distance;
                const int _L2_distance;
                const int _vectorization_factor;
                Nodecl::NodeclBase _object_init;
                Nodecl::NodeclBase _loop;
                objlist_nodecl_t _linear_vars;
                objlist_nodecl_t _not_nested_vaccesses;

                Nodecl::NodeclBase get_prefetch_node(
                        const Nodecl::NodeclBase& address,
                        const PrefetchKind kind,
                        const int distance);

            public:
                Prefetcher(const prefetch_info_t& pref_info,
                        const int vectorization_factor);

                void visit(const Nodecl::ForStatement& n);
                void visit(const Nodecl::VectorLoad& n);
                void visit(const Nodecl::VectorStore& n);
                void visit(const Nodecl::ObjectInit& n);
        };
    }
}

#endif // TL_VECTORIZER_PREFETCHER_HPP
