#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <sqlite3.h>
#include <utility>  //  std::forward, std::move
#endif

#include "row_extractor.h"
#include "mapped_iterator.h"
#include "ast_iterator.h"
#include "prepared_statement.h"
#include "connection_holder.h"
#include "util.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  A C++ view over a result set of objects mapped as tables, returned by `storage_t::iterate<>()`.
         *  
         *  Models a C++ input range and is also a 'borrowed range',
         *  meaning that iterators obtained from it are not tied to the lifetime of the view instance.
         *  
         *  Its `begin()` and `end()` methods are non-const to leave room for different implementation details.
         */
        template<class T, class S, class... Args>
        struct mapped_view {
            using mapped_type = T;
            using storage_type = S;
            using db_objects_type = typename S::db_objects_type;

            storage_type& storage;
            connection_ref connection;
            get_all_t<T, void, Args...> expression;

            mapped_view(storage_type& storage, connection_ref conn, Args&&... args) :
                storage(storage), connection(std::move(conn)), expression{{std::forward<Args>(args)...}} {}

            mapped_iterator<T, db_objects_type> begin() {
                using context_t = serializer_context<db_objects_type>;

                auto& dbObjects = obtain_db_objects(this->storage);
                context_t context{dbObjects};
                context.omit_table_name = false;
                context.replace_bindable_with_question = true;

                const std::string sql = serialize(this->expression, context);
                statement_finalizer stmt{prepare_stmt(this->connection.get(), sql)};
                iterate_ast(this->expression.conditions, conditional_binder{stmt.get()});
                return {dbObjects, std::move(stmt)};
            }

            mapped_iterator<T, db_objects_type> end() {
                return {};
            }
        };
    }
}

#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
template<class T, class S, class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<sqlite_orm::internal::mapped_view<T, S, Args...>> = true;
#endif
