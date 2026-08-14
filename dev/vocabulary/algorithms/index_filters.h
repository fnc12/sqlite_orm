#pragma once

/** @file Closed alias templates for filtering nodes by their traits.
 */

#include "../../functional/mpl.h"
#include "../../tuple_helper/tuple_filter.h"
#include "../node_traits.h"

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
