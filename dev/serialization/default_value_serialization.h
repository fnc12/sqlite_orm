#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#endif

#include "../schema/constraints/default.h"
#include "../schema/db_objects.h"
#include "serializer_context.h"

namespace sqlite_orm::internal {
    template<class T, class Ctx>
    auto serialize(const T& t, const Ctx& context);

    /**
     *  Purpose: write a column's default value as SQL text, apart from a statement.
     *
     *  Intention: serves `schema/column_definitions.h`: a column's default as the storage compares it with the
     *  database's when it syncs the schema; declares the serializer's entry point, which `statement_serializer.h`
     *  defines.
     */
    template<class T>
    std::string serialize_default_value(const default_t<T>& dft) {
        db_objects_tuple<> dbObjects;
        serializer_context<db_objects_tuple<>> context{dbObjects};
        return serialize(dft.value, context);
    }
}
