#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <utility>  //  std::move, std::remove_cvref
#include <functional>  //  std::reference_wrapper
#if defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED) &&           \
    defined(SQLITE_ORM_CPP20_RANGES_SUPPORTED)
#include <ranges>  //  std::ranges::view_interface
#endif
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "row_extractor.h"
#include "result_set_iterator.h"
#include "ast_iterator.h"
#include "connection_holder.h"
#include "util.h"
#include "type_traits.h"
#include "storage_lookup.h"

#if defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)
namespace sqlite_orm::internal {
    /*  
     *  A C++ view over a result set of a select statement, returned by `storage_t::iterate()`.
     *  
     *  `result_set_view` is also a 'borrowed range',
     *  meaning that iterators obtained from it are not tied to the lifetime of the view instance.
     */
    template<class Select, class DBOs>
    struct result_set_view
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
        : std::ranges::view_interface<result_set_view<Select, DBOs>>
#endif
    {
        using db_objects_type = DBOs;
        using expression_type = Select;

        result_set_view(const db_objects_type& dbObjects, connection_ref conn, Select expression) :
            db_objects{dbObjects}, connection{std::move(conn)}, expression{std::move(expression)} {}

        result_set_view(result_set_view&&) = default;
        result_set_view& operator=(result_set_view&&) = default;
        result_set_view(const result_set_view&) = default;
        result_set_view& operator=(const result_set_view&) = default;

        auto begin() {
            const auto& exprDBOs = db_objects_for_expression(this->db_objects.get(), this->expression);
            using ExprDBOs = std::remove_cvref_t<decltype(exprDBOs)>;
            // note: Select can be `select_t` or `with_t`
            using select_type = polyfill::detected_or_t<expression_type, expression_type_t, expression_type>;
            using column_result_type = column_result_of_t<ExprDBOs, select_type>;
            using context_t = serializer_context<ExprDBOs>;
            context_t context{exprDBOs};
            context.skip_table_name = false;
            context.replace_bindable_with_question = true;

            statement_finalizer stmt{prepare_stmt(this->connection.get(), serialize(this->expression, context))};
            iterate_ast(this->expression, conditional_binder{stmt.get()});

            // note: it is enough to only use the 'expression DBOs' at compile-time to determine the column results;
            // because we cannot select objects/structs from a CTE, passing the permanently defined DBOs are enough.
            using iterator_type = result_set_iterator<column_result_type, db_objects_type>;
            return iterator_type{this->db_objects, std::move(stmt)};
        }

        result_set_sentinel_t end() {
            return {};
        }

      private:
        std::reference_wrapper<const db_objects_type> db_objects;
        connection_ref connection;
        expression_type expression;
    };
}

#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
template<class Select, class DBOs>
inline constexpr bool std::ranges::enable_borrowed_range<sqlite_orm::internal::result_set_view<Select, DBOs>> = true;
#endif
#endif
