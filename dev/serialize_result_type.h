#pragma once

#include "functional/cxx_string_view.h"
#ifndef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <string>  //  std::string
#endif

namespace sqlite_orm {
    namespace internal {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        using serialize_result_type = std::string_view;
        using serialize_arg_type = std::string_view;
#else
        using serialize_result_type = std::string;
        using serialize_arg_type = const std::string&;
#endif
    }
}
