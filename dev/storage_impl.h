#pragma once

#include <utility>  //  std::forward, std::move

namespace sqlite_orm {

    namespace internal {

        /**
         *  A chain of schema objects
         */
        template<class... Ts>
        struct storage_impl {};

        template<class H, class... Ts>
        struct storage_impl<H, Ts...> : storage_impl<Ts...> {
            using super = storage_impl<Ts...>;
            using table_type = H;

            storage_impl(H h, Ts... ts) : super{std::forward<Ts>(ts)...}, table{std::move(h)} {}

            table_type table;
        };
    }
}

#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::is_same, std::decay

#include "functional/cxx_universal.h"
#include "functional/static_magic.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "storage_lookup.h"

// interface functions
namespace sqlite_orm {
    namespace internal {

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        auto lookup_table(const S& impl) {
            return static_if<!is_mapped_v<S, Lookup>>(empty_callable<nullptr_t>(), [](const auto& impl) {
                return &pick_table<Lookup>(impl);
            })(impl);
        }

        template<class S, satisfies<is_storage_impl, S> = true>
        std::string find_table_name(const S& impl, const std::type_index& ti) {
            return static_if<std::is_same<S, storage_impl<>>::value>(
                empty_callable<std::string>(),
                [&ti](const auto& tImpl) {
                    using qualified_type = std::decay_t<decltype(tImpl)>;
                    return ti == typeid(storage_object_type_t<qualified_type>)
                               ? tImpl.table.name
                               : find_table_name<typename qualified_type::super>(tImpl, ti);
                })(impl);
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        std::string lookup_table_name(const S& impl) {
            return static_if<!is_mapped_v<S, Lookup>>(empty_callable<std::string>(), [](const auto& impl) {
                return pick_table<Lookup>(impl).name;
            })(impl);
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& impl, F O::*field) {
            return pick_table<O>(impl).find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& impl, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(impl, cp);
            return pick_table<O>(impl).find_column_name(field);
        }
    }
}
