#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <string>  //  std::string
#include <functional>  //  std::function
#include <memory>  // std::shared_ptr
#include <vector>  //  std::vector
#include <sstream>
#include <iomanip>  //  std::flush
#endif

#include "error_code.h"
#include "row_extractor.h"
#include "journal_mode.h"
#include "locking_mode.h"
#include "connection_holder.h"
#include "util.h"
#include "serializing_util.h"

namespace sqlite_orm {

    namespace internal {
        struct storage_base;

        template<class T>
        int getPragmaCallback(void* data, int argc, char** argv, char** x) {
            return extract_single_value<T>(data, argc, argv, x);
        }

        template<>
        inline int getPragmaCallback<std::vector<std::string>>(void* data, int argc, char** argv, char**) {
            auto& res = *(std::vector<std::string>*)data;
            res.reserve(argc);
            const auto rowExtractor = column_text_extractor<std::string>();
            for (int i = 0; i < argc; ++i) {
                res.push_back(rowExtractor.extract(argv[i]));
            }
            return 0;
        }

        struct pragma_t {
            using get_connection_t = std::function<internal::connection_ref()>;

            pragma_t(get_connection_t get_connection_) : get_connection(std::move(get_connection_)) {}

            std::vector<std::string> module_list() {
                return this->get_pragma<std::vector<std::string>>("module_list");
            }

            bool recursive_triggers() {
                return bool(this->get_pragma<int>("recursive_triggers"));
            }

            void recursive_triggers(bool value) {
                this->set_pragma("recursive_triggers", int(value));
            }

            void busy_timeout(int value) {
                this->set_pragma("busy_timeout", value);
            }

            int busy_timeout() {
                return this->get_pragma<int>("busy_timeout");
            }

            sqlite_orm::locking_mode locking_mode() {
                return this->get_pragma<sqlite_orm::locking_mode>("locking_mode");
            }

            void locking_mode(sqlite_orm::locking_mode value) {
                this->set_pragma("locking_mode", value);
            }

            sqlite_orm::journal_mode journal_mode() {
                return this->get_pragma<sqlite_orm::journal_mode>("journal_mode");
            }

            void journal_mode(sqlite_orm::journal_mode value) {
                this->journal_mode_ = -1;
                this->set_pragma("journal_mode", value);
                this->journal_mode_ = static_cast<decltype(this->journal_mode_)>(value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_application_id
             */
            int application_id() {
                return this->get_pragma<int>("application_id");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_application_id
             */
            void application_id(int value) {
                this->set_pragma("application_id", value);
            }

            int synchronous() {
                return this->get_pragma<int>("synchronous");
            }

            void synchronous(int value) {
                this->synchronous_ = -1;
                this->set_pragma("synchronous", value);
                this->synchronous_ = value;
            }

            int user_version() {
                return this->get_pragma<int>("user_version");
            }

            void user_version(int value) {
                this->set_pragma("user_version", value);
            }

            int auto_vacuum() {
                return this->get_pragma<int>("auto_vacuum");
            }

            void auto_vacuum(int value) {
                this->set_pragma("auto_vacuum", value);
            }

            int max_page_count() {
                return this->get_pragma<int>("max_page_count");
            }

            void max_page_count(int value) {
                this->set_pragma("max_page_count", value);
            }

            std::vector<std::string> integrity_check() {
                return this->get_pragma<std::vector<std::string>>("integrity_check");
            }

            template<class T>
            std::vector<std::string> integrity_check(T table_name) {
                std::ostringstream ss;
                ss << "integrity_check(" << table_name << ")" << std::flush;
                return this->get_pragma<std::vector<std::string>>(ss.str());
            }

            std::vector<std::string> integrity_check(int n) {
                std::ostringstream ss;
                ss << "integrity_check(" << n << ")" << std::flush;
                return this->get_pragma<std::vector<std::string>>(ss.str());
            }

            std::vector<std::string> quick_check() {
                return this->get_pragma<std::vector<std::string>>("quick_check");
            }

            // will include generated columns in response as opposed to table_info
            std::vector<sqlite_orm::table_xinfo> table_xinfo(const std::string& tableName) const {
                auto connection = this->get_connection();

                std::vector<sqlite_orm::table_xinfo> result;
                std::ostringstream ss;
                ss << "PRAGMA "
                      "table_xinfo("
                   << streaming_identifier(tableName) << ")" << std::flush;
                perform_exec(
                    connection.get(),
                    ss.str(),
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(std::vector<sqlite_orm::table_xinfo>*)data;
                        if (argc) {
                            auto index = 0;
                            auto cid = atoi(argv[index++]);
                            std::string name = argv[index++];
                            std::string type = argv[index++];
                            bool notnull = !!atoi(argv[index++]);
                            std::string dflt_value = argv[index] ? argv[index] : "";
                            ++index;
                            auto pk = atoi(argv[index++]);
                            auto hidden = atoi(argv[index++]);
                            res.emplace_back(cid,
                                             std::move(name),
                                             std::move(type),
                                             notnull,
                                             std::move(dflt_value),
                                             pk,
                                             hidden);
                        }
                        return 0;
                    },
                    &result);
                return result;
            }

            std::vector<sqlite_orm::table_info> table_info(const std::string& tableName) const {
                auto connection = this->get_connection();

                std::ostringstream ss;
                ss << "PRAGMA "
                      "table_info("
                   << streaming_identifier(tableName) << ")" << std::flush;
                std::vector<sqlite_orm::table_info> result;
                perform_exec(
                    connection.get(),
                    ss.str(),
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(std::vector<sqlite_orm::table_info>*)data;
                        if (argc) {
                            auto index = 0;
                            auto cid = atoi(argv[index++]);
                            std::string name = argv[index++];
                            std::string type = argv[index++];
                            bool notnull = !!atoi(argv[index++]);
                            std::string dflt_value = argv[index] ? argv[index] : "";
                            ++index;
                            auto pk = atoi(argv[index++]);
                            res.emplace_back(cid, std::move(name), std::move(type), notnull, std::move(dflt_value), pk);
                        }
                        return 0;
                    },
                    &result);
                return result;
            }

          private:
            friend struct storage_base;

            int synchronous_ = -1;
            signed char journal_mode_ = -1;  //  if != -1 stores static_cast<sqlite_orm::journal_mode>(journal_mode)
            get_connection_t get_connection;

            template<class T>
            T get_pragma(const std::string& name) {
                auto connection = this->get_connection();
                T result;
                perform_exec(connection.get(), "PRAGMA " + name, getPragmaCallback<T>, &result);
                return result;
            }

            /**
             *  Yevgeniy Zakharov: I wanted to refactor this function with statements and value bindings
             *  but it turns out that bindings in pragma statements are not supported.
             */
            template<class T>
            void set_pragma(const std::string& name, const T& value, sqlite3* db = nullptr) {
                std::stringstream ss;
                ss << "PRAGMA " << name << " = " << value;
                this->set_pragma_impl(ss.str(), db);
            }

            void set_pragma(const std::string& name, sqlite_orm::journal_mode value, sqlite3* db = nullptr) {
                std::stringstream ss;
                ss << "PRAGMA " << name << " = " << journal_mode_to_string(value);
                this->set_pragma_impl(ss.str(), db);
            }

            void set_pragma(const std::string& name, sqlite_orm::locking_mode value, sqlite3* db = nullptr) {
                std::stringstream ss;
                ss << "PRAGMA " << name << " = " << locking_mode_to_string(value);
                this->set_pragma_impl(ss.str(), db);
            }

            void set_pragma_impl(const std::string& query, sqlite3* db = nullptr) {
                auto con = this->get_connection();
                if (db == nullptr) {
                    db = con.get();
                }
                perform_void_exec(db, query);
            }
        };
    }
}
