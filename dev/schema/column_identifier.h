#pragma once

/** @file
 *  Purpose: identify a column by its name.
 *
 *  Intention: a base of the column definitions in `schema/`.
 */

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
