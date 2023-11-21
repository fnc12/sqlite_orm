#pragma once

#include <utility>  //  std::index_sequence
#include <algorithm>  //  std::copy_n

#include "cxx_universal.h"  //  ::size_t

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  Helper class to facilitate user-defined string literal operator template
     */
    template<size_t N>
    struct char_array_template {
        static constexpr size_t size() {
            return N - 1;
        }

        constexpr char_array_template(const char (&charArray)[N]) {
            std::copy_n(charArray, N, this->id);
        }

        char id[N];
    };

    template<template<char...> class Template, char_array_template chars, size_t... Idx>
    consteval auto explode_into(std::index_sequence<Idx...>) {
        return Template<chars.id[Idx]...>{};
    }
}
#endif
