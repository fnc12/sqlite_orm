#pragma once
#ifdef SQLITE_ORM_WITH_VIEW
#include <type_traits>  //  std::remove_cvref
#include <utility>  // std::forward, std::move, std::index_sequence, std::make_index_sequence
#include <cstddef>  //  std::byte
#include <stddef.h>  //  offsetof
#include <boost/pfr.hpp>
#include "../functional/cxx_universal.h"  //  ::size_t
#include "../column_pointer.h"
#include "../select_constraints.h"
#include "column.h"
#include "mapped_object.h"

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
                return (field_type*)(std::byte*)offsetof(Tpl, nth_type::value);
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

namespace sqlite_orm {

    /**
     *  Factory function for a column definition from a relative pointer to an object of the object to be mapped.
     */
    template<class C, class... Op>
        requires(internal::is_column_pointer_v<C>)
    internal::column_t<C, internal::empty_setter, Op...>
    make_column(std::string name, C relativeField, Op... constraints) {
        static_assert(polyfill::conjunction_v<internal::is_column_constraint<Op>...>, "Incorrect constraints pack");

        // attention: do not use `std::make_tuple()` for constructing the tuple member `[[no_unique_address]] column_constraints::constraints`,
        // as this will lead to UB with Clang on MinGW!
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {std::move(name), relativeField, {}, std::tuple<Op...>{std::move(constraints)...}});
    }

    namespace internal {
        template<class Select>
        decltype(auto) get_cte_driving_subselect(const Select& select);

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
        bool compare_any(F O::*m, const column_pointer<O, F*>& relative) {
            constexpr O* object = nullptr;
            return &(object->*m) == relative.field;
        }

        template<class O, class F>
        bool compare_any(const column_pointer<O, F*>& relative, F O::*m) {
            constexpr O* object = nullptr;
            return relative.field == &(object->*m);
        }

        /**
         *  View definition, mapping an aggregate object type to a corresponding select statement.
         */
        template<class O, class Select, class... Cs>
        struct view_t : mapped_object_t<O, Cs...> {
            using base_type = mapped_object_t<O, Cs...>;
            using object_type = typename base_type::object_type;
            using elements_type = typename base_type::elements_type;
            using select_type = Select;

            select_type select;

            //#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            view_t(std::string name, elements_type elements, Select select) :
                base_type{std::move(name), std::move(elements)}, select{std::move(select)} {}
            //#endif
        };

        template<class Select>
        using columns_size_t = std::tuple_size<typename Select::return_type::columns_type>;

        template<class O, size_t... I, class Select>
        auto make_view(std::string name, std::index_sequence<I...>, Select select) {
            namespace pfr = boost::pfr;
            namespace pfrd = pfr::detail;
            namespace pfrs = pfrd::sequence_tuple;

            static_assert(std::is_aggregate_v<O>);

            using PfrTpl = decltype(pfrd::tie_as_tuple(pfrd::fake_object<O>()));
            // object's member types as a tuple
            using TS = pfrs::tuple<std::remove_cvref_t<typename pfrs::tuple_element<I, PfrTpl>::type>...>;

            using DrivingSelect = std::remove_cvref_t<decltype(get_cte_driving_subselect(select))>;

            using columns_size = polyfill::detected_or_t<polyfill::index_constant<1>, columns_size_t, DrivingSelect>;
            static_assert(columns_size::value == PfrTpl::size_v);

            using view_type =
                view_t<O,
                       Select,
                       decltype(make_column<>(std::string(pfr::get_name<I, O>()),
                                              column_pointer<O, decltype(pfr::get_relative_address<O, I, TS>())>{
                                                  pfr::get_relative_address<O, I, TS>()}))...>;

            SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
                return view_type{std::move(name),
                                 {make_column<>(std::string(pfr::get_name<I, O>()),
                                                column_pointer<O, decltype(pfr::get_relative_address<O, I, TS>())>{
                                                    pfr::get_relative_address<O, I, TS>()})...},
                                 std::move(select)});
        }
    }

    template<class O, class Select>
    auto make_view(std::string name, Select select) {
        static_assert(polyfill::conjunction_v<internal::is_select<Select>>, "You must specify a select statement");

        select.highest_level = true;
        return internal::make_view<O>(std::move(name),
                                      std::make_index_sequence<boost::pfr::tuple_size_v<O>>{},
                                      std::move(select));
    }

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
