#pragma once

#include <type_traits>
#include <tuple>

#include "cxx_polyfill.h"
#include "start_macros.h"
#include "type_traits.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Aliased column expression mapped into a CTE, stored as a field in a table column.
         */
        template<class A, class F>
        struct aliased_field {
            ~aliased_field() = delete;
            aliased_field(const aliased_field&) = delete;
            void operator=(const aliased_field&) = delete;

            F field;
        };

        /**
         *  This class captures an expression of a subselect (basically select_t<>::return_type),
         *  and is used as a proxy 
         */
        template<typename Label, typename Expression, typename RefExpressions, typename... Fs>
        class subselect_mapper {
          public:
            subselect_mapper() = delete;

            using index_sequence = std::index_sequence_for<Fs...>;
            // this type name is used to detect the mapping from label to object
            using cte_label_type = Label;
            using fields_type = fields_t<Fs...>;
            // this type name is used to detect the mapping from label to object;
            // it only exists to satisfy a table_t<>'s requirement to have a object_type typename
            using cte_object_type = fields_t<Fs...>;
            // this type captures the expressions forming the columns in a subselect;
            // it is currently unused, however proves to be useful in compilation errors,
            // as it simplifies recognizing errors in column expressions
            using expressions_tuple = tuplify_t<Expression>;
            // this type captures column reference expressions;
            // those are: member pointers, alias holders
            using colref_expressions_tuple = tuplify_t<RefExpressions>;
        };
    }
}
