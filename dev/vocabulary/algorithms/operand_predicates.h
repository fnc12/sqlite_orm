#pragma once

/** @file Combined operand predicates gating the named expression factories.
 *
 *  The named factories (`eq()`, `and_()`, `add()`, `assign()`, ...) accept everything their
 *  operator counterparts accept, plus a raw member pointer (which the named factories
 *  reference without the `c()` wrapper), plus a bindable value - the named factories are
 *  also the only spelling for literal-only SQL expressions such as `SELECT 60 | 13`.
 *  Everything else - arbitrary structs, containers, statement clauses - cannot be
 *  serialized into SQL and is rejected at the factory.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <functional>  //  std::reference_wrapper
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::is_member_pointer
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../node_traits.h"
#include "field_predicates_fwd.h"  // is_bindable

namespace sqlite_orm::internal {
    /**
     *  Whether a type may be referenced as a column expression by a named expression factory:
     *  a member pointer or anything the overloaded operators accept.
     */
    template<class T>
    using is_referencable_operand = std::disjunction<is_operator_argument<T>, std::is_member_pointer<T>>;

    /**
     *  Whether a type may appear as an operand of a named expression factory.
     */
    template<class T>
    using is_operand_or_bindable = std::disjunction<is_operator_argument<T>,
                                                    std::is_member_pointer<T>,
                                                    is_arithmetic_operand<T>,
                                                    is_conditional_operand<T>,
                                                    is_chainable_operand<T>,
                                                    is_bindable<T>,
                                                    //  a scalar subquery
                                                    is_select<T>,
                                                    is_compound_operator<T>,
                                                    //  a row value
                                                    polyfill::is_specialization_of<T, std::tuple>,
                                                    //  a value bound by reference in a prepared statement
                                                    polyfill::is_specialization_of<T, std::reference_wrapper>>;

    template<class L, class R>
    using are_valid_operands = std::conjunction<is_operand_or_bindable<L>, is_operand_or_bindable<R>>;
}
