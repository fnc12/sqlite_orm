#pragma once

#include <set>  //  std::set
#include <string>  //  std::string
#include <functional>  //  std::function
#include <typeindex>  //  std::type_index

#include "select_constraints.h"
#include "alias.h"
#include "core_functions.h"

namespace sqlite_orm {

    namespace internal {

        struct table_name_collector {
            using table_name_set = std::set<std::pair<std::string, std::string>>;
            using find_table_name_t = std::function<std::string(std::type_index)>;

            find_table_name_t find_table_name;
            mutable table_name_set table_names;

            table_name_collector() = default;

            table_name_collector(find_table_name_t _find_table_name) : find_table_name(std::move(_find_table_name)) {}

            template<class T>
            table_name_set operator()(const T &) const {
                return {};
            }

            template<class F, class O>
            void operator()(F O::*, std::string alias = {}) const {
                if(this->find_table_name) {
                    table_names.insert(std::make_pair(this->find_table_name(typeid(O)), move(alias)));
                }
            }

            template<class T, class F>
            void operator()(const column_pointer<T, F> &) const {
                if(this->find_table_name) {
                    table_names.insert({this->find_table_name(typeid(T)), ""});
                }
            }

            template<class T, class C>
            void operator()(const alias_column_t<T, C> &a) const {
                (*this)(a.column, alias_extractor<T>::get());
            }

            template<class T>
            void operator()(const count_asterisk_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    if(!tableName.empty()) {
                        table_names.insert(std::make_pair(move(tableName), ""));
                    }
                }
            }

            template<class T>
            void operator()(const asterisk_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    table_names.insert(std::make_pair(move(tableName), ""));
                }
            }

            template<class T>
            void operator()(const object_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    table_names.insert(std::make_pair(move(tableName), ""));
                }
            }

            template<class T>
            void operator()(const table_rowid_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    table_names.insert(std::make_pair(move(tableName), ""));
                }
            }

            template<class T>
            void operator()(const table_oid_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    table_names.insert(std::make_pair(move(tableName), ""));
                }
            }

            template<class T>
            void operator()(const table__rowid_t<T> &) const {
                if(this->find_table_name) {
                    auto tableName = this->find_table_name(typeid(T));
                    table_names.insert(std::make_pair(move(tableName), ""));
                }
            }
        };

    }

}
