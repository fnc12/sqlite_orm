#pragma once

#include <type_traits>
#include <tuple>

#include "cxx_polyfill.h"
#include "start_macros.h"
#include "type_traits.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Tuple data structure for CTEs
         */
        template<typename Label, typename... Fs>
        class column_results : fields_t<Fs...> {
          public:
            using fields_type = fields_t<Fs...>;

            template<size_t I>
            decltype(auto) cget() const noexcept {
                return std::get<I>(polyfill::as_template_base<std::tuple>(*this));
            }
            template<size_t I>
            void set(std::tuple_element_t<I, fields_type> v) noexcept {
                std::get<I>(polyfill::as_template_base<std::tuple>(*this)) = std::move(v);
            }
        };

        /**
         *  This class maps an expression of a subselect (basically select_t<>::return_type) to
         *  a tuple of fields (a column_results<Label, Fields>)
         */
        template<typename Label, typename Expression, typename... Fs>
        class subselect_mapper {
          public:
            using index_sequence = std::index_sequence_for<Fs...>;
            // this type name is used to detect the mapping from label to object
            using cte_label_type = Label;
            // this type name is used to detect the mapping from label to object
            using cte_object_type = column_results<Label, Fs...>;
            using expressions_tuple = tuplify_t<Expression>;
        };

        template<class O, size_t I>
        SQLITE_ORM_INLINE_VAR auto cte_getter_v = &O::template cget<I>;
        template<class O, size_t I>
        SQLITE_ORM_INLINE_VAR auto cte_setter_v = &O::template set<I>;

        template<class O, size_t I>
        using cte_getter_t = decltype(cte_getter_v<O, I>);
        template<class O, size_t I>
        using cte_setter_t = decltype(cte_setter_v<O, I>);
    }
}
