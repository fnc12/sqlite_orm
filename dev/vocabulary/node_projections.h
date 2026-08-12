#pragma once

/** @file DSL-specific type name alias template projectors for syntactic sugar.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_reference
#endif

#include "../functional/cxx_type_traits_polyfill.h"

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

    // T::alias_type or nonesuch
    template<class T>
    using alias_holder_type_or_none = polyfill::detected<type_t, T>;

    template<class T>
    using alias_holder_type_or_none_t = typename alias_holder_type_or_none<T>::type;
#endif
}
