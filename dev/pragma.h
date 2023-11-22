#pragma once

#include <sqlite3.h>
#include <string>  //  std::string
#include <functional>  //  std::function
#include <memory>  // std::shared_ptr
#include <vector>  //  std::vector
#include <sstream>

#include "error_code.h"
#include "row_extractor.h"
#include "journal_mode.h"
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
            for(int i = 0; i < argc; ++i) {
                auto rowString = rowExtractor.extract(argv[i]);
                res.push_back(std::move(rowString));
            }
            return 0;
        }

        struct pragma_t {
            using get_connection_t = std::function<internal::connection_ref()>;

            pragma_t(get_connection_t get_connection_) : get_connection(std::move(get_connection_)) {}

            /**
             *  https://www.sqlite.org/pragma.html#pragma_module_list
             */
            std::vector<std::string> module_list() {
                return this->get_pragma<std::vector<std::string>>("module_list");
            }

            template<class T>
            void rekey(T value) {
                this->set_pragma("rekey", value);
            }

            template<class T>
            void key(T value) {
                this->set_pragma("key", value);
            }

            template<class T>
            void textrekey(T value) {
                this->set_pragma("textrekey", value);
            }

            template<class T>
            void textkey(T value) {
                this->set_pragma("textkey", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_temp_store
             */
            int temp_store() {
                return this->get_pragma<int>("temp_store");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_temp_store
             */
            void temp_store(int value) {
                this->set_pragma("temp_store", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_cache_size
             */
            void cache_size(int value) {
                this->set_pragma("cache_size", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_cache_size
             */
            int cache_size() {
                return this->get_pragma<int>("cache_size");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_threads
             */
            void threads(int value) {
                this->set_pragma("threads", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_threads
             */
            int threads() {
                return this->get_pragma<int>("threads");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_busy_timeout
             */
            void busy_timeout(int value) {
                this->set_pragma("busy_timeout", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_busy_timeout
             */
            int busy_timeout() {
                return this->get_pragma<int>("busy_timeout");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_journal_mode
             */
            sqlite_orm::journal_mode journal_mode() {
                return this->get_pragma<sqlite_orm::journal_mode>("journal_mode");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_journal_mode
             */
            void journal_mode(sqlite_orm::journal_mode value) {
                this->_journal_mode = -1;
                this->set_pragma("journal_mode", value);
                this->_journal_mode = static_cast<decltype(this->_journal_mode)>(value);
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

            /**
             *  https://www.sqlite.org/pragma.html#pragma_synchronous
             */
            int synchronous() {
                return this->get_pragma<int>("synchronous");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_synchronous
             */
            void synchronous(int value) {
                this->_synchronous = -1;
                this->set_pragma("synchronous", value);
                this->_synchronous = value;
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_user_version
             */
            int user_version() {
                return this->get_pragma<int>("user_version");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_user_version
             */
            void user_version(int value) {
                this->set_pragma("user_version", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_auto_vacuum
             */
            int auto_vacuum() {
                return this->get_pragma<int>("auto_vacuum");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_auto_vacuum
             */
            void auto_vacuum(int value) {
                this->set_pragma("auto_vacuum", value);
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_integrity_check
             */
            std::vector<std::string> integrity_check() {
                return this->get_pragma<std::vector<std::string>>("integrity_check");
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_integrity_check
             */
            template<class T>
            std::vector<std::string> integrity_check(T table_name) {
                std::ostringstream ss;
                ss << "integrity_check(" << table_name << ")" << std::flush;
                return this->get_pragma<std::vector<std::string>>(ss.str());
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_integrity_check
             */
            std::vector<std::string> integrity_check(int n) {
                std::ostringstream ss;
                ss << "integrity_check(" << n << ")" << std::flush;
                return this->get_pragma<std::vector<std::string>>(ss.str());
            }

            /**
             *  https://www.sqlite.org/pragma.html#pragma_table_xinfo
             *  will include generated columns in response as opposed to table_info
             */
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
                        if(argc) {
                            auto index = 0;
                            auto cid = std::atoi(argv[index++]);
                            std::string name = argv[index++];
                            std::string type = argv[index++];
                            bool notnull = !!std::atoi(argv[index++]);
                            std::string dflt_value = argv[index] ? argv[index] : "";
                            ++index;
                            auto pk = std::atoi(argv[index++]);
                            auto hidden = std::atoi(argv[index++]);
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

            /**
             *  https://www.sqlite.org/pragma.html#pragma_table_info
             */
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
                        if(argc) {
                            auto index = 0;
                            auto cid = std::atoi(argv[index++]);
                            std::string name = argv[index++];
                            std::string type = argv[index++];
                            bool notnull = !!std::atoi(argv[index++]);
                            std::string dflt_value = argv[index] ? argv[index] : "";
                            ++index;
                            auto pk = std::atoi(argv[index++]);
                            res.emplace_back(cid, std::move(name), std::move(type), notnull, std::move(dflt_value), pk);
                        }
                        return 0;
                    },
                    &result);
                return result;
            }

          private:
            friend struct storage_base;

            int _synchronous = -1;
            signed char _journal_mode = -1;  //  if != -1 stores static_cast<sqlite_orm::journal_mode>(journal_mode)
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
                auto con = this->get_connection();
                if(!db) {
                    db = con.get();
                }
                std::stringstream ss;
                ss << "PRAGMA " << name << " = " << value << std::flush;
                perform_void_exec(db, ss.str());
            }

            void set_pragma(const std::string& name, const sqlite_orm::journal_mode& value, sqlite3* db = nullptr) {
                auto con = this->get_connection();
                if(!db) {
                    db = con.get();
                }
                std::stringstream ss;
                ss << "PRAGMA " << name << " = " << to_string(value) << std::flush;
                perform_void_exec(db, ss.str());
            }
        };
    }
}
