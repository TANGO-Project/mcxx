#include "codegen-cxx.hpp"
#include "cxx-codegen.h"
#include <cstdio>

// This is a legacy codegen phase. It is here just for A/B testing with the existing code

namespace Codegen {

    std::string CxxLegacy::codegen(const Nodecl::NodeclBase& n)
    {
        if (this->is_file_output())
        {
            char *str = NULL;
            size_t size = 0;
            FILE* temporal_stream = ::open_memstream(&str, &size);

            ::c_cxx_codegen_translation_unit(temporal_stream, n.get_internal_nodecl());

            fclose(temporal_stream);

            std::string result = str;
            free(str);
            return result;
        }
        else
        {
            return c_cxx_codegen_to_str(n.get_internal_nodecl());
        }
    }
}

EXPORT_PHASE(Codegen::CxxLegacy)
