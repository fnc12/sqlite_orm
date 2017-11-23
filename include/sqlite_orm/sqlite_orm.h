
#ifndef sqlite_orm_h
#define sqlite_orm_h

#include <string>
#include <tuple>
#include <type_traits>
#include <sqlite3.h>
#include <stdexcept>
#include <vector>   //  std::vector
#include <sstream>  //  std::stringstream
#include <algorithm>    //  std::find
#include <memory>   //  std::shared_ptr, std::unique_ptr
#include <typeinfo>
#include <regex>
#include <map>
#include <cctype>
#include <initializer_list>
#include <set>  //  std::set
#include <functional>   //  std::function
#include <ostream>  //  std::ostream
#include <iterator> //  std::iterator_traits

#if defined(_MSC_VER)
# if defined(min)
__pragma(push_macro("min"))
# undef min
# define __RESTORE_MIN__
# endif
# if defined(max)
__pragma(push_macro("max"))
# undef max
# define __RESTORE_MAX__
# endif
#endif // defined(_MSC_VER)

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
            void operator()(std::tuple<> &, L) {
                //..
            }
        };
    }
    
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
    
    struct blob_printer {
        inline const std::string& print() {
            static const std::string res = "BLOB";
            return res;
        }
    };
    
    //Note unsigned/signed char and simple char used for storing integer values, not char values.
    template<>
    struct type_printer<unsigned char> : public integer_printer {};
    
    template<>
    struct type_printer<signed char> : public integer_printer {};
    
    template<>
    struct type_printer<char> : public integer_printer {};
    
    template<>
    struct type_printer<unsigned short int> : public integer_printer {};
    
    template<>
    struct type_printer<short> : public integer_printer {};
    
    template<>
    struct type_printer<unsigned int> : public integer_printer {};
    
    template<>
    struct type_printer<int> : public integer_printer {};
    
    template<>
    struct type_printer<unsigned long> : public integer_printer {};
    
    template<>
    struct type_printer<long> : public integer_printer {};
    
    template<>
    struct type_printer<unsigned long long> : public integer_printer {};
    
    template<>
    struct type_printer<long long> : public integer_printer {};
    
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
    
    template<>
    struct type_printer<std::vector<char>> : public blob_printer {};
    
    namespace constraints {
        
        struct autoincrement_t {
            
            operator std::string() const {
                return "AUTOINCREMENT";
            }
        };
        
        template<class ...Cs>
        struct primary_key_t {
            std::tuple<Cs...> columns;
            int asc_option = 0;    //  0 - none, 1 - asc, -1 - desc
            
            primary_key_t(decltype(columns) c):columns(std::move(c)){}
            
            typedef void field_type;    //  for column iteration. Better be deleted
            typedef std::tuple<> constraints_type;
            
            operator std::string() const {
                std::string res = "PRIMARY KEY";
                switch(this->asc_option){
                    case 1:
                        res += " ASC";
                        break;
                    case -1:
                        res += " DESC";
                        break;
                }
                return res;
            }
            
            primary_key_t<Cs...> asc() const {
                auto res = *this;
                res.asc_option = 1;
                return res;
            }
            
            primary_key_t<Cs...> desc() const {
                auto res = *this;
                res.asc_option = -1;
                return res;
            }
        };
        
        struct unique_t {
            
            operator std::string() const {
                return "UNIQUE";
            }
        };
        
        template<class T>
        struct default_t {
            typedef T value_type;
            
            value_type value;
            
            operator std::string() const {
                std::stringstream ss;
                ss << "DEFAULT ";
                auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                if(needQuotes){
                    ss << "'";
                }
                ss << this->value;
                if(needQuotes){
                    ss << "'";
                }
                return ss.str();
            }
        };
        
#if SQLITE_VERSION_NUMBER >= 3006019
        
        template<class C, class R>
        struct foreign_key_t {
            C m = nullptr;
            R r = nullptr;
            
            foreign_key_t(C m_, R r_):m(m_),r(r_){}
            
            typedef void field_type;    //  for column iteration. Better be deleted
            typedef std::tuple<> constraints_type;
            
            template<class L>
            void for_each_column(L) {}
            
            template<class ...Opts>
            constexpr bool has_every() const  {
                return false;
            }
        };
        
        /**
         *  C can be a class member pointer, a getter function member pointer or setter
         *  func member pointer
         */
        template<class C>
        struct foreign_key_intermediate_t {
            C m = nullptr;
            
            foreign_key_intermediate_t(C m_):m(m_){}
            
            template<class O, class F>
            foreign_key_t<C, F O::*> references(F O::*r) {
                typedef foreign_key_t<C, F O::*> ret_type;
                return ret_type(this->m, r);
            }
            
            template<class O, class F>
            foreign_key_t<C, const F& (O::*)() const> references(const F& (O::*getter)() const) {
                typedef foreign_key_t<C, const F& (O::*)() const> ret_type;
                return ret_type(this->m, getter);
            }
            
            template<class O, class F>
            foreign_key_t<C, void (O::*)(F)> references(void (O::*setter)(F)) {
                typedef foreign_key_t<C, void (O::*)(F)> ret_type;
                return ret_type(this->m, setter);
            }
        };
#endif
        
    }
    
#if SQLITE_VERSION_NUMBER >= 3006019
    template<class O, class F>
    constraints::foreign_key_intermediate_t<F O::*> foreign_key(F O::*m) {
        return {m};
    }
    
    template<class O, class F>
    constraints::foreign_key_intermediate_t<const F& (O::*)() const> foreign_key(const F& (O::*getter)() const) {
        typedef constraints::foreign_key_intermediate_t<const F& (O::*)() const> ret_type;
        return ret_type(getter);
    }
    
    template<class O, class F>
    constraints::foreign_key_intermediate_t<void (O::*)(F)> foreign_key(void (O::*setter)(F)) {
        return {setter};
    }
#endif
    
    inline constraints::unique_t unique() {
        return {};
    }
    
    inline constraints::autoincrement_t autoincrement() {
        return {};
    }
    
    template<class ...Cs>
    inline constraints::primary_key_t<Cs...> primary_key(Cs ...cs) {
        typedef constraints::primary_key_t<Cs...> ret_type;
        return ret_type(std::make_tuple(cs...));
    }
    
    template<class T>
    constraints::default_t<T> default_value(T t) {
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
    
    /**
     *  This is class that tells `sqlite_orm` that type is nullable. Nullable types
     *  are mapped to sqlite database as `NULL` and not-nullable are mapped as `NOT NULL`.
     *  Default nullability status for all types is `NOT NULL`. So if you want to map
     *  custom type as `NULL` (for example: boost::optional) you have to create a specialiation
     *  of type_is_nullable for your type and derive from `std::true_type`.
     */
    template<class T>
    struct type_is_nullable : public std::false_type {
        bool operator()(const T &) const {
            return true;
        }
    };
    
    /**
     *  This is a specialization for std::shared_ptr. std::shared_ptr is nullable in sqlite_orm.
     */
    template<class T>
    struct type_is_nullable<std::shared_ptr<T>> : public std::true_type {
        bool operator()(const std::shared_ptr<T> &t) const {
            return static_cast<bool>(t);
        }
    };
    
    /**
     *  This is a specialization for std::unique_ptr. std::unique_ptr is nullable too.
     */
    template<class T>
    struct type_is_nullable<std::unique_ptr<T>> : public std::true_type {
        bool operator()(const std::unique_ptr<T> &t) const {
            return static_cast<bool>(t);
        }
    };
    
    namespace internal {
        
        /**
         *  This class is used in tuple interation to know whether tuple constains `default_value_t`
         *  constraint class and what it's value if it is
         */
        struct default_value_extractor {
            
            template<class A>
            std::shared_ptr<std::string> operator() (const A &) {
                return {};
            }
            
            template<class T>
            std::shared_ptr<std::string> operator() (const constraints::default_t<T> &t) {
                std::stringstream ss;
                /*auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                if(needQuotes){
                    ss << "'";
                }*/
                ss << t.value;
                /*if(needQuotes){
                    ss << "'";
                }*/
                return std::make_shared<std::string>(ss.str());
            }
        };
        
        /**
         *  Result of concatenation || operator
         */
        template<class L, class R>
        struct conc_t {
            L l;
            R r;
        };
        
        /**
         *  This class stores single column info. column_t is a pair of [column_name:member_pointer] mapped to a storage
         *  O is a mapped class, e.g. User
         *  T is a mapped class'es field type, e.g. &User::name
         *  Op... is a constraints pack, e.g. primary_key_t, autoincrement_t etc
         */
        template<class O, class T, class ...Op>
        struct column_t {
            typedef O object_type;
            typedef T field_type;
            typedef std::tuple<Op...> constraints_type;
            typedef field_type object_type::*member_pointer_t;
            typedef const field_type& (object_type::*getter_type)() const;
            typedef void (object_type::*setter_type)(field_type);
            
            /**
             *  Column name. Specified during construction in `make_column`.
             */
            const std::string name;
            
            /**
             *  Member pointer used to read/write member
             */
            member_pointer_t member_pointer/* = nullptr*/;
            
            /**
             *  Getter member function pointer to get a value. If member_pointer is null than 
             *  `getter` and `setter` must be not null
             */
            getter_type getter/* = nullptr*/;
            
            /**
             *  Setter member function.
             */
            setter_type setter/* = nullptr*/;
            
            /**
             *  Constraints tuple
             */
            constraints_type constraints;
            
            bool not_null() const {
                return !type_is_nullable<field_type>::value;
            }
            
            template<class Opt>
            constexpr bool has() const {
                return tuple_helper::tuple_contains_type<Opt, constraints_type>::value;
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
                tuple_helper::iterator<std::tuple_size<constraints_type>::value - 1, Op...>()(constraints, [&res](auto &v){
                    auto dft = internal::default_value_extractor()(v);
                    if(dft){
                        res = dft;
                    }
                });
                return res;
            }
        };
        
        template<class T>
        struct is_column : public std::false_type {};
        
        template<class O, class T, class ...Op>
        struct is_column<column_t<O, T, Op...>> : public std::true_type {};
        
        template<class T>
        struct is_foreign_key : std::false_type{};
        
        template<class C, class R>
        struct is_foreign_key<constraints::foreign_key_t<C, R>> : std::true_type{};
        
        template<class T>
        struct is_primary_key : public std::false_type {};
        
        template<class ...Cs>
        struct is_primary_key<constraints::primary_key_t<Cs...>> : public std::true_type {};
        
        /*template<class R, class F, class T>
        struct case_after_when_t;
        
        template<class R, class F, class ...Args>
        struct case_unclosed_t {
            typedef R result_type;
            typedef std::tuple<Args...> args_type;
            
            F field;    //  case field
            args_type args;
            
            case_unclosed_t(F f):field(std::move(f)){}
            
            template<class T>
            case_after_when_t<R, F, T> when(T t) {
                typedef case_after_when_t<R, F, T> ret_type;
                return ret_type(t);
            }
        };
        
        template<class R, class F, class T>
        struct case_after_when_t {
            T when;
            
            case_after_when_t(T t):when(std::move(t)){}
        };*/
        
        template<class T>
        struct expression_t {
            T t;
            
            expression_t(T t_):t(t_){}
        };
        
        template<class F, class R, class ...Whens>
        struct case_t {
            F field;
            typedef R result_type;
            typedef std::tuple<Whens...> whens_t;
            typedef case_t<F, R, Whens...> self_type;
            
            case_t(F f):field(std::move(f)){}
            
            self_type end() {
                return *this;
            }
            
            self_type end(const std::string &str) {
                this->column_name = str;
                return this->end();
            }
            
        protected:
            std::string column_name;
        };
        
    }
    
    template<class T>
    internal::expression_t<T> c(T t) {
        typedef internal::expression_t<T> result_type;
        return result_type(t);
    }
    
    template<class R, class F>
    internal::case_t<R, F> case_(F f) {
        typedef typename internal::case_t<R, F> ret_type;
        return ret_type(f);
    }
    
    template<class L, class R>
    internal::conc_t<L, R> conc(L l, R r) {
        return {l, r};
    }
    
    /**
     *  Is used to print members mapped to objects in storage_t::dump member function.
     */
    template<class T>
    struct field_printer {
        std::string operator()(const T &t) const {
            std::stringstream stream;
            stream << t;
            return stream.str();
        }
    };

    //upgrade to integer is required when using unsigned char(uint8_t)
    template<>
    struct field_printer<unsigned char> {
        std::string operator()(const unsigned char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };

    //upgrade to integer is required when using signed char(int8_t)
    template<>
    struct field_printer<signed char> {
        std::string operator()(const signed char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };
    
    //  char is neigher signer char nor unsigned char so it has its own specialization
    template<>
    struct field_printer<char> {
        std::string operator()(const char &t) const {
            std::stringstream stream;
            stream << +t;
            return stream.str();
        }
    };
    
    template<>
    struct field_printer<std::string> {
        std::string operator()(const std::string &t) const {
            return t;
        }
    };
    
    template<>
    struct field_printer<std::vector<char>> {
        std::string operator()(const std::vector<char> &t) const {
            std::stringstream ss;
            ss << std::hex;
            for(auto c : t) {
                ss << c;
            }
            return ss.str();
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
    internal::column_t<O, T, Op...> make_column(const std::string &name, T O::*m, Op ...constraints){
        return {name, m, nullptr, nullptr, std::make_tuple(constraints...)};
    }
    
    template<class O, class T, class ...Op>
    internal::column_t<O, T, Op...> make_column(const std::string &name, void (O::*setter)(T), const T& (O::*getter)() const, Op ...constraints) {
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }
    
    template<class O, class T, class ...Op>
    internal::column_t<O, T, Op...> make_column(const std::string &name, const T& (O::*getter)() const, void (O::*setter)(T), Op ...constraints) {
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }
    
    namespace conditions {
        
        struct limit_t {
            int lim;
            bool has_offset = false;
            bool offset_is_implicit = false;
            int off = 0;
            
            limit_t(decltype(lim) lim_):lim(lim_){}
            
            limit_t(decltype(lim) lim_,
                    decltype(has_offset) has_offset_,
                    decltype(offset_is_implicit) offset_is_implicit_,
                    decltype(off) off_):
            lim(lim_),
            has_offset(has_offset_),
            offset_is_implicit(offset_is_implicit_),
            off(off_){}
            
            operator std::string () const {
                return "LIMIT";
            }
        };
        
        struct offset_t {
            int off;
        };
        
        struct condition_t {};
        
        template<class C>
        struct negated_condition_t : public condition_t {
            C c;
            
            negated_condition_t(){}
            
            negated_condition_t(C c_):c(c_){}
            
            operator std::string () const {
                return "NOT";
            }
        };
        
        template<class L, class R>
        struct and_condition_t : public condition_t {
            L l;
            R r;
            
            and_condition_t(){}
            
            and_condition_t(L l_, R r_):l(l_),r(r_){}
            
            operator std::string () const {
                return "AND";
            }
        };
        
        template<class L, class R>
        struct or_condition_t : public condition_t {
            L l;
            R r;
            
            or_condition_t(){}
            
            or_condition_t(L l_, R r_):l(l_),r(r_){}
            
            operator std::string () const {
                return "OR";
            }
        };
        
        template<class L, class R>
        struct binary_condition : public condition_t {
            L l;
            R r;
            
            binary_condition(){}
            
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
            
            operator std::string () const {
                return "IN";
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
        
        template<class C>
        struct where_t {
            C c;
            
            operator std::string() const {
                return "WHERE";
            }
        };
        
        template<class O>
        struct order_by_t {
            O o;
            int ascDesc = 0;   //  1: asc, -1: desc
            
            order_by_t():o(nullptr){}
            
            order_by_t(O o_):o(o_){}
            
            operator std::string() const {
                return "ORDER BY";
            }
            
            order_by_t<O> asc() {
                auto res = *this;
                res.ascDesc = 1;
                return res;
            }
            
            order_by_t<O> desc() {
                auto res = *this;
                res.ascDesc = -1;
                return res;
            }
        };
        
        template<class ...Args>
        struct group_by_t {
            std::tuple<Args...> args;
            
            operator std::string() const {
                return "GROUP BY";
            }
        };
        
        template<class A, class T>
        struct between_t {
            A expr;
            T b1;
            T b2;
            
            operator std::string() const {
                return "BETWEEN";
            }
        };
        
        template<class A, class T>
        struct like_t: public binary_condition<A, T> {
            A a;
            T t;

            using binary_condition<A, T>::binary_condition;
            
            operator std::string() const {
                return "LIKE";
            }
        };
        
        template<class T>
        struct cross_join_t {
            typedef T type;
            
            operator std::string() const {
                return "CROSS JOIN";
            }
        };
        
        template<class T, class O>
        struct left_join_t {
            typedef T type;
            typedef O on_type;
            
            on_type constraint;
            
            operator std::string() const {
                return "LEFT JOIN";
            }
        };
        
        /*template<class T>
        struct left_join_t<T, void> {
            typedef T type;
            
            operator std::string() const {
                return "LEFT JOIN";
            }
        };*/
        
        template<class T, class O>
        struct join_t {
            
            typedef T type;
            typedef O on_type;
            
            on_type constraint;
            
            operator std::string() const {
                return "JOIN";
            }
        };
        
        /*template<class T>
        struct natural_join_t {
            typedef T type;
            
            operator std::string() const {
                return "NATURAL JOIN";
            }
        };*/
        
        template<class T, class O>
        struct left_outer_join_t {
            typedef T type;
            typedef O on_type;
            
            on_type constraint;
            
            operator std::string() const {
                return "LEFT OUTER JOIN";
            }
        };
        
        template<class T>
        struct on_t {
            T t;
            
            operator std::string() const {
                return "ON";
            }
        };
        
        template<class F, class O>
        struct using_t {
            F O::*column;
            
            operator std::string() const {
                return "USING";
            }
        };
        
        template<class T, class O>
        struct inner_join_t {
            typedef T type;
            typedef O on_type;
            
            on_type constraint;
            
            operator std::string() const {
                return "INNER JOIN";
            }
        };
        
    }
    
    //  cute operators for columns
    
    template<class T, class R>
    conditions::lesser_than_t<T, R> operator<(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::lesser_than_t<L, T> operator<(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::lesser_or_equal_t<T, R> operator<=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::lesser_or_equal_t<L, T> operator<=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::greater_than_t<T, R> operator>(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::greater_than_t<L, T> operator>(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::greater_or_equal_t<T, R> operator>=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::greater_or_equal_t<L, T> operator>=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::is_equal_t<T, R> operator==(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::is_equal_t<L, T> operator==(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::is_not_equal_t<T, R> operator!=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::is_not_equal_t<L, T> operator!=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    namespace internal {
        
        template<class ...Args>
        struct join_iterator {
            
            template<class L>
            void operator()(L) {
                //..
            }
        };
        
        template<>
        struct join_iterator<> {
            
            template<class L>
            void operator()(L) {
                //..
            }
        };
        
        template<class H, class ...Tail>
        struct join_iterator<H, Tail...> : public join_iterator<Tail...>{
            H h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                this->super::operator()(l);
            }
            
        };
        
        template<class T, class ...Tail>
        struct join_iterator<conditions::cross_join_t<T>, Tail...> : public join_iterator<Tail...>{
            conditions::cross_join_t<T> h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                l(h);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::left_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            conditions::left_join_t<T, O> h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                l(h);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            conditions::join_t<T, O> h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                l(h);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::left_outer_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            conditions::left_outer_join_t<T, O> h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                l(h);
                this->super::operator()(l);
            }
        };
        
        template<class T, class O, class ...Tail>
        struct join_iterator<conditions::inner_join_t<T, O>, Tail...> : public join_iterator<Tail...> {
            conditions::inner_join_t<T, O> h;
            
            typedef join_iterator<Tail...> super;
            
            template<class L>
            void operator()(L l) {
                l(h);
                this->super::operator()(l);
            }
        };
    }
    
    template<class F, class O>
    conditions::using_t<F, O> using_(F O::*p) {
        return {p};
    }
    
    template<class T>
    conditions::on_t<T> on(T t) {
        return {t};
    }
    
    template<class T>
    conditions::cross_join_t<T> cross_join() {
        return {};
    }
    
    template<class T, class O>
    conditions::left_join_t<T, O> left_join(O o) {
        return {o};
    }
    
    template<class T, class O>
    conditions::join_t<T, O> join(O o) {
        return {o};
    }
    
    /*template<class T>
    conditions::natural_join_t<T> natural_join() {
        return {};
    }*/
    
    template<class T, class O>
    conditions::left_outer_join_t<T, O> left_outer_join(O o) {
        return {o};
    }
    
    template<class T, class O>
    conditions::inner_join_t<T, O> inner_join(O o) {
        return {o};
    }
    
    inline conditions::offset_t offset(int off) {
        return {off};
    }
    
    inline conditions::limit_t limit(int lim) {
        return {lim};
    }
    
    inline conditions::limit_t limit(int off, int lim) {
        return {lim, true, true, off};
    }
    
    inline conditions::limit_t limit(int lim, conditions::offset_t offt) {
        return {lim, true, false, offt.off };
    }
    
    template<
        class L,
        class R,
        typename = typename std::enable_if<std::is_base_of<conditions::condition_t, L>::value && std::is_base_of<conditions::condition_t, R>::value>::type
    >
    conditions::and_condition_t<L, R> operator &&(const L &l, const R &r) {
        return {l, r};
    }
    
    template<
        class L,
        class R,
        typename = typename std::enable_if<std::is_base_of<conditions::condition_t, L>::value && std::is_base_of<conditions::condition_t, R>::value>::type
    >
    conditions::or_condition_t<L, R> operator ||(const L &l, const R &r) {
        return {l, r};
    }
    
    template<class T>
    conditions::is_not_null_t<T> is_not_null(T t) {
        return {t};
    }
    
    template<class T>
    conditions::is_null_t<T> is_null(T t) {
        return {t};
    }
    
    template<class L, class E>
    conditions::in_t<L, E> in(L l, std::vector<E> values) {
        return {std::move(l), std::move(values)};
    }
    
    template<class L, class E>
    conditions::in_t<L, E> in(L l, std::initializer_list<E> values) {
        return {std::move(l), std::move(values)};
    }
    
    template<class L, class R>
    conditions::is_equal_t<L, R> is_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_equal_t<L, R> eq(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_not_equal_t<L, R> is_not_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_not_equal_t<L, R> ne(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_than_t<L, R> greater_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_than_t<L, R> gt(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_or_equal_t<L, R> greater_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_or_equal_t<L, R> ge(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_than_t<L, R> lesser_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_than_t<L, R> lt(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_or_equal_t<L, R> lesser_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_or_equal_t<L, R> le(L l, R r) {
        return {l, r};
    }
    
    template<class C>
    conditions::where_t<C> where(C c) {
        return {c};
    }
    
    template<class O>
    conditions::order_by_t<O> order_by(O o) {
        return {o};
    }
    
    template<class ...Args>
    conditions::group_by_t<Args...> group_by(Args ...args) {
        return {std::make_tuple(args...)};
    }
    
    template<class A, class T>
    conditions::between_t<A, T> between(A expr, T b1, T b2) {
        return {expr, b1, b2};
    }
    
    template<class A, class T>
    conditions::like_t<A, T> like(A a, T t) {
        return {a, t};
    }

    template<class T, class R>
    conditions::like_t<T, R> like(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }


    
    namespace core_functions {
        
        //  base class for operator overloading
        struct core_function_t {};
        
        /**
         *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
         */
        template<class T>
        struct length_t : public core_function_t {
            T t;
            
            length_t(T t_):t(t_){}
            
            operator std::string() const {
                return "LENGTH";
            }
        };
        
        /**
         *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
         */
        template<class T>
        struct abs_t : public core_function_t {
            T t;
            
            abs_t(T t_):t(t_){}
            
            operator std::string() const {
                return "ABS";
            }
        };
        
        /**
         *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
         */
        template<class T>
        struct lower_t : public core_function_t {
            T t;
            
            lower_t(T t_):t(t_){}
            
            operator std::string() const {
                return "LOWER";
            }
        };
        
        /**
         *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
         */
        template<class T>
        struct upper_t : public core_function_t {
            T t;
            
            upper_t(T t_):t(t_){}
            
            operator std::string() const {
                return "UPPER";
            }
        };
        
        /**
         *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
         */
        struct changes_t : public core_function_t {
            
            operator std::string() const {
                return "CHANGES";
            }
        };
        
        /**
         *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class X>
        struct trim_single_t : public core_function_t {
            X x;
            
            trim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "TRIM";
            }
        };
        
        /**
         *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class X, class Y>
        struct trim_double_t : public core_function_t {
            X x;
            Y y;
            
            trim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(trim_single_t<X>(0));
            }
        };
        
        /**
         *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X>
        struct ltrim_single_t : public core_function_t {
            X x;
            
            ltrim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "LTRIM";
            }
        };
        
        /**
         *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X, class Y>
        struct ltrim_double_t : public core_function_t {
            X x;
            Y y;
            
            ltrim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(ltrim_single_t<X>(0));
            }
        };
        
        /**
         *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X>
        struct rtrim_single_t : public core_function_t {
            X x;
            
            rtrim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "RTRIM";
            }
        };
        
        /**
         *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X, class Y>
        struct rtrim_double_t : public core_function_t {
            X x;
            Y y;
            
            rtrim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(rtrim_single_t<X>(0));
            }
        };
        
        
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        /**
         *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
         */
        template<class ...Args>
        struct char_t_ : public core_function_t {
            typedef std::tuple<Args...> args_type;
            args_type args;
            
            char_t_(args_type args_):args(args_){}
            
            operator std::string() const {
                return "CHAR";
            }
        };
        
        struct random_t : public core_function_t {
            
            operator std::string() const {
                return "RANDOM";
            }
        };
        
#endif
        template<class T, class ...Args>
        struct date_t : public core_function_t {
            typedef std::tuple<Args...> modifiers_type;
            T timestring;
            modifiers_type modifiers;
            
            date_t(T timestring_, modifiers_type modifiers_):timestring(timestring_),modifiers(modifiers_){}
            
            operator std::string() const {
                return "DATE";
            }
        };
        
        template<class T, class ...Args>
        struct datetime_t : public core_function_t {
            typedef std::tuple<Args...> modifiers_type;
            
            T timestring;
            modifiers_type modifiers;
            
            datetime_t(T timestring_, modifiers_type modifiers_):timestring(timestring_), modifiers(modifiers_){}
            
            operator std::string() const {
                return "DATETIME";
            }
        };
    }
    
    //  cute operators for core functions
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::lesser_than_t<F, R> operator<(F f, R r) {
        return {f, r};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::lesser_or_equal_t<F, R> operator<=(F f, R r) {
        return {f, r};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::greater_than_t<F, R> operator>(F f, R r) {
        return {f, r};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::greater_or_equal_t<F, R> operator>=(F f, R r) {
        return {f, r};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::is_equal_t<F, R> operator==(F f, R r) {
        return {f, r};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<std::is_base_of<core_functions::core_function_t, F>::value>::type>
    conditions::is_not_equal_t<F, R> operator!=(F f, R r) {
        return {f, r};
    }
    
    inline core_functions::random_t random() {
        return {};
    }
    
    template<class T, class ...Args, class Res = core_functions::date_t<T, Args...>>
    Res date(T timestring, Args ...modifiers) {
        return Res(timestring, std::make_tuple(modifiers...));
    }
    
    template<class T, class ...Args, class Res = core_functions::datetime_t<T, Args...>>
    Res datetime(T timestring, Args ...modifiers) {
        return Res(timestring, std::make_tuple(modifiers...));
    }
    
#if SQLITE_VERSION_NUMBER >= 3007016
    
    template<class ...Args>
    core_functions::char_t_<Args...> char_(Args ...args) {
        typedef core_functions::char_t_<Args...> result_type;
        return result_type(std::make_tuple(std::forward<Args>(args)...));
    }
    
#endif
    
    template<class X, class Res = core_functions::trim_single_t<X>>
    Res trim(X x) {
        return Res(x);
    }
    
    template<class X, class Y, class Res = core_functions::trim_double_t<X, Y>>
    Res trim(X x, Y y) {
        return Res(x, y);
    }
    
    template<class X, class Res = core_functions::ltrim_single_t<X>>
    Res ltrim(X x) {
        return Res(x);
    }
    
    template<class X, class Y, class Res = core_functions::ltrim_double_t<X, Y>>
    Res ltrim(X x, Y y) {
        return Res(x, y);
    }
    
    template<class X, class Res = core_functions::rtrim_single_t<X>>
    Res rtrim(X x) {
        return Res(x);
    }
    
    template<class X, class Y, class Res = core_functions::rtrim_double_t<X, Y>>
    Res rtrim(X x, Y y) {
        return Res(x, y);
    }
    
    inline core_functions::changes_t changes() {
        return {};
    }
    
    template<class T>
    core_functions::length_t<T> length(T t) {
        typedef core_functions::length_t<T> result_type;
        return result_type(t);
    }
    
    template<class T>
    core_functions::abs_t<T> abs(T t) {
        typedef core_functions::abs_t<T> result_type;
        return result_type(t);
    }
    
    template<class T, class Res = core_functions::lower_t<T>>
    Res lower(T t) {
        return Res(t);
    }
    
    template<class T, class Res = core_functions::upper_t<T>>
    Res upper(T t) {
        return Res(t);
    }
    
    namespace aggregate_functions {
        
        template<class T>
        struct avg_t {
            T t;
            
            operator std::string() const {
                return "AVG";
            }
        };
        
        template<class T>
        struct count_t {
            T t;
            
            operator std::string() const {
                return "COUNT";
            }
        };
        
        struct count_asterisk_t {
            
            operator std::string() const {
                return "COUNT";
            }
            
        };
        
        template<class T>
        struct sum_t {
            T t;
            
            operator std::string() const {
                return "SUM";
            }
        };
        
        template<class T>
        struct total_t {
            T t;
            
            operator std::string() const {
                return "TOTAL";
            }
        };
        
        template<class T>
        struct max_t {
            T t;
            
            operator std::string() const {
                return "MAX";
            }
        };
        
        template<class T>
        struct min_t {
            T t;
            
            operator std::string() const {
                return "MIN";
            }
        };
        
        template<class T>
        struct group_concat_single_t {
            T t;
            
            operator std::string() const {
                return "GROUP_CONCAT";
            }
        };
        
        template<class T>
        struct group_concat_double_t {
            T t;
            std::string y;
            
            operator std::string() const {
                return "GROUP_CONCAT";
            }
        };
        
    }
    
    template<class T>
    aggregate_functions::avg_t<T> avg(T t) {
        return {t};
    }
    
    template<class T>
    aggregate_functions::count_t<T> count(T t) {
        return {t};
    }
    
    inline aggregate_functions::count_asterisk_t count() {
        return {};
    }
    
    template<class T>
    aggregate_functions::sum_t<T> sum(T t) {
        return {t};
    }
    
    template<class T>
    aggregate_functions::max_t<T> max(T t) {
        return {t};
    }
    
    template<class T>
    aggregate_functions::min_t<T> min(T t) {
        return {t};
    }
    
    template<class T>
    aggregate_functions::total_t<T> total(T t) {
        return {t};
    }
    
    template<class T>
    aggregate_functions::group_concat_single_t<T> group_concat(T t) {
        return {t};
    }
    
    template<class T, class Y>
    aggregate_functions::group_concat_double_t<T> group_concat(T t, Y y) {
        return {t, y};
    }
    
    namespace internal {
        
        template<class T>
        struct distinct_t {
            T t;
            
            operator std::string() const {
                return "DISTINCT";
            }
        };
        
        template<class ...Args>
        struct columns_t {
            bool distinct = false;
            
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
            T m;
            
            columns_t(decltype(m) m_, Args ...args): Super(args...), m(m_) {}
            
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
        struct set_t {
            
            operator std::string() const {
                return "SET";
            }
            
            template<class F>
            void for_each(F) {
                //..
            }
        };
        
        template<class L, class R, class ...Args>
        struct set_t<L, R, Args...> : public set_t<Args...> {
            L l;
            R r;
            
            typedef set_t<Args...> Super;
            
            set_t(L l_, R r_, Args ...args) : Super(std::forward<Args>(args)...), l(std::move(l_)), r(std::move(r_)) {}
            
            template<class F>
            void for_each(F f) {
                f(l, r);
                Super::for_each(f);
            }
        };
        
        struct database_connection {
            
            database_connection(const std::string &filename) {
                auto rc = sqlite3_open(filename.c_str(), &this->db);
                if(rc != SQLITE_OK){
                    auto msg = sqlite3_errmsg(this->db);
                    throw std::runtime_error(msg);
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
        
        /**
         *  Trait class used to define table mapped type by setter/getter/member
         */
        template<class T>
        struct table_type;
        
        template<class O, class F>
        struct table_type<F O::*> {
            typedef O type;
        };
        
        template<class O, class F>
        struct table_type<const F& (O::*)() const> {
            typedef O type;
        };
        
        template<class O, class F>
        struct table_type<void (O::*)(F)> {
            typedef O type;
        };
    }
    
    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {t};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> distinct(internal::columns_t<Args...> cols) {
        cols.distinct = true;
        return cols;
    }
    
    template<class ...Args>
    internal::set_t<Args...> set(Args ...args) {
        return {args...};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> columns(Args ...args) {
        return {args...};
    }
    
    struct table_info {
        int cid;
        std::string name;
        std::string type;
        bool notnull;
        std::string dflt_value;
        int pk;
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
        void for_each_column(L) {}
        
        template<class L>
        void for_each_column_with_constraints(L) {}
        
        template<class F, class L>
        void for_each_column_with_field_type(L) {}
        
        template<class Op, class L>
        void for_each_column_exept(L){}
        
        template<class Op, class L>
        void for_each_column_with(L) {}
        
        template<class L>
        void for_each_primary_key(L) {}
        
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
         *  column_names_with implementation. Notice that result will be reversed.
         *  It is reversed back in `table` class.
         *  @return vector of column names that have specified Op... conditions.
         */
        template<class ...Op>
        std::vector<std::string> column_names_with() {
            auto res = Super::template column_names_with<Op...>();
            if(this->col.template has_every<Op...>()) {
                res.emplace_back(this->col.name);
            }
            return res;
        }
        
        /**
         *  For each implementation. Calls templated lambda with its column
         *  and passed call to superclass.
         */
        template<class L>
        void for_each_column(L l){
            this->apply_to_col_if(l, internal::is_column<column_type>{});
            Super::for_each_column(l);
        }
        
        /**
         *  For each implementation. Calls templated lambda with its column
         *  and passed call to superclass.
         */
        template<class L>
        void for_each_column_with_constraints(L l){
            l(this->col);
//            this->apply_to_col_if(l, internal::is_column<column_type>{});
            Super::for_each_column_with_constraints(l);
        }

        template<class F, class L>
        void for_each_column_with_field_type(L l) {
            this->apply_to_col_if(l, std::is_same<F, typename column_type::field_type>{});
            Super::template for_each_column_with_field_type<F, L>(l);
        }
        
        /**
         *  Working version of `for_each_column_exept`. Calls lambda if column has no option and fire super's function.
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            using has_opt = tuple_helper::tuple_contains_type<Op, typename column_type::constraints_type>;
            this->apply_to_col_if(l, std::integral_constant<bool, !has_opt::value>{});
            Super::template for_each_column_exept<Op, L>(l);
        }

        /**
         *  Working version of `for_each_column_with`. Calls lambda if column has option and fire super's function.
         */
        template<class Op, class L>
        void for_each_column_with(L l) {
            this->apply_to_col_if(l, tuple_helper::tuple_contains_type<Op, typename column_type::constraints_type>{});
            Super::template for_each_column_with<Op, L>(l);
        }
        
        /**
         *  Calls l(this->col) if H is primary_key_t
         */
        template<class L>
        void for_each_primary_key(L l) {
            this->apply_to_col_if(l, internal::is_primary_key<H>{});
            Super::for_each_primary_key(l);
        }

    protected:
        
        template<class L>
        void apply_to_col_if(L& l, std::true_type) {
            l(this->col);
        }
        
        template<class L>
        void apply_to_col_if(L&, std::false_type) {}
        
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
        
        table_t(decltype(name) name_, decltype(impl) impl_):name(std::move(name_)), impl(std::move(impl_)){}
        
        bool _without_rowid = false;
        
        table_t<Cs...> without_rowid() const {
            auto res = *this;
            res._without_rowid = true;
            return res;
        }
        
        /**
         *  @return vector of column names of table.
         */
        std::vector<std::string> column_names() {
            std::vector<std::string> res;
            this->impl.for_each_column([&res](auto &c){
                res.push_back(c.name);
            });
            return res;
        }
        
        std::vector<std::string> composite_key_columns_names() {
            std::vector<std::string> res;
            this->impl.for_each_primary_key([this, &res](auto c){
                res = this->composite_key_columns_names(c);
            });
            return res;
        }
        
        std::vector<std::string> primary_key_column_names() {
            std::vector<std::string> res;
            this->impl.template for_each_column_with<constraints::primary_key_t<>>([&res](auto &c){
                res.push_back(c.name);
            });
            if(!res.size()){
                res = this->composite_key_columns_names();
            }
            return res;
        }
        
        template<class ...Args>
        std::vector<std::string> composite_key_columns_names(constraints::primary_key_t<Args...> pk) {
            std::vector<std::string> res;
            typedef decltype(pk.columns) pk_columns_tuple;
            res.reserve(std::tuple_size<pk_columns_tuple>::value);
            tuple_helper::iterator<std::tuple_size<pk_columns_tuple>::value - 1, Args...>()(pk.columns, [this, &res](auto &v){
                res.push_back(this->find_column_name(v));
            });
            return res;
        }
        
        int columns_count() const {
            return this->impl.columns_count();
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
        
        template<class F, class O>
        std::string find_column_name(const F& (O::*getter)() const) {
            std::string res;
            this->template for_each_column_with_field_type<F>([&](auto c) {
                if(c.getter == getter) {
                    res = c.name;
                }
            });
            return res;
        }
        
        template<class F, class O>
        std::string find_column_name(void (O::*setter)(F)) {
            std::string res;
            this->template for_each_column_with_field_type<F>([&](auto c) {
                if(c.setter == setter) {
                    res = c.name;
                }
            });
            return res;
        }
        
        /**
         *  @return vector of column names that have constraints provided as template arguments (not_null, autoincrement).
         */
        template<class ...Op>
        std::vector<std::string> column_names_with() {
            auto res = this->impl.template column_names_with<Op...>();
            std::reverse(res.begin(),
                         res.end());
            return res;
        }
        
        /**
         *  Iterates all columns and fires passed lambda. Lambda must have one and only templated argument Otherwise code will
         *  not compile. Excludes table constraints (e.g. foreign_key_t) at the end of the columns list. To iterate columns with
         *  table constraints use for_each_column_with_constraints instead.
         *  L is lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class L>
        void for_each_column(L l) {
            this->impl.for_each_column(l);
        }
        
        template<class L>
        void for_each_column_with_constraints(L l) {
            this->impl.for_each_column_with_constraints(l);
        }
        
        template<class F, class L>
        void for_each_column_with_field_type(L l) {
            this->impl.template for_each_column_with_field_type<F, L>(l);
        }
        
        /**
         *  Iterates all columns exept ones that have specified constraints and fires passed lambda.
         *  Lambda must have one and only templated argument Otherwise code will not compile.
         *  L is lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class Op, class L>
        void for_each_column_exept(L l) {
            this->impl.template for_each_column_exept<Op>(l);
        }
        
        /**
         *  Iterates all columns that have specified constraints and fires passed lambda.
         *  Lambda must have one and only templated argument Otherwise code will not compile.
         *  L is lambda type. Do not specify it explicitly.
         *  @param l Lambda to be called per column itself. Must have signature like this [] (auto col) -> void {}
         */
        template<class Op, class L>
        void for_each_column_with(L l) {
            this->impl.template for_each_column_with<Op>(l);
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
                    col.not_null(),
                    dft,
                    col.template has<constraints::primary_key_t<>>(),
                };
                res.emplace_back(i);
            });
            std::vector<std::string> compositeKeyColumnNames;
            this->impl.for_each_primary_key([this, &compositeKeyColumnNames](auto c){
                compositeKeyColumnNames = this->composite_key_columns_names(c);
            });
            for(size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                auto &columnName = compositeKeyColumnNames[i];
                auto it = std::find_if(res.begin(),
                                       res.end(),
                                       [&columnName](const table_info &ti) {
                                           return ti.name == columnName;
                                       });
                if(it != res.end()){
                    it->pk = static_cast<int>(i + 1);
                }
            }
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
        
        statement_finalizer(decltype(stmt) stmt_):stmt(stmt_){}
        
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
            return sqlite3_bind_int(stmt, index, value);
        }
    };
    
    /**
     *  Specialization for bool.
     */
    template<>
    struct statement_binder<bool> {
        
        int bind(sqlite3_stmt *stmt, int index, const bool &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };
    
    /**
     *  Specialization for short.
     */
    template<>
    struct statement_binder<short> {
        
        int bind(sqlite3_stmt *stmt, int index, const short &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };

    /**
     *  Specialization for unsigned char.
     */
    template<>
    struct statement_binder<unsigned char> {

        int bind(sqlite3_stmt *stmt, int index, const unsigned char &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };

    /**
     *  Specialization for unsigned short int.
     */
    template<>
    struct statement_binder<unsigned short int> {

        int bind(sqlite3_stmt *stmt, int index, const unsigned short int &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };
    
    /**
     *  Specialization for unsigned int.
     */
    template<>
    struct statement_binder<unsigned int> {

        int bind(sqlite3_stmt *stmt, int index, const unsigned int &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };

    /**
     *  Specialization for signed char.
     */
    template<>
    struct statement_binder<signed char> {

        int bind(sqlite3_stmt *stmt, int index, const signed char &value) {
            return sqlite3_bind_int(stmt, index, value);
        }
    };

    /**
     *  Specialization for long.
     */
    template<>
    struct statement_binder<long> {
        
        int bind(sqlite3_stmt *stmt, int index, const long &value) {
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
        }
    };
    
    /**
     *  Specialization for long long.
     */
    template<>
    struct statement_binder<long long> {
        
        int bind(sqlite3_stmt *stmt, int index, const long long &value) {
            return sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
        }
    };
    
    /**
     *  Specialization for double.
     */
    template<>
    struct statement_binder<double> {
        
        int bind(sqlite3_stmt *stmt, int index, const double &value) {
            return sqlite3_bind_double(stmt, index, value);
        }
    };
    
    /**
     *  Specialization for std::string.
     */
    template<>
    struct statement_binder<std::string> {
        
        int bind(sqlite3_stmt *stmt, int index, const std::string &value) {
            return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
        }
    };
    
    /**
     *  Specialization for C-string
     */
    template<>
    struct statement_binder<const char*> {
        
        int bind(sqlite3_stmt *stmt, int index, const char *value) {
            return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
        }
    };
    
    /**
     *  Specialization for std::nullptr_t.
     */
    template<>
    struct statement_binder<std::nullptr_t> {
        
        int bind(sqlite3_stmt *stmt, int index, const std::nullptr_t &) {
            return sqlite3_bind_null(stmt, index);
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
     *  Specialization for optional type (std::unique_ptr).
     */
    template<class T>
    struct statement_binder<std::unique_ptr<T>>{
        
        int bind(sqlite3_stmt *stmt, int index, const std::shared_ptr<T> &value) {
            if(value){
                return statement_binder<T>().bind(stmt, index, *value);
            }else{
                return statement_binder<std::nullptr_t>().bind(stmt, index, nullptr);
            }
        }
    };
    
    /**
     *  Specialization for optional type (std::vector<char>).
     */
    template<>
    struct statement_binder<std::vector<char>>{
        
        int bind(sqlite3_stmt *stmt, int index, const std::vector<char> &value) {
            return sqlite3_bind_blob(stmt, index, (const void *)&value.front(), int(value.size()), SQLITE_TRANSIENT);
        }
    };
    
    /**
     *  Helper class used to cast values from argv to V class 
     *  which depends from column type.
     *
     */
    template<class V>
    struct row_extrator {
        
        //  used in sqlite3_exec (select)
        V extract(const char *row_value);
        
        //  used in sqlite_column (iteration, get_all)
        V extract(sqlite3_stmt *stmt, int columnIndex);
    };
    
    /**
     *  Specialization for int.
     */
    template<>
    struct row_extrator<int> {
        int extract(const char *row_value) {
            return atoi(row_value);
        }
        
        int extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for unsigned char.
     */
    template<>
    struct row_extrator<unsigned char> {
        unsigned char extract(const char *row_value) {
            return atoi(row_value);
        }
        
        unsigned char extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for unsigned short int.
     */
    template<>
    struct row_extrator<unsigned short int> {
        unsigned short int extract(const char *row_value) {
            return atoi(row_value);
        }
        
        unsigned short int extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for unsigned int.
     */
    template<>
    struct row_extrator<unsigned int> {
        unsigned int extract(const char *row_value) {
            return atoi(row_value);
        }
        
        unsigned int extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for signed char.
     */
    template<>
    struct row_extrator<signed char> {
        signed char extract(const char *row_value) {
            return atoi(row_value);
        }
        
        signed char extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };

    /**
     *  Specialization for bool.
     */
    template<>
    struct row_extrator<bool> {
        bool extract(const char *row_value) {
            return atoi(row_value);
        }
        
        bool extract(sqlite3_stmt *stmt, int columnIndex) {
            return sqlite3_column_int(stmt, columnIndex);
        }
    };
    
    /**
     *  Specialization for short.
     */
    template<>
    struct row_extrator<short> {
        short extract(const char *row_value) {
            return atoi(row_value);
        }
        
        short extract(sqlite3_stmt *stmt, int columnIndex) {
            return static_cast<short>(sqlite3_column_int(stmt, columnIndex));
        }
    };
    
    /**
     *  Specialization for long.
     */
    template<>
    struct row_extrator<long> {
        long extract(const char *row_value) {
            return atol(row_value);
        }
        
        long extract(sqlite3_stmt *stmt, int columnIndex) {
            return static_cast<long>(sqlite3_column_int64(stmt, columnIndex));
        }
    };
    
    /**
     *  Specialization for long long.
     */
    template<>
    struct row_extrator<long long> {
        long long extract(const char *row_value) {
            return atoll(row_value);
        }
        
        long long extract(sqlite3_stmt *stmt, int columnIndex) {
            return static_cast<long long>(sqlite3_column_int64(stmt, columnIndex));
        }
    };
    
    /**
     *  Specialization for std::string.
     */
    template<>
    struct row_extrator<std::string> {
        std::string extract(const char *row_value) {
            if(row_value){
                return row_value;
            }else{
                return {};
            }
        }
        
        std::string extract(sqlite3_stmt *stmt, int columnIndex) {
            auto cStr = (const char*)sqlite3_column_text(stmt, columnIndex);
            if(cStr){
                return cStr;
            }else{
                return {};
            }
        }
    };
    
    /**
     *  Specialization for std::vector<char>.
     */
    template<>
    struct row_extrator<std::vector<char>> {
        std::vector<char> extract(const char *row_value) {
            if(row_value){
                auto len = ::strlen(row_value);
                return this->go(row_value, static_cast<int>(len));
            }else{
                return {};
            }
        }
        
        std::vector<char> extract(sqlite3_stmt *stmt, int columnIndex) {
            auto bytes = static_cast<const char *>(sqlite3_column_blob(stmt, columnIndex));
            auto len = sqlite3_column_bytes(stmt, columnIndex);
            return this->go(bytes, len);
        }
        
    protected:
        
        std::vector<char> go(const char *bytes, int len) {
            if(len){
                std::vector<char> res;
                res.reserve(len);
                std::copy(bytes,
                          bytes + len,
                          std::back_inserter(res));
                return res;
            }else{
                return {};
            }
        }
    };
    
    /**
     *  Specialization for double.
     */
    template<>
    struct row_extrator<double> {
        double extract(const char *row_value) {
            return atof(row_value);
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
        
        std::tuple<Args...> extract(sqlite3_stmt *stmt, int columnIndex) {
            (void)columnIndex;
            std::tuple<Args...> res;
            this->extract<std::tuple_size<decltype(res)>::value>(res, stmt);
            return res;
        }
        
    protected:
        
        template<size_t I, typename std::enable_if<I != 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, sqlite3_stmt *stmt) {
            typedef typename std::tuple_element<I - 1, typename std::tuple<Args...>>::type tuple_type;
            std::get<I - 1>(t) = row_extrator<tuple_type>().extract(stmt, I - 1);
            this->extract<I - 1>(t, stmt);
        }
        
        template<size_t I, typename std::enable_if<I == 0>::type * = nullptr>
        void extract(std::tuple<Args...> &, sqlite3_stmt *) {
            //..
        }
        
        template<size_t I, typename std::enable_if<I != 0>::type * = nullptr>
        void extract(std::tuple<Args...> &t, char **argv) {
            typedef typename std::tuple_element<I - 1, typename std::tuple<Args...>>::type tuple_type;
            std::get<I - 1>(t) = row_extrator<tuple_type>().extract(argv[I - 1]);
            this->extract<I - 1>(t, argv);
        }
        
        template<size_t I, typename std::enable_if<I == 0>::type * = nullptr>
        void extract(std::tuple<Args...> &, char **) {
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
    
    
    enum class sync_schema_result {
        /**
         *  created new table, table with the same tablename did not exist
         */
        new_table_created,
        
        /**
         *  table schema is the same as storage, nothing to be done
         */
        already_in_sync,
        
        /**
         *  removed excess columns in table (than storage) without dropping a table
         */
        old_columns_removed,
        
        /**
         *  lacking columns in table (than storage) added without dropping a table
         */
        new_columns_added,
        
        /**
         *  both old_columns_removed and new_columns_added
         */
        new_columns_added_and_old_columns_removed,
        
        /**
         *  old table is dropped and new is recreated. Reasons :
         *      1. delete excess columns in the table than storage if preseve = false 
         *      2. Lacking columns in the table cannot be added due to NULL and DEFAULT constraint 
         *      3. Reasons 1 and 2 both together
         *      4. data_type mismatch between table and storage.
         */
        dropped_and_recreated,
    };


    inline std::ostream& operator<<(std::ostream &os, sync_schema_result value) {
        switch(value){
            case sync_schema_result::new_table_created: return os << "new table created";
            case sync_schema_result::already_in_sync: return os << "table and storage is already in sync.";
            case sync_schema_result::old_columns_removed: return os << "old excess columns removed";
            case sync_schema_result::new_columns_added: return os << "new columns added";
            case sync_schema_result::new_columns_added_and_old_columns_removed: return os << "old excess columns removed and new columns added";
            case sync_schema_result::dropped_and_recreated: return os << "old table dropped and recreated";
        }
    }
    
    namespace internal {
        
        template<class ...Cols>
        struct index_t {
            typedef std::tuple<Cols...> columns_type;
            typedef void object_type;
            
            std::string name;
            bool unique;
            columns_type columns;
            
            template<class L>
            void for_each_column_with_constraints(L) {}
        };
    }
    
    template<class ...Cols>
    internal::index_t<Cols...> make_index(const std::string &name, Cols ...cols) {
        return {name, false, std::make_tuple(cols...)};
    }
    
    template<class ...Cols>
    internal::index_t<Cols...> make_unique_index(const std::string &name, Cols ...cols) {
        return {name, true, std::make_tuple(cols...)};
    }

    /**
     *  This is a generic implementation. Used as a tail in storage_impl inheritance chain
     */
    template<class ...Ts>
    struct storage_impl {
        
        template<class L>
        void for_each(L) {}
        
        /*template<class T>
        constexpr bool type_is_mapped() const {
            return std::integral_constant<bool, false>::value;
        }*/
        
        int foreign_keys_count() {
            return 0;
        }
        
        template<class O>
        std::string dump(const O &, sqlite3 *, std::nullptr_t) {
            throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
        }
        
        bool table_exists(const std::string &tableName, sqlite3 *db) {
            bool res = false;
            std::stringstream ss;
            ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '" << "table" << "' AND name = '" << tableName << "'";
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
                                       auto &res = *(bool*)data;
                                       if(argc){
                                           res = !!std::atoi(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
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
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
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
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
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
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        /*void drop_table(const std::string &tableName, sqlite3 *db) {
            
        }*/
        
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
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        std::string current_timestamp(sqlite3 *db) {
            std::string res;
            std::stringstream ss;
            ss << "SELECT CURRENT_TIMESTAMP";
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv, char **) -> int {
                                       auto &res = *(std::string*)data;
                                       if(argc){
                                           if(argv[0]){
                                               res = row_extrator<std::string>().extract(argv[0]);
                                           }
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
    };
    
    template<class H, class ...Ts>
    struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
        typedef H table_type;
        
        storage_impl(H h, Ts ...ts) : Super(ts...), table(h) {}
        
        table_type table;
        
        template<class L>
        void for_each(L l) {
            Super::for_each(l);
            l(this);
        }
        
#if SQLITE_VERSION_NUMBER >= 3006019
        
        //  returns foreign keys count in table definition
        int foreign_keys_count(){
            auto res = 0;
            this->table.for_each_column_with_constraints([&res](auto c){
                if(internal::is_foreign_key<decltype(c)>::value) {
                    ++res;
                }
            });
            return res;
        }
        
#endif
        
        /*template<class T, class HH = typename H::object_type>
        constexpr bool type_is_mapped(typename std::enable_if<std::is_same<T, HH>::value>::type* = nullptr) const {
            return std::integral_constant<bool, true>::value;
        }*/
        
        /*template<class T, class HH = typename H::object_type>
        constexpr bool type_is_mapped(typename std::enable_if<!std::is_same<T, HH>::value>::type* = nullptr) const {
            return Super::template type_is_mapped<T>();
        }*/
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(F O::*m, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return this->table.find_column_name(m);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(F O::*m, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::column_name(m);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(const F& (O::*g)() const, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return this->table.find_column_name(g);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(const F& (O::*g)() const, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::column_name(g);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(void (O::*s)(F), typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            return this->table.find_column_name(s);
        }
        
        template<class O, class F, class HH = typename H::object_type>
        std::string column_name(void (O::*s)(F), typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::column_name(s);
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
        
        template<class O, class HH = typename H::object_type>
        std::string dump(const O &o, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
            return Super::dump(o, db, nullptr);
        }
        
        template<class O, class HH = typename H::object_type>
        std::string dump(const O &o, sqlite3 *, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
            std::stringstream ss;
            ss << "{ ";
//            auto columnsCount = this->table.columns_count();
//            auto index = 0;
            std::vector<std::pair<std::string, std::string>> pairs;
            this->table.for_each_column([&pairs, &o] (auto c) {
                typedef typename decltype(c)::field_type field_type;
                const field_type *value = nullptr;
                if(c.member_pointer){
                    value = &(o.*c.member_pointer);
                }else{
                    value = &((o).*(c.getter))();
                }
                
                pairs.push_back(std::make_pair(c.name, field_printer<field_type>()(*value)));
                /*ss << c.name << " : '" << field_printer<field_type>()(*value) << "'";
                if(index < columnsCount - 1) {
                    ss << ", ";
                }else{
                    ss << " }";
                }
                ++index;*/
            });
            for(size_t i = 0; i < pairs.size(); ++i) {
                auto &p = pairs[i];
                ss << p.first << " : '" << p.second << "'";
                if(i < pairs.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " }";
                }
            }
            return ss.str();
        }
        
        std::vector<table_info> get_table_info(const std::string &tableName, sqlite3 *db) {
            std::vector<table_info> res;
            auto query = "PRAGMA table_info('" + tableName + "')";
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **) -> int {
                                       auto &res = *(std::vector<table_info>*)data;
                                       if(argc){
                                           auto index = 0;
                                           auto cid = std::atoi(argv[index++]);
                                           std::string name = argv[index++];
                                           std::string type = argv[index++];
                                           bool notnull = !!std::atoi(argv[index++]);
                                           std::string dflt_value = argv[index] ? argv[index] : "";
                                           index++;
                                           auto pk = std::atoi(argv[index++]);
                                           res.push_back(table_info{cid, name, type, notnull, dflt_value, pk});
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
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
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else{
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
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
                columnNames.emplace_back(c.name);
            });
            auto columnNamesCount = columnNames.size();
            ss << "INSERT INTO " << name << " (";
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << columnNames[i];
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << ") ";
            ss << "SELECT ";
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << columnNames[i];
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << " FROM '" << this->table.name << "' ";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else{
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }

        sync_schema_result schema_status(sqlite3 *db, bool preserve) {
            
            auto res = sync_schema_result::already_in_sync;
            
            //  first let's see if table with such name exists..
            auto gottaCreateTable = !this->table_exists(this->table.name, db);
            if(!gottaCreateTable){
                
                //  get table info provided in `make_table` call..
                auto storageTableInfo = this->table.get_table_info();
                
                //  now get current table info from db using `PRAGMA table_info` query..
                auto dbTableInfo = get_table_info(this->table.name, db);
                
                //  this vector will contain pointers to columns that gotta be added..
                std::vector<table_info*> columnsToAdd;
                
                if(get_remove_add_columns(columnsToAdd,
                                          storageTableInfo,
                                          dbTableInfo)) {
                    gottaCreateTable = true;
                }

                if(!gottaCreateTable){  //  if all storage columns are equal to actual db columns but there are excess columns at the db..
                    if(dbTableInfo.size() > 0){
                        //extra table columns than storage columns
                        if(!preserve){
                            gottaCreateTable = true;
                        }else{                           
                            res = decltype(res)::old_columns_removed;
                        }
                    }
                }
                if(gottaCreateTable){
                    res = decltype(res)::dropped_and_recreated;
                }else{
                    if(columnsToAdd.size()){
                        //extra storage columns than table columns
                        for(auto columnPointer : columnsToAdd) {
                            if(columnPointer->notnull && columnPointer->dflt_value.empty()){
                                gottaCreateTable = true;
                                break;
                            }
                        }
                        if(!gottaCreateTable){

                            if(res == decltype(res)::old_columns_removed) res = decltype(res)::new_columns_added_and_old_columns_removed;
                            else res = decltype(res)::new_columns_added;
                        }else{

                            res = decltype(res)::dropped_and_recreated;
                        }
                    }else{
                        if(res != decltype(res)::old_columns_removed){
                            res = decltype(res)::already_in_sync;
                        }
                    }
                }
            }else{
                res = decltype(res)::new_table_created;
            }
            return res;
        }

        static bool get_remove_add_columns(std::vector<table_info*>& columnsToAdd,
                                    std::vector<table_info>& storageTableInfo,
                                    std::vector<table_info>& dbTableInfo)
        {
            bool notEqual = false;

            //  iterate through storage columns
            for(size_t storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size(); ++storageColumnInfoIndex) {
                
                //  get storage's column info
                auto &storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                auto &columnName = storageColumnInfo.name;
                
                //  search for a column in db eith the same name
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
                            notEqual = true;
                            break;
                        }
                        dbTableInfo.erase(dbColumnInfoIt);
                        storageTableInfo.erase(storageTableInfo.begin() + storageColumnInfoIndex);
                        --storageColumnInfoIndex;
                    }else{
                        
                        //  undefined type/types
                        notEqual = true;
                        break;
                    }
                }else{
                    columnsToAdd.push_back(&storageColumnInfo);
                }
            }
            return notEqual;
        }
        

    private:
        typedef storage_impl<Ts...> Super;
        typedef storage_impl<H, Ts...> Self;
    };
    
    namespace internal {
        
        /**
         *  This is a proxy class used to define what type must have result type depending on select 
         *  arguments (member pointer, aggregate functions, etc). Below you can see specializations
         *  for different types. E.g. specialization for core_functions::length_t has `type` int cause
         *  LENGTH returns INTEGER in sqlite. Every column_result_t must have `type` type that equals
         *  c++ SELECT return type for T
         *  T - C++ type
         *  Ts - tables pack from storage. Rarely used. Required in asterisk to define columns mapped for a type
         */
        template<class T, class ...Ts>
        struct column_result_t;
        
        template<class O, class F, class ...Ts>
        struct column_result_t<F O::*, Ts...> {
            typedef F type;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<const F& (O::*)() const, Ts...> {
            typedef F type;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<void (O::*)(F), Ts...> {
            typedef F type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::length_t<T>, Ts...> {
            typedef int type;
        };
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        template<class ...Args, class ...Ts>
        struct column_result_t<core_functions::char_t_<Args...>, Ts...> {
            typedef std::string type;
        };
#endif
        
        template<class ...Ts>
        struct column_result_t<core_functions::random_t, Ts...> {
            typedef int type;
        };
        
        template<class ...Ts>
        struct column_result_t<core_functions::changes_t, Ts...> {
            typedef int type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::abs_t<T>, Ts...> {
            typedef std::shared_ptr<double> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::lower_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::upper_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::trim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::trim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::ltrim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::ltrim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::rtrim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::rtrim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::date_t<T, Args...>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::datetime_t<T, Args...>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::avg_t<T>, Ts...> {
            typedef double type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::count_t<T>, Ts...> {
            typedef int type;
        };
        
        template<class ...Ts>
        struct column_result_t<aggregate_functions::count_asterisk_t, Ts...> {
            typedef int type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::sum_t<T>, Ts...> {
            typedef std::shared_ptr<double> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::total_t<T>, Ts...> {
            typedef double type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_single_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_double_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::max_t<T>, Ts...> {
            typedef std::shared_ptr<typename column_result_t<T>::type> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::min_t<T>, Ts...> {
            typedef std::shared_ptr<typename column_result_t<T>::type> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::distinct_t<T>, Ts...> {
            typedef typename column_result_t<T>::type type;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::conc_t<L, R>, Ts...> {
            typedef std::string type;
        };
    }
    
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
            std::shared_ptr<internal::database_connection> connection;
            
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
                std::shared_ptr<sqlite3_stmt *> stmt;
                view_t<T, Args...> &view;
                std::shared_ptr<T> temp;
                
                void extract_value(decltype(temp) &temp) {
                    temp = std::make_shared<T>();
                    auto &storage = this->view.storage;
                    auto &impl = storage.template get_impl<T>();
                    auto index = 0;
                    impl.table.for_each_column([&index, &temp, this] (auto c) {
                        auto value = row_extrator<typename decltype(c)::field_type>().extract(*this->stmt, index++);
                        if(c.member_pointer){
                            auto member_pointer = c.member_pointer;
                            (*temp).*member_pointer = value;
                        }else{
                            ((*temp).*(c.setter))(std::move(value));
                        }
                    });
                }
                
            public:
                iterator_t(sqlite3_stmt * stmt_, view_t<T, Args...> &view_):stmt(std::make_shared<sqlite3_stmt *>(stmt_)),view(view_){
                    this->operator++();
                }
                
                ~iterator_t() {
                    if(this->stmt){
                        statement_finalizer f{*this->stmt};
                    }
                }
                
                T& operator*() {
                    if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                    if(!this->temp){
                        this->extract_value(this->temp);
                    }
                    return *this->temp;
                }
                
                T* operator->() {
                    if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                    if(!this->temp){
                        this->extract_value(this->temp);
                    }
                    return &*this->temp;
                }
                
                void operator++() {
                    if(this->stmt && *this->stmt){
                        auto ret = sqlite3_step(*this->stmt);
                        switch(ret){
                            case SQLITE_ROW:
                                this->temp = nullptr;
                                break;
                            case SQLITE_DONE:{
                                statement_finalizer f{*this->stmt};
                                *this->stmt = nullptr;
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
                
				//removed const return type to remove complier warning
                bool operator==(const iterator_t &other) const {
                    if(this->stmt && other.stmt){
                        return *this->stmt == *other.stmt;
                    }else{
                        if(!this->stmt && !other.stmt){
                            return true;
                        }else{
                            return false;
                        }
                    }
                }
                
				//removed const return type to remove complier warning
                bool operator!=(const iterator_t &other) const {
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
        
        struct transaction_guard_t {
            typedef storage_t<Ts...> storage_type;
            
            /**
             *  This is a public lever to tell a guard what it must do in its destructor
             *  if `gotta_fire` is true
             */
            bool commit_on_destroy = false;
            
            transaction_guard_t(storage_type &s):storage(s){}
            
            ~transaction_guard_t() {
                if(this->gotta_fire){
                    if(!this->commit_on_destroy){
                        this->storage.rollback();
                    }else{
                        this->storage.commit();
                    }
                }
            }
            
            /**
             *  Call `COMMIT` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void commit() {
                this->storage.commit();
                this->gotta_fire = false;
            }
            
            /**
             *  Call `ROLLBACK` explicitly. After this call
             *  guard will not call `COMMIT` or `ROLLBACK`
             *  in its destructor.
             */
            void rollback() {
                this->storage.rollback();
                this->gotta_fire = false;
            }
            
        protected:
            storage_type &storage;
            bool gotta_fire = true;
        };
        
        std::function<void(sqlite3*)> on_open;
        
        transaction_guard_t transaction_guard() {
            this->begin_transaction();
            return {*this};
        }
        
        /**
         *  @param filename_ database filename.
         */
        storage_t(const std::string &filename_, impl_type impl_):
        filename(filename_),
        impl(impl_),
        inMemory(filename_.empty() || filename_ == ":memory:"){
            if(inMemory){
                this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                this->on_open_internal(this->currentTransaction->get_db());
            }
        }
        
    protected:
        
        template<class O, class T, class ...Op>
        std::string serialize_column_schema(internal::column_t<O, T, Op...> c) {
            std::stringstream ss;
            ss << "'" << c.name << "' ";
            typedef typename decltype(c)::field_type field_type;
            typedef typename decltype(c)::constraints_type constraints_type;
            ss << type_printer<field_type>().print() << " ";
            tuple_helper::iterator<std::tuple_size<constraints_type>::value - 1, Op...>()(c.constraints, [&](auto &v){
                ss << static_cast<std::string>(v) << ' ';
            });
            if(c.not_null()){
                ss << "NOT NULL ";
            }
            return ss.str();
        }
        
        template<class ...Cs>
        std::string serialize_column_schema(constraints::primary_key_t<Cs...> fk) {
            std::stringstream ss;
            ss << static_cast<std::string>(fk) << " (";
            std::vector<std::string> columnNames;
            columnNames.reserve(std::tuple_size<decltype(fk.columns)>::value);
            tuple_helper::iterator<std::tuple_size<decltype(fk.columns)>::value - 1, Cs...>()(fk.columns, [&](auto &c){
                columnNames.push_back(this->impl.column_name(c));
            });
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }
            }
            ss << ") ";
            return ss.str();
        }
        
#if SQLITE_VERSION_NUMBER >= 3006019
        
        template<class C, class R>
        std::string serialize_column_schema(constraints::foreign_key_t<C, R> fk) {
            std::stringstream ss;
            typedef typename internal::table_type<decltype(fk.r)>::type ref_type;
            auto refTableName = this->impl.template find_table_name<ref_type>();
            auto refColumnName = this->impl.column_name(fk.r);
            ss << "FOREIGN KEY(" << this->impl.column_name(fk.m) << ") REFERENCES ";
            ss << refTableName << "(" << refColumnName << ") ";
            return ss.str();
        }
#endif
        
        template<class I>
        void create_table(sqlite3 *db, const std::string &tableName, I *impl) {
            std::stringstream ss;
            ss << "CREATE TABLE '" << tableName << "' ( ";
            auto columnsCount = impl->table.columns_count();
            auto index = 0;
            impl->table.for_each_column_with_constraints([columnsCount, &index, &ss, this] (auto c) {
                ss << this->serialize_column_schema(c);
                if(index < columnsCount - 1) {
                    ss << ", ";
                }
                index++;
            });
            ss << ") ";
            if(impl->table._without_rowid) {
                ss << "WITHOUT ROWID ";
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        template<class I>
        void backup_table(sqlite3 *db, I *impl) {
            
            //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
            //  a name already exists we append suffix 1, then 2, etc until we find a free name..
            auto backupTableName = impl->table.name + "_backup";
            if(impl->table_exists(backupTableName, db)){
                int suffix = 1;
                do{
                    std::stringstream stream;
                    stream << suffix;
                    auto anotherBackupTableName = backupTableName + stream.str();
                    if(!impl->table_exists(anotherBackupTableName, db)){
                        backupTableName = anotherBackupTableName;
                        break;
                    }
                    ++suffix;
                }while(true);
            }
            
            this->create_table(db, backupTableName, impl);
            
            impl->copy_table(db, backupTableName);
            
            this->drop_table_internal(impl->table.name, db);
            
            impl->rename_table(db, backupTableName, impl->table.name);
        }
        
        template<class O>
        void assert_mapped_type() {
            typedef std::tuple<typename Ts::object_type...> mapped_types_tuples;
            static_assert(tuple_helper::has_type<O, mapped_types_tuples>::value, "type is not mapped to a storage");
        }
        
        template<class O>
        auto& get_impl() {
            return this->impl.template get_impl<O>();
        }
        
        std::string escape(std::string text) {
            for(size_t i = 0; i < text.length(); ) {
                if(text[i] == '\''){
                    text.insert(text.begin() + i, '\'');
                    i += 2;
                }
                else
                    ++i;
            }
            return text;
        }
        
        template<class T>
        std::string string_from_expression(T t, bool /*noTableName*/ = false, bool escape = false) {
            auto isNullable = type_is_nullable<T>::value;
            if(isNullable && !type_is_nullable<T>()(t)){
                return "NULL";
            }else{
                auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                std::stringstream ss;
                if(needQuotes){
                    ss << "'";
                }
                std::string text = field_printer<T>()(t);
                if(escape){
                    text = this->escape(text);
                }
                ss << text;
                if(needQuotes){
                    ss << "'";
                }
                return ss.str();
            }
        }
        
        std::string string_from_expression(const std::string &t, bool /*noTableName*/ = false, bool escape = false) {
            std::stringstream ss;
            std::string text = t;
            if(escape){
                text = this->escape(text);
            }
            ss << "'" << text << "'";
            return ss.str();
        }
        
        std::string string_from_expression(const char *t, bool /*noTableName*/ = false, bool escape = false) {
            std::stringstream ss;
            std::string text = t;
            if(escape){
                text = this->escape(text);
            }
            ss << "'" << text << "'";
            return ss.str();
        }
        
        template<class F, class O>
        std::string string_from_expression(F O::*m, bool noTableName = false, bool /*escape*/ = false) {
            std::stringstream ss;
            if(!noTableName){
                ss << " '" << this->impl.template find_table_name<O>() << "'.";
            }
            ss << "\"" << this->impl.column_name(m) << "\"";
            return ss.str();
        }
        
        template<class F, class O>
        std::string string_from_expression(const F* (O::*g)() const, bool noTableName = false, bool /*escape*/ = false) {
            std::stringstream ss;
            if(!noTableName){
                ss << " '" << this->impl.template find_table_name<O>() << "'.";
            }
            ss << "\"" << this->impl.column_name(g) << "\"";
            return ss.str();
        }
        
        template<class F, class O>
        std::string string_from_expression(void (O::*s)(F), bool noTableName = false, bool /*escape*/ = false) {
            std::stringstream ss;
            if(!noTableName){
                ss << " '" << this->impl.template find_table_name<O>() << "'.";
            }
            ss << "\"" << this->impl.column_name(s) << "\"";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::group_concat_double_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            auto expr2 = this->string_from_expression(f.y);
            ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::group_concat_single_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class L, class R>
        std::string string_from_expression(internal::conc_t<L, R> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto lhs = this->string_from_expression(f.l);
            auto rhs = this->string_from_expression(f.r);
            ss << "(" << lhs << " || " << rhs << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::min_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::max_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::total_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::sum_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        std::string string_from_expression(aggregate_functions::count_asterisk_t &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            ss << static_cast<std::string>(f) << "(*) ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::count_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(aggregate_functions::avg_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(a.t);
            ss << static_cast<std::string>(a) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(internal::distinct_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.t);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class X, class Y>
        std::string string_from_expression(core_functions::rtrim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            auto expr2 = this->string_from_expression(f.y);
            ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
            return ss.str();
        }
        
        template<class X>
        std::string string_from_expression(core_functions::rtrim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class X, class Y>
        std::string string_from_expression(core_functions::ltrim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            auto expr2 = this->string_from_expression(f.y);
            ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
            return ss.str();
        }
        
        template<class X>
        std::string string_from_expression(core_functions::ltrim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class X, class Y>
        std::string string_from_expression(core_functions::trim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            auto expr2 = this->string_from_expression(f.y);
            ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
            return ss.str();
        }
        
        template<class X>
        std::string string_from_expression(core_functions::trim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(f.x);
            ss << static_cast<std::string>(f) << "(" << expr << ") ";
            return ss.str();
        }
        
        std::string string_from_expression(core_functions::changes_t &ch, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            ss << static_cast<std::string>(ch) << "() ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(core_functions::length_t<T> &len, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(len.t);
            ss << static_cast<std::string>(len) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T, class ...Args>
        std::string string_from_expression(core_functions::datetime_t<T, Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            ss << static_cast<std::string>(f) << "(" << this->string_from_expression(f.timestring);
            typedef std::tuple<Args...> tuple_t;
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                ss << ", " << this->string_from_expression(v);
            });
            ss << ") ";
            return ss.str();
        }
        
        template<class T, class ...Args>
        std::string string_from_expression(core_functions::date_t<T, Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            ss << static_cast<std::string>(f) << "(" << this->string_from_expression(f.timestring);
            typedef std::tuple<Args...> tuple_t;
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                ss << ", " << this->string_from_expression(v);
            });
            ss << ") ";
            return ss.str();
        }
        
        std::string string_from_expression(core_functions::random_t &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            ss << static_cast<std::string>(f) << "() ";
            return ss.str();
        }
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        template<class ...Args>
        std::string string_from_expression(core_functions::char_t_<Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            typedef decltype(f.args) tuple_t;
            std::vector<std::string> args;
            args.reserve(std::tuple_size<tuple_t>::value);
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.args, [&](auto &v){
                auto expression = this->string_from_expression(v);
                args.insert(args.begin(), expression);
            });
            ss << static_cast<std::string>(f) << "(";
            auto lim = int(args.size());
            for(auto i = 0; i < lim; ++i) {
                ss << args[i];
                if(i < lim - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << ") ";
            return ss.str();
        }
#endif
        
        template<class T>
        std::string string_from_expression(core_functions::upper_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(a.t);
            ss << static_cast<std::string>(a) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(core_functions::lower_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(a.t);
            ss << static_cast<std::string>(a) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string string_from_expression(core_functions::abs_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
            std::stringstream ss;
            auto expr = this->string_from_expression(a.t);
            ss << static_cast<std::string>(a) << "(" << expr << ") ";
            return ss.str();
        }
        
        template<class T>
        std::string process_where(conditions::is_null_t<T> &c) {
            std::stringstream ss;
            ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
            return ss.str();
        }
        
        template<class T>
        std::string process_where(conditions::is_not_null_t<T> &c) {
            std::stringstream ss;
            ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
            return ss.str();
        }
        
        template<class C>
        std::string process_where(conditions::negated_condition_t<C> &c) {
            std::stringstream ss;
            ss << " " << static_cast<std::string>(c) << " ";
            auto cString = this->process_where(c.c);
            ss << " (" << cString << " ) ";
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(conditions::and_condition_t<L, R> &c) {
            std::stringstream ss;
            ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
            return ss.str();
        }
        
        template<class L, class R>
        std::string process_where(conditions::or_condition_t<L, R> &c) {
            std::stringstream ss;
            ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
            return ss.str();
        }
        
        /**
         *  Common case. Is used to process binary conditions like is_equal, not_equal
         */
        template<class C>
        std::string process_where(C c) {
            auto leftString = this->string_from_expression(c.l, false, true);
            auto rightString = this->string_from_expression(c.r, false, true);
            std::stringstream ss;
            ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
            return ss.str();
        }
        
        template<class L, class E>
        std::string process_where(conditions::in_t<L, E> &inCondition) {
            std::stringstream ss;
            auto leftString = this->string_from_expression(inCondition.l);
            ss << leftString << " " << static_cast<std::string>(inCondition) << " (";
            for(size_t index = 0; index < inCondition.values.size(); ++index) {
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
        std::string process_where(conditions::between_t<A, T> &bw) {
            std::stringstream ss;
            auto expr = this->string_from_expression(bw.expr);
            ss << expr << " " << static_cast<std::string>(bw) << " " << this->string_from_expression(bw.b1) << " AND " << this->string_from_expression(bw.b2) << " ";
            return ss.str();
        }
        
        template<class O>
        std::string process_order_by(conditions::order_by_t<O> &orderBy) {
            std::stringstream ss;
            auto columnName = this->string_from_expression(orderBy.o);
            ss << columnName << " ";
            switch(orderBy.ascDesc){
                case 1:
                    ss << "ASC ";
                    break;
                case -1:
                    ss << "DESC ";
                    break;
            }
            return ss.str();
        }
        
        template<class T>
        void process_join_constraint(std::stringstream &ss, conditions::on_t<T> &t) {
            ss << static_cast<std::string>(t) << " " << this->process_where(t.t) << " ";
        }
        
        template<class F, class O>
        void process_join_constraint(std::stringstream &ss, conditions::using_t<F, O> &u) {
            ss << static_cast<std::string>(u) << " (" << this->string_from_expression(u.column, true) << " ) ";
        }
        
        void process_single_condition(std::stringstream &ss, conditions::limit_t limt) {
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
        
        template<class O>
        void process_single_condition(std::stringstream &ss, conditions::cross_join_t<O> c) {
            ss << static_cast<std::string>(c) << " ";
            ss << " '" << this->impl.template find_table_name<O>() << "' ";
        }
        
        template<class T, class O>
        void process_single_condition(std::stringstream &ss, conditions::inner_join_t<T, O> l) {
            ss << static_cast<std::string>(l) << " ";
            ss << " '" << this->impl.template find_table_name<T>() << "' ";
            this->process_join_constraint(ss, l.constraint);
        }
        
        template<class T, class O>
        void process_single_condition(std::stringstream &ss, conditions::left_outer_join_t<T, O> l) {
            ss << static_cast<std::string>(l) << " ";
            ss << " '" << this->impl.template find_table_name<T>() << "' ";
            this->process_join_constraint(ss, l.constraint);
        }
        
        template<class T, class O>
        void process_single_condition(std::stringstream &ss, conditions::left_join_t<T, O> l) {
            ss << static_cast<std::string>(l) << " ";
            ss << " '" << this->impl.template find_table_name<T>() << "' ";
            this->process_join_constraint(ss, l.constraint);
        }
        
        template<class T, class O>
        void process_single_condition(std::stringstream &ss, conditions::join_t<T, O> l) {
            ss << static_cast<std::string>(l) << " ";
            ss << " '" << this->impl.template find_table_name<T>() << "' ";
            this->process_join_constraint(ss, l.constraint);
        }
        
        /*template<class T>
        void process_single_condition(std::stringstream &ss, conditions::natural_join_t<T> l) {
            ss << static_cast<std::string>(l) << " ";
            ss << " '" << this->impl.template find_table_name<T>() << "' ";
//            this->process_join_constraint(ss, l.constraint);
        }*/
        
        template<class C>
        void process_single_condition(std::stringstream &ss, conditions::where_t<C> w) {
            ss << static_cast<std::string>(w) << " ";
            auto whereString = this->process_where(w.c);
            ss << "( " << whereString << ") ";
        }
        
        template<class O>
        void process_single_condition(std::stringstream &ss, conditions::order_by_t<O> orderBy) {
            ss << static_cast<std::string>(orderBy) << " ";
            auto orderByString = this->process_order_by(orderBy);
            ss << orderByString << " ";
        }
        
        template<class ...Args>
        void process_single_condition(std::stringstream &ss, conditions::group_by_t<Args...> groupBy) {
            std::vector<std::string> expressions;
            typedef std::tuple<Args...> typle_t;
            tuple_helper::iterator<std::tuple_size<typle_t>::value - 1, Args...>()(groupBy.args, [&](auto &v){
                auto expression = this->string_from_expression(v);
                expressions.push_back(expression);
            });
            ss << static_cast<std::string>(groupBy) << " ";
            for(size_t i = 0; i < expressions.size(); ++i) {
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
        void process_conditions(std::stringstream &, Args .../*args*/) {
            //..
        }
        
        template<class C, class ...Args>
        void process_conditions(std::stringstream &ss, C c, Args ...args) {
            this->process_single_condition(ss, c);
            this->process_conditions(ss, args...);
        }
        
        void on_open_internal(sqlite3 *db) {
            
#if SQLITE_VERSION_NUMBER >= 3006019
            if(this->foreign_keys_count()){
                this->foreign_keys(db, true);
            }
#endif
            if(this->on_open){
                this->on_open(db);
            }
            
        }
        
#if SQLITE_VERSION_NUMBER >= 3006019
        
        //  returns foreign keys count in storage definition
        int foreign_keys_count() {
            auto res = 0;
            this->impl.for_each([&res](auto impl){
                res += impl->foreign_keys_count();
            });
            return res;
        }
#endif
        
    public:
        
        template<class T, class ...Args>
        view_t<T, Args...> iterate(Args ...args) {
            this->assert_mapped_type<T>();
            
            std::shared_ptr<internal::database_connection> connection;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                this->on_open_internal(connection->get_db());
            }else{
                connection = this->currentTransaction;
            }
            return {*this, connection, args...};
        }
        
        template<class O, class ...Args>
        void remove_all(Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::stringstream ss;
            ss << "DELETE FROM '" << impl.table.name << "' ";
            this->process_conditions(ss, args...);
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
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
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::stringstream ss;
            ss << "DELETE FROM '" << impl.table.name << "' ";
            ss << "WHERE ";
            std::vector<std::string> primaryKeyColumnNames;
            impl.table.for_each_column([&] (auto c) {
                if(c.template has<constraints::primary_key_t<>>()) {
                    primaryKeyColumnNames.emplace_back(c.name);
                }
            });
            for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                ss << "\"" << primaryKeyColumnNames[i] << "\"" << " =  ?";
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
                impl.table.template for_each_column_with<constraints::primary_key_t<>>([&] (auto c) {
                    typedef typename decltype(c)::field_type field_type;
                    statement_binder<field_type>().bind(stmt, index++, id);
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else{
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        /**
         *  Update routine. Sets all non primary key fields where primary key is equal.
         *  O is an object type. May be not specified explicitly cause it can be deduced by
         *      compiler from first parameter.
         *  @param o object to be updated.
         */
        template<class O>
        void update(const O &o) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::stringstream ss;
            ss << "UPDATE '" << impl.table.name << "' SET ";
            std::vector<std::string> setColumnNames;
            impl.table.for_each_column([&](auto c) {
                if(!c.template has<constraints::primary_key_t<>>()) {
                    setColumnNames.emplace_back(c.name);
                }
            });
            for(size_t i = 0; i < setColumnNames.size(); ++i) {
                ss << "\"" << setColumnNames[i] << "\"" << " = ?";
                if(i < setColumnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "WHERE ";
//            auto primaryKeyColumnNames = impl.table.template column_names_with<constraints::primary_key_t<>>();
            auto primaryKeyColumnNames = impl.table.primary_key_column_names();
            for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ?";
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
                impl.table.for_each_column([&o, stmt, &index] (auto c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        typedef typename decltype(c)::field_type field_type;
                        const field_type *value = nullptr;
                        if(c.member_pointer){
                            value = &(o.*c.member_pointer);
                        }else{
                            value = &((o).*(c.getter))();
                        }
                        statement_binder<field_type>().bind(stmt, index++, *value);
                    }
                });
                impl.table.for_each_column([&o, stmt, &index] (auto c) {
                    if(c.template has<constraints::primary_key_t<>>()) {
                        typedef typename decltype(c)::field_type field_type;
                        const field_type *value = nullptr;
                        if(c.member_pointer){
                            value = &(o.*c.member_pointer);
                        }else{
                            value = &((o).*(c.getter))();
                        }
                        statement_binder<field_type>().bind(stmt, index++, *value);
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return;
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        template<class ...Args, class ...Wargs>
        void update_all(internal::set_t<Args...> set, Wargs ...wh) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            
            std::stringstream ss;
            ss << "UPDATE ";
            std::set<std::string> tableNamesSet;
            set.for_each([this, &tableNamesSet](auto &lhs, auto &/*rhs*/){
                auto tableName = this->parse_table_name(lhs);
                tableNamesSet.insert(tableName.begin(), tableName.end());
            });
            if(tableNamesSet.size()){
                if(tableNamesSet.size() == 1){
                    ss << " '" << *tableNamesSet.begin() << "' ";
                    ss << static_cast<std::string>(set) << " ";
                    std::vector<std::string> setPairs;
                    set.for_each([this, &setPairs](auto &lhs, auto &rhs){
                        std::stringstream sss;
                        sss << this->string_from_expression(lhs, true) << " = " << this->string_from_expression(rhs) << " ";
                        setPairs.push_back(sss.str());
                    });
                    auto setPairsCount = setPairs.size();
                    for(size_t i = 0; i < setPairsCount; ++i) {
                        ss << setPairs[i] << " ";
                        if(i < setPairsCount - 1) {
                            ss << ", ";
                        }
                    }
                    this->process_conditions(ss, wh...);
                    auto query = ss.str();
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                        statement_finalizer finalizer{stmt};
                        if (sqlite3_step(stmt) == SQLITE_DONE) {
                            return;
                        }else{
                            auto msg = sqlite3_errmsg(db);
                            throw std::runtime_error(msg);
                        }
                    }else {
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("too many tables specified - UPDATE can be performed only for a single table");
                }
            }else{
                throw std::runtime_error("incorrect SET fields specified");
            }
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
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss
                << "'" << impl.table.name << "'."
                << "\""
                << columnNames[i]
                << "\""
                ;
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM '" << impl.table.name << "' ";
            this->process_conditions(ss, args...);
            if(query){
                *query = ss.str();
            }
            return impl;
        }
        
        template<class T>
        std::set<std::string> parse_table_name(T &) {
            return {};
        }
        
        template<class F, class O>
        std::set<std::string> parse_table_name(F O::*) {
            return {this->impl.template find_table_name<O>()};
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::min_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::max_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::sum_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::total_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::group_concat_double_t<T> &f) {
            auto res = this->parse_table_name(f.t);
            auto secondSet = this->parse_table_name(f.y);
            res.insert(secondSet.begin(), secondSet.end());
            return res;
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::group_concat_single_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::count_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(aggregate_functions::avg_t<T> &a) {
            return this->parse_table_name(a.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(core_functions::length_t<T> &len) {
            return this->parse_table_name(len.t);
        }
        
        template<class T, class ...Args>
        std::set<std::string> parse_table_name(core_functions::date_t<T, Args...> &f) {
            auto res = this->parse_table_name(f.timestring);
            typedef decltype(f.modifiers) tuple_t;
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                auto tableNames = this->parse_table_name(v);
                res.insert(tableNames.begin(), tableNames.end());
            });
            return res;
        }
        
        template<class T, class ...Args>
        std::set<std::string> parse_table_name(core_functions::datetime_t<T, Args...> &f) {
            auto res = this->parse_table_name(f.timestring);
            typedef decltype(f.modifiers) tuple_t;
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                auto tableNames = this->parse_table_name(v);
                res.insert(tableNames.begin(), tableNames.end());
            });
            return res;
        }
        
        template<class X>
        std::set<std::string> parse_table_name(core_functions::trim_single_t<X> &f) {
            return this->parse_table_name(f.x);
        }
        
        template<class X, class Y>
        std::set<std::string> parse_table_name(core_functions::trim_double_t<X, Y> &f) {
            auto res = this->parse_table_name(f.x);
            auto res2 = this->parse_table_name(f.y);
            res.insert(res2.begin(), res2.end());
            return res;
        }
        
        template<class X>
        std::set<std::string> parse_table_name(core_functions::rtrim_single_t<X> &f) {
            return this->parse_table_name(f.x);
        }
        
        template<class X, class Y>
        std::set<std::string> parse_table_name(core_functions::rtrim_double_t<X, Y> &f) {
            auto res = this->parse_table_name(f.x);
            auto res2 = this->parse_table_name(f.y);
            res.insert(res2.begin(), res2.end());
            return res;
        }
        
        template<class X>
        std::set<std::string> parse_table_name(core_functions::ltrim_single_t<X> &f) {
            return this->parse_table_name(f.x);
        }
        
        template<class X, class Y>
        std::set<std::string> parse_table_name(core_functions::ltrim_double_t<X, Y> &f) {
            auto res = this->parse_table_name(f.x);
            auto res2 = this->parse_table_name(f.y);
            res.insert(res2.begin(), res2.end());
            return res;
        }
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        template<class ...Args>
        std::set<std::string> parse_table_name(core_functions::char_t_<Args...> &f) {
            std::set<std::string> res;
            typedef decltype(f.args) tuple_t;
            tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.args, [&](auto &v){
                auto tableNames = this->parse_table_name(v);
                res.insert(tableNames.begin(), tableNames.end());
            });
            return res;
        }
        
#endif
        
        std::set<std::string> parse_table_name(core_functions::random_t &f) {
            return {};
        }
        
        template<class T>
        std::set<std::string> parse_table_name(core_functions::upper_t<T> &a) {
            return this->parse_table_name(a.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(core_functions::lower_t<T> &a) {
            return this->parse_table_name(a.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(core_functions::abs_t<T> &a) {
            return this->parse_table_name(a.t);
        }
        
        template<class T>
        std::set<std::string> parse_table_name(internal::distinct_t<T> &f) {
            return this->parse_table_name(f.t);
        }
        
        template<class ...Args>
        std::set<std::string> parse_table_names(Args .../*args*/) {
            return {};
        }
        
        template<class H, class ...Args>
        std::set<std::string> parse_table_names(H h, Args ...args) {
            auto res = this->parse_table_names(std::forward<Args>(args)...);
            auto tableName = this->parse_table_name(h);
            res.insert(tableName.begin(),
                       tableName.end());
            return res;
        }
        
        template<class ...Args>
        std::set<std::string> parse_table_names(internal::columns_t<Args...> &cols) {
            std::set<std::string> res;
            cols.for_each([&](auto &m){
                auto tableName = this->parse_table_name(m);
                res.insert(tableName.begin(),
                           tableName.end());
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
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            C res;
            std::string query;
            auto &impl = this->generate_select_asterisk<O>(&query, args...);
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                int stepRes;
                do{
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            O obj;
                            auto index = 0;
                            impl.table.for_each_column([&index, &obj, stmt] (auto c) {
                                typedef typename decltype(c)::field_type field_type;
                                auto value = row_extrator<field_type>().extract(stmt, index++);
                                if(c.member_pointer){
                                    obj.*c.member_pointer = value;
                                }else{
                                    ((obj).*(c.setter))(std::move(value));
                                }
                            });
                            res.push_back(std::move(obj));
                        }break;
                        case SQLITE_DONE: break;
                        default:{
                            auto msg = sqlite3_errmsg(db);
                            throw std::runtime_error(msg);
                        }
                    }
                }while(stepRes != SQLITE_DONE);
                return res;
            }else{
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        /**
         *  Select * by id routine.
         *  throws sqlite_orm::not_found_exeption if object not found with given id.
         *  throws std::runtime_error in case of db error.
         *  O is an object type to be extracted. Must be specified explicitly.
         *  @return Object of type O where id is equal parameter passed or throws `not_found_exception`
         *  if there is no object with such id.
         */
        template<class O, class ...Ids>
        O get(Ids ...ids) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::shared_ptr<O> res;
            std::stringstream ss;
            ss << "SELECT ";
            auto columnNames = impl.table.column_names();
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM '" << impl.table.name << "' WHERE ";
            auto primaryKeyColumnNames = impl.table.primary_key_column_names();
            if(primaryKeyColumnNames.size()){
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << "AND ";
                    }
                    ss << ' ';
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    auto idsTuple = std::make_tuple(std::forward<Ids>(ids)...);
                    constexpr const auto idsCount = std::tuple_size<decltype(idsTuple)>::value;
                    tuple_helper::iterator<idsCount - 1, Ids...>()(idsTuple, [stmt, &index](auto &v){
                        typedef typename std::remove_reference<decltype(v)>::type field_type;
                        statement_binder<field_type>().bind(stmt, index++, v);
                    });
                    auto stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            O res;
                            index = 0;
                            impl.table.for_each_column([&index, &res, stmt] (auto c) {
                                typedef typename decltype(c)::field_type field_type;
                                auto value = row_extrator<field_type>().extract(stmt, index++);
                                if(c.member_pointer){
                                    res.*c.member_pointer = value;
                                }else{
                                    ((res).*(c.setter))(std::move(value));
                                }
                            });
                            return res;
                        }break;
                        case SQLITE_DONE:{
                            throw not_found_exception{};
                        }break;
                        default:{
                            auto msg = sqlite3_errmsg(db);
                            throw std::runtime_error(msg);
                        }
                    }
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else{
                throw std::runtime_error("table " + impl.table.name + " has no primary key column");
            }
        }
        
        /**
         *  The same as `get` function but doesn't throw an exeption if noting found but returns std::shared_ptr with null value.
         *  throws std::runtime_error iin case of db error.
         */
        template<class O, class ...Ids>
        std::shared_ptr<O> get_no_throw(Ids ...ids) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::shared_ptr<O> res;
            std::stringstream ss;
            ss << "SELECT ";
            auto columnNames = impl.table.column_names();
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            ss << "FROM '" << impl.table.name << "' WHERE ";
            auto primaryKeyColumnNames = impl.table.primary_key_column_names();
            if(primaryKeyColumnNames.size() && primaryKeyColumnNames.front().length()){
//                ss << "\"" << primaryKeyColumnNames.front() << "\"" << " = " << string_from_expression(id);
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << "AND ";
                    }
                    ss << ' ';
                }
//                auto idsTuple = std::make_tuple(ids...);
//                typedef decltype(idsTuple) IdsTuple;
                auto query = ss.str();
                /*std::vector<std::string> memberStrings;
                memberStrings.reserve(columnNames.size());
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &memberStrings = *(std::vector<std::string>*)data;
                                           if(argc){
                                               for(auto i = 0; i < argc; ++i) {
                                                   auto str = argv[i];
                                                   if(str){
                                                       memberStrings.push_back(argv[i]);
                                                   }else{
                                                       memberStrings.push_back({});
                                                   }
                                               }
                                           }
                                           return 0;
                                       }, &memberStrings, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }else{
                    if(memberStrings.size()){
                        res = std::make_shared<O>();
                        auto index = 0;
                        impl.table.for_each_column([&index, &res, &memberStrings] (auto c) {
                            typedef typename decltype(c)::field_type field_type;
                            auto &o = *res;
                            auto value = row_extrator<field_type>().extract(memberStrings[index++].c_str());
                            if(c.member_pointer){
                                o.*c.member_pointer = value;
                            }else{
                                ((o).*(c.setter))(std::move(value));
                            }
                        });
                        return res;
                    }else{
//                        throw not_found_exception{};
                        return res;
                    }
                }
                return res;*/
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    auto idsTuple = std::make_tuple(std::forward<Ids>(ids)...);
                    constexpr const auto idsCount = std::tuple_size<decltype(idsTuple)>::value;
                    tuple_helper::iterator<idsCount - 1, Ids...>()(idsTuple, [stmt, &index](auto &v){
                        typedef typename std::remove_reference<decltype(v)>::type field_type;
                        statement_binder<field_type>().bind(stmt, index++, v);
                    });
                    auto stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            O res;
                            index = 0;
                            impl.table.for_each_column([&index, &res, stmt] (auto c) {
                                typedef typename decltype(c)::field_type field_type;
                                //                            auto &o = *res;
                                //                            auto value = row_extrator<field_type>().extract(memberStrings[index++].c_str());
                                auto value = row_extrator<field_type>().extract(stmt, index++);
                                if(c.member_pointer){
                                    res.*c.member_pointer = value;
                                }else{
                                    ((res).*(c.setter))(std::move(value));
                                }
                            });
                            return std::make_shared<O>(std::move(res));
                        }break;
                        case SQLITE_DONE:{
//                            throw not_found_exception{};
                            return {};
                        }break;
                        default:{
                            auto msg = sqlite3_errmsg(db);
                            throw std::runtime_error(msg);
                        }
                    }
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
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
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            
            auto &impl = this->get_impl<O>();
            int res = 0;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::count()) << "(*) FROM '" << impl.table.name << "' ";
            this->process_conditions(ss, args...);
            auto query = ss.str();
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **) -> int {
                                       auto &res = *(int*)data;
                                       if(argc){
                                           res = row_extrator<int>().extract(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
        
        /**
         *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
         *  @param m member pointer to class mapped to the storage.
         */
        template<class F, class O, class ...Args>
        int count(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            int res = 0;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::count(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '"<< impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(int*)data;
                                           if(argc){
                                               res = row_extrator<int>().extract(argv[0]);
                                           }
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
         *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return average value from db.
         */
        template<class F, class O, class ...Args>
        double avg(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            double res = 0;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::avg(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '"<< impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &res = *(double*)data;
                                           if(argc){
                                               res = row_extrator<double>().extract(argv[0]);
                                           }
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
         *  GROUP_CONCAT(X) query.  https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::string res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::group_concat(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '"<< impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               res = row_extrator<std::string>().extract(argv[0]);
                                           }
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
         *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return group_concat query result.
         */
        template<class F, class O, class ...Args>
        std::string group_concat(F O::*m, const std::string &y, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::string res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::group_concat(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ",\"" << y << "\") FROM '"<< impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               res = row_extrator<std::string>().extract(argv[0]);
                                           }
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
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::max(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                               }
                                           }
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
         *  MIN(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return std::shared_ptr with min value or null if sqlite engine returned null.
         */
        template<class F, class O, class ...Args>
        std::shared_ptr<F> min(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::min(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                               }
                                           }
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
         *  SUM(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return std::shared_ptr with sum value or null if sqlite engine returned null.
         */
        template<class F, class O, class ...Args>
        std::shared_ptr<F> sum(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = this->get_impl<O>();
            std::shared_ptr<F> res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::sum(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") FROM '"<< impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &res = *(std::shared_ptr<F>*)data;
                                           if(argc){
                                               res = std::make_shared<F>(row_extrator<F>().extract(argv[0]));
                                           }
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
         *  TOTAL(x) query.
         *  @param m is a class member pointer (the same you passed into make_column).
         *  @return total value (the same as SUM but not nullable. More details here https://www.sqlite.org/lang_aggfunc.html)
         */
        template<class F, class O, class ...Args>
        double total(F O::*m, Args ...args) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            double res;
            std::stringstream ss;
            ss << "SELECT " << static_cast<std::string>(sqlite_orm::total(0)) << "(";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << ") ";
                auto tableNamesSet = this->parse_table_names(m);
                if(tableNamesSet.size()){
                    ss << "FROM " ;
                    std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                    for(size_t i = 0; i < tableNames.size(); ++i) {
                        ss << " '" << tableNames[i] << "' ";
                        if(i < tableNames.size() - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                }
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **)->int{
                                           auto &res = *(double*)data;
                                           if(argc){
                                               res = row_extrator<double>().extract(argv[0]);
                                           }
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
         *  Select a single column into std::vector<T>.
         */
        template<class T, class ...Args, class R = typename internal::column_result_t<T, Ts...>::type>
        std::vector<R> select(T m, Args ...args) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::stringstream ss;
            ss << "SELECT ";
            auto columnName = this->string_from_expression(m);
            if(columnName.length()){
                ss << columnName << " ";
                auto tableNamesSet = this->parse_table_names(m);
                if(tableNamesSet.size()){
                    ss << "FROM " ;
                    std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                    for(size_t i = 0; i < tableNames.size(); ++i) {
                        ss << " '" << tableNames[i] << "' ";
                        if(i < tableNames.size() - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                }
                this->process_conditions(ss, args...);
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    std::vector<R> res;
                    int stepRes;
                    do{
                        stepRes = sqlite3_step(stmt);
                        switch(stepRes){
                            case SQLITE_ROW:{
                                res.push_back(row_extrator<R>().extract(stmt, 0));
                            }break;
                            case SQLITE_DONE: break;
                            default:{
                                auto msg = sqlite3_errmsg(db);
                                throw std::runtime_error(msg);
                            }
                        }
                    }while(stepRes != SQLITE_DONE);
                    return res;
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
                /*auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int, char **argv,char **) -> int {
                                           auto &res = *(std::vector<R>*)data;
                                           auto value = row_extrator<R>().extract(argv[0]);
                                           res.push_back(value);
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }*/
            }else{
                throw std::runtime_error("column not found");
            }
        }
        
        /**
         *  Select several columns into std::vector<std::tuple<...>>.
         */
        template<class ...Args,
        class R = std::tuple<typename internal::column_result_t<Args, Ts...>::type...>,
        class ...Conds
        >
        std::vector<R> select(internal::columns_t<Args...> cols, Conds ...conds) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::vector<R> res;
            std::stringstream ss;
            ss << "SELECT ";
            if(cols.distinct) {
                ss << static_cast<std::string>(distinct(0)) << " ";
            }
            std::vector<std::string> columnNames;
            columnNames.reserve(cols.count());
            cols.for_each([&](auto &m) {
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    columnNames.push_back(columnName);
                }else{
                    throw std::runtime_error("column not found");
                }
            });
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ", ";
                }else{
                    ss << " ";
                }
            }
            auto tableNamesSet = this->parse_table_names(cols);
            internal::join_iterator<Conds...>()([&](auto c){
                typedef typename decltype(c)::type crossJoinType;
                auto crossJoinedTableName = this->impl.template find_table_name<crossJoinType>();
                tableNamesSet.erase(crossJoinedTableName);
            });
            if(tableNamesSet.size()){
                ss << " FROM ";
                std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                for(size_t i = 0; i < tableNames.size(); ++i) {
                    ss << " '" << tableNames[i] << "' ";
                    if(int(i) < int(tableNames.size()) - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
            }
            this->process_conditions(ss, conds...);
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                std::vector<R> res;
                int stepRes;
                do{
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            res.push_back(row_extrator<R>().extract(stmt, 0));
                        }break;
                        case SQLITE_DONE: break;
                        default:{
                            auto msg = sqlite3_errmsg(db);
                            throw std::runtime_error(msg);
                        }
                    }
                }while(stepRes != SQLITE_DONE);
                return res;
            }else{
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            /*auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int, char **argv,char **) -> int {
                                       auto &res = *(std::vector<R>*)data;
                                       auto value = row_extrator<R>().extract(argv);
                                       res.push_back(value);
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }*/
            return res;
        }
        
        /**
         *  Returns a string representation of object of a class mapped to the storage.
         *  Type of string has json-like style.
         */
        template<class O>
        std::string dump(const O &o) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return this->impl.dump(o, db);
        }
        
        /**
         *  This is REPLACE (INSERT OR REPLACE) function.
         *  Also if you need to insert value with knows id you should
         *  also you this function instead of insert cause inserts ignores
         *  id and creates own one.
         */
        template<class O>
        void replace(const O &o) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = get_impl<O>();
            std::stringstream ss;
            ss << "REPLACE INTO '" << impl.table.name << "' (";
            auto columnNames = impl.table.column_names();
            auto columnNamesCount = columnNames.size();
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ") ";
                }
            }
            ss << "VALUES(";
            for(size_t i = 0; i < columnNamesCount; ++i) {
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
                impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                    typedef typename decltype(c)::field_type field_type;
                    const field_type *value = nullptr;
                    if(c.member_pointer){
                        value = &(o.*c.member_pointer);
                    }else{
                        value = &((o).*(c.getter))();
                    }
                    statement_binder<field_type>().bind(stmt, index++, *value);
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        template<class It>
        void replace_range(It from, It to) {
            typedef typename std::iterator_traits<It>::value_type O;
            this->assert_mapped_type<O>();
            if(from == to) {
                return;
            }
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = get_impl<O>();
            std::stringstream ss;
            ss << "REPLACE INTO '" << impl.table.name << "' (";
            auto columnNames = impl.table.column_names();
            auto columnNamesCount = columnNames.size();
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ") ";
                }
            }
            ss << "VALUES ";
            auto valuesString = [columnNamesCount]{
                std::stringstream ss;
                ss << "(";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "?";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                return ss.str();
            }();
            auto valuesCount = static_cast<int>(std::distance(from, to));
            for(auto i = 0; i < valuesCount; ++i) {
                ss << valuesString;
                if(i < valuesCount - 1) {
                    ss << ",";
                }
                ss << " ";
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                auto index = 1;
                for(auto it = from; it != to; ++it) {
                    auto &o = *it;
                    impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                        typedef typename decltype(c)::field_type field_type;
                        const field_type *value = nullptr;
                        if(c.member_pointer){
                            value = &(o.*c.member_pointer);
                        }else{
                            value = &((o).*(c.getter))();
                        }
                        statement_binder<field_type>().bind(stmt, index++, *value);
                    });
                }
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        /**
         *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
         *  object doesn't matter.
         *  @return id of just created object.
         */
        template<class O>
        int insert(const O &o) {
            this->assert_mapped_type<O>();
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = get_impl<O>();
            int res = 0;
            std::stringstream ss;
            ss << "INSERT INTO '" << impl.table.name << "' (";
            std::vector<std::string> columnNames;
            auto compositeKeyColumnNames = impl.table.composite_key_columns_names();
            impl.table.for_each_column([&impl, &columnNames, &compositeKeyColumnNames] (auto c) {
                if(impl.table._without_rowid || !c.template has<constraints::primary_key_t<>>()) {
                    auto it = std::find(compositeKeyColumnNames.begin(),
                                        compositeKeyColumnNames.end(),
                                        c.name);
                    if(it == compositeKeyColumnNames.end()){
                        columnNames.emplace_back(c.name);
                    }
                }
            });
            
            auto columnNamesCount = columnNames.size();
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ") ";
                }
            }
            ss << "VALUES (";
            for(size_t i = 0; i < columnNamesCount; ++i) {
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
                impl.table.for_each_column([&o, &index, &stmt, &impl, &compositeKeyColumnNames] (auto c) {
                    if(impl.table._without_rowid || !c.template has<constraints::primary_key_t<>>()){
                        auto it = std::find(compositeKeyColumnNames.begin(),
                                            compositeKeyColumnNames.end(),
                                            c.name);
                        if(it == compositeKeyColumnNames.end()){
                            typedef typename decltype(c)::field_type field_type;
                            const field_type *value = nullptr;
                            if(c.member_pointer){
                                value = &(o.*c.member_pointer);
                            }else{
                                value = &((o).*(c.getter))();
                            }
                            statement_binder<field_type>().bind(stmt, index++, *value);
                        }
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    res = int(sqlite3_last_insert_rowid(db));
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
        
        template<class It>
        void insert_range(It from, It to) {
            typedef typename std::iterator_traits<It>::value_type O;
            this->assert_mapped_type<O>();
            if(from == to) {
                return;
            }
            
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            auto &impl = get_impl<O>();
            
            std::stringstream ss;
            ss << "INSERT INTO '" << impl.table.name << "' (";
            std::vector<std::string> columnNames;
            impl.table.for_each_column([&] (auto c) {
                if(!c.template has<constraints::primary_key_t<>>()) {
                    columnNames.emplace_back(c.name);
                }
            });
            
            auto columnNamesCount = columnNames.size();
            for(size_t i = 0; i < columnNamesCount; ++i) {
                ss << "\"" << columnNames[i] << "\"";
                if(i < columnNamesCount - 1) {
                    ss << ", ";
                }else{
                    ss << ") ";
                }
            }
            ss << "VALUES ";
            auto valuesString = [columnNamesCount]{
                std::stringstream ss;
                ss << "(";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "?";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                return ss.str();
            }();
            auto valuesCount = static_cast<int>(std::distance(from, to));
            for(auto i = 0; i < valuesCount; ++i) {
                ss << valuesString;
                if(i < valuesCount - 1) {
                    ss << ",";
                }
                ss << " ";
            }
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                auto index = 1;
                for(auto it = from; it != to; ++it) {
                    auto &o = *it;
                    impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                        if(!c.template has<constraints::primary_key_t<>>()){
                            typedef typename decltype(c)::field_type field_type;
                            const field_type *value = nullptr;
                            if(c.member_pointer){
                                value = &(o.*c.member_pointer);
                            }else{
                                value = &((o).*(c.getter))();
                            }
                            statement_binder<field_type>().bind(stmt, index++, *value);
                        }
                    });
                }
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        void drop_index(const std::string &indexName) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::stringstream ss;
            ss << "DROP INDEX '" << indexName + "'";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
    protected:
        
        void drop_table_internal(const std::string &tableName, sqlite3 *db) {
            std::stringstream ss;
            ss << "DROP TABLE '" << tableName + "'";
            auto query = ss.str();
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                statement_finalizer finalizer{stmt};
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }else {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
    public:
        
        /**
         *  Drops table with given name.
         */
        void drop_table(const std::string &tableName) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            this->drop_table_internal(tableName, db);
        }
        
        /**
         *  sqlite3_changes function.
         */
        int changes() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_changes(db);
        }
        
        /**
         *  sqlite3_total_changes function.
         */
        int total_changes() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_total_changes(db);
        }
        
        int64 last_insert_rowid() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_last_insert_rowid(db);
        }
        
        /**
         *  Returns libsqltie3 lib version, not sqlite_orm
         */
        std::string libversion() {
            return sqlite3_libversion();
        }
        
    protected:
        
        template<class ...Tss, class ...Cols>
        sync_schema_result sync_table(storage_impl<internal::index_t<Cols...>, Tss...> *impl, sqlite3 *db, bool) {
            auto res = sync_schema_result::already_in_sync;
            std::stringstream ss;
            ss << "CREATE ";
            if(impl->table.unique){
                ss << "UNIQUE ";
            }
            typedef typename decltype(impl->table)::columns_type columns_type;
            typedef typename std::tuple_element<0, columns_type>::type head_t;
            typedef typename internal::table_type<head_t>::type indexed_type;
            ss << "INDEX IF NOT EXISTS " << impl->table.name << " ON '" << this->impl.template find_table_name<indexed_type>() << "' ( ";
            std::vector<std::string> columnNames;
            tuple_helper::iterator<std::tuple_size<columns_type>::value - 1, Cols...>()(impl->table.columns, [&columnNames, this](auto &v){
                columnNames.push_back(this->impl.column_name(v));
            });
            for(size_t i = 0; i < columnNames.size(); ++i) {
                ss << columnNames[i];
                if(i < columnNames.size() - 1) {
                    ss << ",";
                }
                ss << " ";
            }
            ss << ") ";
            auto query = ss.str();
            auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
        
        template<class ...Tss, class ...Cs>
        sync_schema_result sync_table(storage_impl<table_t<Cs...>, Tss...> *impl, sqlite3 *db, bool preserve) {
            auto res = sync_schema_result::already_in_sync;
            
            auto schema_stat = impl->schema_status(db, preserve);
            if(schema_stat != decltype(schema_stat)::already_in_sync) {
                if(schema_stat == decltype(schema_stat)::new_table_created) {
                    this->create_table(db, impl->table.name, impl);
                    res = decltype(res)::new_table_created;
                }else{
                    if(schema_stat == sync_schema_result::old_columns_removed ||
                       schema_stat == sync_schema_result::new_columns_added ||
                       schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {
                        
                        //  get table info provided in `make_table` call..
                        auto storageTableInfo = impl->table.get_table_info();
                        
                        //  now get current table info from db using `PRAGMA table_info` query..
                        auto dbTableInfo = impl->get_table_info(impl->table.name, db);
                        
                        //  this vector will contain pointers to columns that gotta be added..
                        std::vector<table_info*> columnsToAdd;
                        
                        impl->get_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);
                        
                        
                        if(schema_stat == sync_schema_result::old_columns_removed) {
                            
                            //  extra table columns than storage columns
                            this->backup_table(db, impl);
                            res = decltype(res)::old_columns_removed;
                        }
                        
                        if(schema_stat == sync_schema_result::new_columns_added) {
                            for(auto columnPointer : columnsToAdd) {
                                impl->add_column(*columnPointer, db);
                            }
                            res = decltype(res)::new_columns_added;
                        }
                        
                        if(schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {
                            
                            //remove extra columns
                            this->backup_table(db, impl);
                            for(auto columnPointer : columnsToAdd) {
                                impl->add_column(*columnPointer, db);
                            }
                            res = decltype(res)::new_columns_added_and_old_columns_removed;
                        }
                    } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                        this->drop_table_internal(impl->table.name, db);
                        this->create_table(db, impl->table.name, impl);
                        res = decltype(res)::dropped_and_recreated;
                    }
                }
            }
            return res;
        }
        
    public:
        
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
         *  Warning: sync_schema doesn't check foreign keys cause it is unable to do so in sqlite3. If you know how to get foreign key info
         *  please submit an issue https://github.com/fnc12/sqlite_orm/issues
         *  @return std::map with std::string key equal table name and `sync_schema_result` as value. `sync_schema_result` is a enum value that stores 
         *  table state after syncing a schema. `sync_schema_result` can be printed out on std::ostream with `operator<<`.
         */
        std::map<std::string, sync_schema_result> sync_schema(bool preserve = false) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
//            return this->impl.sync_schema(db, preserve);
            std::map<std::string, sync_schema_result> result;
            this->impl.for_each([&result, db, preserve, this](auto impl){
                auto res = this->sync_table(impl, db, preserve);
                result.insert({impl->table.name, res});
            });
            return result;
        }
        
        /**
         *  This function returns the same map that `sync_schema` returns but it
         *  doesn't perform `sync_schema` actually - just simulates it in case you want to know
         *  what will happen if you sync your schema.
         */
        std::map<std::string, sync_schema_result> sync_schema_simulate(bool preserve = false) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::map<std::string, sync_schema_result> result;
            this->impl.for_each([&result, db, preserve](auto impl){
                result.insert({impl->table.name, impl->schema_status(db, preserve)});
            });
            return result;
        }
        
        bool transaction(std::function<bool()> f) {
            this->begin_transaction();
            auto db = this->currentTransaction->get_db();
            auto shouldCommit = f();
            if(shouldCommit){
                this->impl.commit(db);
            }else{
                this->impl.rollback(db);
            }
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
            return shouldCommit;
        }
        
        void begin_transaction() {
            if(!inMemory){
                if(this->currentTransaction) throw std::runtime_error("cannot start a transaction within a transaction");
                this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                this->on_open_internal(this->currentTransaction->get_db());
            }
            auto db = this->currentTransaction->get_db();
            impl.begin_transaction(db);
        }
        
        void commit() {
            if(!inMemory){
                if(!this->currentTransaction) throw std::runtime_error("cannot commit - no transaction is active");
            }
            auto db = this->currentTransaction->get_db();
            this->impl.commit(db);
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
        }
        
        void rollback() {
            if(!inMemory){
                if(!this->currentTransaction) throw std::runtime_error("cannot rollback - no transaction is active");
            }
            auto db = this->currentTransaction->get_db();
            this->impl.rollback(db);
            if(!inMemory){
                this->currentTransaction = nullptr;
            }
        }
        
        std::string current_timestamp() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return impl.current_timestamp(db);
        }
        
    protected:
        
#if SQLITE_VERSION_NUMBER >= 3006019
        
        void foreign_keys(sqlite3 *db, bool value) {
            std::stringstream ss;
            ss << "PRAGMA foreign_keys = " << value;
            auto query = ss.str();
            auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
        bool foreign_keys(sqlite3 *db) {
            std::string query = "PRAGMA foreign_keys";
            auto res = false;
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **) -> int {
                                       auto &res = *(bool*)data;
                                       if(argc){
                                           res = row_extrator<bool>().extract(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
        
#endif
        
    public:
        
        int user_version() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::string query = "PRAGMA user_version";
            int res = -1;
            auto rc = sqlite3_exec(db,
                                   query.c_str(),
                                   [](void *data, int argc, char **argv,char **) -> int {
                                       auto &res = *(int*)data;
                                       if(argc){
                                           res = row_extrator<int>().extract(argv[0]);
                                       }
                                       return 0;
                                   }, &res, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return res;
        }
        
        void user_version(int value) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            std::stringstream ss;
            ss << "PRAGMA user_version = " << value;
            auto query = ss.str();
            auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
            if(rc != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
        }
        
#if SQLITE_VERSION_NUMBER >= 3007010
        /**
        * \fn db_release_memory
        * \brief Releases freeable memory of database. It is function can/should be called periodically by application,
        * if application has less memory usage constraint.
        * \note sqlite3_db_release_memory added in 3.7.10 https://sqlite.org/changes.html
        */
        int db_release_memory() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return sqlite3_db_release_memory(db);
        }
#endif         
        
        /**
         *  Checks whether table exists in db. Doesn't check storage itself - works only with actual database.
         *  Note: table can be not mapped to a storage
         *  @return true if table with a given name exists in db, false otherwise.
         */
        bool table_exists(const std::string &tableName) {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }
            return this->impl.table_exists(tableName, db);
        }

        /**
         *  Returns existing permanent table names in database. Doesn't check storage itself - works only with actual database.
         *  @return Returns list of tables in database.
         */
        std::vector<std::string> table_names() {
            std::shared_ptr<internal::database_connection> connection;
            sqlite3 *db;
            if(!this->currentTransaction){
                connection = std::make_shared<internal::database_connection>(this->filename);
                db = connection->get_db();
                this->on_open_internal(db);
            }else{
                db = this->currentTransaction->get_db();
            }

            std::vector<std::string> tableNames;
            std::string sql = std::string("SELECT name FROM sqlite_master WHERE type='table'");
            typedef std::vector<std::string> Data;
            int res = sqlite3_exec(db, sql.c_str(),
                                   [] (void *data, int argc, char **argv, char **/*columnName*/) -> int {
//                                       (void)columnName;
                                       Data& tableNames = *(Data*)data;
                                       for(int i = 0; i < argc; i++) {
                                           if(argv[i]){
                                               tableNames.push_back(argv[i]);
                                           }                
                                       }
                                       return 0;
                                   }, &tableNames,nullptr);

            if(res != SQLITE_OK) {
                auto msg = sqlite3_errmsg(db);
                throw std::runtime_error(msg);
            }
            return tableNames;
        }        
        
        
    protected:
        std::string filename;
        impl_type impl;
        std::shared_ptr<internal::database_connection> currentTransaction;
        const bool inMemory;
    };
    
    template<class ...Ts>
    storage_t<Ts...> make_storage(const std::string &filename, Ts ...tables) {
        return {filename, storage_impl<Ts...>(tables...)};
    }
    
}

#if defined(_MSC_VER)
# if defined(__RESTORE_MIN__)
__pragma(pop_macro("min"))
# undef __RESTORE_MIN__
# endif
# if defined(__RESTORE_MAX__)
__pragma(pop_macro("max"))
# undef __RESTORE_MAX__
# endif
#endif // defined(_MSC_VER)

#endif /* sqlite_orm_h */
