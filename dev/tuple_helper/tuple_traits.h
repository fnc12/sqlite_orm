#pragma once

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"

namespace sqlite_orm {
    // convenience metafunction algorithms
    namespace internal {
        /*
         *  Higher-order trait metafunction that checks whether a tuple contains a type with given trait (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...>
                 class TraitFn,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has = mpl::invoke_t<check_if_has<TraitFn>, Pack, mpl::quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that checks whether a tuple contains the specified type (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack, class Type, template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has_type = mpl::invoke_t<check_if_has_type<Type>, Pack, mpl::quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that checks whether a tuple contains the specified class template (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...>
                 class Template,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has_template = mpl::invoke_t<check_if_has_template<Template>, Pack, mpl::quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the desired type in a tuple (possibly projected).
         */
        template<class Pack, class Type, template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_type = mpl::invoke_t<finds_if_has_type<Type>, Pack, mpl::quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the desired class template in a tuple (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...>
                 class Template,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_template = mpl::invoke_t<finds_if_has_template<Template>, Pack, mpl::quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that counts the types having the specified trait in a tuple (possibly projected).
         *  
         *  `Pred` is a predicate metafunction with a nested bool member named `value`
         *  `ProjOp` is a metafunction
         */
        template<class Pack, template<class...> class Pred, template<class...> class ProjOp = polyfill::type_identity_t>
        using count_tuple = mpl::invoke_t<counts_if_has<Pred>, Pack, mpl::quote_fn<ProjOp>>;
    }
}
