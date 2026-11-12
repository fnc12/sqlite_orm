#pragma once

/** @file
 *  Purpose: find the positions of the nodes in a tuple that match a trait.
 *
 *  Intention: an algorithm of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md` holds; composes
 *  the grammar and field traits into index sequences over the elements of a table, the database objects of a schema
 *  or a flattened expression.
 */

#include "../../functional/mpl/convenience.h"
#include "../../functional/tuple_helper/tuple_filter.h"
#include "../node_traits.h"
#include "field_predicates_fwd.h"  //  is_bindable

// Filters over the columns of a table or view definition
namespace sqlite_orm::internal {
    template<class Elements>
    using col_index_sequence_of = filter_tuple_sequence_t<Elements, is_column>;

    template<class Elements, class F>
    using col_index_sequence_with_field_type = filter_tuple_sequence_t<Elements,
                                                                       check_if_is_type<F>::template fn,
                                                                       field_type_t,
                                                                       filter_tuple_sequence_t<Elements, is_column>>;

    template<class Elements, template<class...> class TraitFn>
    using col_index_sequence_with = filter_tuple_sequence_t<Elements,
                                                            check_if_has<TraitFn>::template fn,
                                                            constraints_type_t,
                                                            filter_tuple_sequence_t<Elements, is_column>>;

    template<class Elements, template<class...> class TraitFn>
    using col_index_sequence_excluding = filter_tuple_sequence_t<Elements,
                                                                 check_if_has_not<TraitFn>::template fn,
                                                                 constraints_type_t,
                                                                 filter_tuple_sequence_t<Elements, is_column>>;

    template<class Elements>
    using hidden_col_index_sequence_of = filter_tuple_sequence_t<Elements, is_hidden_column>;

    template<class Elements, class F>
    using all_col_index_sequence_with_field_type = filter_tuple_sequence_t<
        Elements,
        check_if_is_type<F>::template fn,
        field_type_t,
        filter_tuple_sequence_t<Elements, mpl::disjunction_fn<is_column, is_hidden_column>::template fn>>;
}

// Filters over the database objects of a schema
namespace sqlite_orm::internal {
    /**
     *  The positions of the ordinary tables in a tuple of database objects.
     */
    template<class DBOs>
    using tables_index_sequence = filter_tuple_sequence_t<DBOs, is_ordinary_table>;

#ifdef SQLITE_ORM_WITH_VIEW
    /**
     *  The positions of the views in a tuple of database objects.
     */
    template<class DBOs>
    using views_index_sequence = filter_tuple_sequence_t<DBOs, is_view>;
#endif
}

// Filters over the nodes of an expression, as flattened by `node_tuple`
namespace sqlite_orm::internal {
    template<class Nodes>
    using bindable_index_sequence_of = filter_tuple_sequence_t<Nodes, is_bindable>;
}
