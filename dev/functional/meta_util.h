#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <array>  //  std::array
#include <meta>  //  std::define_static_array, std::meta::access_context, std::meta::nonstatic_data_members_of, std::meta::identifier_of, std::meta::annotations_of
#include <tuple>  //  std::tuple
#include <utility>  //  std::index_sequence, std::make_index_sequence
#endif
#endif

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    /**
     *  Reflects the non-static data members of `T` and its base classes
     *  and returns them as a fixed-size span of `std::meta::info` reflections.
     */
    template<class T>
    consteval auto extract_members() {
        constexpr auto ctx = std::meta::access_context::current();

        constexpr auto collect = []<class U>(this const auto& self) -> std::vector<std::meta::info> {
            std::vector<std::meta::info> result;

            // Recurse into direct base classes first (preserves layout order)
            template for (constexpr std::meta::info base : std::define_static_array(bases_of(^^U, ctx))) {
                using base_type = typename[:type_of(base):];
                result.append_range(self.template operator()<base_type>());
            }

            // Then this class's own non-static data members
            result.append_range(nonstatic_data_members_of(^^U, ctx));

            return result;
        };

        return std::define_static_array(collect.template operator()<T>());
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
