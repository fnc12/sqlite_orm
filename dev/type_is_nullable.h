#pragma once

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
    template<class T>
    using type_is_nullable = polyfill::disjunction<
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        polyfill::is_specialization_of<T, std::optional>,
#endif
        polyfill::is_specialization_of<T, std::unique_ptr>,
        polyfill::is_specialization_of<T, std::shared_ptr>>;

}
