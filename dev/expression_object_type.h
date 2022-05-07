#pragma once

#include <type_traits>  //  std::decay
#include <functional>  //  std::reference_wrapper

#include "prepared_statement.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct expression_object_type;

        template<class T>
        struct expression_object_type<update_t<T>> : std::decay<T> {};

        template<class T>
        struct expression_object_type<update_t<std::reference_wrapper<T>>> : std::decay<T> {};

        template<class T>
        struct expression_object_type<replace_t<T>> : std::decay<T> {};

        template<class T>
        struct expression_object_type<replace_t<std::reference_wrapper<T>>> : std::decay<T> {};

        template<class It, class L, class O>
        struct expression_object_type<replace_range_t<It, L, O>> {
            using type = typename replace_range_t<It, L, O>::object_type;
        };

        template<class It, class L, class O>
        struct expression_object_type<replace_range_t<std::reference_wrapper<It>, L, O>> {
            using type = typename replace_range_t<std::reference_wrapper<It>, L, O>::object_type;
        };

        template<class T, class... Ids>
        struct expression_object_type<remove_t<T, Ids...>> {
            using type = T;
        };

        template<class T, class... Ids>
        struct expression_object_type<remove_t<std::reference_wrapper<T>, Ids...>> {
            using type = T;
        };

        template<class T>
        struct expression_object_type<insert_t<T>> : std::decay<T> {};

        template<class T>
        struct expression_object_type<insert_t<std::reference_wrapper<T>>> : std::decay<T> {};

        template<class It, class L, class O>
        struct expression_object_type<insert_range_t<It, L, O>> {
            using type = typename insert_range_t<It, L, O>::object_type;
        };

        template<class It, class L, class O>
        struct expression_object_type<insert_range_t<std::reference_wrapper<It>, L, O>> {
            using type = typename insert_range_t<std::reference_wrapper<It>, L, O>::object_type;
        };

        template<class T, class... Cols>
        struct expression_object_type<insert_explicit<T, Cols...>> : std::decay<T> {};

        template<class T, class... Cols>
        struct expression_object_type<insert_explicit<std::reference_wrapper<T>, Cols...>> : std::decay<T> {};

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
            using arg_type = std::decay_t<T>;
            get_ref_t<arg_type> g;
            return g(t);
        }

        template<class T>
        struct get_object_t;

        template<class T>
        struct get_object_t<const T> : get_object_t<T> {};

        template<class T>
        auto& get_object(T& t) {
            using expression_type = std::decay_t<T>;
            get_object_t<expression_type> obj;
            return obj(t);
        }

        template<class T>
        struct get_object_t<replace_t<T>> {
            using expression_type = replace_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.object);
            }
        };

        template<class T>
        struct get_object_t<insert_t<T>> {
            using expression_type = insert_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.object);
            }
        };

        template<class T>
        struct get_object_t<update_t<T>> {
            using expression_type = update_t<T>;

            template<class O>
            auto& operator()(O& e) const {
                return get_ref(e.object);
            }
        };
    }
}
