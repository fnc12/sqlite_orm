#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <meta>  //  std::meta::info, std::meta::identifier_of, std::define_static_string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple, std::tuple_size_v, std::get
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
     *  Annotation that overrides the name a C++ entity is mapped to in the database:
     *  - on a class, the database object name (table or view);
     *  - on a non-static data member, the column name.
     *  When absent, the name falls back to the entity's reflected identifier.
     *
     *  The string is embedded in the type's bytes via `cstring_literal<N>` rather than
     *  carried by pointer + size: pointers to string literals are not accepted as
     *  annotation values by current reflection implementations (the underlying object
     *  has no linkage), so a self-contained fixed-size byte array is required.
     */
    template<size_t N>
    struct mapped_name_literal : cstring_literal<N> {
        constexpr mapped_name_literal(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        constexpr orm_gsl::czstring name() const noexcept {
            return this->cstr;
        }

        constexpr operator std::string_view() const noexcept {
            return this->cstr;
        }
    };

    template<class T>
    constexpr bool is_mapped_name_literal_v = false;

    template<size_t N>
    constexpr bool is_mapped_name_literal_v<mapped_name_literal<N>> = true;

    template<class T>
    using is_mapped_name_literal = std::bool_constant<is_mapped_name_literal_v<T>>;

    /**
     *  Returns a copy of `tuple` with all `mapped_name_literal<…>` elements removed.
     */
    template<class Tuple>
    constexpr auto filter_out_mapped_name(Tuple&& tuple) {
        using constraints_index_sequence =
            filter_tuple_sequence_t<Tuple, check_if_not<is_mapped_name_literal>::template fn>;
        return create_from_tuple<std::tuple>(std::forward<Tuple>(tuple), constraints_index_sequence{});
    }

    /**
     *  Returns sqlite_orm's own annotations of the entity `refl`, a class or a non-static data member, as a tuple.
     *
     *  An annotation is sqlite_orm's own if its type is declared within namespace `sqlite_orm`; annotations of
     *  other libraries on the same entity are skipped. sqlite_orm's own annotations are not vetted here:
     *  the factories consuming them validate them as elements of the definition they are placed in.
     */
    template<std::meta::info refl>
    consteval auto extract_orm_annotations() {
        return splice_annotations_within<refl, ^^::sqlite_orm>();
    }

    /**
     *  Returns the name the entity `refl`, a class or a non-static data member, is mapped to:
     *  the value of its `mapped_name_literal<…>` annotation, or its reflected identifier when it has none.
     *
     *  The name is resolved entirely at compile time and returned as a static string. This keeps run-time code
     *  free of expressions involving a reflection, which are only allowed in a constant-evaluated context.
     */
    template<std::meta::info refl>
    consteval orm_gsl::czstring mapped_name_of() {
        auto annotations = extract_orm_annotations<refl>();
        using annotations_type = decltype(annotations);
        static_assert(count_tuple<annotations_type, is_mapped_name_literal>::value <= 1,
                      "An entity can only have 1 mapped name annotation");
        using name_index = find_tuple_element<annotations_type, is_mapped_name_literal>;

        if constexpr (name_index::value < std::tuple_size_v<annotations_type>) {
            return std::define_static_string(std::string_view{std::get<name_index::value>(annotations)});
        } else {
            return std::define_static_string(std::meta::identifier_of(refl));
        }
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline namespace literals {
        /**
         *  Mapped name annotation factory.
         *  Use as a class-scope annotation to name the table or view:
         *  `struct [[="users"_orm_name]] User { ... };`
         *  `struct [[= sqlite_orm::operator""_orm_name<"users">()]] User { ... };`
         *  Use as a member annotation to name the column:
         *  `[[="user_id"_orm_name]] int64 userId;`
         *  `make_table<T>()` and `make_view<T>()` consume this annotation.
         */
        template<internal::mapped_name_literal mappedName>
        [[nodiscard]] consteval auto operator""_orm_name() {
            return mappedName;
        }
    }

    /**
     *  Mapped name annotation factory as a fallback to the literal operator.
     *  Use as a class-scope annotation: `struct [[=orm_name("users")]] User { ... };`,
     *  or as a member annotation: `[[=orm_name("user_id")]] int64 userId;`.
     *  `make_table<T>()` and `make_view<T>()` consume this annotation.
     */
    template<size_t N>
    consteval internal::mapped_name_literal<N> orm_name(const char (&mappedName)[N]) {
        return {mappedName};
    }
}
#endif
