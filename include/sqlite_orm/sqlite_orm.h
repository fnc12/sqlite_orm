
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
#include <set>

using std::cout;
using std::endl;

namespace sqlite_orm {
    
    typedef sqlite_int64 int64;
    typedef sqlite_uint64 uint64;
    
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
        
        template<size_t N, class ...Args>
        struct iterator {
            
            template<class L>
            void operator()(std::tuple<Args...> &t, L l) {
                l(std::get<N>(t));
                iterator<N - 1, Args...>()(t, l);
            }
        };
        
        template<class ...Args>
        struct iterator<0, Args...>{
            template<class L>
            void operator()(std::tuple<Args...> &t, L l) {
                l(std::get<0>(t));
            }
        };
        
        template<size_t N>
        struct iterator<N> {
            
            template<class L>
            void operator()(std::tuple<> &t, L l) {
                //..
            }
        };
        
        /*template<typename F, typename Tuple, bool Enough, int TotalArgs, int... N>
        struct call_impl
        {
            auto static call(F f, Tuple&& t)
            {
                return call_impl<F, Tuple, TotalArgs == 1 + sizeof...(N),
                TotalArgs, N..., sizeof...(N)
                >::call(f, std::forward<Tuple>(t));
            }
        };
        
        template<typename F, typename Tuple, int TotalArgs, int... N>
        struct call_impl<F, Tuple, true, TotalArgs, N...>
        {
            auto static call(F f, Tuple&& t)
            {
                return f(std::get<N>(std::forward<Tuple>(t))...);
            }
        };
        
        template<typename F, typename Tuple>
        auto call(F f, Tuple&& t)
        {
            typedef typename std::decay<Tuple>::type type;
            return call_impl<F, Tuple, 0 == std::tuple_size<type>::value,
            std::tuple_size<type>::value
            >::call(f, std::forward<Tuple>(t));
        }*/
        
        /*template<typename F, typename Tuple, size_t ...S >
        decltype(auto) apply_tuple_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
        {
            return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
        }
        template<typename F, typename Tuple>
        decltype(auto) apply_from_tuple(F&& fn, Tuple&& t)
        {
            std::size_t constexpr tSize
            = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
            return apply_tuple_impl(std::forward<F>(fn),
                                    std::forward<Tuple>(t),
                                    std::make_index_sequence<tSize>());
        }*/
    }
    
    template<class T>
    struct field_extractor;
    
    template<class F, class O>
    struct field_extractor<F O::*> {
        typedef F type;
    };
    
    template<class ...Args>
    struct object_type_extractor;
    
    template<class F, class O, class ...Args>
    struct object_type_extractor<F O::*, Args...> {
        typedef O type;
    };
    
    struct autoincrement {};
    
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
    inline std::shared_ptr<sqlite_type> to_sqlite_type(const std::string &str) {
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
    struct type_is_nullable : public std::false_type {
        bool operator()(const T &t) const {
            return true;
        }
    };
    
    template<class T>
    struct type_is_nullable<std::shared_ptr<T>> : public std::true_type {
        bool operator()(const std::shared_ptr<T> &t) const {
            return static_cast<bool>(t);
        }
    };
    
    template<class T>
    struct type_is_nullable<std::unique_ptr<T>> : public std::true_type {
        bool operator()(const std::unique_ptr<T> &t) const {
            return static_cast<bool>(t);
        }
    };
    
    /**
     *  This class accepts c++ type and transfers it to sqlite name (int -> INTEGER, std::string -> TEXT)
     */
    template<class T>
    struct type_printer;
    
    struct integer_printer {
        inline const std::string& print() {
            static const std::string res = "INTEGER";
            return res;
        }
    };
    
    struct text_printer {
        inline const std::string& print() {
            static const std::string res = "TEXT";
            return res;
        }
    };
    
    struct real_printer {
        inline const std::string& print() {
            static const std::string res = "REAL";
            return res;
        }
    };
    
    template<>
    struct type_printer<int> : public integer_printer {};
    
    template<>
    struct type_printer<bool> : public integer_printer {};
    
    template<>
    struct type_printer<std::string> : public text_printer {};
    
    template<>
    struct type_printer<const char*> : public text_printer {};
    
    template<>
    struct type_printer<double> : public real_printer {};
    
    template<class T>
    struct type_printer<std::shared_ptr<T>> : public type_printer<T> {};
    
    template<class T>
    struct type_printer<std::unique_ptr<T>> : public type_printer<T> {};
    
//    template<class T>
    struct default_value_extractor {
        
        template<class A>
        std::shared_ptr<std::string> operator() (const A &a) {
            return {};
        }
        
        template<class T>
        std::shared_ptr<std::string> operator() (const default_t<T> &t) {
            std::stringstream ss;
            auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
            if(needQuotes){
                ss << "'";
            }
            ss << t.value;
            if(needQuotes){
                ss << "'";
            }
            return std::make_shared<std::string>(ss.str());
        }
    };
    
    template<class O, class T, class ...Op>
    struct column_t {
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
        
        std::shared_ptr<std::string> default_value() {
            std::shared_ptr<std::string> res;
            tuple_helper::iterator<std::tuple_size<options_type>::value - 1, Op...>()(options, [&](auto &v){
                auto dft = default_value_extractor()(v);
                if(dft){
                    res = dft;
                }
            });
            return res;
        }
    };
    
    /**
     *  Used to print members mapped to objects.
     */
    template<class T>
    struct field_printer {
        std::string operator()(const T &t) const {
            return std::to_string(t);
        }
    };
    
    template<>
    struct field_printer<std::string> {
        std::string operator()(const std::string &t) const {
            return t;
        }
    };
    
    template<>
    struct field_printer<std::nullptr_t> {
        std::string operator()(const std::nullptr_t &) const {
            return "null";
        }
    };
    
    template<class T>
    struct field_printer<std::shared_ptr<T>> {
        std::string operator()(const std::shared_ptr<T> &t) const {
            if(t){
                return field_printer<T>()(*t);
            }else{
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };
    
    template<class T>
    struct field_printer<std::unique_ptr<T>> {
        std::string operator()(const std::unique_ptr<T> &t) const {
            if(t){
                return field_printer<T>()(*t);
            }else{
                return field_printer<std::nullptr_t>()(nullptr);
            }
        }
    };
    
    /**
     *  Column builder function. You should use it to create columns and not constructor.
     */
    template<class O, class T, class ...Op>
    column_t<O, T, Op...> make_column(const std::string &name, T O::*m, Op ...options){
        return {name, m, std::make_tuple(options...)};
    }
    
    struct limit_t {
        int lim;
        bool has_offset = false;
        bool offset_is_implicit = false;
        int off = 0;
        
        operator std::string () const {
            return "LIMIT";
        }
    };
    
    struct offset_t {
        int off;
    };
    
    inline offset_t offset(int off) {
        return {off};
    }
    
    inline limit_t limit(int lim) {
        return {lim};
    }
    
    inline limit_t limit(int off, int lim) {
        return {lim, true, true, off};
    }
    
    inline limit_t limit(int lim, offset_t offt) {
        return {lim, true, false, offt.off };
    }
    
    struct condition_t {};
    
    template<class C>
    struct negated_condition_t : public condition_t {
        C c;
        
        negated_condition_t(C c_):c(c_){}
        
        operator std::string () const {
            return "NOT";
        }
    };
    
    template<class L, class R>
    struct and_condition_t : public condition_t {
        L l;
        R r;
        
        and_condition_t(L l_, R r_):l(l_),r(r_){}
        
        operator std::string () const {
            return "AND";
        }
    };
    
    template<class L, class R>
    struct or_condition_t : public condition_t {
        L l;
        R r;
        
        or_condition_t(L l_, R r_):l(l_),r(r_){}
        
        operator std::string () const {
            return "OR";
        }
    };
    
    template<class L, class R, typename = typename std::enable_if<std::is_base_of<condition_t, L>::value && std::is_base_of<condition_t, R>::value>::type>
    and_condition_t<L, R> operator &&(const L &l, const R &r) {
        return {l, r};
    }
    
    template<class L, class R, typename = typename std::enable_if<std::is_base_of<condition_t, L>::value && std::is_base_of<condition_t, R>::value>::type>
    or_condition_t<L, R> operator ||(const L &l, const R &r) {
        return {l, r};
    }
    
    template<class L, class R>
    struct binary_condition : public condition_t {
        L l;
        R r;
        
        binary_condition(L l_, R r_):l(l_),r(r_){}
    };
    
    template<class L, class R>
    struct is_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "=";
        }
        
        negated_condition_t<is_equal_t<L, R>> operator!() const {
            return {*this};
        }
        
    };
    
    template<class L, class R>
    struct is_not_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "!=";
        }
        
        negated_condition_t<is_not_equal_t<L, R>> operator!() const {
            return {*this};
        }
    };
    
    template<class L, class R>
    struct greater_than_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return ">";
        }
        
        negated_condition_t<greater_than_t<L, R>> operator!() const {
            return {*this};
        }
    };
    
    template<class L, class R>
    struct greater_or_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return ">=";
        }
        
        negated_condition_t<greater_or_equal_t<L, R>> operator!() const {
            return {*this};
        }
    };
    
    template<class L, class R>
    struct lesser_than_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "<";
        }
        
        negated_condition_t<lesser_than_t<L, R>> operator!() const {
            return {*this};
        }
    };
    
    template<class L, class R>
    struct lesser_or_equal_t : public binary_condition<L, R> {
        
        using binary_condition<L, R>::binary_condition;
        
        operator std::string () const {
            return "<=";
        }
        
        negated_condition_t<lesser_or_equal_t<L, R>> operator!() const {
            return {*this};
        }
    };
    
    template<class L, class E>
    struct in_t {
        L l;    //  left expression..
        std::vector<E> values;       //  values..
        
        negated_condition_t<in_t<L, E>> operator!() const {
            return {*this};
        }
    };
    
    template<class T>
    struct is_null_t {
        T t;
        
        negated_condition_t<is_null_t<T>> operator!() const {
            return {*this};
        }
        
        operator std::string () const {
            return "IS NULL";
        }
    };
    
    template<class T>
    struct is_not_null_t {
        T t;
        
        negated_condition_t<is_not_null_t<T>> operator!() const {
            return {*this};
        }
        
        operator std::string () const {
            return "IS NOT NULL";
        }
    };
    
    template<class T>
    is_not_null_t<T> is_not_null(T t) {
        return {t};
    }
    
    template<class T>
    is_null_t<T> is_null(T t) {
        return {t};
    }
    
    template<class L, class E>
    in_t<L, E> in(L l, std::vector<E> values) {
        return {std::move(l), std::move(values)};
    }
    
    template<class L, class E>
    in_t<L, E> in(L l, std::initializer_list<E> values) {
        return {std::move(l), std::move(values)};
    }
    
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
        
        operator std::string() const {
            return "WHERE";
        }
    };
    
    template<class C>
    where_t<C> where(C c) {
        return {c};
    }
    
    template<class O>
    struct order_by_t {
        O o;
        
        operator std::string() const {
            return "ORDER BY";
        }
    };
    
    template<class O>
    struct asc_t {
        O o;
        
        operator std::string() const {
            return "ASC";
        }
    };
    
    template<class O>
    struct desc_t {
        O o;
        
        operator std::string() const {
            return "DESC";
        }
    };
    
    template<class O>
    order_by_t<O> order_by(O o) {
        return {o};
    }
    
    template<class O>
    asc_t<O> asc(O o) {
        return {o};
    }
    
    template<class O>
    desc_t<O> desc(O o) {
        return {o};
    }
    
    template<class ...Args>
    struct group_by_t {
        std::tuple<Args...> args;
        
        operator std::string() const {
            return "GROUP BY";
        }
    };
    
    template<class ...Args>
    group_by_t<Args...> group_by(Args ...args) {
        return {std::make_tuple(args...)};
    }
    
    template<class A, class T>
    struct between_t {
        A expr;
        T b1;
        T b2;
        
        operator std::string() const {
            return "BETWEEN";
        }
    };
    
    template<class T>
    struct length_t {
        T t;
        
        operator std::string() const {
            return "LENGTH";
        }
    };
    
    template<class T>
    length_t<T> length(T t) {
        return {t};
    }
    
    template<class A, class T>
    between_t<A, T> between(A expr, T b1, T b2) {
        return {expr, b1, b2};
    }
    
    template<class T>
    struct column_result_t;
    
    template<class O, class F>
    struct column_result_t<F O::*> {
        typedef F type;
    };
    
    template<class T>
    struct column_result_t<length_t<T>> {
        typedef int type;
    };
    
    template<class ...Args>
    struct columns_t {
        
        template<class L>
        void for_each(L ) {
            //..
        }
        
        int count() {
            return 0;
        }
    };
    
    template<class T, class ...Args>
    struct columns_t<T, Args...> : public columns_t<Args...> {
//        F O::*m;
        T m;
        
        columns_t(decltype(m) m_, Args ...args):m(m_), Super(args...){}
        
        template<class L>
        void for_each(L l) {
            l(this->m);
            Super::for_each(l);
        }
        
        int count() {
            return 1 + Super::count();
        }
    private:
        typedef columns_t<Args...> Super;
    };
    
    template<class ...Args>
    columns_t<Args...> columns(Args ...args) {
        return {args...};
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
        void for_each_column(L /*l*/) {}
        
        template<class F, class L>
        void for_each_column_with_field_type(L /*l*/) {}
        
        template<class Op, class L>
        void for_each_column_exept(L /*l*/){}
        
        template<class Op, class L>
        void for_each_column_with(L /*l*/) {}
        
        int columns_count() const {
            return 0;
        }
        
    };
    
    template<typename H, typename... T>
    struct table_impl<H, T...> : private table_impl<T...> {
        typedef H column_type;
        typedef std::tuple<T...> tail_types;
        
        table_impl(H h, T ...t) : Super(t...), col(h) {}
        
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
    struct table_t {
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
        
        std::string primary_key_column_name() {
            std::string res;
            impl.template for_each_column_with<primary_key>([&](auto &c){
                if(res.empty()){
                    res = c.name;
                }else{
                    throw std::runtime_error("table " + this->name + " has > 1 primary key columns");
                }
            });
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
                }
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
         *  L is lambda type. Do not specify it explicitly.
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
         *  L is lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            impl.template for_each_column_exept<Op>(l);
        }
        
        /**
         *  Iterates all columns that have specified options and fires passed lambda.
         *  Lambda must have one and only templated argument Otherwise code will not compile.
         *  L is lambda type. Do not specify it explicitly.
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
                std::string dft;
                if(auto d = col.default_value()) {
                    typedef typename decltype(col)::field_type field_type;
                    auto needQuotes = std::is_base_of<text_printer, type_printer<field_type>>::value;
                    if(needQuotes){
                        dft = "'" + *d + "'";
                    }else{
                        dft = *d;
                    }
                }
                table_info i{
                    -1,
                    col.name,
                    type_printer<typename decltype(col)::field_type>().print(),
//                    col.template has<not_null>(),
                    col.not_null(),
                    dft,
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
    table_t<Cs...> make_table(const std::string &name, Cs ...args) {
        return {name, table_impl<Cs...>(args...)};
    }
    
    struct statement_finalizer {
        sqlite3_stmt *stmt = nullptr;
        
//        inline statement_finalizer(sqlite3_stmt *stmt_):stmt(stmt_){}
        
        inline ~statement_finalizer() {
            sqlite3_finalize(this->stmt);
        }
    
    };
    
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
    
    /**
     *  Specialization for bool.
     */
    template<>
    struct statement_binder<bool> {
        
        int bind(sqlite3_stmt *stmt, int index, const bool &value) {
            return sqlite3_bind_int(stmt, index++, value);
        }
    };
    
    /**
     *  Specialization for long.
     */
    template<>
    struct statement_binder<long> {
        
        int bind(sqlite3_stmt *stmt, int index, const long &value) {
//            return sqlite3_bind_int(stmt, index++, value);
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
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
        
        //  used in sqlite3_exec (get_all and select)
        V extract(const char *row_value);
        
        //  used in sqlite_column (iteration)
        V extract(sqlite3_stmt *stmt, int columnIndex);
    };
    
    /**
     *  Specialization for int.
     */
    template<>
    struct row_extrator<int> {
        int extract(const char *row_value) {
            return std::atoi(row_value);
        }
        
        int extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for bool.
     */
    template<>
    struct row_extrator<bool> {
        bool extract(const char *row_value) {
            return std::atoi(row_value);
        }
        
        bool extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for long.
     */
    template<>
    struct row_extrator<long> {
        long extract(const char *row_value) {
            return std::atol(row_value);
        }
        
        long extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int64(stmt, columnIndex);
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
        
        std::string extract(sqlite3_stmt *stmt, int columnIndex) {
            return (const char*)sqlite3_column_text(stmt, columnIndex);
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
        
        double extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_double(stmt, columnIndex);
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
        
        std::shared_ptr<T> extract(sqlite3_stmt *stmt, int columnIndex) {
            auto type = sqlite3_column_type(stmt, columnIndex);
            if(type != SQLITE_NULL){
                return std::make_shared<T>(row_extrator<T>().extract(stmt, columnIndex));
            }else{
                return {};
            }
        }
    };
    
    template<class T>
    struct row_extrator<std::unique_ptr<T>> {
        std::unique_ptr<T> extract(const char *row_value) {
            if(row_value){
                return std::make_unique<T>(row_extrator<T>().extract(row_value));
            }else{
                return {};
            }
        }
        
        std::unique_ptr<T> extract(sqlite3_stmt *stmt, int columnIndex) {
            auto type = sqlite3_column_type(stmt, columnIndex);
            if(type != SQLITE_NULL){
                return std::make_unique<T>(row_extrator<T>().extract(stmt, columnIndex));
            }else{
                return {};
            }
        }
    };
    
    template<class ...Args>
    struct row_extrator<std::tuple<Args...>> {
        
        std::tuple<Args...> extract(char **argv) {
            std::tuple<Args...> res;
            this->extract<std::tuple_size<decltype(res)>::value>(res, argv);
            return res;
        }
        
    protected:
        
        template<size_t I, typename std::enable_if<I != 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, char **argv) {
            typedef typename std::tuple_element<I - 1, typename std::tuple<Args...>>::type tuple_type;
            std::get<I - 1>(t) = row_extrator<tuple_type>().extract(argv[I - 1]);
            this->extract<I - 1>(t, argv);
        }
        
        template<size_t I, typename std::enable_if<I == 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, char **argv) {
            //..
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
    
    struct database_connection {
        
        database_connection(const std::string &filename) {
            auto rc = sqlite3_open(filename.c_str(), &this->db);
            if(rc != SQLITE_OK){
                throw std::runtime_error(sqlite3_errmsg(this->db));
            }
        }
        
        ~database_connection() {
            sqlite3_close(this->db);
        }
        
        sqlite3* get_db() {
            return this->db;
        }
        
    protected:
        sqlite3 *db = nullptr;
    };
    
    template<class ...Ts>
    struct storage_impl {
        
        template<class T>
        constexpr bool type_is_mapped() const {
            return false;
        }
        
        template<class O>
        auto& get_impl() {
            throw std::runtime_error("no impl with type " + std::string(typeid(O).name()));
        }
        
        template<class O>
        int insert(const O &/*o*/, sqlite3 */*db*/, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in insert");
        }
        
        /*template<class O, class C, class ...Args>
        C get_all(sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_all");
        }*/
        
        template<class O, class I>
        O get(I /*id*/, sqlite3 */*db*/, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get");
        }
        
        template<class O>
        void update(const O &/*o*/, sqlite3 */*db*/, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in update");
        }
        
        template<class O, class I>
        std::shared_ptr<O> get_no_throw(I /*id*/, sqlite3 */*db*/, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in get_no_throw");
        }
        
        template<class O, class I>
        void remove(I /*id*/, sqlite3 */*db*/, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in remove");
        }
        
        /*template<class O>
        void remove_all(sqlite3 *, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in remove");
        }*/
        
        /*template<class O, class ...Args>
        int count(sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }*/
        
        /*template<class F, class O, class ...Args>
        double avg(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in avg");
        }*/
        
        /*template<class F, class O, class ...Args>
        std::shared_ptr<F> sum(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in sum");
        }*/
        
        /*template<class F, class O, class ...Args>
        double total(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in total");
        }*/
        
        /*template<class F, class O, class ...Args>
        std::vector<F> select(F O::*, sqlite3 *, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in select");
        }*/
        
        /*template<
        class ...Args,
        class R = std::tuple<typename field_extractor<Args>::type...>,
        class O = typename object_type_extractor<Args...>::type,
        class ...Conds>
        std::vector<R> select(sqlite3 *, columns_t<Args...>, std::nullptr_t, Conds ...conds) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in select");
        }*/
        
        /*template<class F, class O, class ...Args>
        int count(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in count");
        }*/
        
        /*template<class F, class O, class ...Args>
        std::string group_concat(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }
        
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*, const std::string &y, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in group_concat");
        }*/
        
        /*template<class F, class O, class ...Args>
        std::shared_ptr<F> max(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
        }*/
        
        /*template<class F, class O, class ...Args>
        std::shared_ptr<F> min(F O::*, sqlite3 *, std::nullptr_t, Args ...args) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in min");
        }*/
        
        template<class O>
        std::string dump(const O &, sqlite3 *, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
        }
        
        void sync_schema(sqlite3 */*db*/, bool /*preserve*/) {
            return;
        }
        
        void begin_transaction(sqlite3 *db) {
            std::stringstream ss;
            ss << "BEGIN TRANSACTION";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        void commit(sqlite3 *db) {
            std::stringstream ss;
            ss << "COMMIT";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        void rollback(sqlite3 *db) {
            std::stringstream ss;
            ss << "ROLLBACK";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        void drop_table(const std::string &tableName, sqlite3 *db) {
            std::stringstream ss;
            ss << "DROP TABLE " << tableName;
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        void rename_table(sqlite3 *db, const std::string &oldName, const std::string &newName) {
            std::stringstream ss;
            ss << "ALTER TABLE " << oldName << " RENAME TO " << newName;
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        std::string current_timestamp(sqlite3 *db) {
            std::string res;
            std::stringstream ss;
            ss << "SELECT CURRENT_TIMESTAMP";
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **/*azColName*/)->int{
                                       auto &res = *(std::string*)data;
                                       if(argc){
                                           if(argv[0]){
                                               res = row_extrator<std::string>().extract(argv[0]);
                                           }
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
    };
    
    template<class H, class ...Ts>
    struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
        typedef H table_type;
        
        storage_impl(H h, Ts ...ts) : Super(ts...), table(h) {}
        
        table_type table;
        
        template<class T>
        constexpr bool type_is_mapped() const {
            return Super::template type_is_mapped<T>();
        }
        
        void create_table(sqlite3 *db, const std::string &tableName) {
            std::stringstream ss;
            ss << "CREATE TABLE " << tableName << " ( ";
            auto columnsCount = this->table.columns_count();
            auto index = 0;
            this->table.for_each_column([columnsCount, &index, &ss] (auto c) {
                ss << c.name << " ";
                typedef typename decltype(c)::field_type field_type;
                ss << type_printer<field_type>().print() << " ";
                if(c.template has<primary_key>()) {
                    ss << "PRIMARY KEY ";
                }
                if(c.template has<autoincrement>()) {
                    ss << "AUTOINCREMENT ";
                }
                if(auto defaultValue = c.default_value()){
                    ss << "DEFAULT " << *defaultValue << " ";
                }
                if(c.not_null()){
                    ss << "NOT NULL ";
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
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        template<class O, class HH = typename H::object_type>
        auto& get_impl(typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return *this;
        }
        
        template<class O, class HH = typename H::object_type>
        auto& get_impl(typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::template get_impl<O>();
        }
        
        template<class O, class HH = typename H::object_type>
        std::string find_table_name(typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return this->table.name;
        }
        
        template<class O, class HH = typename H::object_type>
        std::string find_table_name(typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return this->Super::template find_table_name<O>();
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(F O::*m, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return this->table.find_column_name(m);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(F O::*m, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::column_name(m);
        }
        
        template<class O, class HH = typename H::object_type>
        int insert(const O &o, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::template insert(o, db);
        }
        
        template<class O, class HH = typename H::object_type>
        int insert(const O &o, sqlite3 *db, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            int res = 0;
            std::stringstream ss;
            ss << "INSERT INTO " << this->table.name << " (";
            std::vector<std::string> columnNames;
            this->table.for_each_column([&] (auto c) {
                if(!c.template has<primary_key>()) {
                    columnNames.emplace_back(c.name);
                }
            });
            
            auto columnNamesCount = int(columnNames.size());
            for(auto i = 0; i < columnNamesCount; ++i) {
                ss << columnNames[i];
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ") ";
                }
            }
            ss << "VALUES(";
            for(auto i = 0; i < columnNamesCount; ++i) {
                ss << "?";
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ")";
                }
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                auto index = 1;
                this->table.for_each_column([&o, &index, &stmt] (auto c) {
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
            return res;
        }
        
        template<class O, class HH = typename H::object_type>
        void update(const O &o, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            Super::template update(o, db, nullptr);
        }
        
        template<class O, class HH = typename H::object_type>
        void update(const O &o, sqlite3 *db, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            std::stringstream ss;
            ss << "UPDATE " << this->table.name << " SET ";
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
            ss << "WHERE ";
            auto primaryKeyColumnNames = this->table.template column_names_with<primary_key>();
            for(auto i = 0; i < primaryKeyColumnNames.size(); ++i) {
                ss << primaryKeyColumnNames[i] << " = ?";
                if(i < primaryKeyColumnNames.size() - 1) {
                    ss << " AND ";
                }else{
                    ss << " ";
                }
            }
            
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
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
        }
        
        template<class O, class HH = typename H::object_type>
        std::string dump(const O &o, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::dump(o, db, nullptr);
        }
        
        template<class O, class HH = typename H::object_type>
        std::string dump(const O &o, sqlite3 *db, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            std::stringstream ss;
            ss << "{ ";
            auto columnsCount = this->table.columns_count();
            auto index = 0;
            this->table.for_each_column([&] (auto c) {
                typedef typename decltype(c)::field_type field_type;
                auto &value = o.*c.member_pointer;
                ss << c.name << " : '" << field_printer<field_type>()(value) << "'";
                if(index < columnsCount - 1) {
                    ss << ", ";
                }else{
                    ss << " }";
                }
                ++index;
            });
            return ss.str();
        }
        
        template<class O, class I, class HH = typename H::object_type>
        void remove(I id, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            Super::template remove<O>(id, db, nullptr);
        }
        
        template<class O, class I, class HH = typename H::object_type>
        void remove(I id, sqlite3 *db, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            std::stringstream ss;
            ss << "DELETE FROM " << this->table.name;
            ss << " WHERE ";
            std::vector<std::string> primaryKeyColumnNames;
            this->table.for_each_column([&] (auto c) {
                if(c.template has<primary_key>()) {
                    primaryKeyColumnNames.emplace_back(c.name);
                }
            });
            for(auto i = 0; i < primaryKeyColumnNames.size(); ++i) {
                ss << primaryKeyColumnNames[i] << " =  ?";
                if(i < primaryKeyColumnNames.size() - 1) {
                    ss << " AND ";
                }else{
                    ss << " ";
                }
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
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
        }
        
        bool table_exists(const std::string &tableName, sqlite3 *db) {
            bool res = false;
            std::stringstream ss;
            ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '" << "table" << "' AND name = '" << tableName << "'";
            auto query = ss.str();
//            data_t<bool, storage_impl*> data{this, &res};
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
//                                       auto &d = *(data_t<bool, storage_impl*>*)data;
//                                       auto &res = *d.res;
                                       auto &res = *(bool*)data;
                                       if(argc){
                                           res = !!std::atoi(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
        
        std::vector<table_info> get_table_info(const std::string &tableName, sqlite3 *db) {
            std::vector<table_info> res;
            auto query = "PRAGMA table_info(" + tableName + ")";
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
                                       auto &res = *(std::vector<table_info>*)data;
                                       if(argc){
                                           auto index = 0;
                                           auto cid = std::atoi(argv[index++]);
                                           std::string name = argv[index++];
                                           std::string type = argv[index++];
                                           bool notnull = !!std::atoi(argv[index++]);
                                           std::string dflt_value = argv[index] ? argv[index] : "";
                                           index++;
                                           bool pk = !!std::atoi(argv[index++]);
                                           res.push_back(table_info{cid, name, type, notnull, dflt_value, pk});
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
        
        void add_column(table_info &ti, sqlite3 *db) {
            std::stringstream ss;
            ss << "ALTER TABLE " << this->table.name << " ADD COLUMN " << ti.name << " ";
            ss << ti.type << " ";
            if(ti.pk){
                ss << "PRIMARY KEY ";
            }
            if(ti.notnull){
                ss << "NOT NULL ";
            }
            if(ti.dflt_value.length()) {
                ss << "DEFAULT " << ti.dflt_value << " ";
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            auto prepareResult = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            if (prepareResult == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        /**
         *  Copies current table to another table with a given **name**.
         *  Performs CREATE TABLE %name% AS SELECT %this->table.columns_names()% FROM &this->table.name%;
         */
        void copy_table(sqlite3 *db, const std::string &name) {
            std::stringstream ss;
            std::vector<std::string> columnNames;
            this->table.for_each_column([&] (auto c) {
                //                if(!c.template has<primary_key>()) {
                columnNames.emplace_back(c.name);
                //                }
            });
            auto columnNamesCount = int(columnNames.size());
            ss << "INSERT INTO " << name << " (";
            for(auto i = 0; i < columnNamesCount; ++i) {
                ss << columnNames[i];
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << ") ";
            ss << "SELECT ";
            for(auto i = 0; i < columnNamesCount; ++i) {
                ss << columnNames[i];
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << " FROM " << this->table.name << " ";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        void sync_schema(sqlite3 *db, bool preserve) {
            
            //  first let's see if table with such name exists..
            auto gottaCreateTable = !this->table_exists(this->table.name, db);
            if(!gottaCreateTable){
                
                //  now get current table info from db using `PRAGMA table_info` query..
                auto dbTableInfo = get_table_info(this->table.name, db);
                
                //  get table info provided in `make_table` call..
                auto storageTableInfo = this->table.get_table_info();
                
                //  this vector will contain pointers to columns that gotta be added..
                std::vector<table_info*> columnsToAdd;
                
//                auto storageTableInfoCount = int(storageTableInfo.size());
                for(auto storageColumnInfoIndex = 0; storageColumnInfoIndex < int(storageTableInfo.size()); ++storageColumnInfoIndex) {
                    
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
                        auto storageColumnInfoType = to_sqlite_type(storageColumnInfo.type);
                        if(dbColumnInfoType && storageColumnInfoType) {
                            auto columnsAreEqual = dbColumnInfo.name == storageColumnInfo.name &&
                            *dbColumnInfoType == *storageColumnInfoType &&
                            dbColumnInfo.notnull == storageColumnInfo.notnull &&
                            bool(dbColumnInfo.dflt_value.length()) == bool(storageColumnInfo.dflt_value.length()) &&
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
                    if(dbTableInfo.size() > 0){
                        if(!preserve){
                            gottaCreateTable = true;
                        }else{
                            
                            //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
                            //  a name already exists we append suffix 1, then 2, etc until we find a free name..
                            auto backupTableName = this->table.name + "_backup";
                            if(this->table_exists(backupTableName, db)){
                                int suffix = 1;
                                do{
                                    auto anotherBackupTableName = backupTableName + std::to_string(suffix);
                                    if(!this->table_exists(anotherBackupTableName, db)){
                                        backupTableName = anotherBackupTableName;
                                        break;
                                    }
                                    ++suffix;
                                }while(true);
                            }
                            
                            this->create_table(db, backupTableName);
                            
                            this->copy_table(db, backupTableName);
                            
                            this->drop_table(this->table.name, db);
                            
                            this->rename_table(db, backupTableName, this->table.name);
                        }
                    }
                }
                if(gottaCreateTable){
                    this->drop_table(this->table.name, db);
                    this->create_table(db, this->table.name);
                    cout<<"table "<<this->table.name<<" dropped and recreated"<<endl;
                }else{
                    if(columnsToAdd.size()){
                        for(auto columnPointer : columnsToAdd) {
                            if(columnPointer->notnull && columnPointer->dflt_value.empty()){
                                gottaCreateTable = true;
                                break;
                            }
                        }
                        if(!gottaCreateTable){
                            for(auto columnPointer : columnsToAdd) {
                                this->add_column(*columnPointer, db);
                                cout<<"column "<<columnPointer->name<<" added to table "<<this->table.name<<endl;
                            }
                        }else{
                            this->drop_table(this->table.name, db);
                            this->create_table(db, this->table.name);
                            cout<<"table "<<this->table.name<<" dropped and recreated"<<endl;
                        }
                    }else{
                        cout<<"table "<<this->table.name<<" is synced"<<endl;
                    }
                }
            }else{
                this->create_table(db, this->table.name);
                cout<<"table "<<this->table.name<<" created"<<endl;
            }
            Super::sync_schema(db, preserve);
        }
        
    private:
        typedef storage_impl<Ts...> Super;
        typedef storage_impl<H, Ts...> Self;
    };
    
    /**
     *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage` function.
     */
    template<class ...Ts>
    struct storage_t {
        typedef storage_impl<Ts...> impl_type;
        
        template<class T, class ...Args>
        struct view_t {
            typedef T mapped_type;
            
            storage_t &storage;
            std::shared_ptr<database_connection> connection;
//            std::tuple<Args...> options;
            
            const std::string query;
            
            view_t(storage_t &stor, decltype(connection) conn, Args ...args):
            storage(stor),
            connection(conn),
            query([&]{
                std::string q;
                stor.template generate_select_asterisk<T>(&q, args...);
                return q;
            }()){}
            
            struct iterator_t {
                
            protected:
                sqlite3_stmt *stmt = nullptr;
                view_t<T, Args...> &view;
                std::shared_ptr<T> temp;
                
                void extract_value(decltype(temp) &temp) {
                    temp = std::make_shared<T>();
                    auto &storage = this->view.storage;
                    auto &impl = storage.template get_impl<T>();
                    auto index = 0;
                    impl.table.for_each_column([&index, &temp, this] (auto c) {
                        auto member_pointer = c.member_pointer;
                        auto value = row_extrator<typename decltype(c)::field_type>().extract(this->stmt, index++);
                        (*temp).*member_pointer = value;
                    });
                }
                
            public:
                iterator_t(decltype(stmt) stmt_, view_t<T, Args...> &view_):stmt(stmt_),view(view_){
                    this->operator++();
                }
                
                ~iterator_t() {
                    statement_finalizer f{this->stmt};
                }
                
                T& operator*() {
                    if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                    if(!this->temp){
//                        this->temp = std::make_shared<T>();
                        this->extract_value(this->temp);
                    }
                    return *this->temp;
                }
                
                T* operator->() {
                    if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                    if(!this->temp){
//                        this->temp = std::make_shared<T>();
                        this->extract_value(this->temp);
                    }
                    return &*this->temp;
                }
                
                void operator++() {
                    if(this->stmt){
                        auto ret = sqlite3_step(this->stmt);
                        switch(ret){
                            case SQLITE_ROW:
                                this->temp = nullptr;
                                break;
                            case SQLITE_DONE:{
                                statement_finalizer f{this->stmt};
                                this->stmt = nullptr;
                            }break;
                            default:{
                                auto db = this->view.connection->get_db();
                                auto msg = sqlite3_errmsg(db);
                                throw std::runtime_error(msg);
                            }
                        }
                    }
                }
                
                void operator++(int) {
                    this->operator++();
                }
                
                const bool operator==(const iterator_t &other) const {
                    return this->stmt == other.stmt;
                }
                
                const bool operator!=(const iterator_t &other) const {
                    return !(*this == other);
                }
            };
            
            size_t size() {
                return this->storage.template count<T>();
            }
            
            bool empty() {
                return !this->size();
            }
            
            iterator_t end() {
                return {nullptr, *this};
            }
            
            iterator_t begin() {
                sqlite3_stmt *stmt = nullptr;
                auto db = this->connection->get_db();
                auto ret = sqlite3_prepare_v2(db, this->query.c_str(), -1, &stmt, nullptr);
                if(ret == SQLITE_OK){
                    return {stmt, *this};
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
        };
        
        /**
         *  @param filename_ database filename.
         */
        storage_t(const std::string &filename_, impl_type impl_):
        filename(filename_),
        impl(impl_),
        inMemory(filename_.empty() or filename_ == ":memory:"){
            if(inMemory){
                currentTransaction = std::make_shared<database_connection>(this->filename);
            }
        }
        
    protected:
        
        template<class O>
        auto& get_impl() {
            return this->impl.template get_impl<O>();
        }
        
        template<class T>
        std::string string_from_expression(T t) {
            auto isNullable = type_is_nullable<T>::value;
            if(isNullable and !type_is_nullable<T>()(t)){
                return "NULL";
            }else{
                auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                std::stringstream ss;
                if(needQuotes){
                    ss << "'";
                }
                ss << field_printer<T>()(t);
                if(needQuotes){
                    ss << "'";
                }
                return ss.str();
            }
        }
        
        std::string string_from_expression(const std::string &t) {
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
//            return this->table.name + "." + this->table.find_column_name(m);
            return this->impl.column_name(m);
        }
        
        template<class T>
        std::string string_from_expression(length_t<T> &len) {
            std::stringstream ss;
            auto expr = this->string_from_expression(len.t);
            ss << static_cast<std::string>(len) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class O>
        std::string string_from_expression(asc_t<O> &a) {
            std::stringstream ss;
            ss << this->string_from_expression(a.o) << " " << static_cast<std::string>(a) << " ";
            return ss.str();
        }
        
        
        template<class O>
        std::string string_from_expression(desc_t<O> &a) {
            std::stringstream ss;
            ss << this->string_from_expression(a.o) << " " << static_cast<std::string>(a) << " ";
            return ss.str();
        }
        
        template<class T>
        std::string process_where(is_null_t<T> &c) {
            std::stringstream ss;
            ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
            return ss.str();
        }
        
        template<class T>
        std::string process_where(is_not_null_t<T> &c) {
            std::stringstream ss;
            ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
            return ss.str();
        }
        
        template<class C>
        std::string process_where(negated_condition_t<C> &c) {
            std::stringstream ss;
            ss << " " << static_cast<std::string>(c) << " ";
            auto cString = this->process_where(c.c);
            ss << " (" << cString << " ) ";
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(and_condition_t<L, R> &c) {
            std::stringstream ss;
            ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(or_condition_t<L, R> &c) {
            std::stringstream ss;
            ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
            return ss.str();
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
        
        template<class A, class T>
        std::string process_where(between_t<A, T> &bw) {
            std::stringstream ss;
            auto expr = this->string_from_expression(bw.expr);
            ss << expr << " " << static_cast<std::string>(bw) << " " << this->string_from_expression(bw.b1) << " AND " << this->string_from_expression(bw.b2) << " ";
            return ss.str();
        }
        
        template<class O>
        std::string process_order_by(order_by_t<O> &orderBy) {
            std::stringstream ss;
            auto columnName = this->string_from_expression(orderBy.o);
            ss << columnName << " ";
            return ss.str();
        }
        
        void process_single_condition(std::stringstream &ss, limit_t limt) {
            ss << static_cast<std::string>(limt) << " ";
            if(limt.has_offset) {
                if(limt.offset_is_implicit){
                    ss << limt.off << ", " << limt.lim;
                }else{
                    ss << limt.lim << " OFFSET " << limt.off;
                }
            }else{
                ss << limt.lim;
            }
            ss << " ";
        }
        
        template<class C>
        void process_single_condition(std::stringstream &ss, where_t<C> w) {
            ss << static_cast<std::string>(w) << " ";
            auto whereString = this->process_where(w.c);
            ss << "(" << whereString << ") ";
        }
        
        template<class O>
        void process_single_condition(std::stringstream &ss, order_by_t<O> orderBy) {
            ss << static_cast<std::string>(orderBy) << " ";
            auto orderByString = this->process_order_by(orderBy);
            ss << orderByString << " ";
        }
        
        template<class ...Args>
        void process_single_condition(std::stringstream &ss, group_by_t<Args...> groupBy) {
            std::vector<std::string> expressions;
            typedef std::tuple<Args...> typle_t;
            tuple_helper::iterator<std::tuple_size<typle_t>::value - 1, Args...>()(groupBy.args, [&](auto &v){
                auto expression = this->string_from_expression(v);
                expressions.push_back(expression);
            });
            ss << static_cast<std::string>(groupBy) << " ";
            for(auto i = 0; i < expressions.size(); ++i) {
                ss << expressions[i];
                if(i < expressions.size() - 1) {
                    ss << ", ";
                }
            }
            ss << " ";
        }
        
        /**
         *  Recursion end.
         */
        template<class ...Args>
        void process_conditions(std::stringstream &, Args ...args) {
            //..
        }
        
        template<class C, class ...Args>
        void process_conditions(std::stringstream &ss, C c, Args ...args) {
            this->process_single_condition(ss, c);
            this->process_conditions(ss, args...);
        }
        
    public:
        
        template<class T, class ...Args>
        view_t<T, Args...> view(Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return {*this, connection, args...};
        }
        
        template<class O, class ...Args>
        void remove_all(Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            impl.template remove_all<O>(db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::stringstream ss;
            ss << "DELETE FROM " << impl.table.name << " ";
            this->process_conditions(ss, args...);
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        
        /**
         *  Delete routine.
         *  O is an object's type. Must be specified explicitly.
         *  @param id id of object to be removed.
         */
        template<class O, class I>
        void remove(I id) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            this->impl.template remove<O>(id, db);
        }
        
        /**
         *  Update routine. Sets all non primary key fields where primary key is equal.
         *  O is an object type. May be not specified explicitly cause it can be deduced by
         *      compiler from first parameter.
         *  @param o object to be updated.
         */
        template<class O>
        void update(const O &o) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            this->impl.update(o, db);
        }
        
    protected:
        
        /**
         *  O - mapped type
         *  Args - conditions
         *  @param query - result query string
         *  @return impl for O
         */
        template<class O, class ...Args>
        auto& generate_select_asterisk(std::string *query, Args ...args) {
            std::stringstream ss;
            ss << "SELECT ";
            auto &impl = this->get_impl<O>();
            auto columnNames = impl.table.column_names();
            for(auto i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM " << impl.table.name << " ";
            this->process_conditions(ss, args...);
            if(query){
                *query = ss.str();
            }
            return impl;
        }
        
        template<class T>
        std::string parse_table_name(T &t) {
            return {};
        }
        
        template<class F, class O>
        std::string parse_table_name(F O::*m) {
            return this->impl.template find_table_name<O>();
        }
        
        template<class T>
        std::string parse_table_name(length_t<T> &len) {
            return this->parse_table_name(len.t);
        }
        
        template<class ...Args>
        std::vector<std::string> parse_table_names(Args ...args) {
            return {};
        }
        
        template<class H, class ...Args>
        std::vector<std::string> parse_table_names(H h, Args ...args) {
            auto res = this->parse_table_names(std::forward<Args>(args)...);
            auto tableName = this->parse_table_name(h);
            if(tableName.length()){
                if(std::find(res.begin(),
                             res.end(),
                             tableName) == res.end()) {
                    res.push_back(tableName);
                }
            }
            return res;
        }
        
        template<class ...Args>
        std::vector<std::string> parse_table_names(columns_t<Args...> &cols) {
            std::vector<std::string> res;
            cols.for_each([&](auto &m){
                auto tableName = this->parse_table_name(m);
                if(tableName.length()){
                    if(std::find(res.begin(),
                                 res.end(),
                                 tableName) == res.end()) {
                        res.push_back(tableName);
                    }
                }
            });
            return res;
        }
        
    public:
        
        /**
         *  Select * with no conditions routine.
         *  O is an object type to be extracted. Must be specified explicitly.
         *  @return All objects of type O stored in database at the moment.
         */
        template<class O, class C = std::vector<O>, class ...Args>
        C get_all(Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.template get_all<O, C>(db, nullptr, args...);
            C res;
            
            /*std::stringstream ss;
            ss << "SELECT ";
            auto &impl = this->get_impl<O>();
            auto columnNames = impl.table.column_names();
            for(auto i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM " << impl.table.name << " ";
            impl.process_conditions(ss, args...);
            auto query = ss.str();*/
            std::string query;
            auto &impl = this->generate_select_asterisk<O>(&query, args...);
            
            typedef std::tuple<C*, decltype(&impl)> date_tuple_t;
            date_tuple_t data{&res, &impl};
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
                                       auto &d = *(date_tuple_t*)data;
                                       auto &res = *std::get<0>(d);
                                       auto t = std::get<1>(d);
                                       if(argc){
                                           O o;
                                           auto index = 0;
                                           t->table.for_each_column([&index, &o, argv] (auto c) {
                                               auto member_pointer = c.member_pointer;
                                               auto value = row_extrator<typename decltype(c)::field_type>().extract(argv[index++]);
                                               o.*member_pointer = value;
                                           });
                                           res.push_back(std::move(o));
                                       }
                                       return 0;
                                   }, &data, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
        
        /**
         *  Select * by id routine.
         *  throws sqlite_orm::not_found_exeption if object not found with given id.
         *  throws std::runtime_error in case of db error.
         *  O is an object type to be extracted. Must be specified explicitly.
         *  @return Object of type O where id is equal parameter passed or throws `not_found_exception`
         *  if there is no object with such id.
         */
        template<class O, class I>
        O get(I id) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.template get<O>(id, db);
            auto &impl = this->get_impl<O>();
            std::shared_ptr<O> res;
            std::stringstream ss;
            ss << "SELECT ";
            auto columnNames = impl.table.column_names();
            for(auto i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM " << impl.table.name << " WHERE ";
            auto primaryKeyColumnName = impl.table.primary_key_column_name();
            if(primaryKeyColumnName.size()){
                ss << primaryKeyColumnName << " = " << string_from_expression(id);
                auto query = ss.str();
                typedef std::tuple<decltype(&impl), decltype(res)*> data_tuple_t;
                data_tuple_t dataTuple = std::make_tuple(&impl, &res);
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &d = *(data_tuple_t*)data;
                                           auto &res = *std::get<1>(d);
                                           auto t = std::get<0>(d);
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
                                       }, &dataTuple, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
                if(res){
                    return *res;
                }else{
                    throw not_found_exception{};
                }
            }else{
                throw std::runtime_error("table " + impl.table.name + " has no primary key column");
            }
        }
        
        /**
         *  The same as `get` function but doesn't throw an exeption if noting found but returns std::shared_ptr with null value.
         *  throws std::runtime_error iin case of db error.
         */
        template<class O, class I>
        std::shared_ptr<O> get_no_throw(I id) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.template get_no_throw<O>(id, db);
            auto &impl = this->get_impl<O>();
            std::shared_ptr<O> res;
            std::stringstream ss;
            ss << "SELECT ";
            auto columnNames = impl.table.column_names();
            for(auto i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM " << impl.table.name << " WHERE ";
//            ss << "SELECT * FROM " << impl.table.name << " WHERE ";
            auto primaryKeyColumnName = impl.table.primary_key_column_name();
            if(primaryKeyColumnName.size()){
                ss << primaryKeyColumnName << " = " << string_from_expression(id);
                auto query = ss.str();
                typedef std::tuple<decltype(&impl), decltype(&res)> Data;
                Data aTuple = std::make_tuple(&impl, &res);
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &aTuple = *(Data*)data;
                                           auto &res = *std::get<1>(aTuple);
                                           auto t = std::get<0>(aTuple);
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
                                       }, &aTuple, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
                return res;
            }else{
                throw std::runtime_error("table " + impl.table.name + " has no primary key column");
            }
        }
        
        /**
         *  SELECT COUNT(*) with no conditions routine. https://www.sqlite.org/lang_aggfunc.html#count
         *  @return Number of O object in table.
         */
        template<class O, class ...Args>
        int count(Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.template count<O>(db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            int res = 0;
            std::stringstream ss;
            ss << "SELECT COUNT(*) FROM " << impl.table.name << " ";
            this->process_conditions(ss, args...);
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **azColName)->int{
                                       auto &res = *(int*)data;
                                       if(argc){
                                           res = std::atoi(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
        
        /**
         *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
         *  @param m member pointer to class mapped to the storage.
         */
        template<class F, class O, class ...Args>
        int count(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.count(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            int res = 0;
            std::stringstream ss;
            ss << "SELECT COUNT(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &res = *(int*)data;
                                           if(argc){
                                               res = std::atoi(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return average value from db.
         */
        template<class F, class O, class ...Args>
        double avg(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.avg(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            double res = 0;
            std::stringstream ss;
            ss << "SELECT AVG(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &res = *(double*)data;
                                           if(argc){
                                               res = std::atof(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  GROUP_CONCAT(X) query.  https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.group_concat(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::string res;
            std::stringstream ss;
            ss << "SELECT GROUP_CONCAT(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               res = argv[0];
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*m, const std::string &y, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.group_concat(m, y, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::string res;
            std::stringstream ss;
            ss << "SELECT GROUP_CONCAT(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ",\"" << y << "\") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               res = argv[0];
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*m, const char *y, Args ...args) {
            return this->group_concat(m, std::string(y), args...);
        }
        
        /**
         *  MAX(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return std::shared_ptr with max value or null if sqlite engine returned null.
         */
        template<class F, class O, class ...Args>
        std::shared_ptr<F> max(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.max(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT MAX(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM " << impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **azColName)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                               }
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  MIN(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return std::shared_ptr with min value or null if sqlite engine returned null.
         */
        template<class F, class O, class ...Args>
        std::shared_ptr<F> min(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.min(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT MIN(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM " << impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **/*azColName*/)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                               }
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  SUM(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return std::shared_ptr with sum value or null if sqlite engine returned null.
         */
        template<class F, class O, class ...Args>
        std::shared_ptr<F> sum(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.sum(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT SUM(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **/*azColName*/)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  TOTAL(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return total value (the same as SUM but not nullable. More details here https://www.sqlite.org/lang_aggfunc.html)
         */
        template<class F, class O, class ...Args>
        double total(F O::*m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.total(m, db, nullptr, args...);
            auto &impl = this->get_impl<O>();
            double res;
            std::stringstream ss;
            ss << "SELECT TOTAL(";
            auto columnName = impl.table.find_column_name(m);
            if(columnName.length()){
                ss << columnName << ") FROM "<< impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **/*azColName*/)->int{
                                           auto &res = *(double*)data;
                                           if(argc){
                                               res = row_extrator<double>().extract(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  Select a single column into std::vector<T>.
         */
        template<class T, class ...Args, class R = typename column_result_t<T>::type>
        std::vector<R> select(T m, Args ...args) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.select(m, db, nullptr, args...);
            std::vector<R> res;
            std::stringstream ss;
            ss << "SELECT ";
//            auto &impl = this->get_impl<O>();
            //            auto columnName = this->table.find_column_name(m);
//            std::vector<std::string> tablesNames;
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << " ";
                auto tableNames = this->parse_table_names(m);
                if(tableNames.size()){
                    ss << "FROM " ;
                    for(auto i = 0; i < int(tableNames.size()); ++i) {
                        ss << tableNames[i];
                        if(i < int(tableNames.size()) - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                }
//                ss << impl.table.name << " ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
                                           auto &res = *(std::vector<R>*)data;
                                           auto value = row_extrator<R>().extract(argv[0]);
                                           res.push_back(value);
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else{
                throw std::runtime_error("column not found");
            }
            return res;
        }
        
        /**
         *  Select several columns into std::vector<std::tuple<...>>.
         */
        template<class ...Args,
        class R = std::tuple<typename column_result_t<Args>::type...>,
        class ...Conds//,
//        class O = typename object_type_extractor<Args...>::type
        >
        std::vector<R> select(columns_t<Args...> cols, Conds ...conds) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
//            return impl.select(db, cols, nullptr, conds...);
//            auto &impl = this->get_impl<O>();
            std::vector<R> res;
            std::stringstream ss;
            ss << "SELECT ";
            std::vector<std::string> columnNames;
            columnNames.reserve(cols.count());
            cols.for_each([&](auto &m) {
//                auto columnName = impl.table.find_column_name(m);
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    columnNames.push_back(columnName);
                }else{
                    throw std::runtime_error("column not found");
                }
            });
            for(auto i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
//            ss << impl.table.name << " ";
            auto tableNames = this->parse_table_names(cols);
            if(tableNames.size()){
                ss << " FROM ";
                for(auto i = 0; i < int(tableNames.size()); ++i) {
                    ss << tableNames[i];
                    if(i < int(tableNames.size()) - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
            }
            this->process_conditions(ss, conds...);
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **azColName) -> int {
                                       auto &res = *(std::vector<R>*)data;
                                       auto value = row_extrator<R>().extract(argv);
                                       res.push_back(value);
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            return res;
        }
        
        /**
         *  Returns a string representation of object of a class mapped to the storage.
         *  Type of string has json-like style.
         */
        template<class O>
        std::string dump(const O &o) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return impl.dump(o, db);
        }
        
        /**
         *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
         *  object doesn't matter.
         *  @return id of just created object.
         */
        template<class O>
        int insert(const O &o) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return impl.insert(o, db);
        }
        
        /**
         *  Drops table with given name.
         */
        void drop_table(const std::string &tableName) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            impl.drop_table(tableName, db);
        }
        
        /**
         *  sqlite3_changes function.
         */
        int changes() {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_changes(db);
        }
        
        /**
         *  sqlite3_total_changes function.
         */
        int total_changes() {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_total_changes(db);
        }
        
        int64 last_insert_rowid() {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_last_insert_rowid(db);
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
         *  @param preserve affects on function behaviour in case it is needed to remove a column. If it is `false` so table will be dropped 
         *  if there is column to remove, if `true` -  table is copies into another table, dropped and copied table is renamed with source table name.
         */
        void sync_schema(bool preserve = false) {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            this->impl.sync_schema(db, preserve);
        }
        
        bool transaction(std::function<bool()> f) {
            this->begin_transaction();
            auto db = this->currentTransaction->get_db();
            auto shouldCommit = f();
            if(shouldCommit){
                impl.commit(db);
            }else{
                impl.rollback(db);
            }
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
            return shouldCommit;
        }
        
        void begin_transaction() {
            if(!inMemory){
                if(this->currentTransaction) throw std::runtime_error("cannot start a transaction within a transaction");
                this->currentTransaction = std::make_shared<database_connection>(this->filename);
            }
            auto db = this->currentTransaction->get_db();
            impl.begin_transaction(db);
        }
        
        void commit() {
            if(!inMemory){
                if(!this->currentTransaction) throw std::runtime_error("cannot commit - no transaction is active");
            }
            auto db = this->currentTransaction->get_db();
            impl.commit(db);
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
        }
        
        void rollback() {
            if(!inMemory){
                if(!this->currentTransaction) throw std::runtime_error("cannot rollback - no transaction is active");
            }
            auto db = this->currentTransaction->get_db();
            impl.rollback(db);
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
        }
        
        std::string current_timestamp() {
            std::shared_ptr<database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<database_connection>(this->filename);
                db = connection->get_db();
            }else{
                db = this->currentTransaction->get_db();
            }
            return impl.current_timestamp(db);
        }
        
    protected:
        std::string filename;
        impl_type impl;
        std::shared_ptr<database_connection> currentTransaction;
        const bool inMemory;
    };
    
    template<class ...Ts>
    storage_t<Ts...> make_storage(const std::string &filename, Ts ...tables) {
        return {filename, storage_impl<Ts...>(tables...)};
    }
    
}

#endif /* sqlite_orm_h */
