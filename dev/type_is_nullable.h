#pragma once

#include <type_traits>  //  std::false_type, std::true_type, std::enable_if
#include <memory>  //  std::shared_ptr, std::unique_ptr
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    /**
     *  This is class that tells `sqlite_orm` that type is nullable. Nullable types
     *  are mapped to sqlite database as `NULL` and not-nullable are mapped as `NOT NULL`.
     *  Default nullability status for all types is `NOT NULL`. So if you want to map
     *  custom type as `NULL` (for example: boost::optional) you have to create a specialiation
     *  of type_is_nullable for your type and derive from `std::true_type`.
     */
    template<class T, class SFINAE = void>
    struct type_is_nullable : std::false_type {
        bool operator()(const T&) const {
            return true;
        }
    };

    /**
     *  This is a specialization for std::shared_ptr, std::unique_ptr, std::optional, which are nullable in sqlite_orm.
     */
    template<class T>
    struct type_is_nullable<T,
                            std::enable_if_t<polyfill::disjunction_v<
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                polyfill::is_specialization_of<T, std::optional>,
#endif
                                polyfill::is_specialization_of<T, std::unique_ptr>,
                                polyfill::is_specialization_of<T, std::shared_ptr>>>> : std::true_type {
        bool operator()(const T& t) const {
            return static_cast<bool>(t);
        }
    };

}
