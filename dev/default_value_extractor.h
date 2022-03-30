#pragma once

#include <memory>  //  std::unique_ptr
#include <string>  //  std::string

#include "constraints.h"
#include "serializer_context.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class I>
        std::string serialize(const T& t, const serializer_context<I>& context);

        /**
         *  This class is used in tuple iteration to know whether tuple constains `default_value_t`
         *  constraint class and what it's value if it is
         */
        struct default_value_extractor {

            template<class A>
            std::unique_ptr<std::string> operator()(const A&) const {
                return {};
            }

            template<class T>
            std::unique_ptr<std::string> operator()(const default_t<T>& t) const {
                storage_impl<> storage;
                serializer_context<storage_impl<>> context{storage};
                return std::make_unique<std::string>(serialize(t.value, context));
            }
        };

    }

}
