#pragma once

#include <sqlite3.h>
#include <memory>  //  std::shared_ptr
#include <string>  //  std::string
#include <utility>  //  std::forward, std::move
#include <tuple>  //  std::tuple, std::make_tuple

#include "row_extractor.h"
#include "error_code.h"
#include "iterator.h"
#include "ast_iterator.h"
#include "prepared_statement.h"
#include "connection_holder.h"

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
            using self = view_t<T, S, Args...>;

            storage_type& storage;
            connection_ref connection;
            get_all_t<T, std::vector<T>, Args...> args;

            view_t(storage_type& stor, decltype(connection) conn, Args&&... args_) :
                storage(stor), connection(std::move(conn)), args{std::make_tuple(std::forward<Args>(args_)...)} {}

            size_t size() {
                return this->storage.template count<T>();
            }

            bool empty() {
                return !this->size();
            }

            iterator_t<self> begin() {
                using context_t = serializer_context<typename storage_type::impl_type>;
                context_t context{obtain_const_impl(this->storage)};
                context.skip_table_name = false;
                context.replace_bindable_with_question = true;
                auto query = serialize(this->args, context);

                sqlite3* db = this->connection.get();
                sqlite3_stmt* stmt = nullptr;
                int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
                if(ret != SQLITE_OK) {
                    throw_translated_sqlite_error(db);
                }
                iterate_ast(this->args.conditions, conditional_binder{stmt});
                return {stmt, *this};
            }

            iterator_t<self> end() {
                return {};
            }
        };
    }
}
