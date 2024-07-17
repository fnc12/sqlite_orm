#pragma once

#ifndef _IMPORT_STD_MODULE
#include <string>  //  std::string
#endif

#include "constraints.h"
#include "serializer_context.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class C>
        auto serialize(const T& t, const C& context);

        /**
         *  Serialize default value of a column's default valu
         */
        template<class T>
        std::string serialize_default_value(const default_t<T>& dft) {
            db_objects_tuple<> dbObjects;
            serializer_context<db_objects_tuple<>> context{dbObjects};
            return serialize(dft.value, context);
        }

    }

}
