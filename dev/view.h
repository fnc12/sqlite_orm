#pragma once

#include <sqlite3.h>
#include <utility>  //  std::forward, std::move

#include "row_extractor.h"
#include "error_code.h"
#include "iterator.h"
#include "ast_iterator.h"
#include "prepared_statement.h"
#include "connection_holder.h"
#include "util.h"

namespace sqlite_orm {

    namespace internal {

        /**
         * This class does not related to SQL view. This is a container like class which is returned by
         * by storage_t::iterate function. This class contains STL functions:
         *  -   size_t size()
         *  -   bool empty()
         *  -   iterator end()
         *  -   iterator begin()
         *  All these functions are not right const cause all of them may open SQLite connections.
         */
        template<class T, class S, class... Args>
        struct view_t {
            using mapped_type = T;
            using storage_type = S;
            using db_objects_type = typename S::db_objects_type;

            storage_type& storage;
            // Note: This is deliberately a pointer, so that an iterator's reference to this pointer is null if the view goes out of scope,
            // and no dangling reference is accessed if an iterator accidentally outlives the view [lifetime]
            const db_objects_type* db_objects;
            connection_ref connection;
            get_all_t<T, void, Args...> expression;

            view_t(storage_type& storage, connection_ref conn, Args&&... args) :
                storage(storage), db_objects(&obtain_db_objects(storage)), connection(std::move(conn)),
                expression{std::forward<Args>(args)...} {}

            ~view_t() {
                this->db_objects = nullptr;
            }

            size_t size() const {
                return this->storage->template count<T>();
            }

            bool empty() const {
                return !this->size();
            }

            iterator_t<T, db_objects_type> begin() {
                using context_t = serializer_context<db_objects_type>;
                context_t context{*this->db_objects};
                context.skip_table_name = false;
                context.replace_bindable_with_question = true;

                statement_finalizer stmt{prepare_stmt(this->connection.get(), serialize(this->expression, context))};
                iterate_ast(this->expression.conditions, conditional_binder{stmt.get()});
                return {this->db_objects, std::move(stmt)};
            }

            iterator_t<T, db_objects_type> end() {
                return {};
            }
        };
    }
}
