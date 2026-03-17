#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::forward, std::move
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {

    struct unbounded_preceding_t {};

    template<class E>
    struct preceding_t {
        E expression;
    };

    struct current_row_t {};

    template<class E>
    struct following_t {
        E expression;
    };

    struct unbounded_following_t {};

    enum class frame_type_t { rows, range, groups };
    enum class frame_exclude_t { no_others, current_row, group, ties };

    template<class Start, class End>
    struct frame_spec_t {
        frame_type_t type;
        Start start;
        End end;
        frame_exclude_t exclude = frame_exclude_t::no_others;

        frame_spec_t exclude_current_row() const {
            auto res = *this;
            res.exclude = frame_exclude_t::current_row;
            return res;
        }

        frame_spec_t exclude_group() const {
            auto res = *this;
            res.exclude = frame_exclude_t::group;
            return res;
        }

        frame_spec_t exclude_ties() const {
            auto res = *this;
            res.exclude = frame_exclude_t::ties;
            return res;
        }

        frame_spec_t exclude_no_others() const {
            auto res = *this;
            res.exclude = frame_exclude_t::no_others;
            return res;
        }
    };

    template<class... Args>
    struct partition_by_t {
        using arguments_type = std::tuple<Args...>;
        arguments_type arguments;
    };

    template<class T>
    inline constexpr bool is_partition_by_v = polyfill::is_specialization_of_v<T, partition_by_t>;

    template<class T>
    using is_partition_by = polyfill::bool_constant<is_partition_by_v<T>>;

    struct window_ref_t {
        std::string name;
    };

    template<class F, class... Args>
    struct over_t {
        using function_type = F;
        using arguments_type = std::tuple<Args...>;

        function_type function;
        arguments_type arguments;
    };

    template<class T>
    inline constexpr bool is_over_v = polyfill::is_specialization_of_v<T, over_t>;

    template<class T>
    using is_over = polyfill::bool_constant<is_over_v<T>>;

    template<class... Args>
    struct window_defn_t {
        std::string name;
        using arguments_type = std::tuple<Args...>;
        arguments_type arguments;
    };

    template<class T>
    inline constexpr bool is_window_defn_v = polyfill::is_specialization_of_v<T, window_defn_t>;

    template<class T>
    using is_window_defn = polyfill::bool_constant<is_window_defn_v<T>>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  UNBOUNDED PRECEDING frame boundary.
     *  https://sqlite.org/windowfunctions.html
     */
    inline internal::unbounded_preceding_t unbounded_preceding() {
        return {};
    }

    /**
     *  expr PRECEDING frame boundary.
     *  https://sqlite.org/windowfunctions.html
     */
    template<class E>
    internal::preceding_t<E> preceding(E expression) {
        return {std::move(expression)};
    }

    /**
     *  CURRENT ROW frame boundary.
     *  https://sqlite.org/windowfunctions.html
     */
    inline internal::current_row_t current_row() {
        return {};
    }

    /**
     *  expr FOLLOWING frame boundary.
     *  https://sqlite.org/windowfunctions.html
     */
    template<class E>
    internal::following_t<E> following(E expression) {
        return {std::move(expression)};
    }

    /**
     *  UNBOUNDED FOLLOWING frame boundary.
     *  https://sqlite.org/windowfunctions.html
     */
    inline internal::unbounded_following_t unbounded_following() {
        return {};
    }

    /**
     *  ROWS BETWEEN start AND end frame specification.
     *  Example: rows(unbounded_preceding(), current_row())
     */
    template<class Start, class End>
    internal::frame_spec_t<Start, End> rows(Start start, End end) {
        return {internal::frame_type_t::rows, std::move(start), std::move(end)};
    }

    /**
     *  RANGE BETWEEN start AND end frame specification.
     *  Example: range(current_row(), unbounded_following())
     */
    template<class Start, class End>
    internal::frame_spec_t<Start, End> range(Start start, End end) {
        return {internal::frame_type_t::range, std::move(start), std::move(end)};
    }

    /**
     *  GROUPS BETWEEN start AND end frame specification.
     *  Example: groups(unbounded_preceding(), current_row())
     */
    template<class Start, class End>
    internal::frame_spec_t<Start, End> groups(Start start, End end) {
        return {internal::frame_type_t::groups, std::move(start), std::move(end)};
    }

    /**
     *  PARTITION BY expression list for window functions.
     *  Example: partition_by(&Employee::departmentId)
     */
    template<class... Args>
    internal::partition_by_t<Args...> partition_by(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  Reference to a named window definition (OVER window_name).
     *  Example: row_number().over(window_ref("win"))
     */
    inline internal::window_ref_t window_ref(std::string name) {
        return {std::move(name)};
    }

    /**
     *  Named window definition (WINDOW name AS (...)).
     *  Passed as a condition to select().
     *  Example: window("win", order_by(&Employee::salary))
     */
    template<class... Args>
    internal::window_defn_t<Args...> window(std::string name, Args... args) {
        return {std::move(name), {std::forward<Args>(args)...}};
    }
}
