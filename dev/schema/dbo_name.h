#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::bool_constant
#include <utility>  //  std::forward
#endif
#endif

#include "../functional/gsl.h"
#include "../functional/cstring_literal.h"
#include "../functional/meta_util.h"
#include "../functional/mpl.h"
#include "../tuple_helper/tuple_filter.h"
#include "../tuple_helper/tuple_traits.h"

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    /**
     *  Class-scope annotation that overrides the database object name (table or view).
     *  When absent, the name falls back to `std::meta::identifier_of(^^T)`.
     *
     *  The string is embedded in the type's bytes via `cstring_literal<N>` rather than
     *  carried by pointer + size: pointers to string literals are not accepted as
     *  annotation values by current reflection implementations (the underlying object
     *  has no linkage), so a self-contained fixed-size byte array is required.
     */
    template<size_t N>
    struct dbo_name_literal : cstring_literal<N> {
        constexpr dbo_name_literal(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        constexpr orm_gsl::czstring name() const noexcept {
            return this->cstr;
        }

        constexpr operator std::string_view() const noexcept {
            return this->cstr;
        }
    };

    template<class T>
    constexpr bool is_dbo_name_literal_v = false;

    template<size_t N>
    constexpr bool is_dbo_name_literal_v<dbo_name_literal<N>> = true;

    template<class T>
    using is_dbo_name_literal = std::bool_constant<is_dbo_name_literal_v<T>>;

    /**
     *  Returns the database object name carried by the `dbo_name_literal<…>` element of `annotations`,
     *  or the type's reflected identifier when no such element is present.
     */
    template<class T, class Tuple>
    constexpr std::string_view resolve_dbo_name(const Tuple& annotations) {
        using name_index = find_tuple_element<Tuple, is_dbo_name_literal>;

        if constexpr (name_index::value < std::tuple_size_v<Tuple>) {
            return std::get<name_index::value>(annotations).name();
        } else {
            return extract_type_identifier<T>();
        }
    }

    /**
     *  Returns a copy of `tuple` with all `dbo_name_literal<…>` elements removed.
     */
    template<class Tuple>
    constexpr auto filter_out_dbo_name(Tuple&& tuple) {
        using constraints_index_sequence =
            filter_tuple_sequence_t<Tuple, check_if_not<is_dbo_name_literal>::template fn>;
        return create_from_tuple<std::tuple>(std::forward<Tuple>(tuple), constraints_index_sequence{});
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline namespace literals {
        /**
         *  Database object name annotation factory.
         *  Use as a class-scope annotation:
         *  `struct [[="users"_orm_name]] User { ... };`
         *  `struct [[= sqlite_orm::operator""_orm_name<"users">()]] User { ... };`
         *  `make_view<T>()` consumes this annotation.
         */
        template<internal::dbo_name_literal dboName>
        [[nodiscard]] consteval auto operator""_orm_name() {
            return dboName;
        }
    }

    /**
     *  Database object name annotation factory as a fallback to the literal operator.
     *  Use as a class-scope annotation: `struct [[=orm_name("users")]] User { ... };`.
     *  `make_view<T>()` consumes this annotation.
     */
    template<size_t N>
    consteval internal::dbo_name_literal<N> orm_name(const char (&dboName)[N]) {
        return {dboName};
    }
}
#endif
