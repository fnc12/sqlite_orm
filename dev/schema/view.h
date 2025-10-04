#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_VIEW
#include <type_traits>  //  std::remove_cvref
#include <utility>  // std::forward, std::move, std::index_sequence, std::make_index_sequence
#include <cstddef>  //  std::byte
#if __cpp_impl_reflection >= 202500L
#include <meta>
#endif
#endif
#endif

#ifdef SQLITE_ORM_WITH_VIEW
#if __cpp_impl_reflection < 202500L
#ifdef SQLITE_ORM_HAS_BOOST_PFR
#include <boost/pfr.hpp>
#endif
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../column_pointer.h"
#include "../select_constraints.h"
#include "column.h"
#include "table_base.h"

#ifdef SQLITE_ORM_WITH_VIEW
#if __cpp_impl_reflection >= 202500L
#elif BOOST_PFR_ENABLED == 1
namespace boost::pfr {
    namespace detail {
        namespace sequence_tuple {
            template<std::size_t N, class W>
            consteval auto get_nth_base(const base_from_member<N, W>& t) noexcept {
                // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
                return t;
            }

            template<std::size_t N, class Tpl>
            constexpr auto* get_nth_relative_address() noexcept {
                // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
                static_assert(N < Tpl::size_v);
                using nth_type = decltype(get_nth_base<N>(std::declval<Tpl>()));
                using field_type = decltype(nth_type::value);

                return (field_type*)(std::byte*)
                    // offsetof - the official one cannot be used because of some implementations using the compiler intrinsic builtin
                    ((::size_t)&reinterpret_cast<char const volatile&>((((Tpl*)0)->nth_type::value)));
            }
        }
    }

    template<class O, std::size_t I, class TS>
    constexpr auto* get_relative_address() {
        static_assert(sizeof(O) == sizeof(TS),
                      "====================> Boost.PFR: Member sequence does not indicate correct size for struct "
                      "type! Maybe the user-provided type is not a SimpleAggregate?");
        static_assert(
            alignof(O) == alignof(TS),
            "====================> Boost.PFR: Member sequence does not indicate correct alignment for struct type!");

        return detail::sequence_tuple::get_nth_relative_address<I, TS>();
    }
}
#endif
#endif

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_WITH_VIEW
    /**
     *  View definition, mapping an aggregate object type to a corresponding select statement.
     */
    template<class O, class Select, class... Cs>
    struct query_view : table_identifier, table_definition<Cs...> {
        using definition_base_type = table_definition<Cs...>;
        using object_type = O;
        using elements_type = typename definition_base_type::elements_type;
        using select_type = Select;

        select_type select;
    };

    template<class T>
    inline constexpr bool is_view_v = polyfill::is_specialization_of_v<T, query_view>;
#else
    template<class T>
    inline constexpr bool is_view_v = false;
#endif

    template<class T>
    struct is_view : polyfill::bool_constant<is_view_v<T>> {};
}

#ifdef SQLITE_ORM_WITH_VIEW
#if __cpp_impl_reflection >= 202500L
#elif BOOST_PFR_ENABLED == 1
namespace sqlite_orm::internal {
    /**
     *  Factory function for a column definition from a relative pointer to an object of the object to be mapped.
     */
    template<class C, class... Op>
        requires (internal::is_column_pointer_v<C>)
    internal::column_t<C, internal::empty_setter, Op...>
    make_column(std::string name, C relativeField, Op... constraints) {
        static_assert(polyfill::conjunction_v<internal::is_column_constraint<Op>...>, "Incorrect constraints pack");

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), relativeField, {}, std::tuple<Op...>{std::move(constraints)...}});
    }

    /*  
     *  A column field carrying a relative address to a member of an object.
     *  
     *  Internal note: According to my tests msvc or compilers in general have a hard time to use pointer-to-members at compile-time.
     *  That's why we use a relative address.
     */
    template<class O, class F>
    struct column_field<column_pointer<O, F*>, empty_setter> {
        using member_pointer_t = F O::*;
        using setter_type = empty_setter;
        using object_type = O;
        using field_type = F;

        /**
         *  Relative pointer to member (offset) used to read and write a field value.
         */
        const column_pointer<O, F*> member_pointer;

        SQLITE_ORM_NOUNIQUEADDRESS
        const empty_setter setter;

        /**
         *  Simplified interface for `NOT NULL` constraint
         */
        constexpr bool is_not_null() const {
            return !type_is_nullable<field_type>::value;
        }
    };

    template<class O, class F>
    bool compare_fields(F O::* m, const column_pointer<O, F*>& relative) {
        constexpr O* object = nullptr;
        return &(object->*m) == relative.field;
    }

    template<class O, class F>
    bool compare_fields(const column_pointer<O, F*>& relative, F O::* m) {
        constexpr O* object = nullptr;
        return relative.field == &(object->*m);
    }

    template<class O, size_t... I, class Select>
    auto make_view(std::string name, std::index_sequence<I...>, Select select) {
        namespace pfr = boost::pfr;
        namespace pfrd = pfr::detail;
        namespace pfrs = pfrd::sequence_tuple;

#if __cpp_lib_is_aggregate >= 201703L
        static_assert(std::is_aggregate_v<O>);
#endif

        using PfrTpl = decltype(pfrd::tie_as_tuple(pfrd::fake_object<O>()));
        // object's member types as a tuple
        using TS = pfrs::tuple<polyfill::remove_cvref_t<typename pfrs::tuple_element<I, PfrTpl>::type>...>;

        using view_type = query_view<O,
                                     Select,
                                     decltype(internal::make_column<>(
                                         std::string(pfr::get_name<I, O>()),
                                         column_pointer<O, decltype(pfr::get_relative_address<O, I, TS>())>{
                                             pfr::get_relative_address<O, I, TS>()}))...>;

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return view_type{
            std::move(name),
            std::tuple{internal::make_column<>(std::string(pfr::get_name<I, O>()),
                                               column_pointer<O, decltype(pfr::get_relative_address<O, I, TS>())>{
                                                   pfr::get_relative_address<O, I, TS>()})...},
            std::move(select)});
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class O, class Select>
        requires (internal::is_select_expression_v<Select>)
    auto make_view(std::string name, Select select) {
        using namespace ::sqlite_orm::internal;

        if constexpr (is_select_v<Select>) {
            select.highest_level = true;
        }
        return internal::make_view<O>(std::move(name),
                                      std::make_index_sequence<boost::pfr::tuple_size_v<O>>{},
                                      std::move(select));
    }
}
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a view definition.
     *  
     *  The mapped object type is explicitly specified, columns and their names are deferred from the object type.
     *  The object type must be an aggregate.
     */
    template<orm_table_reference auto table, class Select>
    auto make_view(std::string name, Select select) {
        return make_view<internal::auto_decay_table_ref_t<table>>(std::move(name), std::forward<Select>(select));
    }
#endif
}
#endif
