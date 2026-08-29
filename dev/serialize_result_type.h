#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#endif

namespace sqlite_orm::internal {
    using serialize_result_type = std::string_view;
    using serialize_arg_type = std::string_view;
    using string_constant_type = std::string_view;
}
