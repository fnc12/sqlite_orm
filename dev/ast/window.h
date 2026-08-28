#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::is_same
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/node_algorithms.h"  // is_statement_clause, is_frame_start_bound_v, is_frame_end_bound_v

namespace sqlite_orm::internal {

    struct unbounded_preceding_t {};

    template<class T>
    constexpr bool is_unbounded_preceding_v = std::is_same<T, unbounded_preceding_t>::value;

    template<class E>
    struct preceding_t {
        E expression;
    };

    template<class T>
    constexpr bool is_preceding_v = polyfill::is_specialization_of_v<T, preceding_t>;

    struct current_row_t {};

    template<class T>
    constexpr bool is_current_row_v = std::is_same<T, current_row_t>::value;

    template<class E>
    struct following_t {
        E expression;
    };

    template<class T>
    constexpr bool is_following_v = polyfill::is_specialization_of_v<T, following_t>;

    struct unbounded_following_t {};

    template<class T>
    constexpr bool is_unbounded_following_v = std::is_same<T, unbounded_following_t>::value;

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

    template<class T>
    constexpr bool is_frame_spec_v = polyfill::is_specialization_of_v<T, frame_spec_t>;

    template<class... Args>
    struct partition_by_t {
        using args_type = std::tuple<Args...>;
        args_type arguments;
    };

    template<class T>
    constexpr bool is_partition_by_v = polyfill::is_specialization_of_v<T, partition_by_t>;

    struct window_ref_t {
        std::string name;
    };

    template<class T>
    constexpr bool is_window_ref_v = std::is_same<T, window_ref_t>::value;

    template<class F, class... Args>
    struct over_t {
        using function_type = F;
        using args_type = std::tuple<Args...>;

        function_type function;
        args_type arguments;
    };

    template<class T>
    constexpr bool is_over_v = polyfill::is_specialization_of_v<T, over_t>;

    template<class... Args>
    struct window_defn_t {
        using args_type = std::tuple<Args...>;

        std::string name;
        args_type arguments;
    };

    template<class T>
    constexpr bool is_window_defn_v = polyfill::is_specialization_of_v<T, window_defn_t>;

    template<class... Args>
    constexpr void validate_over_arguments() {
        static_assert(are_valid_over_arguments_v<Args...>,
                      "an OVER clause takes either a single window_ref(), or the elements of an inline window "
                      "definition: partition_by(), order_by() and a rows()/range()/groups() frame");
    }

    template<class... Args>
    constexpr void validate_window_arguments() {
        static_assert((is_window_defn_element_v<Args> && ...),
                      "a window definition takes partition_by(), order_by() and a rows()/range()/groups() frame");
    }
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
        static_assert(!internal::is_statement_clause<E>::value,
                      "a frame boundary must be an expression, not a statement clause");
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
        static_assert(!internal::is_statement_clause<E>::value,
                      "a frame boundary must be an expression, not a statement clause");
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
        static_assert(internal::is_frame_start_bound_v<Start>,
                      "a frame must start with unbounded_preceding(), preceding(), current_row() or following()");
        static_assert(internal::is_frame_end_bound_v<End>,
                      "a frame must end with preceding(), current_row(), following() or unbounded_following()");
        return {internal::frame_type_t::rows, std::move(start), std::move(end)};
    }

    /**
     *  RANGE BETWEEN start AND end frame specification.
     *  Example: range(current_row(), unbounded_following())
     */
    template<class Start, class End>
    internal::frame_spec_t<Start, End> range(Start start, End end) {
        static_assert(internal::is_frame_start_bound_v<Start>,
                      "a frame must start with unbounded_preceding(), preceding(), current_row() or following()");
        static_assert(internal::is_frame_end_bound_v<End>,
                      "a frame must end with preceding(), current_row(), following() or unbounded_following()");
        return {internal::frame_type_t::range, std::move(start), std::move(end)};
    }

    /**
     *  GROUPS BETWEEN start AND end frame specification.
     *  Example: groups(unbounded_preceding(), current_row())
     */
    template<class Start, class End>
    internal::frame_spec_t<Start, End> groups(Start start, End end) {
        static_assert(internal::is_frame_start_bound_v<Start>,
                      "a frame must start with unbounded_preceding(), preceding(), current_row() or following()");
        static_assert(internal::is_frame_end_bound_v<End>,
                      "a frame must end with preceding(), current_row(), following() or unbounded_following()");
        return {internal::frame_type_t::groups, std::move(start), std::move(end)};
    }

    /**
     *  PARTITION BY expression list for window functions.
     *  Example: partition_by(&Employee::departmentId)
     */
    template<class... Args>
    internal::partition_by_t<Args...> partition_by(Args... args) {
        static_assert((!internal::is_statement_clause<Args>::value && ...),
                      "a PARTITION BY term must be an expression, not a statement clause");
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
        internal::validate_window_arguments<Args...>();
        return {std::move(name), {std::forward<Args>(args)...}};
    }
}
