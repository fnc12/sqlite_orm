#pragma once

/** @file
 *  Purpose: identify a table, in the wide sense, by its name.
 *
 *  Intention: a base of the ordinary table, view, virtual table and CTE nodes in `schema/`.
 */

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
