#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/is_base_template_of.h"
#include "../tuple_helper/tuple_iteration.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  Base for UNION, UNION ALL, EXCEPT and INTERSECT
     */
    template<class... E>
    struct compound_operator {
        using expressions_tuple = std::tuple<E...>;

        expressions_tuple compound;

        constexpr compound_operator(expressions_tuple compound) : compound{std::move(compound)} {
            iterate_tuple(this->compound, [](auto& expression) SQLITE_ORM_STATIC_CALLOP {
                expression.highest_level = true;
            });
        }
    };

    template<class T>
    constexpr bool is_compound_operator_v = is_base_template_of<compound_operator, T>::value;

    struct union_base {
        bool all = false;

        operator std::string() const {
            if (!this->all) {
                return "UNION";
            } else {
                return "UNION ALL";
            }
        }
    };

    /**
     *  UNION object type.
     */
    template<class... E>
    struct union_t : public compound_operator<E...>, union_base {
        using typename compound_operator<E...>::expressions_tuple;

        constexpr union_t(expressions_tuple compound, bool all) :
            compound_operator<E...>{std::move(compound)}, union_base{all} {}
    };

    struct except_string {
        operator std::string() const {
            return "EXCEPT";
        }
    };

    /**
     *  EXCEPT object type.
     */
    template<class... E>
    struct except_t : compound_operator<E...>, except_string {
        using super = compound_operator<E...>;

        using super::super;
    };

    struct intersect_string {
        operator std::string() const {
            return "INTERSECT";
        }
    };
    /**
     *  INTERSECT object type.
     */
    template<class... E>
    struct intersect_t : compound_operator<E...>, intersect_string {
        using super = compound_operator<E...>;

        using super::super;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Public function for UNION operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class... E>
    constexpr internal::union_t<E...> union_(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        static_assert((std::disjunction_v<internal::is_select<E>, internal::is_compound_operator<E>> && ...),
                      "Compound operators can only be applied to select statements");
        return {{std::forward<E>(expressions)...}, false};
    }

    /**
     *  Public function for UNION ALL operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class... E>
    constexpr internal::union_t<E...> union_all(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        static_assert((std::disjunction_v<internal::is_select<E>, internal::is_compound_operator<E>> && ...),
                      "Compound operators can only be applied to select statements");
        return {{std::forward<E>(expressions)...}, true};
    }

    /**
     *  Public function for EXCEPT operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/except.cpp
     */
    template<class... E>
    constexpr internal::except_t<E...> except(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        static_assert((std::disjunction_v<internal::is_select<E>, internal::is_compound_operator<E>> && ...),
                      "Compound operators can only be applied to select statements");
        return {{std::forward<E>(expressions)...}};
    }

    template<class... E>
    constexpr internal::intersect_t<E...> intersect(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        static_assert((std::disjunction_v<internal::is_select<E>, internal::is_compound_operator<E>> && ...),
                      "Compound operators can only be applied to select statements");
        return {{std::forward<E>(expressions)...}};
    }
}
