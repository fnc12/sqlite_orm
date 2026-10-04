#pragma once

/** @file The built-in JSON functions https://www.sqlite.org/json1.html, including the aggregate functions
 *        JSON_GROUP_ARRAY() and JSON_GROUP_OBJECT().
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple, std::tuple_size
#include <utility>  //  std::move, std::forward
#include <string_view>  //  std::string_view
#endif

#include "../../ast/builtin_function.h"

#ifdef SQLITE_ORM_JSON_SUPPORTED

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in JSON function nodes.
     */
    struct json_string {
        std::string_view serialize() const {
            return "JSON";
        }
    };

    struct json_array_string {
        std::string_view serialize() const {
            return "JSON_ARRAY";
        }
    };

    struct json_array_length_string {
        std::string_view serialize() const {
            return "JSON_ARRAY_LENGTH";
        }
    };

    struct json_extract_string {
        std::string_view serialize() const {
            return "JSON_EXTRACT";
        }
    };

    struct json_insert_string {
        std::string_view serialize() const {
            return "JSON_INSERT";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3053000
    struct json_array_insert_string {
        std::string_view serialize() const {
            return "JSON_ARRAY_INSERT";
        }
    };
#endif
    struct json_replace_string {
        std::string_view serialize() const {
            return "JSON_REPLACE";
        }
    };

    struct json_set_string {
        std::string_view serialize() const {
            return "JSON_SET";
        }
    };

    struct json_object_string {
        std::string_view serialize() const {
            return "JSON_OBJECT";
        }
    };

    struct json_patch_string {
        std::string_view serialize() const {
            return "JSON_PATCH";
        }
    };

    struct json_remove_string {
        std::string_view serialize() const {
            return "JSON_REMOVE";
        }
    };

    struct json_type_string {
        std::string_view serialize() const {
            return "JSON_TYPE";
        }
    };

    struct json_valid_string {
        std::string_view serialize() const {
            return "JSON_VALID";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3042000
    struct json_error_position_string {
        std::string_view serialize() const {
            return "JSON_ERROR_POSITION";
        }
    };
#endif
    struct json_quote_string {
        std::string_view serialize() const {
            return "JSON_QUOTE";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3046000
    struct json_pretty_string {
        std::string_view serialize() const {
            return "JSON_PRETTY";
        }
    };
#endif
    struct json_group_array_string {
        std::string_view serialize() const {
            return "JSON_GROUP_ARRAY";
        }
    };

    struct json_group_object_string {
        std::string_view serialize() const {
            return "JSON_GROUP_OBJECT";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::json_string, X> json(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_array_string, Args...> json_array(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    template<class R = int, class X>
    constexpr internal::builtin_function_t<R, internal::json_array_length_string, X> json_array_length(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    template<class R = int, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::json_array_length_string, X, Y> json_array_length(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    template<class R, class X, class... Args>
    constexpr internal::builtin_function_t<R, internal::json_extract_string, X, Args...> json_extract(X x,
                                                                                                      Args... args) {
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }

    template<class X, class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_insert_string, X, Args...>
    json_insert(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_insert must be odd");
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }

#if SQLITE_VERSION_NUMBER >= 3053000
    /**
     *  JSON_ARRAY_INSERT(X,P,V,...) function: inserts values into the arrays of X at the paths P,
     *  shifting the existing elements to the right. https://www.sqlite.org/json1.html#jarrins
     */
    template<class X, class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_array_insert_string, X, Args...>
    json_array_insert(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_array_insert must be odd");
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }
#endif
    template<class X, class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_replace_string, X, Args...>
    json_replace(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_replace must be odd");
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }

    template<class X, class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_set_string, X, Args...> json_set(X x,
                                                                                                        Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_set must be odd");
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }

    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::json_object_string, Args...>
    json_object(Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_object must be even");
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::json_patch_string, X, Y> json_patch(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    template<class R = std::string, class X, class... Args>
    constexpr internal::builtin_function_t<R, internal::json_remove_string, X, Args...> json_remove(X x, Args... args) {
        return {std::tuple<X, Args...>{std::forward<X>(x), std::forward<Args>(args)...}};
    }

    template<class R = std::string, class X>
    constexpr internal::builtin_function_t<R, internal::json_type_string, X> json_type(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    template<class R = std::string, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::json_type_string, X, Y> json_type(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    template<class X>
    constexpr internal::builtin_function_t<bool, internal::json_valid_string, X> json_valid(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

#if SQLITE_VERSION_NUMBER >= 3045000
    /**
     *  JSON_VALID(X,Y) function: validates X against the conformance flags Y.
     *  https://www.sqlite.org/json1.html#jvalid
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<bool, internal::json_valid_string, X, Y> json_valid(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3042000
    /**
     *  JSON_ERROR_POSITION(X) function: the character offset of the first syntax error in X,
     *  or 0 if X is well-formed. https://www.sqlite.org/json1.html#jerr
     */
    template<class X>
    constexpr internal::builtin_function_t<int, internal::json_error_position_string, X> json_error_position(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
    template<class R, class X>
    constexpr internal::builtin_function_t<R, internal::json_quote_string, X> json_quote(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

#if SQLITE_VERSION_NUMBER >= 3046000
    /**
     *  JSON_PRETTY(X) function: pretty-prints X with four-space indentation.
     *  https://www.sqlite.org/json1.html#jpretty
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::json_pretty_string, X> json_pretty(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  JSON_PRETTY(X,Y) function: pretty-prints X, indenting with the string Y.
     *  https://www.sqlite.org/json1.html#jpretty
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::json_pretty_string, X, Y> json_pretty(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::json_group_array_string, X> json_group_array(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::json_group_object_string, X, Y>
    json_group_object(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in JSON function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace - as copies, or wrapped by a function template
     *  where the public function takes the return type as a template argument or checks its arguments.
     */
    inline constexpr auto json = "JSON"_builtin.scalar<std::string(anything)>();
    inline constexpr auto json_array = "JSON_ARRAY"_builtin.scalar<std::string(variadic<anything>)>();
    inline constexpr auto json_array_length =
        "JSON_ARRAY_LENGTH"_builtin.scalar<int(anything), int(anything, std::string_view)>();
#if SQLITE_VERSION_NUMBER >= 3053000
    inline constexpr auto json_array_insert =
        "JSON_ARRAY_INSERT"_builtin.scalar<std::string(anything, std::string_view, anything, variadic<anything>)>();
#endif
    inline constexpr auto json_extract =
        "JSON_EXTRACT"_builtin.scalar<std::string(anything, std::string_view, variadic<std::string_view>)>();
    inline constexpr auto json_insert =
        "JSON_INSERT"_builtin.scalar<std::string(anything, std::string_view, anything, variadic<anything>)>();
    inline constexpr auto json_replace =
        "JSON_REPLACE"_builtin.scalar<std::string(anything, std::string_view, anything, variadic<anything>)>();
    inline constexpr auto json_set =
        "JSON_SET"_builtin.scalar<std::string(anything, std::string_view, anything, variadic<anything>)>();
    inline constexpr auto json_object = "JSON_OBJECT"_builtin.scalar<std::string(variadic<anything>)>();
    inline constexpr auto json_patch = "JSON_PATCH"_builtin.scalar<std::string(anything, anything)>();
    inline constexpr auto json_remove =
        "JSON_REMOVE"_builtin.scalar<std::string(anything, variadic<std::string_view>)>();
    inline constexpr auto json_type =
        "JSON_TYPE"_builtin.scalar<std::string(anything), std::string(anything, std::string_view)>();
#if SQLITE_VERSION_NUMBER >= 3045000
    inline constexpr auto json_valid = "JSON_VALID"_builtin.scalar<bool(anything), bool(anything, int)>();
#else
    inline constexpr auto json_valid = "JSON_VALID"_builtin.scalar<bool(anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3042000
    inline constexpr auto json_error_position = "JSON_ERROR_POSITION"_builtin.scalar<int(anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3046000
    inline constexpr auto json_pretty =
        "JSON_PRETTY"_builtin.scalar<std::string(anything), std::string(anything, std::string_view)>();
#endif
    inline constexpr auto json_quote = "JSON_QUOTE"_builtin.scalar<std::string(anything)>();
    inline constexpr auto json_group_array = "JSON_GROUP_ARRAY"_builtin.aggregate<std::string(anything)>();
    inline constexpr auto json_group_object = "JSON_GROUP_OBJECT"_builtin.aggregate<std::string(anything, anything)>();
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline constexpr orm_builtin_function auto json = internal::json;

    inline constexpr orm_builtin_function auto json_array = internal::json_array;

    template<class R = int, class... Args>
    constexpr auto json_array_length(Args... args) {
        return internal::json_array_length.template operator()<R>(std::move(args)...);
    }

    template<class R, class... Args>
    constexpr auto json_extract(Args... args) {
        return internal::json_extract.template operator()<R>(std::move(args)...);
    }

    template<class X, class... Args>
    constexpr auto json_insert(X x, Args... args) {
        static_assert(sizeof...(Args) % 2 == 0, "number of arguments in json_insert must be odd");
        return internal::json_insert(std::move(x), std::move(args)...);
    }

#if SQLITE_VERSION_NUMBER >= 3053000
    /**
     *  JSON_ARRAY_INSERT(X,P,V,...) function: inserts values into the arrays of X at the paths P,
     *  shifting the existing elements to the right. https://www.sqlite.org/json1.html#jarrins
     */
    template<class X, class... Args>
    constexpr auto json_array_insert(X x, Args... args) {
        static_assert(sizeof...(Args) % 2 == 0, "number of arguments in json_array_insert must be odd");
        return internal::json_array_insert(std::move(x), std::move(args)...);
    }
#endif
    template<class X, class... Args>
    constexpr auto json_replace(X x, Args... args) {
        static_assert(sizeof...(Args) % 2 == 0, "number of arguments in json_replace must be odd");
        return internal::json_replace(std::move(x), std::move(args)...);
    }

    template<class X, class... Args>
    constexpr auto json_set(X x, Args... args) {
        static_assert(sizeof...(Args) % 2 == 0, "number of arguments in json_set must be odd");
        return internal::json_set(std::move(x), std::move(args)...);
    }

    inline constexpr orm_builtin_function auto json_object = internal::json_object;

    inline constexpr orm_builtin_function auto json_patch = internal::json_patch;

    template<class R = std::string, class... Args>
    constexpr auto json_remove(Args... args) {
        return internal::json_remove.template operator()<R>(std::move(args)...);
    }

    template<class R = std::string, class... Args>
    constexpr auto json_type(Args... args) {
        return internal::json_type.template operator()<R>(std::move(args)...);
    }

    /**
     *  JSON_VALID(X) and, as of SQLite 3.45.0, JSON_VALID(X,Y) function: validates X,
     *  against the conformance flags Y. https://www.sqlite.org/json1.html#jvalid
     */
    inline constexpr orm_builtin_function auto json_valid = internal::json_valid;

#if SQLITE_VERSION_NUMBER >= 3042000
    /**
     *  JSON_ERROR_POSITION(X) function: the character offset of the first syntax error in X,
     *  or 0 if X is well-formed. https://www.sqlite.org/json1.html#jerr
     */
    inline constexpr orm_builtin_function auto json_error_position = internal::json_error_position;
#endif
#if SQLITE_VERSION_NUMBER >= 3046000
    /**
     *  JSON_PRETTY(X) and JSON_PRETTY(X,Y) function: pretty-prints X with four-space indentation,
     *  or indenting with the string Y. https://www.sqlite.org/json1.html#jpretty
     */
    inline constexpr orm_builtin_function auto json_pretty = internal::json_pretty;
#endif
    template<class R, class X>
    constexpr auto json_quote(X x) {
        return internal::json_quote.template operator()<R>(std::move(x));
    }

    inline constexpr orm_builtin_function auto json_group_array = internal::json_group_array;

    inline constexpr orm_builtin_function auto json_group_object = internal::json_group_object;
}
#endif
#endif
