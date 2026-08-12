#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#endif

namespace sqlite_orm::internal {
    struct column_identifier {

        /**
         *  Column name.
         */
        std::string name;
    };
}
