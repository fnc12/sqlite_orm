
#ifndef sqlite_orm_h
#define sqlite_orm_h

#include <string>
#include <tuple>
#include <type_traits>
#include <sqlite3.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>
#include <typeinfo>

using std::cout;
using std::endl;

namespace sqlite_orm {
    
    //  got from here http://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
    namespace tuple_helper {
        
        template <typename T, typename Tuple>
        struct has_type;
        
        template <typename T>
        struct has_type<T, std::tuple<>> : std::false_type {};
        
        template <typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};
        
        template <typename T, typename... Ts>
        struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};
        
        template <typename T, typename Tuple>
        using tuple_contains_type = typename has_type<T, Tuple>::type;
    }
    
    struct autoincrement {};
    
    struct not_null {};
    
    struct primary_key {};
    
    template<class T, T t>
    struct default_value {
        typedef T value_type;
        
        static value_type value = t;
    };
    
    template<class O, class T, class ...Op>
    struct column {
        typedef O object_type;
        typedef T field_type;
        typedef std::tuple<Op...> options_type;
        typedef field_type object_type::*member_pointer_t;
        
        const std::string name;
        field_type object_type::*member_pointer;
        
        template<class Opt>
        constexpr bool has() {
            return tuple_helper::tuple_contains_type<Opt, options_type>::value;
        }
        
        template<class O1, class O2, class ...Opts>
        constexpr bool has_every() {
            if(has<O1>() && has<O2>()) {
                return true;
            }else{
                return has_every<Opts...>();
            }
        }
        
        template<class O1>
        constexpr bool has_every() {
            return has<O1>();
        }
    };
    
    /**
     *  This class accepts c++ type and transfers it to sqlite name (int -> integer, std::string -> text)
     */
    template<class T>
    struct type_printer;
    
    template<>
    struct type_printer<int> {
        inline const std::string& print() {
            static const std::string res = "INTEGER";
            return res;
        }
    };
    
    template<>
    struct type_printer<std::string> {
        inline const std::string& print() {
            static const std::string res = "TEXT";
            return res;
        }
    };
    
    template<>
    struct type_printer<double> {
        inline const std::string& print() {
            static const std::string res = "REAL";
            return res;
        }
    };
    
    /**
     *  Column builder function. You should use it to create columns and not constructor.
     */
    template<class O, class T, class ...Op>
    column<O, T, Op...> make_column(const std::string &name, T O::*m, Op ...options){
        return {name, m};
    }
    
    /**
     *  Common case for table_impl class.
     */
    template<typename... Args>
    struct table_impl {
        
        std::vector<std::string> column_names() { return {}; }
        
        template<class ...Op>
        std::vector<std::string> column_names_exept() { return {}; }
        
        template<class ...Op>
        std::vector<std::string> column_names_with() { return{}; }
        
        template<class L>
        void for_each_column(L l) {}
        
        template<class F, class L>
        void for_each_column_with_field_type(L l) {}
        
        template<class Op, class L>
        void for_each_column_exept(L l){}
        
        template<class Op, class L>
        void for_each_column_with(L l) {}
        
        int columns_count() const {
            return 0;
        }
        
    };
    
    template<typename H, typename... T>
    struct table_impl<H, T...> : private table_impl<T...> {
        typedef H column_type;
        typedef std::tuple<T...> tail_types;
        
        table_impl(H h, T ...t) : col(h), Super(t...) {}
        
        column_type col;
        
        int columns_count() const {
            return 1 + Super::columns_count();
        }
        
        /**
         *  column_names implementation. Notice that result will be reversed.
         *  It is reversed back in `table` class.
         */
        std::vector<std::string> column_names() {
            auto res = Super::column_names();
            res.emplace_back(col.name);
            return res;
        }
        
        /**
         *  column_names_with implementation. Notice that result will be reversed.
         *  It is reversed back in `table` class.
         *  @return vector of column names that have specified Op... conditions.
         */
        template<class ...Op>
        std::vector<std::string> column_names_with() {
            auto res = Super::template column_names_with<Op...>();
            if(col.template has_every<Op...>()) {
                res.emplace_back(col.name);
            }
            return res;
        }
        
        /**
         *  For each implementation. Calls templated lambda with its column
         *  and passed call to superclass.
         */
        template<class L>
        void for_each_column(L l){
            l(col);
            Super::for_each_column(l);
        }
        
//        template<typename CFT = typename column_type::field_type>
        /*template<class F, class L, class = typename std::enable_if<std::is_same<F, typename column_type::field_type>::value>::type>
        void for_each_column_with_field_type(L l) {
            l(col);
            Super::template for_each_column_with_field_type<F, L>(l);
        }*/
        
        template<class F, class L>
        void for_each_column_with_field_type(L l) {
//            cout<<"typename column_type::field_type = "<<typeid(typename column_type::field_type).name()<<endl;
//            cout<<"F = "<<typeid(F).name()<<endl;
//            cout<<"std::is_same<F, typename column_type::field_type>::value = "<<std::is_same<F, typename column_type::field_type>::value<<endl;
            Super::template for_each_column_with_field_type<F, L>(l);
        }
        
        template<class F, class L>
        void for_each_column_with_field_type(typename std::enable_if<std::is_same<F, typename column_type::field_type>::value, L>::type l) {
//            cout<<"typename column_type::field_type = "<<typeid(typename column_type::field_type).name()<<endl;
//            cout<<"F = "<<typeid(F).name()<<endl;
//            cout<<"custom version"<<endl;
            l(col);
            Super::template for_each_column_with_field_type<F, L>(l);
        }
        
        /**
         *  Working version of `for_each_column_exept`. Calls lambda and fire super's function.
         */
        template<class Op, class L, std::enable_if<!tuple_helper::tuple_contains_type<Op, typename column_type::options_type>::value>>
        void for_each_column_exept(L l) {
            l(col);
            Super::template for_each_column_exept<Op, L>(l);
        }
        
        /**
         *  Version of `for_each_column_exept` for case if column has not option. Doesn't call lambda and just fure super's function.
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            Super::template for_each_column_exept<Op, L>(l);
        }
        
        /**
         *  Working version of `for_each_column_with`. Calls lambda and fire super's function.
         */
        template<class Op, class L, std::enable_if<tuple_helper::tuple_contains_type<Op, typename column_type::options_type>::value>>
        void for_each_column_with(L l) {
            l(col);
            Super::template for_each_column_with<Op, L>(l);
        }
        
        /**
         *  Version of `for_each_column_with` for case if column has not option. Doesn't call lambda and just fure super's function.
         */
        template<class Op, class L>
        void for_each_column_with(L l) {
            Super::template for_each_column_with<Op, L>(l);
        }
    private:
        typedef table_impl<T...> Super;
    };
    
    /**
     *  Table interface class. Implementation is hidden in `table_impl` class.
     */
    template<class ...Cs>
    struct table {
        typedef table_impl<Cs...> impl_type;
        typedef typename std::tuple_element<0, std::tuple<Cs...>>::type::object_type object_type;
        
        /**
         *  Table name.
         */
        const std::string name;
        
        /**
         *  Implementation that stores columns information.
         */
        impl_type impl;
        
        /**
         *  @return vector of column names of table.
         */
        std::vector<std::string> column_names() {
            auto res = impl.column_names();
            std::reverse(res.begin(),
                         res.end());
            return res;
        }
        
        int columns_count() const {
            return impl.columns_count();
        }
        
        /**
         *  Searches column name by class member pointer passed as first argument.
         *  @return column name or empty string if nothing found.
         */
        template<class F, class O>
        std::string find_column_name(F O::*m) throw() {
            std::string res;
            this->template for_each_column_with_field_type<F>([&](auto c) {
                if(c.member_pointer == m) {
                    res = c.name;
                }/*else{
                    cout<<"not found"<<endl;
                }*/
            });
            return res;
        }
        
        /**
         *  @return vector of column names that have options provided as template arguments (not_null, autoincrement).
         */
        template<class ...Op>
        std::vector<std::string> column_names_with() {
            auto res = impl.template column_names_with<Op...>();
            std::reverse(res.begin(),
                         res.end());
            return res;
        }
        
        /**
         *  Iterates all columns and fires passed lambda. Lambda must have one and only templated argument Otherwise code will
         *  not compile.
         *  @param L Lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class L>
        void for_each_column(L l) {
            impl.for_each_column(l);
        }
        
        template<class F, class L>
        void for_each_column_with_field_type(L l) {
            impl.template for_each_column_with_field_type<F, L>(l);
        }
        
        /**
         *  Iterates all columns exept ones that have specified options and fires passed lambda. 
         *  Lambda must have one and only templated argument Otherwise code will not compile.
         *  @param L Lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            impl.template for_each_column_exept<Op>(l);
        }
        
        /**
         *  Iterates all columns that have specified options and fires passed lambda.
         *  Lambda must have one and only templated argument Otherwise code will not compile.
         *  @param L Lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class Op, class L>
        void for_each_column_with(L l) {
            impl.template for_each_column_with<Op>(l);
        }
        
    };
    
    /**
     *  Function used for table creation. Do not use table constructor - use this function
     *  cause table class is templated and its constructing too (just like std::make_shared or std::make_pair).
     */
    template<class ...Cs>
    table<Cs...> make_table(const std::string &name, Cs ...args) {
        return {name, table_impl<Cs...>(args...)};
    }
    
    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V>
    struct statement_binder;
    
    /**
     *  Specialization for int.
     */
    template<>
    struct statement_binder<int> {
        
        int bind(sqlite3_stmt *stmt, int index, const int &value) {
            return sqlite3_bind_int(stmt, index++, value);
        }
    };
    
    template<>
    struct statement_binder<double> {
        
        int bind(sqlite3_stmt *stmt, int index, const double &value) {
            return sqlite3_bind_double(stmt, index++, value);
//            return sqlite3_bind_int(stmt, index++, value);
        }
    };
    
    /**
     *  Specialization for std::string.
     */
    template<>
    struct statement_binder<std::string> {
        
        int bind(sqlite3_stmt *stmt, int index, const std::string &value) {
            return sqlite3_bind_text(stmt, index++, value.c_str(), -1, SQLITE_TRANSIENT);
        }
    };
    
    /**
     *  Specialization for std::nullptr_t.
     */
    template<>
    struct statement_binder<std::nullptr_t> {
        
        int bind(sqlite3_stmt *stmt, int index, const std::nullptr_t &) {
            return sqlite3_bind_null(stmt, index++);
        }
    };
    
    /**
     *  Specialization for optional type (std::shared_ptr).
     */
    template<class T>
    struct statement_binder<std::shared_ptr<T>>{
        
        int bind(sqlite3_stmt *stmt, int index, const std::shared_ptr<T> &value) {
            if(value){
                return statement_binder<T>().bind(stmt, index, *value);
            }else{
//                return sqlite3_bind_null(stmt, index++);
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }
    };
    
    /**
     *  Helper class used to cast values from argv to V class 
     *  which depends from column type.
     *
     */
    template<class V>
    struct row_extrator {
        V extract(const char *row_value);
    };
    
    /**
     *  Specialization for int.
     */
    template<>
    struct row_extrator<int> {
        int extract(const char *row_value) {
            return std::atoi(row_value);
        }
    };
    
    /**
     *  Specialization for std::string.
     */
    template<>
    struct row_extrator<std::string> {
        std::string extract(const char *row_value) {
            return row_value;
        }
    };
    
    /**
     *  Exeption thrown if nothing was found in database with specified id.
     */
    struct not_found_exception : public std::exception {
        
        virtual const char* what() const throw() override {
            return "Not found";
        };
    };
    
    /**
     *  Help class. Used to pass data to sqlite C callback. Doesn't need
     *  to be used explicitly by user.
     */
    template<class O, class H>
    struct data_t {
        H t;
        O *res;
    };
    
    template<class ...Ts>
    struct storage_impl {
        
        template<class T>
        constexpr bool type_is_mapped() const {
            return false;
        }
        
        template<class O>
        int insert(const O &o, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in insert");
        }
        
        template<class O>
        std::vector<O> get_all(const std::string &filename) throw(std::runtime_error){
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_all");
        }
        
        template<class O>
        O get(int id, const std::string &filename) throw (not_found_exception, std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get");
        }
        
        template<class O>
        void update(const O &o, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in update");
        }
        
        template<class O>
        std::shared_ptr<O> get_no_throw(int id, const std::string &filename) throw(std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_no_throw");
        }
        
        template<class O>
        void remove(int id, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in remove");
        }
        
        template<class O>
        int count(const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }
        
        template<class F, class O>
        double avg(F O::*m, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in avg");
        }
        
        template<class F, class O>
        int count(F O::*m, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }
        
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }
        
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &y, const std::string &filename) throw (std::runtime_error) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }
    };
    
    template<class H, class ...Ts>
    struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
        typedef H table_type;
        
        storage_impl(H h, Ts ...ts) : table(h), Super(ts...) {}
        
        table_type table;
        
        template<class T>
        constexpr bool type_is_mapped() const {
            return Super::template type_is_mapped<T>();
//            return res;
        }
        
        /*template<class T, class HH = H>
        constexpr typename std::enable_if<std::is_same<T, HH>::value, bool>::type type_is_mapped() {
            return true;
        }*/
        
        void create_table(const std::string &filename) throw (std::runtime_error) {
            withDatabase([=] (auto db) {
                std::stringstream ss;
                ss << "CREATE TABLE " << table.name << " ( ";
                auto columnsCount = table.columns_count();
                auto index = 0;
                table.for_each_column([&] (auto c) {
                    ss << c.name << " ";
                    ss << type_printer<typename decltype(c)::field_type>().print() << " ";
                    if(c.template has<primary_key>()) {
                        ss << "primary key ";
                    }
                    if(c.template has<autoincrement>()) {
                        ss << "autoincrement ";
                    }
                    if(c.template has<not_null>()) {
                        ss << "not null ";
                    }
                    if(index < columnsCount - 1) {
                        ss << ", ";
                    }
                    index++;
                });
                ss << ") ";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, int>::type insert(const O &o, const std::string &filename) throw (std::runtime_error) {
//            cout<<"std::is_same<O, HH>::value = "<<std::is_same<O, HH>::value<<endl;
//            cout<<"O = "<<typeid(O).name()<<", HH = "<<typeid(HH).name()<<endl;
            return Super::template insert(o, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, int>::type insert(const O &o, const std::string &filename) throw (std::runtime_error) {
            int res = 0;
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "INSERT INTO " << table.name << " (";
                std::vector<std::string> columnNames;
                table.template for_each_column([&] (auto c) {
                    if(!c.template has<primary_key>()) {
                        columnNames.emplace_back(c.name);
                    }
                });
                
                for(auto i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << ") ";
                    }
                }
                ss << "VALUES(";
                for(auto i = 0; i < columnNames.size(); ++i) {
                    ss << "?";
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    auto index = 1;
                    table.template for_each_column([&] (auto c) {
                        if(!c.template has<primary_key>()){
                            auto &value = o.*c.member_pointer;
                            statement_binder<typename decltype(c)::field_type>().bind(stmt, index++, value);
                        }
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        res = int(sqlite3_last_insert_rowid(db));
                    }else{
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, std::string>::type group_concat(F O::*m, const std::string &y, const std::string &filename) throw (std::runtime_error) {
            return Super::group_concat(m, y, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, std::string>::type group_concat(F O::*m, const std::string &y, const std::string &filename) throw (std::runtime_error) {
            std::string res;
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select group_concat(";
                auto columnName = table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ",\"" << y << "\") from "<< table.name;
                    auto query = ss.str();
                    data_t<std::string, decltype(this)> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::string, decltype(this)>*)data;
                                               auto &res = *d.res;
                                               if(argc){
                                                   res = argv[0];
                                               }
                                               return 0;
                                           }, &data, nullptr);
                    if(rc != SQLITE_OK) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
            }, filename);
            return res;
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, std::string>::type group_concat(F O::*m, const std::string &filename) throw (std::runtime_error) {
            return Super::group_concat(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, std::string>::type group_concat(F O::*m, const std::string &filename) throw (std::runtime_error) {
            std::string res;
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select group_concat(";
                auto columnName = table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< table.name;
                    auto query = ss.str();
                    data_t<std::string, decltype(this)> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::string, decltype(this)>*)data;
                                               auto &res = *d.res;
                                               if(argc){
                                                   res = argv[0];
                                               }
                                               return 0;
                                           }, &data, nullptr);
                    if(rc != SQLITE_OK) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
            }, filename);
            return res;
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, double>::type avg(F O::*m, const std::string &filename) throw (std::runtime_error) {
            return Super::template avg(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, double>::type avg(F O::*m, const std::string &filename) throw (std::runtime_error) {
            double res = 0;
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select avg(";
                auto columnName = table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< table.name;
                    auto query = ss.str();
                    data_t<double, decltype(this)> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<double, decltype(this)>*)data;
                                               auto &res = *d.res;
//                                               auto t = d.t;
                                               if(argc){
                                                   res = std::atof(argv[0]);
                                               }
                                               return 0;
                                           }, &data, nullptr);
                    if(rc != SQLITE_OK) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
            }, filename);
            return res;
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, int>::type count(F O::*m, const std::string &filename) throw (std::runtime_error) {
            return Super::template count(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, int>::type count(F O::*m, const std::string &filename) throw (std::runtime_error) {
            int res = 0;
            withDatabase([&](auto db){
                std::stringstream ss;
                ss << "select count(";
                auto columnName = table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< table.name;
                    auto query = ss.str();
                    data_t<int, decltype(this)> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<int, decltype(this)>*)data;
                                               auto &res = *d.res;
//                                               auto t = d.t;
                                               if(argc){
                                                   res = std::atoi(argv[0]);
                                               }
                                               return 0;
                                           }, &data, nullptr);
                    if(rc != SQLITE_OK) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
            }, filename);
            return res;
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, int>::type count(const std::string &filename) throw (std::runtime_error) {
            return Super::template count<O>(filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, int>::type count(const std::string &filename) throw (std::runtime_error){
            int res = 0;
            withDatabase([&](auto db) {
                auto query = "select count(*) from " + table.name;
                data_t<int, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_t<int, decltype(this)>*)data;
                                           auto &res = *d.res;
//                                           auto t = d.t; 
                                           if(argc){
                                               res = std::atoi(argv[0]);
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, std::shared_ptr<O>>::type get_no_throw(int id, const std::string &filename) throw(std::runtime_error) {
            return Super::template get_no_throw<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, std::shared_ptr<O>>::type get_no_throw(int id, const std::string &filename) throw(std::runtime_error) {
            std::shared_ptr<O> res;
            withDatabase([&](auto db) {
                auto query = "select * from " + table.name + " where id = " + std::to_string(id);
                data_t<std::shared_ptr<O>, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_t<std::shared_ptr<O>, decltype(this)>*)data;
                                           auto &res = *d.res;
                                           auto t = d.t;
                                           if(argc){
                                               res = std::make_shared<O>();
                                               auto index = 0;
                                               t->table.for_each_column([&] (auto c) {
                                                   auto &o = *res;
                                                   auto member_pointer = c.member_pointer;
                                                   auto value = row_extrator<typename decltype(c)::field_type>().extract(argv[index++]);
                                                   o.*member_pointer = value;
                                               });
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, O>::type get(int id, const std::string &filename) throw (not_found_exception, std::runtime_error) {
            return Super::template get<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, O>::type get(int id, const std::string &filename) throw (not_found_exception, std::runtime_error) {
            std::shared_ptr<O> res;
            withDatabase([&](auto db) {
                auto query = "select * from " + table.name + " where id = " + std::to_string(id);
                data_t<std::shared_ptr<O>, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_t<std::shared_ptr<O>, decltype(this)>*)data;
                                           auto &res = *d.res;
                                           auto t = d.t;
                                           if(argc){
                                               res = std::make_shared<O>();
                                               auto index = 0;
                                               t->table.for_each_column([&] (auto c) {
                                                   auto &o = *res;
                                                   auto member_pointer = c.member_pointer;
                                                   auto value = row_extrator<typename decltype(c)::field_type>().extract(argv[index++]);
                                                   o.*member_pointer = value;
                                               });
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            if(res){
                return *res;
            }else{
                throw not_found_exception{};
            }
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<!std::is_same<O, HH>::value, std::vector<O>>::type get_all(const std::string &filename) throw(std::runtime_error) {
            return Super::template get_all<O>(filename);
        }
        
        template<class O, class HH = typename H::object_type>
        typename std::enable_if<std::is_same<O, HH>::value, std::vector<O>>::type get_all(const std::string &filename) throw(std::runtime_error) {
            std::vector<O> res;
            withDatabase([&](auto db) {
                auto query = "select * from " + table.name;
                data_t<std::vector<O>, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(data_t<std::vector<O>, decltype(this)>*)data;
                                           auto &res = *d.res;
                                           auto t = d.t;
                                           if(argc){
                                               O o;
                                               auto index = 0;
                                               t->table.for_each_column([&] (auto c) {
                                                   auto member_pointer = c.member_pointer;
                                                   auto value = row_extrator<typename decltype(c)::field_type>().extract(argv[index++]);
                                                   o.*member_pointer = value;
                                               });
                                               res.emplace_back(std::move(o));
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        template<class O, class HH = typename H::object_type>
        void update(const O &o, const typename std::enable_if<!std::is_same<O, HH>::value, std::string>::type &filename) throw (std::runtime_error) {
            Super::template update(o, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        void update(const O &o, const typename std::enable_if<std::is_same<O, HH>::value, std::string>::type &filename) throw (std::runtime_error) {
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "update " << table.name << " set ";
                std::vector<std::string> setColumnNames;
                table.template for_each_column( [&] (auto c) {
                    if(!c.template has<primary_key>()) {
                        setColumnNames.emplace_back(c.name);
                    }
                });
                for(auto i = 0; i < setColumnNames.size(); ++i) {
                    ss << setColumnNames[i] << " = ?";
                    if(i < setColumnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << "where ";
                auto primaryKeyColumnNames = table.template column_names_with<primary_key>();
                for(auto i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << primaryKeyColumnNames[i] << " = ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " and ";
                    }else{
                        ss << " ";
                    }
                }
                
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    auto index = 1;
                    table.template for_each_column([&] (auto c) {
                        if(!c.template has<primary_key>()) {
                            auto &value = o.*c.member_pointer;
                            statement_binder<typename decltype(c)::field_type>().bind(stmt, index++, value);
                        }
                    });
                    table.template for_each_column([&] (auto c) {
                        if(c.template has<primary_key>()) {
                            auto &value = o.*c.member_pointer;
                            statement_binder<typename decltype(c)::field_type>().bind(stmt, index++, value);
                        }
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        void remove(int id, const typename std::enable_if<!std::is_same<O, HH>::value, std::string>::type &filename) throw (std::runtime_error) {
            Super::template remove<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        void remove(int id, const typename std::enable_if<std::is_same<O, HH>::value, std::string>::type &filename) throw (std::runtime_error) {
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "delete from " << table.name;
                ss << " where ";
                std::vector<std::string> primaryKeyColumnNames;
                table.template for_each_column([&] (auto c) {
                    if(c.template has<primary_key>()) {
                        primaryKeyColumnNames.emplace_back(c.name);
                    }
                });
                for(auto i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << primaryKeyColumnNames[i] << " =  ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " and ";
                    }else{
                        ss << " ";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    auto index = 1;
                    table.template for_each_column_with<primary_key>([&] (auto c) {
                        typedef typename decltype(c)::field_type field_type;
                        statement_binder<field_type>().bind(stmt, index++, std::to_string(id));
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
        }
        
//    protected:
        
        void withDatabase(std::function<void(sqlite3*)> lambda, const std::string &filename) {
            sqlite3 *db;
            auto rc = sqlite3_open(filename.c_str(), &db);
            if(rc == SQLITE_OK){
                lambda(db);
            }else{
                std::cerr << "error " << sqlite3_errmsg(db) << std::endl;
            }
            sqlite3_close(db);
        }
        
    private:
        typedef storage_impl<Ts...> Super;
    };
    
    /**
     *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage` function.
     */
    template<class ...Ts>
    struct storage {
        typedef storage_impl<Ts...> impl_type;
        
        /**
         *  @param filename database filename.
         *  @param table table created by function `make_table`
         */
        storage(const std::string &filename_, impl_type impl_):filename(filename_), impl(impl_){}
        
        /**
         *  Delete routine.
         *  @param O Object's type. Must be specified explicitly.
         *  @param id id of object to be removed.
         */
        template<class O>
        void remove(int id) throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in remove");
//            static_assert(true, "ototo");
            impl.template remove<O>(id, filename);
        }
        
        /**
         *  Update routine. Sets all non primary key fields where primary key is equal.
         *  @param O Object type. May be not specified explicitly cause it can be deduced by
         *      compiler from first parameter.
         *  @param o object to be updated.
         */
        template<class O>
        void update(const O &o) throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in update");
            impl.template update(o, filename);
        }
        
        /**
         *  Select * with no conditions routine.
         *  @param O Object type to be extracted. Must be specified explicitly.
         *  @return All objects of type O stored in database at the moment.
         */
        template<class O>
        std::vector<O> get_all() throw(std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get_all");
            return impl.template get_all<O>(filename);
        }
        
        /**
         *  Select * by id routine.
         *  @param O Object type to be extracted. Must be specified explicitly.
         *  @return Object of type O where id is equal parameter passed or throws `not_found_exception`
         *  if there is no object with such id.
         */
        template<class O>
        O get(int id) throw (not_found_exception, std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get");
            return impl.template get<O>(id, filename);
        }
        
        /**
         *  The same as `get` function but doesn't throw an exeption if noting found but returns std::shared_ptr with null value.
         */
        template<class O>
        std::shared_ptr<O> get_no_throw(int id) throw(std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get_no_throw");
            return impl.template get_no_throw<O>(id, filename);
        }
        
        /**
         *  Select count(*) with no conditions routine.
         *  @return Number of O object in table.
         */
        template<class O>
        int count() throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in count");
            return impl.template count<O>(filename);
        }
        
        /**
         *  Select count(X)
         *  @param m member pointer to class mapped to the storage.
         */
        template<class F, class O>
        int count(F O::*m) throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in count");
            return impl.template count(m, filename);
        }
        
        /**
         *  AVG(X) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return average value from db.
         */
        template<class F, class O>
        double avg(F O::*m) throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in avg");
            return impl.template avg(m, filename);
        }
        
        /**
         *  GROUP_CONCAT(X) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O>
        std::string group_concat(F O::*m) throw (std::runtime_error) {
            return impl.group_concat(m, filename);
        }
        
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &y) throw (std::runtime_error) {
            return impl.group_concat(m, y, filename);
        }
        
        /**
         *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
         *  object doesn't matter.
         *  @return id of just created object.
         */
        template<class O>
        int insert(const O &o) throw (std::runtime_error) {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in insert");
            return impl.template insert(o, filename);
        }
        
        /**
         *  Drops table with given name.
         */
        void drop_table(const std::string &tableName) throw (std::runtime_error) {
            withDatabase([=] (auto db) {
                std::stringstream ss;
                ss << "DROP TABLE " << tableName;
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                }else {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            });
        }
        
        /*void check_schema() throw (std::runtime_error) {
            auto gottaCreateTable = !this->table_exists(table.name);
            if(!gottaCreateTable){
                //  TODO:   get table_info, compare with actual schema and recreate table if needed..
            }else{
                create_table();
            }
        }*/
        
        bool table_exists(const std::string &tableName) throw(std::runtime_error) {
            bool res = false;
            impl.template withDatabase([&] (auto db) {
                std::stringstream ss;
                ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '" << "table" << "' AND name = '" << tableName << "'";
                auto query = ss.str();
                data_t<bool, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(data_t<bool, decltype(this)>*)data;
                                           auto &res = *d.res;
//                                           auto t = d.t;
                                           if(argc){
                                               res = std::atoi(argv[0]);
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        struct table_info {
            int cid;
            std::string name;
            std::string type;
            bool notnull;
            std::string dflt_value;
            bool pk;
        };
        
        std::vector<table_info> get_table_info(const std::string &tableName) throw (std::runtime_error) {
            std::vector<table_info> res;
            impl.template withDatabase([&] (auto db) {
                auto query = "PRAGMA table_info(" + tableName + ")";
                //                auto query = "SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name = ?";
                data_t<std::vector<table_info>, decltype(this)> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(data_t<std::vector<table_info>, decltype(this)>*)data;
                                           auto &res = *d.res;
                                           if(argc){
                                               auto index = 0;
                                               auto cid = std::atoi(argv[index++]);
                                               std::string name = argv[index++];
                                               std::string type = argv[index++];
                                               bool notnull = std::atoi(argv[index++]);
                                               std::string dflt_value = argv[index] ? argv[index] : "";
                                               index++;
                                               bool pk = std::atoi(argv[index++]);
                                               res.emplace_back(table_info{cid, name, type, notnull, dflt_value, pk});
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
    protected:
        std::string filename;
        impl_type impl;
    };
    
    template<class ...Ts>
    storage<Ts...> make_storage(const std::string &filename, Ts ...tables) {
        return {filename, storage_impl<Ts...>(tables...)};
    }
    
}

#endif /* sqlite_orm_h */
