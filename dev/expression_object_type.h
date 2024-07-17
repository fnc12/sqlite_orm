#pragma once

#ifndef _IMPORT_STD_MODULE
#include <type_traits>  //  std::decay, std::remove_reference
#include <functional>  //  std::reference_wrapper
#endif

#include "type_traits.h"
#include "prepared_statement.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct expression_object_type;

        template<class T>
        using expression_object_type_t = typename expression_object_type<T>::type;

        template<typename S>
        using statement_object_type_t = expression_object_type_t<expression_type_t<std::remove_reference_t<S>>>;

        template<class T>
        struct expression_object_type<update_t<T>, void> : value_unref_type<T> {};

        template<class T>
        struct expression_object_type<replace_t<T>, void> : value_unref_type<T> {};

        template<class T>
        struct expression_object_type<T, match_if<is_replace_range, T>> {
            using type = object_type_t<T>;
        };

        template<class T, class... Ids>
        struct expression_object_type<remove_t<T, Ids...>, void> : value_unref_type<T> {};

        template<class T>
        struct expression_object_type<insert_t<T>, void> : value_unref_type<T> {};

        template<class T>
        struct expression_object_type<T, match_if<is_insert_range, T>> {
            using type = object_type_t<T>;
        };

        template<class T, class... Cols>
        struct expression_object_type<insert_explicit<T, Cols...>, void> : value_unref_type<T> {};

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
