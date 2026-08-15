#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#endif

namespace sqlite_orm::internal {
    struct table_identifier {

        /**
         *  Table name.
         */
        std::string name;
    };
}
