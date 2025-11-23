#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <meta>  //  std::meta::access_context, std::meta::nonstatic_data_members_of, std::meta::identifier_of
#include <tuple>  //  std::tuple, std::get
#include <utility>  //  std::index_sequence, std::make_index_sequence
#endif
#endif

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    template<class T>
    consteval auto extract_member_names() {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr size_t N = nonstatic_data_members_of(^^T, ctx).size();
        auto members = nonstatic_data_members_of(^^T, ctx);

        return [&members]<size_t... I>(std::index_sequence<I...>) consteval {
            return std::tuple{std::meta::identifier_of(members[I])...};
        }(std::make_index_sequence<N>{});
    }

    template<class T>
    consteval auto extract_member_pointers() {
        constexpr auto ctx = std::meta::access_context::current();
        constexpr size_t N = nonstatic_data_members_of(^^T, ctx).size();

        return [&ctx]<size_t... I>(std::index_sequence<I...>) consteval {
            return std::tuple{&[:nonstatic_data_members_of(^^T, ctx)[I]:]...};
        }(std::make_index_sequence<N>{});
    }

    template<class T>
    consteval auto count_members() {
        constexpr auto ctx = std::meta::access_context::current();
        return nonstatic_data_members_of(^^T, ctx).size();
    }
}
#endif
