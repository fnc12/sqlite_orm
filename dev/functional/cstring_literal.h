#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <utility>  //  std::index_sequence
#include <algorithm>  //  std::copy_n
#endif
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  Wraps a C string of fixed size.
     *  Its main purpose is to enable the user-defined string literal operator template.
     */
    template<size_t N>
    struct cstring_literal {
        static constexpr size_t size() {
            return N - 1;
        }

        constexpr cstring_literal(const char (&cstr)[N]) {
            std::copy_n(cstr, N, this->cstr);
        }

        char cstr[N];
    };

    template<template<char...> class Template, cstring_literal literal, size_t... Idx>
    consteval auto explode_into(std::index_sequence<Idx...>) {
        return Template<literal.cstr[Idx]...>{};
    }
}
#endif
