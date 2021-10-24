#pragma once

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <string_view>  //  string_view
#else
#include <string>  //  std::string
#endif

namespace sqlite_orm {
    namespace internal {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
        using serialize_result_type = std::string_view;
#else
        using serialize_result_type = std::string;
#endif
    }
}
