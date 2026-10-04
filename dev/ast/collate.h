#pragma once

/** @file The COLLATE operator applied to an expression - with one of the built-in collating functions,
 *        or with a named, application-defined one.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../builtin/collations.h"  //  collate_argument
#include "../tags.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  Collated something
     */
    template<class T>
    struct collate_t : condition_t {
        using expression_type = T;

        expression_type expression;
        collate_argument argument;

        collate_t(expression_type expression_, collate_argument argument_) :
            expression(std::move(expression_)), argument(argument_) {}
    };

    template<class T>
    constexpr bool is_collate_v = polyfill::is_specialization_of_v<T, collate_t>;

    struct named_collate_base {
        std::string name;
    };

    /**
     *  Collated something with custom collate function
     */
    template<class T>
    struct named_collate : named_collate_base {
        using expression_type = T;

        expression_type expression;

        named_collate(expression_type expression_, std::string name_) :
            named_collate_base{std::move(name_)}, expression(std::move(expression_)) {}
    };

    template<class T>
    constexpr bool is_named_collate_v = polyfill::is_specialization_of_v<T, named_collate>;
}
