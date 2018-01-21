
#ifndef dev_sqlite_orm_h
#define dev_sqlite_orm_h

#include <string>
#include <stdexcept>
#include <vector>   //  std::vector
#include <sstream>  //  std::stringstream
#include <algorithm>    //  std::find
#include <memory>   //  std::shared_ptr, std::unique_ptr
#include <typeinfo>
#include <map>
#include <cctype>
#include <initializer_list>
#include <set>  //  std::set
#include <functional>   //  std::function
#include <ostream>  //  std::ostream
#include <iterator> //  std::iterator_traits

#include "conditions/conditions.h"
#include "internal/constraints/constraints.h"
#include "tuple_helper/tuple_helper.h"
#include "type_printer.h"
#include "internal/collate_argument.h"
#include "internal/constraints/constraints.h"
#include "internal/sqlite_type.h"
#include "type_is_nullable.h"
#include "internal/default_value_extractor.h"
#include "internal/column.h"
#include "field_printer.h"
#include "internal/join_iterator.h"
#include "core_functions/core_functions.h"
#include "aggregate_functions/aggregate_functions.h"
#include "internal/internal.h"
#include "internal/table.h"
#include "statement_binder.h"
#include "row_extractor.h"
#include "sync_schema_result.h"
#include "internal/index.h"
#include "not_found_exception.h"
#include "internal/storage_impl.h"
#include "internal/column_result.h"
#include "internal/storage.h"

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
    
#if SQLITE_VERSION_NUMBER >= 3006019
    
    /**
     *  FOREIGN KEY constraint construction function that takes member pointer as argument
     *  Available in SQLite 3.6.19 or higher
     */
    template<class O, class F>
    internal::constraints::foreign_key_intermediate_t<F O::*> foreign_key(F O::*m) {
        return {m};
    }
    
    /**
     *  FOREIGN KEY constraint construction function that takes getter function pointer as argument
     *  Available in SQLite 3.6.19 or higher
     */
    template<class O, class F>
    internal::constraints::foreign_key_intermediate_t<const F& (O::*)() const> foreign_key(const F& (O::*getter)() const) {
        using ret_type = internal::constraints::foreign_key_intermediate_t<const F& (O::*)() const>;
        return ret_type(getter);
    }
    
    /**
     *  FOREIGN KEY constraint construction function that takes setter function pointer as argument
     *  Available in SQLite 3.6.19 or higher
     */
    template<class O, class F>
    internal::constraints::foreign_key_intermediate_t<void (O::*)(F)> foreign_key(void (O::*setter)(F)) {
        return {setter};
    }
#endif
    
    /**
     *  UNIQUE constraint builder function.
     */
    inline internal::constraints::unique_t unique() {
        return {};
    }
    
    inline internal::constraints::autoincrement_t autoincrement() {
        return {};
    }
    
    template<class ...Cs>
    inline internal::constraints::primary_key_t<Cs...> primary_key(Cs ...cs) {
        using ret_type = internal::constraints::primary_key_t<Cs...>;
        return ret_type(std::make_tuple(cs...));
    }
    
    template<class T>
    internal::constraints::default_t<T> default_value(T t) {
        return {t};
    }
    
    inline internal::constraints::collate_t collate_nocase() {
        return {internal::collate_argument::nocase};
    }
    
    inline internal::constraints::collate_t collate_binary() {
        return {internal::collate_argument::binary};
    }
    
    inline internal::constraints::collate_t collate_rtrim() {
        return {internal::collate_argument::rtrim};
    }
    
    template<class T>
    internal::expression_t<T> c(T t) {
        using result_type = internal::expression_t<T>;
        return result_type(t);
    }
    
    template<class L, class R>
    internal::conc_t<L, R> conc(L l, R r) {
        return {l, r};
    }
    
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
    conditions::group_by_t<Args...> group_by(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    template<class A, class T>
    conditions::between_t<A, T> between(A expr, T b1, T b2) {
        return {expr, b1, b2};
    }
    
    template<class A, class T>
    conditions::like_t<A, T> like(A a, T t) {
        return {a, t};
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
    core_functions::char_t_<Args...> char_(Args&& ...args) {
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
        using result_type = core_functions::length_t<T>;
        return result_type(t);
    }
    
    template<class T>
    core_functions::abs_t<T> abs(T t) {
        using result_type = core_functions::abs_t<T>;
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
    
    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {t};
    }
    
    template<class T>
    internal::all_t<T> all(T t) {
        return {t};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> distinct(internal::columns_t<Args...> cols) {
        cols.distinct = true;
        return cols;
    }
    
    template<class ...Args>
    internal::set_t<Args...> set(Args&& ...args) {
        return {std::forward<Args>(args)...};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> columns(Args&& ...args) {
        return {std::forward<Args>(args)...};
    }
    
    /**
     *  Function used for table creation. Do not use table constructor - use this function
     *  cause table class is templated and its constructing too (just like std::make_shared or std::make_pair).
     */
    template<class ...Cs>
    internal::table_t<Cs...> make_table(const std::string &name, Cs&& ...args) {
        return {name, internal::table_impl<Cs...>(std::forward<Cs>(args)...)};
    }
    
    template<class ...Cols>
    internal::index_t<Cols...> make_index(const std::string &name, Cols ...cols) {
        return {name, false, std::make_tuple(cols...)};
    }
    
    template<class ...Cols>
    internal::index_t<Cols...> make_unique_index(const std::string &name, Cols ...cols) {
        return {name, true, std::make_tuple(cols...)};
    }
    
    template<class ...Ts>
    internal::storage_t<Ts...> make_storage(const std::string &filename, Ts ...tables) {
        return {filename, internal::storage_impl<Ts...>(tables...)};
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

#endif /* dev_sqlite_orm_h */

