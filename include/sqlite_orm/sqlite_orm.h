
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
#include <regex>
#include <map>
#include <cctype>
#include <initializer_list>

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
    
//    struct not_null {};
    
    struct primary_key {};
    
    template<class T>
    struct default_t {
        typedef T value_type;
        
        value_type value;
    };
    
    template<class T>
    default_t<T> default_value(T t) {
        return {t};
    }
    
    enum class sqlite_type {
        INTEGER,
        TEXT,
        BLOB,
        REAL,
//        NUMERIC,      //  numeric and real are the same for c++
    };
    
    /**
     *  @param str case doesn't matter - it is uppercased before comparing.
     */
    std::shared_ptr<sqlite_type> to_sqlite_type(const std::string &str) {
        auto asciiStringToUpper = [](std::string &s){
            std::transform(s.begin(),
                           s.end(),
                           s.begin(),
                           [](char c){
                               return std::toupper(c);
                           });
        };
        auto upperStr = str;
        asciiStringToUpper(upperStr);
        
        static std::map<sqlite_type, std::vector<std::regex>> typeMap = {
            { sqlite_type::INTEGER, {
                std::regex("INT"),
                std::regex("INT.*"),
//                std::regex("INTEGER"),
                std::regex("TINYINT"),
                std::regex("SMALLINT"),
                std::regex("MEDIUMINT"),
                std::regex("BIGINT"),
                std::regex("UNSIGNED BIG INT"),
                std::regex("INT2"),
                std::regex("INT8"),
            } }, { sqlite_type::TEXT, {
                std::regex("CHARACTER\\([[:digit:]]+\\)"),
                std::regex("VARCHAR\\([[:digit:]]+\\)"),
                std::regex("VARYING CHARACTER\\([[:digit:]]+\\)"),
                std::regex("NCHAR\\([[:digit:]]+\\)"),
                std::regex("NATIVE CHARACTER\\([[:digit:]]+\\)"),
                std::regex("NVARCHAR\\([[:digit:]]+\\)"),
                std::regex("CLOB"),
                std::regex("TEXT"),
            } }, { sqlite_type::BLOB, {
                std::regex("BLOB"),
            } }, { sqlite_type::REAL, {
                std::regex("REAL"),
                std::regex("DOUBLE"),
                std::regex("DOUBLE PRECISION"),
                std::regex("FLOAT"),
                std::regex("NUMERIC"),
                std::regex("DECIMAL\\([[:digit:]]+,[[:digit:]]+\\)"),
                std::regex("BOOLEAN"),
                std::regex("DATE"),
                std::regex("DATETIME"),
            } },
        };
        for(auto &p : typeMap) {
            for(auto &r : p.second) {
                if(std::regex_match(upperStr, r)){
                    return std::make_shared<sqlite_type>(p.first);
                }
            }
        }
        
        return {};
    }
    
    template<class T>
    struct type_is_nullable : public std::false_type {};
    
    template<class T>
    struct type_is_nullable<std::shared_ptr<T>> : public std::true_type {};
    
    template<class T>
    struct type_is_nullable<std::unique_ptr<T>> : public std::true_type {};
    
    template<class O, class T, class ...Op>
    struct column {
        typedef O object_type;
        typedef T field_type;
        typedef std::tuple<Op...> options_type;
        typedef field_type object_type::*member_pointer_t;
        
        const std::string name;
        field_type object_type::*member_pointer;
        options_type options;
        
        bool not_null() const {
            return !type_is_nullable<field_type>::value;
        }
        
        template<class Opt>
        constexpr bool has() const {
            return tuple_helper::tuple_contains_type<Opt, options_type>::value;
        }
        
        template<class O1, class O2, class ...Opts>
        constexpr bool has_every() const  {
            if(has<O1>() && has<O2>()) {
                return true;
            }else{
                return has_every<Opts...>();
            }
        }
        
        template<class O1>
        constexpr bool has_every() const {
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
    
    template<class T>
    struct type_printer<std::shared_ptr<T>> {
        inline const std::string& print() {
            /*static const std::string res = "REAL";
            return res;*/
            return type_printer<T>().print();
        }
    };
    
    template<class T>
    struct type_printer<std::unique_ptr<T>> {
        inline const std::string& print() {
            /*static const std::string res = "REAL";
             return res;*/
            return type_printer<T>().print();
        }
    };
    
    /**
     *  Column builder function. You should use it to create columns and not constructor.
     */
    template<class O, class T, class ...Op>
    column<O, T, Op...> make_column(const std::string &name, T O::*m, Op ...options){
        return {name, m, std::make_tuple(options...)};
    }
    
    template<class L, class R>
    struct binary_condition {
        L l;
        R r;
        
        binary_condition(L l_, R r_):l(l_),r(r_){}
    };
    
    /*template<typename L, typename R>
    std::true_type is_base_of_binary_condition_impl( binary_condition<L, R> const volatile& );
    
    std::false_type is_base_of_binary_condition_impl( ... );
    
    template<typename T>
    bool is_base_of_binary_condition(T&& t) {
        return decltype(is_base_of_binary_condition_impl(t))::value;
    }*/
    
    template<class L, class R>
    struct is_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "=";
        }
        
    };
    
    template<class L, class R>
    struct is_not_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "!=";
        }
    };
    
    template<class L, class R>
    struct greater_than_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return ">";
        }
    };
    
    template<class L, class R>
    struct greater_or_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return ">=";
        }
    };
    
    template<class L, class R>
    struct lesser_than_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "<";
        }
    };
    
    template<class L, class R>
    struct lesser_or_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "<=";
        }
    };
    
    template<class L, class E>
    struct in_t {
        L l;    //  left expression..
        std::vector<E> values;       //  values..
    };
    
    template<class L, class E>
    in_t<L, E> in(L l, std::vector<E> values) {
        return {std::move(l), std::move(values)};
    }
    
    /*template<class L, class E, class O>
    in_t<L, E> in(L l, O beg, O en) {
        return {std::move(l), std::vector<E>(beg, en)};
    }*/
    
    template<class L, class R>
    is_equal_t<L, R> is_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    is_not_equal_t<L, R> is_not_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    greater_than_t<L, R> greater_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    greater_or_equal_t<L, R> greater_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    lesser_than_t<L, R> lesser_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    lesser_or_equal_t<L, R> lesser_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class C>
    struct where_t {
        C c;
    };
    
    template<class C>
    where_t<C> where(C c) {
        return {c};
    }
    
    struct table_info {
        int cid;
        std::string name;
        std::string type;
        bool notnull;
        std::string dflt_value;
        bool pk;
    };
    
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

        template<class F, class L>
        void for_each_column_with_field_type(L l) {
//            cout<<"typename column_type::field_type = "<<typeid(typename column_type::field_type).name()<<endl;
//            cout<<"F = "<<typeid(F).name()<<endl;
//            cout<<"custom version"<<endl;
            apply_to_col_if(l, std::is_same<F, typename column_type::field_type>{});
            Super::template for_each_column_with_field_type<F, L>(l);
        }
        
        /**
         *  Working version of `for_each_column_exept`. Calls lambda if column has no option and fire super's function.
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            using has_opt = tuple_helper::tuple_contains_type<Op, typename column_type::options_type>;
            apply_to_col_if(l, std::integral_constant<bool, !has_opt::value>{});
            Super::template for_each_column_exept<Op, L>(l);
        }

        /**
         *  Working version of `for_each_column_with`. Calls lambda if column has option and fire super's function.
         */
        template<class Op, class L>
        void for_each_column_with(L l) {
            apply_to_col_if(l, tuple_helper::tuple_contains_type<Op, typename column_type::options_type>{});
            Super::template for_each_column_with<Op, L>(l);
        }

    protected:
        
        template<class L>
        void apply_to_col_if(L& l, std::true_type) {
            l(col);
        }
        
        template<class L>
        void apply_to_col_if(L& l, std::false_type) {}
        
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
        std::string find_column_name(F O::*m) {
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
         *  @class L Lambda type. Do not specify it explicitly.
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
        
        std::vector<table_info> get_table_info() {
            std::vector<table_info> res;
            res.reserve(size_t(this->columns_count()));
            this->for_each_column([&res](auto col){
                table_info i{
                    -1,
                    col.name,
                    type_printer<typename decltype(col)::field_type>().print(),
//                    col.template has<not_null>(),
                    col.not_null(),
                    "",
                    col.template has<primary_key>(),
                };
                res.emplace_back(i);
            });
            return res;
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
     *  Specialization for double.
     */
    template<>
    struct row_extrator<double> {
        double extract(const char *row_value) {
            return std::atof(row_value);
        }
    };
    
    template<class T>
    struct row_extrator<std::shared_ptr<T>> {
        std::shared_ptr<T> extract(const char *row_value) {
            if(row_value){
                return std::make_shared<T>(row_extrator<T>().extract(row_value));
            }else{
                return {};
            }
        }
    };
    
    template<class T>
    struct row_extrator<std::unique_ptr<T>> {
        std::shared_ptr<T> extract(const char *row_value) {
            if(row_value){
                return std::make_shared<T>(row_extrator<T>().extract(row_value));
            }else{
                return {};
            }
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
        int insert(const O &o, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in insert");
        }
        
        template<class O, class ...Args>
        std::vector<O> get_all(const std::string &filename, Args ...args) /*throw(std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_all");
        }
        
        template<class O>
        O get(int id, const std::string &filename) /*throw (not_found_exception, std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get");
        }
        
        template<class O>
        void update(const O &o, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in update");
        }
        
        template<class O>
        std::shared_ptr<O> get_no_throw(int id, const std::string &filename) /*throw(std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_no_throw");
        }
        
        template<class O>
        void remove(int id, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in remove");
        }
        
        template<class O>
        int count(const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }
        
        template<class F, class O>
        double avg(F O::*m, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in avg");
        }
        
        template<class F, class O>
        int count(F O::*m, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }
        
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }
        
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &y, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }
        
        template<class F, class O>
        std::shared_ptr<F> max(F O::*m, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
        }
        
        template<class O>
        std::string dump(const O &o, const std::string &filename) /*throw (std::runtime_error)*/ {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
        }
        
        void sync_schema(const std::string &filename) /*throw (std::runtime_error)*/ {
            return;
        }
        
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
        
        void drop_table(const std::string &tableName, const std::string &filename) /*throw (std::runtime_error)*/ {
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
            }, filename);
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
        
        void create_table(const std::string &filename) /*throw (std::runtime_error)*/ {
            this->withDatabase([=] (auto db) {
                std::stringstream ss;
                ss << "CREATE TABLE " << this->table.name << " ( ";
                auto columnsCount = this->table.columns_count();
                auto index = 0;
                this->table.for_each_column([columnsCount, &index, &ss] (auto c) {
                    ss << c.name << " ";
                    ss << type_printer<typename decltype(c)::field_type>().print() << " ";
                    if(c.template has<primary_key>()) {
                        ss << "primary key ";
                    }
                    if(c.template has<autoincrement>()) {
                        ss << "autoincrement ";
                    }
                    if(c.not_null()){
//                    if(c.template has<not_null>()) {
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
        int insert(const O &o, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::template insert(o, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        int insert(const O &o, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            int res = 0;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "INSERT INTO " << this->table.name << " (";
                std::vector<std::string> columnNames;
                this->table.for_each_column([&] (auto c) {
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
                    this->table.for_each_column([&, &o = o] (auto c) {
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
        std::string group_concat(F O::*m, const std::string &y, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::group_concat(m, y, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        std::string group_concat(F O::*m, const std::string &y, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            std::string res;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select group_concat(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ",\"" << y << "\") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<std::string, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::string, storage_impl*>*)data;
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
        std::shared_ptr<F> min(F O::*m, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::min(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        std::shared_ptr<F> min(F O::*m, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            std::shared_ptr<F> res;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select min(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<std::shared_ptr<F>, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::shared_ptr<F>, storage_impl*>*)data;
                                               auto &res = *d.res;
                                               if(argc){
                                                   if(argv[0]){
                                                       res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                                   }
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
        std::shared_ptr<F> max(F O::*m, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::max(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        std::shared_ptr<F> max(F O::*m, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            std::shared_ptr<F> res;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select max(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<std::shared_ptr<F>, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::shared_ptr<F>, storage_impl*>*)data;
                                               auto &res = *d.res;
                                               if(argc){
                                                   if(argv[0]){
                                                       res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                                   }
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
        std::string group_concat(F O::*m, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::group_concat(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        std::string group_concat(F O::*m, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            std::string res;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select group_concat(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<std::string, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<std::string, storage_impl*>*)data;
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
        double avg(F O::*m, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::template avg(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        double avg(F O::*m, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            double res = 0;
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "select avg(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<double, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<double, storage_impl*>*)data;
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
        int count(F O::*m, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::template count(m, filename);
        }
        
        template<class F, class O, class HH = typename H::object_type>
        int count(F O::*m, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            int res = 0;
            this->withDatabase([&](auto db){
                std::stringstream ss;
                ss << "select count(";
                auto columnName = this->table.find_column_name(m);
                if(columnName.length()){
                    ss << columnName << ") from "<< this->table.name;
                    auto query = ss.str();
                    data_t<int, storage_impl*> data{this, &res};
                    auto rc = sqlite3_exec(db,
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **azColName)->int{
                                               auto &d = *(data_t<int, storage_impl*>*)data;
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
        int count(const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::template count<O>(filename);
        }
        
        template<class O, class HH = typename H::object_type>
        int count(const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            int res = 0;
            this->withDatabase([&](auto db) {
                auto query = "select count(*) from " + this->table.name;
                data_t<int, storage_impl*> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_t<int, storage_impl*>*)data;
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
        std::shared_ptr<O> get_no_throw(int id, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw(std::runtime_error)*/ {
            return Super::template get_no_throw<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        std::shared_ptr<O> get_no_throw(int id, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw(std::runtime_error)*/ {
            std::shared_ptr<O> res;
            this->withDatabase([&](auto db) {
                auto query = "select * from " + this->table.name + " where id = " + std::to_string(id);
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
        O get(int id, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (not_found_exception, std::runtime_error)*/ {
            return Super::template get<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        O get(int id, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (not_found_exception, std::runtime_error)*/ {
            std::shared_ptr<O> res;
            withDatabase([&](auto db) {
                auto query = "select * from " + this->table.name + " where id = " + std::to_string(id);
                data_t<std::shared_ptr<O>, storage_impl*> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_t<std::shared_ptr<O>, storage_impl*>*)data;
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
        
        template<class T>
        std::string string_from_expression(T t) {
            return std::to_string(t);
        }
        
        std::string string_from_expression(const std::string &t) {
//            return t;
            std::stringstream ss;
            ss << "'" << t << "'";
            return ss.str();
        }
        
        std::string string_from_expression(const char *t) {
            std::stringstream ss;
            ss << "'" << t << "'";
            return ss.str();
        }
        
        template<class F, class O>
        std::string string_from_expression(F O::*m) {
            return this->table.find_column_name(m);
        }
        
        template<class C>
        std::string process_where(C c) {
            auto leftString = this->string_from_expression(c.l);
            auto rightString = this->string_from_expression(c.r);
            std::stringstream ss;
            ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
            return ss.str();
        }
        
        template<class L, class E>
        std::string process_where(in_t<L, E> &inCondition) {
            std::stringstream ss;
            auto leftString = this->string_from_expression(inCondition.l);
            ss << leftString << " IN (";
            for(auto index = 0; index < inCondition.values.size(); ++index) {
                auto &value = inCondition.values[index];
                ss << " " << this->string_from_expression(value);
                if(index < inCondition.values.size() - 1) {
                    ss << ", ";
                }
            }
            ss << " )";
            return ss.str();
        }
        
        /*template<class L, class R>
        std::string process_where(is_not_equal_t<L, R> c) {
            auto leftString = this->string_from_condition_half(c.l);
            auto rightString = this->string_from_condition_half(c.r);
            std::stringstream ss;
            ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(greater_then_t<L, R> c) {
            auto leftString = this->string_from_condition_half(c.l);
            auto rightString = this->string_from_condition_half(c.r);
            std::stringstream ss;
            ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(lesser_then_t<L, R> c) {
            auto leftString = this->string_from_condition_half(c.l);
            auto rightString = this->string_from_condition_half(c.r);
            std::stringstream ss;
            ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
            return ss.str();
        }*/
        
        template<class C>
        void process_single_condition(std::stringstream &ss, where_t<C> w) {
            ss << " where ";
            auto whereString = this->process_where(w.c);
            ss << "(" << whereString << ")";
        }
        
        /**
         *  Recursion end.
         */
        template<class ...Args>
        void process_conditions(std::stringstream &ss, Args ...args) {
            //..
        }
        
        template<class C, class ...Args>
        void process_conditions(std::stringstream &ss, C c, Args ...args) {
            this->process_single_condition(ss, c);
            this->process_conditions(ss, args...);
        }
        
        template<class O, class HH = typename H::object_type, class ...Args>
        std::vector<O> get_all(const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type *, Args ...args) {
            return Super::template get_all<O>(filename, nullptr, args...);
        }
        
        template<class O, class HH = typename H::object_type, class ...Args>
        std::vector<O> get_all(const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type *, Args ...args) {
            std::vector<O> res;
            this->withDatabase([&res, this, &args...](auto db) {
                std::stringstream ss;
                ss << "select * from " << this->table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
//                auto query = "select * from " + this->table.name;
//                data_t<std::vector<O>, storage_impl*> data{this, &res};
                typedef std::tuple<std::vector<O>*, storage_impl*> date_tuple_t;
                date_tuple_t data{&res, this};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(date_tuple_t*)data;
//                                           auto &res = *d.res;
                                           auto &res = *std::get<0>(d);
//                                           auto t = d.t;
                                           auto t = std::get<1>(d);
                                           if(argc){
                                               O o;
                                               auto index = 0;
                                               t->table.for_each_column([&index, &o, argv] (auto c) {
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
        void update(const O &o, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            Super::template update(o, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        void update(const O &o, const std::string &filename,  typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            this->withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "update " << this->table.name << " set ";
                std::vector<std::string> setColumnNames;
                this->table.for_each_column( [&] (auto c) {
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
                auto primaryKeyColumnNames = this->table.template column_names_with<primary_key>();
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
                    this->table.for_each_column([&, &o = o] (auto c) {
                        if(!c.template has<primary_key>()) {
                            auto &value = o.*c.member_pointer;
                            statement_binder<typename decltype(c)::field_type>().bind(stmt, index++, value);
                        }
                    });
                    this->table.for_each_column([&, &o = o] (auto c) {
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
        std::string dump(const O &o, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            return Super::dump(o, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        std::string dump(const O &o, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            std::stringstream ss;
            ss << "{ ";
            auto columnsCount = this->table.columns_count();
            auto index = 0;
            this->table.for_each_column([&] (auto c) {
                auto &value = o.*c.member_pointer;
                ss << c.name << " : '" << value << "'";
                if(index < columnsCount - 1) {
                    ss << ", ";
                }else{
                    ss << " }";
                }
                ++index;
            });
            return ss.str();
        }
        
        template<class O, class HH = typename H::object_type>
        void remove(int id, const std::string &filename, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            Super::template remove<O>(id, filename);
        }
        
        template<class O, class HH = typename H::object_type>
        void remove(int id, const std::string &filename, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) /*throw (std::runtime_error)*/ {
            withDatabase([&](auto db) {
                std::stringstream ss;
                ss << "delete from " << this->table.name;
                ss << " where ";
                std::vector<std::string> primaryKeyColumnNames;
                this->table.for_each_column([&] (auto c) {
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
                    this->table.template for_each_column_with<primary_key>([&] (auto c) {
                        typedef typename decltype(c)::field_type field_type;
                        statement_binder<field_type>().bind(stmt, index++, id);
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
        
        bool table_exists(const std::string &tableName, const std::string &filename) /*throw(std::runtime_error)*/ {
            bool res = false;
            this->withDatabase([&] (auto db) {
                std::stringstream ss;
                ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '" << "table" << "' AND name = '" << tableName << "'";
                auto query = ss.str();
                data_t<bool, storage_impl*> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(data_t<bool, storage_impl*>*)data;
                                           auto &res = *d.res;
                                           //                                           auto t = d.t;
                                           if(argc){
                                               res = !!std::atoi(argv[0]);
                                           }
                                           return 0;
                                       }, &data, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }, filename);
            return res;
        }
        
        std::vector<table_info> get_table_info(const std::string &tableName, const std::string &filename) /*throw (std::runtime_error)*/ {
            std::vector<table_info> res;
            this->withDatabase([&] (auto db) {
                auto query = "PRAGMA table_info(" + tableName + ")";
                //                auto query = "SELECT COUNT(*) FROM sqlite_master WHERE type = ? AND name = ?";
                data_t<std::vector<table_info>, storage_impl*> data{this, &res};
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName) -> int {
                                           auto &d = *(data_t<std::vector<table_info>, storage_impl*>*)data;
                                           auto &res = *d.res;
                                           if(argc){
                                               auto index = 0;
                                               auto cid = std::atoi(argv[index++]);
                                               std::string name = argv[index++];
                                               std::string type = argv[index++];
                                               bool notnull = !!std::atoi(argv[index++]);
                                               std::string dflt_value = argv[index] ? argv[index] : "";
                                               index++;
                                               bool pk = !!std::atoi(argv[index++]);
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
        
//        template<class C>
        void add_column(table_info &ti, const std::string &filename) /*throw (std::runtime_error)*/ {
            this->withDatabase([&] (auto db) {
                std::stringstream ss;
                ss << "ALTER TABLE " << this->table.name << " ADD COLUMN " << ti.name << " ";
                ss << ti.type << " ";
                if(ti.pk){
//                if(c.template has<primary_key>()) {
                    ss << "primary key ";
                }
                /*if(c.template has<autoincrement>()) {
                    ss << "autoincrement ";
                }*/
                if(ti.notnull){
//                if(c.template has<not_null>()) {
                    ss << "not null ";
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
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
        
        void sync_schema(const std::string &filename) /*throw (std::runtime_error)*/ {
            auto gottaCreateTable = !this->table_exists(this->table.name, filename);
            if(!gottaCreateTable){
                //  TODO:   get table_info, compare with actual schema and recreate table if needed..
                auto dbTableInfo = get_table_info(this->table.name, filename);
                auto storageTableInfo = this->table.get_table_info();
//                cout<<endl;
                std::vector<table_info*> columnsToAdd;
                for(auto storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size(); ++storageColumnInfoIndex) {
//                for(auto &storageColumnInfo : storageTableInfo) {
                    auto &storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                    auto &columnName = storageColumnInfo.name;
                    auto dbColumnInfoIt = std::find_if(dbTableInfo.begin(),
                                                       dbTableInfo.end(),
                                                       [&](auto &ti){
                                                           return ti.name == columnName;
                                                       });
                    if(dbColumnInfoIt != dbTableInfo.end()){
                        auto &dbColumnInfo = *dbColumnInfoIt;
                        auto dbColumnInfoType = to_sqlite_type(dbColumnInfo.type);
//                        auto dbColumnInfoTypeLowerCase = dbColumnInfo.type;
                        auto storageColumnInfoType = to_sqlite_type(storageColumnInfo.type);
//                        auto storageColumnInfoTypeLowerCase = storageColumnInfo.type;
                        /*std::transform(dbColumnInfoTypeLowerCase.begin(),
                                       dbColumnInfoTypeLowerCase.end(),
                                       dbColumnInfoTypeLowerCase.begin(),
                                       [=](char c){
                                           return std::tolower(int(c));
                                       });
                        std::transform(storageColumnInfoTypeLowerCase.begin(),
                                       storageColumnInfoTypeLowerCase.end(),
                                       storageColumnInfoTypeLowerCase.begin(),
                                       [=](char c){
                                           return std::tolower(int(c));
                                       });*/
                        if(dbColumnInfoType && storageColumnInfoType) {
                            auto columnsAreEqual = dbColumnInfo.name == storageColumnInfo.name &&
                            *dbColumnInfoType == *storageColumnInfoType &&
                            dbColumnInfo.notnull == storageColumnInfo.notnull &&
//                            dbColumnInfo.dflt_value == storageColumnInfo.dflt_value &&
                            dbColumnInfo.pk == storageColumnInfo.pk;
                            if(!columnsAreEqual){
                                gottaCreateTable = true;
                                break;
                            }
                            dbTableInfo.erase(dbColumnInfoIt);
                            storageTableInfo.erase(storageTableInfo.begin() + storageColumnInfoIndex);
                            --storageColumnInfoIndex;
                        }else{
                            if(!storageColumnInfoType){
                                cout<<"unknown column type "<<storageColumnInfo.type<<endl;
                            }
                            if(!dbColumnInfoType){
                                cout<<"unknown column type "<<dbColumnInfo.type<<endl;
                            }
                            gottaCreateTable = true;
                            break;
                        }
                    }else{
                        columnsToAdd.push_back(&storageColumnInfo);
                    }
                }
                if(!gottaCreateTable){  //  if all storage columns are equal to actual db columns but there are excess columns at the db..
                    gottaCreateTable = dbTableInfo.size() > 0;
                }
                if(gottaCreateTable){
                    this->drop_table(this->table.name, filename);
                    this->create_table(filename);
                    cout<<"table "<<this->table.name<<" dropped and recreated"<<endl;
                }else{
                    if(columnsToAdd.size()){
//                        cout<<"columnsToAdd = "<<columnsToAdd.size()<<endl;
                        for(auto columnPointer : columnsToAdd) {
                            this->add_column(*columnPointer, filename);
                            cout<<"column "<<columnPointer->name<<" added to table "<<this->table.name<<endl;
                        }
                    }else{
                        cout<<"table "<<this->table.name<<" is synced"<<endl;
                    }
//                    cout<<"excess columns = "<<dbTableInfo.size()<<endl;
                }
            }else{
                this->create_table(filename);
                cout<<"table "<<this->table.name<<" created"<<endl;
            }
            Super::sync_schema(filename);
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
         *  @param filename_ database filename.
         */
        storage(const std::string &filename_, impl_type impl_):filename(filename_), impl(impl_){}
        
        /**
         *  Delete routine.
         *  O is an object's type. Must be specified explicitly.
         *  @param id id of object to be removed.
         */
        template<class O>
        void remove(int id) /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in remove");
//            static_assert(true, "ototo");
            impl.template remove<O>(id, filename);
        }
        
        /**
         *  Update routine. Sets all non primary key fields where primary key is equal.
         *  O is an object type. May be not specified explicitly cause it can be deduced by
         *      compiler from first parameter.
         *  @param o object to be updated.
         */
        template<class O>
        void update(const O &o) /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in update");
            impl.update(o, filename);
        }
        
        /**
         *  Select * with no conditions routine.
         *  O is an object type to be extracted. Must be specified explicitly.
         *  @return All objects of type O stored in database at the moment.
         */
        template<class O, class ...Args>
        std::vector<O> get_all(Args ...args) /*throw(std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get_all");
            return impl.template get_all<O>(filename, nullptr, args...);
        }
        
        /**
         *  Select * by id routine.
         *  O is an object type to be extracted. Must be specified explicitly.
         *  @return Object of type O where id is equal parameter passed or throws `not_found_exception`
         *  if there is no object with such id.
         */
        template<class O>
        O get(int id) /*throw (not_found_exception, std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get");
            return impl.template get<O>(id, filename);
        }
        
        /**
         *  The same as `get` function but doesn't throw an exeption if noting found but returns std::shared_ptr with null value.
         */
        template<class O>
        std::shared_ptr<O> get_no_throw(int id) /*throw(std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in get_no_throw");
            return impl.template get_no_throw<O>(id, filename);
        }
        
        /**
         *  SELECT COUNT(*) with no conditions routine. https://www.sqlite.org/lang_aggfunc.html#count
         *  @return Number of O object in table.
         */
        template<class O>
        int count() /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in count");
            return impl.template count<O>(filename);
        }
        
        /**
         *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
         *  @param m member pointer to class mapped to the storage.
         */
        template<class F, class O>
        int count(F O::*m) /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in count");
            return impl.count(m, filename);
        }
        
        /**
         *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return average value from db.
         */
        template<class F, class O>
        double avg(F O::*m) /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in avg");
            return impl.avg(m, filename);
        }
        
        /**
         *  GROUP_CONCAT(X) query.  https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O>
        std::string group_concat(F O::*m) /*throw (std::runtime_error)*/ {
            return impl.group_concat(m, filename);
        }
        
        /**
         *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O>
        std::string group_concat(F O::*m, const std::string &y) /*throw (std::runtime_error)*/ {
            return impl.group_concat(m, y, filename);
        }
        
        template<class F, class O>
        std::shared_ptr<F> max(F O::*m) /*throw (std::runtime_error)*/ {
            return impl.max(m, filename);
        }
        
        template<class F, class O>
        std::shared_ptr<F> min(F O::*m) /*throw (std::runtime_error)*/ {
            return impl.min(m, filename);
        }
        
        template<class O>
        std::string dump(const O &o) /*throw (std::runtime_error)*/ {
            return impl.dump(o, filename);
        }
        
        /**
         *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
         *  object doesn't matter.
         *  @return id of just created object.
         */
        template<class O>
        int insert(const O &o) /*throw (std::runtime_error)*/ {
//            static_assert(impl.template type_is_mapped<O>(), "Type is not mapped in insert");
            return impl.insert(o, filename);
        }
        
        /**
         *  Drops table with given name.
         */
        void drop_table(const std::string &tableName) /*throw (std::runtime_error)*/ {
            impl.drop_table(tableName, filename);
            /*withDatabase([=] (auto db) {
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
            });*/
        }
        
        /**
         *  This is a cute function used to replace migration up/down functionality.
         *  It performs check storage schema with actual db schema and:
         *  * if there are excess tables exist in db they are ignored (not dropped)
         *  * every table from storage is compared with it's db analog and 
         *      * if table doesn't exist it is created
         *      * if table exists its colums are being compared with table_info from db and
         *          * if there are columns in db that do not exist in storage (excess) table will be dropped and recreated
         *          * if there are columns in storage that are not exist in db they will be added using `ALTER TABLE ... ADD COLUMN ...' command
         *          * if there is any column existing in both db and storage but differs by any of properties (type, pk, notnull, dflt_value) table will be dropped and recreated
         *  Be aware that `sync_schema` doesn't guarantee that data will not be dropped. It guarantees only that it will make db schema the same
         *  as you specified in `make_storage` function call. A good point is that if you have no db file at all it will be created and 
         *  all tables also will be created with exact tables and columns you specified in `make_storage`, `make_table` and `make_column` call.
         *  The best practice is to call this function right after storage creation.
         */
        void sync_schema() /*throw (std::runtime_error)*/ {
            impl.sync_schema(filename);
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
