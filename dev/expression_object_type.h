#pragma once

#include <type_traits>  //  std::decay
#include <functional>  //  std::reference_wrapper

#include "prepared_statement.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct expression_object_type;

        template<class T>
        struct expression_object_type<update_t<T>> {
            using type = typename std::decay<T>::type;
        };

        template<class T>
        struct expression_object_type<update_t<std::reference_wrapper<T>>> {
            using type = typename std::decay<T>::type;
        };

        template<class T>
        struct expression_object_type<replace_t<T>> {
            using type = typename std::decay<T>::type;
        };

        template<class T>
        struct expression_object_type<replace_t<std::reference_wrapper<T>>> {
            using type = typename std::decay<T>::type;
        };

        template<class It, class L, class O>
        struct expression_object_type<replace_range_t<It, L, O>> {
            using type = typename replace_range_t<It, L, O>::object_type;
        };

        template<class It, class L, class O>
        struct expression_object_type<replace_range_t<std::reference_wrapper<It>, L, O>> {
            using type = typename replace_range_t<std::reference_wrapper<It>, L, O>::object_type;
        };

        template<class T>
        struct expression_object_type<insert_t<T>> {
            using type = typename std::decay<T>::type;
        };

        template<class T>
        struct expression_object_type<insert_t<std::reference_wrapper<T>>> {
            using type = typename std::decay<T>::type;
        };

        template<class It, class L, class O>
        struct expression_object_type<insert_range_t<It, L, O>> {
            using transformer_type = L;
            using type = typename insert_range_t<It, L, O>::object_type;
        };

        template<class It, class L, class O>
        struct expression_object_type<insert_range_t<std::reference_wrapper<It>, L, O>> {
            using type = typename insert_range_t<std::reference_wrapper<It>, L, O>::object_type;
        };

        template<class T, class... Cols>
        struct expression_object_type<insert_explicit<T, Cols...>> {
            using type = typename std::decay<T>::type;
        };

        template<class T, class... Cols>
        struct expression_object_type<insert_explicit<std::reference_wrapper<T>, Cols...>> {
            using type = typename std::decay<T>::type;
        };

        template<class T>
        struct get_ref_t {

            template<class O>
            auto& operator()(O& t) const {
                return t;
            }
        };

        template<class T>
        struct get_ref_t<std::reference_wrapper<T>> {

            template<class O>
            auto& operator()(O& t) const {
                return t.get();
            }
        };

        template<class T>
        auto& get_ref(T& t) {
            using arg_type = typename std::decay<T>::type;
            get_ref_t<arg_type> g;
            return g(t);
        }

        template<class T>
        struct get_object_t;

        template<class T>
        struct get_object_t<const T> : get_object_t<T> {};

        template<class T>
        auto& get_object(T& t) {
            using expression_type = typename std::decay<T>::type;
            get_object_t<expression_type> obj;
            return obj(t);
        }

        template<class T>
        struct get_object_t<replace_t<T>> {
            using expression_type = replace_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.obj);
            }
        };

        template<class T>
        struct get_object_t<insert_t<T>> {
            using expression_type = insert_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.obj);
            }
        };

        template<class T>
        struct get_object_t<update_t<T>> {
            using expression_type = update_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.obj);
            }
        };
    }
}
