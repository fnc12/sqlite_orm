#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same, std::is_arithmetic, std::is_base_of
#include <functional>  //  std::reference_wrapper
#include <optional>  //  std::optional
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl.h"
#include "functional/type_traits.h"  //  common_type_of_t
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_fy.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "member_traits/member_traits.h"
#include "vocabulary/node_traits.h"
#include "vocabulary/node_algorithms.h"  //  substitute_arguments, is_bindable_v, is_text_value
#include "mapped_type_proxy.h"
#include "column_result_proxy.h"
#include "ast/alias.h"
#include "cte_types.h"
#include "storage_traits.h"
#include "schema/algorithms/table_lookup.h"  // schema_pick_table_t
#include "ast/app_function.h"

namespace sqlite_orm::internal {
    /**
     *  Obtains the result type of expressions that form the columns of a select statement.
     *  
     *  This is a proxy class used to define what type must have result type depending on select
     *  arguments (member pointer, aggregate functions, etc). Below you can see specializations
     *  for different types. E.g. specialization for internal::length_t has `type` int cause
     *  LENGTH returns INTEGER in sqlite. Every column_result_t must have `type` type that equals
     *  c++ SELECT return type for T
     *  DBOs - db_objects_tuple type
     *  T - C++ type
     *  SFINAE - sfinae argument
     */
    template<class DBOs, class T, class SFINAE = void>
    struct column_result_t {
#ifdef __FUNCTION__
        // produce an error message that reveals `T` and `DBOs`
        static constexpr bool reveal() {
            static_assert(polyfill::always_false_v<T>, "T not found in DBOs - " __FUNCTION__);
        }
        static constexpr bool trigger = reveal();
#endif
    };

    template<class DBOs, class T>
    using column_result_of_t = typename column_result_t<DBOs, T>::type;

    template<class DBOs, class Tpl>
    using column_result_for_tuple_t = transform_tuple_t<Tpl, mpl::bind_front_fn<column_result_of_t, DBOs>::template fn>;

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_as_optional, T>> {
        using type = std::optional<column_result_of_t<DBOs, expression_type_t<T>>>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, std::optional<T>, void> {
        using type = std::optional<T>;
    };

    /**
     *  Result for the most simple queries like `SELECT 1`
     */
    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<std::is_arithmetic, T>> {
        using type = T;
    };

    /**
     *  Result for the most simple queries like `SELECT 'ototo'`
     */
    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_text_value, T>> {
        using type = std::string;
    };

    /**
     *  The concatenated results of a list of column expressions, with a tuple result of a single expression
     *  spliced into the sequence rather than nested in it.
     */
    template<class DBOs, class... Args>
    struct column_result_t<DBOs, std::tuple<Args...>, void> : conc_tuple<tuplify_t<column_result_of_t<DBOs, Args>>...> {
    };

    template<class DBOs, class T>
    struct column_result_t<
        DBOs,
        T,
        std::enable_if_t<std::disjunction<is_any_in<T>, is_between<T>, is_is_null<T>, is_is_not_null<T>>::value>> {
        using type = bool;
    };

    template<class DBOs, class T>
    struct column_result_t<
        DBOs,
        T,
        std::enable_if_t<std::disjunction<is_current_time<T>, is_current_date<T>, is_current_timestamp<T>>::value>> {
        using type = std::string;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<std::is_member_pointer, T>> : member_field_type<T> {};

    /*
     *  The result type of a built-in function's call argument, as the return type placeholders see it:
     *  a bindable value stands for itself - except a text value, which yields `std::string`
     *  like a select of it does -, anything else (a member pointer, a column pointer, a nested expression)
     *  for its column result.
     */
    template<class DBOs, class Arg>
    using argument_result_of_t = typename std::conditional_t<is_bindable_v<Arg> && !is_text_value<Arg>::value,
                                                             polyfill::type_identity<Arg>,
                                                             column_result_t<DBOs, Arg>>::type;

    /**
     *  The declared return type of a built-in function, with the `argument<I>` and `common_argument_type<I...>`
     *  placeholders replaced by the results of the call arguments.
     */
    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_builtin_function_call, T>>
        : substitute_arguments<return_type_t<T>, args_tuple_t<T>, mpl::bind_front_fn<argument_result_of_t, DBOs>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_app_function_call, T>> {
        using type = typename callable_arguments<udf_type_t<T>>::return_type;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_count_asterisk, T>> {
        using type = int;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_filtered_aggregate_function, T>>
        : column_result_t<DBOs, function_type_t<T>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_over, T>> : column_result_t<DBOs, function_type_t<T>> {};

    template<class DBOs>
    struct column_result_t<DBOs, std::nullptr_t, void> {
        using type = std::nullptr_t;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_rowset_deduplicator, T>> : column_result_t<DBOs, expression_type_t<T>> {
    };

    //  note: an assignment has no result type - it is no column expression
    template<class DBOs, class T>
    struct column_result_t<DBOs,
                           T,
                           std::enable_if_t<std::disjunction<is_binary_operator<T>, is_unary_operator<T>>::value>> {
        using type = result_type_t<T>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_any_rowid, T>> {
        using type = int64;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_alias_column, T>> : column_result_t<DBOs, column_type_t<T>> {};

    /*
     *  The result of a column pointer: that of its field -
     *  or, for a column alias referenced in a CTE, the type of the column the CTE maps it to.
     */
    template<class DBOs, class CP, bool = is_alias_holder_v<field_type_t<CP>>>
    struct column_pointer_result : column_result_t<DBOs, field_type_t<CP>> {};

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    template<class DBOs, class CP>
    struct column_pointer_result<DBOs, CP, true> {
        using table_type = schema_pick_table_t<type_t<CP>, DBOs>;
        using cte_mapper_type = cte_mapper_type_t<table_type>;

        // lookup the column alias in the final column references
        using colalias_index = find_tuple_type<typename cte_mapper_type::final_colrefs_tuple, field_type_t<CP>>;
        static_assert(colalias_index::value < std::tuple_size_v<typename cte_mapper_type::final_colrefs_tuple>,
                      "No such column mapped into the CTE");
        using type = std::tuple_element_t<colalias_index::value, typename cte_mapper_type::fields_type>;
    };
#endif

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_column_pointer, T>> : column_pointer_result<DBOs, T> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_columns, T>> : column_result_t<DBOs, columns_type_t<T>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_struct, T>> {
        using type = structure<object_type_t<T>, column_result_of_t<DBOs, columns_type_t<T>>>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_select, T>> : column_result_t<DBOs, return_type_t<T>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_compound_operator, T>> {
        using type = polyfill::detected_t<common_type_of_t, column_result_for_tuple_t<DBOs, expressions_tuple_t<T>>>;
        static_assert(!std::is_same<type, polyfill::nonesuch>::value,
                      "Compound select statements must return a common type");
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_binary_condition, T>> {
        using type = result_type_t<T>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_as_node, T>> : column_result_t<DBOs, expression_type_t<T>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_asterisk, T>>
        : storage_traits::storage_mapped_columns<DBOs, mapped_type_proxy_t<type_t<T>>> {};

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_object_node, T>> {
        using type = table_reference<type_t<T>>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_cast, T>> {
        using type = to_type_t<T>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, T, match_if<is_case_expression, T>> {
        using type = return_type_t<T>;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs,
                           T,
                           std::enable_if_t<std::disjunction<is_like<T>, is_glob<T>, is_negated_condition<T>>::value>> {
        using type = bool;
    };

    template<class DBOs, class T>
    struct column_result_t<DBOs, std::reference_wrapper<T>, void> : column_result_t<DBOs, T> {};
}
