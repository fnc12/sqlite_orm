#pragma once

/** @file DSL-specific type name alias template projectors for syntactic sugar.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_reference
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../type_traits.h"
#include "../member_traits/member_traits.h"

// Plain accessors
namespace sqlite_orm::internal {
    template<typename T>
    using field_type_t = typename T::field_type;

    template<typename T>
    using constraints_type_t = typename T::constraints_type;

    template<typename T>
    using columns_type_t = typename T::columns_type;

    template<typename T>
    using columns_tuple_t = typename T::columns_tuple;

    template<typename T>
    using object_type_t = typename T::object_type;

    /**
     *  The mapped object type of the schema object a database object is defined on
     *  (e.g. of an index's or a trigger's target table).
     */
    template<typename T>
    using table_mapped_type_t = typename T::table_mapped_type;

    template<typename T>
    using elements_type_t = typename T::elements_type;

    template<typename T>
    using target_type_t = typename T::target_type;

    template<typename T>
    using left_type_t = typename T::left_type;

    template<typename T>
    using right_type_t = typename T::right_type;

    template<typename T>
    using on_type_t = typename T::on_type;

    template<typename T>
    using expression_type_t = typename T::expression_type;

    template<typename T>
    using args_type_t = typename T::args_type;

    template<typename T>
    using offset_expression_type_t = typename T::offset_expression_type;

    template<typename T>
    using expressions_tuple_t = typename T::expressions_tuple;

    template<typename T>
    using conditions_type_t = typename T::conditions_type;

    template<class As>
    using alias_type_t = typename As::alias_type;

    template<class T>
    using enclosing_type_t = typename T::enclosing_type;

    template<class T, class O>
    using enclosing_type_of_t = typename T::template _of<O>::enclosing_type;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<class T>
    using udf_type_t = typename T::udf_type;

    template<decltype(auto) a>
    using auto_udf_type_t = typename std::remove_reference_t<decltype(a)>::udf_type;
#endif

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    template<typename T>
    using cte_moniker_type_t = typename T::cte_moniker_type;

    template<typename T>
    using cte_mapper_type_t = typename T::cte_mapper_type;
#endif
}

// Detected-or-fallback accessors
namespace sqlite_orm::internal {
    template<class T>
    using field_type_or_type_t = polyfill::detected_or_t<T, type_t, member_field_type<T>>;

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    template<class T>
    using alias_holder_type_or_none_t = polyfill::detected_t<type_t, T>;
#endif
}
