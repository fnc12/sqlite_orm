#pragma once

#include <string>  //  std::string

#include "constraints.h"
#include "serializer_context.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class Ctx>
        auto serialize(const T& t, const Ctx& context);

        /**
         *  Serialize a column's default value.
         */
        template<class T>
        std::string serialize_default_value(const default_t<T>& dft) {
            db_objects_tuple<> dbObjects;
            serializer_context<db_objects_tuple<>> context{dbObjects};
            return serialize(dft.value, context);
        }

    }

}
