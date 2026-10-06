#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <array>  //  std::array
#include <meta>  //  std::define_static_array, std::meta::access_context, std::meta::nonstatic_data_members_of, std::meta::annotations_of, std::meta::constant_of, std::meta::parent_of
#include <span>  //  std::span
#include <tuple>  //  std::tuple
#include <utility>  //  std::index_sequence, std::make_index_sequence
#include <vector>  //  std::vector
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
     *  Splices a non-static data member reflection into a member-pointer expression.
     *  Encapsulated here so the splice operator does not leak into consumer headers.
     */
    template<std::meta::info member>
    consteval auto splice_member_pointer() {
        return &[:member:];
    }

    /**
     *  Returns whether `type` is declared within namespace `ns`, directly or in a nested scope.
     *  A class template specialization counts by the scope of its template, not by the scopes of its template
     *  arguments; types without a scope (fundamental types, pointers, arrays) are declared within no namespace.
     */
    consteval bool is_declared_within(std::meta::info type, std::meta::info ns) {
        std::meta::info scope = std::meta::dealias(std::meta::remove_cvref(type));
        if (std::meta::has_template_arguments(scope)) {
            scope = std::meta::template_of(scope);
        }
        while (std::meta::has_parent(scope)) {
            scope = std::meta::parent_of(scope);
            if (scope == ns) {
                return true;
            }
        }
        return false;
    }

    /**
     *  Returns the indices of a reflection's annotations whose type is declared within namespace `ns`.
     */
    template<std::meta::info refl, std::meta::info ns>
    consteval std::span<const size_t> annotation_indices_within() {
        const std::vector<std::meta::info> annotations = std::meta::annotations_of(refl);
        std::vector<size_t> indices;
        for (size_t i = 0; i < annotations.size(); ++i) {
            if (is_declared_within(std::meta::type_of(annotations[i]), ns)) {
                indices.push_back(i);
            }
        }
        return std::define_static_array(indices);
    }

    /**
     *  Splices a reflection's annotations whose type is declared within namespace `ns` into a tuple of values,
     *  skipping all others. The reflection may be a type or a non-static data member.
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
    template<std::meta::info refl, std::meta::info ns>
    consteval auto splice_annotations_within() {
        return []<size_t... I>(std::index_sequence<I...>) consteval {
            return std::tuple{[:std::meta::constant_of(
                                    std::meta::annotations_of(refl)[annotation_indices_within<refl, ns>()[I]]):]...};
        }(std::make_index_sequence<annotation_indices_within<refl, ns>().size()>{});
    }
}
#endif
