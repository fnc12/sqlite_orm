#pragma once

#include <string>  //  std::string

#include "constraints.h"
#include "serializer_context.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class I>
        std::string serialize(const T& t, const serializer_context<I>& context);

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
