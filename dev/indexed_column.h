#pragma once

#include <string>  //  std::string

namespace sqlite_orm {

    namespace internal {

        template<class C>
        struct indexed_column_t {
            using column_type = C;

            indexed_column_t(column_type _column_or_expression) :
                column_or_expression(std::move(_column_or_expression)) {}

            column_type column_or_expression;
            std::string _collation_name;
            int _order = 0;  //  -1 = desc, 1 = asc, 0 = not specified

            indexed_column_t<column_type> collate(std::string name) {
                auto res = std::move(*this);
                res._collation_name = move(name);
                return res;
            }

            indexed_column_t<column_type> asc() {
                auto res = std::move(*this);
                res._order = 1;
                return res;
            }

            indexed_column_t<column_type> desc() {
                auto res = std::move(*this);
                res._order = -1;
                return res;
            }
        };

        template<class C>
        struct indexed_column_maker {
            using type = indexed_column_t<C>;

            indexed_column_t<C> operator()(C col) const {
                return {std::move(col)};
            }
        };

        template<class C>
        struct indexed_column_maker<indexed_column_t<C>> {
            using type = indexed_column_t<C>;

            indexed_column_t<C> operator()(indexed_column_t<C> col) const {
                return std::move(col);
            }
        };

        template<class C>
        auto make_indexed_column(C col) {
            indexed_column_maker<C> maker;
            return maker(std::move(col));
        }

    }

    /**
     * Use this function to specify indexed column inside `make_index` function call.
     * Example: make_index("index_name", indexed_column(&User::id).asc())
     */
    template<class C>
    internal::indexed_column_t<C> indexed_column(C column_or_expression) {
        return {std::move(column_or_expression)};
    }

}
