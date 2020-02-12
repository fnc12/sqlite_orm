#pragma once

#include <tuple>  //  std::tuple
#include <utility>  //  std::pair
#include <functional>  //  std::reference_wrapper

#include "conditions.h"
#include "operators.h"
#include "select_constraints.h"
#include "prepared_statement.h"
#include "optional_container.h"
#include "core_functions.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct node_tuple {
            using type = std::tuple<T>;
        };

        template<>
        struct node_tuple<void, void> {
            using type = std::tuple<>;
        };

        template<class T>
        struct node_tuple<std::reference_wrapper<T>, void> {
            using type = typename node_tuple<T>::type;
        };

        template<class C>
        struct node_tuple<conditions::where_t<C>, void> {
            using node_type = conditions::where_t<C>;
            using type = typename node_tuple<C>::type;
        };

        template<class T>
        struct node_tuple<T,
                          typename std::enable_if<is_base_of_template<T, conditions::binary_condition>::value>::type> {
            using node_type = T;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_node_tuple = typename node_tuple<left_type>::type;
            using right_node_tuple = typename node_tuple<right_type>::type;
            using type = typename conc_tuple<left_node_tuple, right_node_tuple>::type;
        };

        template<class L, class R, class... Ds>
        struct node_tuple<binary_operator<L, R, Ds...>, void> {
            using node_type = binary_operator<L, R, Ds...>;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_node_tuple = typename node_tuple<left_type>::type;
            using right_node_tuple = typename node_tuple<right_type>::type;
            using type = typename conc_tuple<left_node_tuple, right_node_tuple>::type;
        };

        template<class... Args>
        struct node_tuple<columns_t<Args...>, void> {
            using node_type = columns_t<Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };

        template<class L, class A>
        struct node_tuple<conditions::in_t<L, A>, void> {
            using node_type = conditions::in_t<L, A>;
            using left_tuple = typename node_tuple<L>::type;
            using right_tuple = typename node_tuple<A>::type;
            using type = typename conc_tuple<left_tuple, right_tuple>::type;
        };

        template<class T>
        struct node_tuple<T, typename std::enable_if<is_base_of_template<T, compound_operator>::value>::type> {
            using node_type = T;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_tuple = typename node_tuple<left_type>::type;
            using right_tuple = typename node_tuple<right_type>::type;
            using type = typename conc_tuple<left_tuple, right_tuple>::type;
        };

        template<class T, class... Args>
        struct node_tuple<select_t<T, Args...>, void> {
            using node_type = select_t<T, Args...>;
            using columns_tuple = typename node_tuple<T>::type;
            using args_tuple = typename conc_tuple<typename node_tuple<Args>::type...>::type;
            using type = typename conc_tuple<columns_tuple, args_tuple>::type;
        };

        template<class T, class... Args>
        struct node_tuple<get_all_t<T, Args...>, void> {
            using node_type = get_all_t<T, Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };

        template<class T, class... Args>
        struct node_tuple<get_all_pointer_t<T, Args...>, void> {
            using node_type = get_all_pointer_t<T, Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct node_tuple<get_all_optional_t<T, Args...>, void> {
            using node_type = get_all_optional_t<T, Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class... Args, class... Wargs>
        struct node_tuple<update_all_t<set_t<Args...>, Wargs...>, void> {
            using node_type = update_all_t<set_t<Args...>, Wargs...>;
            using set_tuple = typename conc_tuple<typename node_tuple<Args>::type...>::type;
            using conditions_tuple = typename conc_tuple<typename node_tuple<Wargs>::type...>::type;
            using type = typename conc_tuple<set_tuple, conditions_tuple>::type;
        };

        template<class T, class... Args>
        struct node_tuple<remove_all_t<T, Args...>, void> {
            using node_type = remove_all_t<T, Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };

        template<class T>
        struct node_tuple<conditions::having_t<T>, void> {
            using node_type = conditions::having_t<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class T, class E>
        struct node_tuple<conditions::cast_t<T, E>, void> {
            using node_type = conditions::cast_t<T, E>;
            using type = typename node_tuple<E>::type;
        };

        template<class T>
        struct node_tuple<conditions::exists_t<T>, void> {
            using node_type = conditions::exists_t<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class T>
        struct node_tuple<optional_container<T>, void> {
            using node_type = optional_container<T>;
            using type = typename node_tuple<T>::type;
        };

        template<>
        struct node_tuple<optional_container<void>, void> {
            using node_type = optional_container<void>;
            using type = std::tuple<>;
        };

        template<class A, class T, class E>
        struct node_tuple<conditions::like_t<A, T, E>, void> {
            using node_type = conditions::like_t<A, T, E>;
            using arg_tuple = typename node_tuple<A>::type;
            using pattern_tuple = typename node_tuple<T>::type;
            using escape_tuple = typename node_tuple<E>::type;
            using type = typename conc_tuple<arg_tuple, pattern_tuple, escape_tuple>::type;
        };

        template<class A, class T>
        struct node_tuple<conditions::glob_t<A, T>, void> {
            using node_type = conditions::glob_t<A, T>;
            using arg_tuple = typename node_tuple<A>::type;
            using pattern_tuple = typename node_tuple<T>::type;
            using type = typename conc_tuple<arg_tuple, pattern_tuple>::type;
        };

        template<class A, class T>
        struct node_tuple<conditions::between_t<A, T>, void> {
            using node_type = conditions::between_t<A, T>;
            using expression_tuple = typename node_tuple<A>::type;
            using lower_tuple = typename node_tuple<T>::type;
            using upper_tuple = typename node_tuple<T>::type;
            using type = typename conc_tuple<expression_tuple, lower_tuple, upper_tuple>::type;
        };

        template<class T>
        struct node_tuple<conditions::named_collate<T>, void> {
            using node_type = conditions::named_collate<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class T>
        struct node_tuple<conditions::is_null_t<T>, void> {
            using node_type = conditions::is_null_t<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class T>
        struct node_tuple<conditions::is_not_null_t<T>, void> {
            using node_type = conditions::is_not_null_t<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class C>
        struct node_tuple<conditions::negated_condition_t<C>, void> {
            using node_type = conditions::negated_condition_t<C>;
            using type = typename node_tuple<C>::type;
        };

        template<class R, class S, class... Args>
        struct node_tuple<core_function_t<R, S, Args...>, void> {
            using node_type = core_function_t<R, S, Args...>;
            using type = typename conc_tuple<typename node_tuple<Args>::type...>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::left_join_t<T, O>, void> {
            using node_type = conditions::left_join_t<T, O>;
            using type = typename node_tuple<O>::type;
        };

        template<class T>
        struct node_tuple<conditions::on_t<T>, void> {
            using node_type = conditions::on_t<T>;
            using type = typename node_tuple<T>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::join_t<T, O>, void> {
            using node_type = conditions::join_t<T, O>;
            using type = typename node_tuple<O>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::left_outer_join_t<T, O>, void> {
            using node_type = conditions::left_outer_join_t<T, O>;
            using type = typename node_tuple<O>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::inner_join_t<T, O>, void> {
            using node_type = conditions::inner_join_t<T, O>;
            using type = typename node_tuple<O>::type;
        };

        template<class R, class T, class E, class... Args>
        struct node_tuple<simple_case_t<R, T, E, Args...>, void> {
            using node_type = simple_case_t<R, T, E, Args...>;
            using case_tuple = typename node_tuple<T>::type;
            using args_tuple = typename conc_tuple<typename node_tuple<Args>::type...>::type;
            using else_tuple = typename node_tuple<E>::type;
            using type = typename conc_tuple<case_tuple, args_tuple, else_tuple>::type;
        };

        template<class L, class R>
        struct node_tuple<std::pair<L, R>, void> {
            using node_type = std::pair<L, R>;
            using left_tuple = typename node_tuple<L>::type;
            using right_tuple = typename node_tuple<R>::type;
            using type = typename conc_tuple<left_tuple, right_tuple>::type;
        };

        template<class T, class E>
        struct node_tuple<as_t<T, E>, void> {
            using node_type = as_t<T, E>;
            using type = typename node_tuple<E>::type;
        };

        template<class T>
        struct node_tuple<conditions::limit_t<T, false, false, void>, void> {
            using node_type = conditions::limit_t<T, false, false, void>;
            using type = typename node_tuple<T>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::limit_t<T, true, false, O>, void> {
            using node_type = conditions::limit_t<T, true, false, O>;
            using type = typename conc_tuple<typename node_tuple<T>::type, typename node_tuple<O>::type>::type;
        };

        template<class T, class O>
        struct node_tuple<conditions::limit_t<T, true, true, O>, void> {
            using node_type = conditions::limit_t<T, true, true, O>;
            using type = typename conc_tuple<typename node_tuple<O>::type, typename node_tuple<T>::type>::type;
        };
    }
}
