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
            schema_objects<> impl;
            serializer_context<schema_objects<>> context{impl};
            return serialize(dft.value, context);
        }

    }

}
