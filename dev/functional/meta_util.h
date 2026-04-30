#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <array>  //  std::array
#include <meta>  //  std::meta::access_context, std::meta::nonstatic_data_members_of, std::meta::identifier_of, std::meta::annotations_of
#include <tuple>  //  std::tuple
#include <utility>  //  std::index_sequence, std::make_index_sequence
#endif
#endif

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    /**
     *  Reflects the non-static data members of `T` and returns them as a fixed-size array
     *  of `std::meta::info` reflections.
     */
    template<class T>
    consteval auto extract_members() {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr size_t N = nonstatic_data_members_of(^^T, ctx).size();

        return [&ctx]<size_t... I>(std::index_sequence<I...>) consteval {
            return std::array<std::meta::info, N>{nonstatic_data_members_of(^^T, ctx)[I]...};
        }(std::make_index_sequence<N>{});
    }

    /**
     *  Returns the identifier of `T`.
     */
    template<class T>
    consteval auto extract_type_identifier() {
        return std::meta::identifier_of(^^T);
    }

    /**
     *  Splices a non-static data member reflection into a member-pointer expression.
     *  Encapsulated here so the splice operator does not leak into consumer headers.
     */
    template<std::meta::info member>
    consteval auto splice_member_pointer() {
        return &[:member:];
    }

    /**
     *  Splices a reflection's annotations into a tuple of values. The reflection may be
     *  a type or a non-static data member.
     *  Encapsulated here so the splice operator does not leak into consumer headers.
     *
     *  Two P3394 details inform this implementation:
     *  - Annotation reflections returned by `annotations_of` are not directly spliceable;
     *    they must first be routed through `std::meta::constant_of`, which returns a
     *    splice-able constant reflection.
     *  - `std::meta::annotations_of` returns a `std::vector<std::meta::info>`, whose heap
     *    allocation is transient under C++20 constexpr rules and cannot be bound to a
     *    `constexpr` variable. The size and per-index lookups therefore re-call
     *    `annotations_of` inline so each transient vector dies within its own constant
     *    expression.
     */
    template<std::meta::info refl>
    consteval auto splice_annotations() {
        return []<size_t... I>(std::index_sequence<I...>) consteval {
            return std::tuple{[:std::meta::constant_of(std::meta::annotations_of(refl)[I]):]...};
        }(std::make_index_sequence<std::meta::annotations_of(refl).size()>{});
    }

    /**
     *  Returns the class-scope annotations of `T` as a tuple.
     */
    template<class T>
    consteval auto extract_type_annotations() {
        return splice_annotations<^^T>();
    }
}
#endif
