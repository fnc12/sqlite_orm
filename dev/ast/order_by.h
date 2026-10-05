#pragma once

/** @file The ORDER BY clause, in each of the DSL spellings sqlite_orm offers for it - a single ordering term,
 *        a static list of ordering terms, or a list assembled at runtime.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::is_base_of, std::disjunction
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple
#include <utility>  //  std::move, std::forward
#include <sstream>  //  std::stringstream
#include <ostream>  //  std::flush
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/type_traits.h"
#include "../builtin/collations.h"  //  collate_argument, collate_argument_to_string
#include "../serializer_context.h"
#include "../type_printer.h"
#include "../literal.h"
#include "../vocabulary/node_algorithms.h"  // is_statement_clause
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct order_by_base {
        std::string _collate_argument;
        int _order = 0;  //  -1 = desc, 1 = asc, 0 = unspecified
        int _nulls = 0;  //  1 = nulls first, -1 = nulls last, 0 = unspecified
    };

    struct order_by_string {
        operator std::string() const {
            return "ORDER BY";
        }
    };

    /**
     *  ORDER BY argument holder.
     */
    template<class O>
    struct order_by_t : order_by_base, order_by_string {
        using expression_type = O;

        expression_type _expression;

        order_by_t(expression_type expression) : order_by_base(), _expression(std::move(expression)) {}

        order_by_t asc() const {
            auto res = *this;
            res._order = 1;
            return res;
        }

        order_by_t desc() const {
            auto res = *this;
            res._order = -1;
            return res;
        }

#if SQLITE_VERSION_NUMBER >= 3030000
        order_by_t nulls_first() const {
            auto res = *this;
            res._nulls = 1;
            return res;
        }

        order_by_t nulls_last() const {
            auto res = *this;
            res._nulls = -1;
            return res;
        }
#endif

        order_by_t collate_binary() const {
            auto res = *this;
            res._collate_argument = collate_argument_to_string(collate_argument::binary);
            return res;
        }

        order_by_t collate_nocase() const {
            auto res = *this;
            res._collate_argument = collate_argument_to_string(collate_argument::nocase);
            return res;
        }

        order_by_t collate_rtrim() const {
            auto res = *this;
            res._collate_argument = collate_argument_to_string(collate_argument::rtrim);
            return res;
        }

        order_by_t collate(std::string name) const {
            auto res = *this;
            res._collate_argument = std::move(name);
            return res;
        }

        template<class C>
        order_by_t collate() const {
            std::stringstream ss;
            ss << C::name() << std::flush;
            return this->collate(ss.str());
        }
    };

    template<class T>
    constexpr bool is_order_by_v = polyfill::is_specialization_of_v<T, order_by_t>;

    /**
     *  ORDER BY pack holder.
     */
    template<class... Args>
    struct multi_order_by_t : order_by_string {
        using args_type = std::tuple<Args...>;

        args_type args;

        multi_order_by_t(args_type args_) : args{std::move(args_)} {}
    };

    template<class T>
    constexpr bool is_multi_order_by_v = polyfill::is_specialization_of_v<T, multi_order_by_t>;

    struct dynamic_order_by_entry_t : order_by_base {
        std::string name;

        dynamic_order_by_entry_t(decltype(name) name_, std::string collate_argument_, int asc_desc_, int nulls_) :
            order_by_base{std::move(collate_argument_), asc_desc_, nulls_}, name(std::move(name_)) {}
    };

    /**
     *  C - serializer context class
     */
    template<class C>
    struct dynamic_order_by_t : order_by_string {
        using context_t = C;
        using entry_t = dynamic_order_by_entry_t;
        using const_iterator = typename std::vector<entry_t>::const_iterator;

        dynamic_order_by_t(const context_t& context_) : context(context_) {}

        template<class T, satisfies<is_order_by, T> = true>
        void push_back(T orderBy) {
            auto newContext = this->context;
            newContext.omit_table_name = false;
            auto columnName = serialize(orderBy._expression, newContext);
            this->entries.emplace_back(std::move(columnName),
                                       std::move(orderBy._collate_argument),
                                       orderBy._order,
                                       orderBy._nulls);
        }

        const_iterator begin() const {
            return this->entries.begin();
        }

        const_iterator end() const {
            return this->entries.end();
        }

        void clear() {
            this->entries.clear();
        }

      protected:
        std::vector<entry_t> entries;
        context_t context;
    };

    template<class T>
    constexpr bool is_dynamic_order_by_v = polyfill::is_specialization_of_v<T, dynamic_order_by_t>;

    template<class T>
    constexpr bool is_any_order_by_v = std::disjunction_v<is_order_by<T>, is_multi_order_by<T>, is_dynamic_order_by<T>>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  ORDER BY column, column alias or expression
     *
     *  Examples:
     *  storage.select(&User::name, order_by(&User::id))
     *  storage.select(as<colalias_a>(&User::name), order_by(get<colalias_a>()))
     */
    template<class O, internal::satisfies_not<std::is_base_of, integer_printer, type_printer<O>> = true>
    internal::order_by_t<O> order_by(O o) {
        static_assert(!internal::is_statement_clause<O>::value,
                      "an ORDER BY term must be an expression, not a statement clause");
        return {std::move(o)};
    }

    /**
     *  ORDER BY positional ordinal
     *
     *  Examples:
     *  storage.select(&User::name, order_by(1))
     */
    template<class O, internal::satisfies<std::is_base_of, integer_printer, type_printer<O>> = true>
    internal::order_by_t<internal::literal_holder<O>> order_by(O o) {
        return {{std::move(o)}};
    }

    /**
     *  ORDER BY column1, column2
     *  Example: storage.get_all<Singer>(multi_order_by(order_by(&Singer::name).asc(), order_by(&Singer::gender).desc())
     */
    template<class... Args>
    internal::multi_order_by_t<Args...> multi_order_by(Args... args) {
        //  the grammar production is `ordering-term`, which is narrower than `expr`, hence a positive check
        static_assert((internal::is_order_by<Args>::value && ...),
                      "every argument of a multi ORDER BY must be an ORDER BY term");
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  ORDER BY column1, column2
     *  Difference from `multi_order_by` is that `dynamic_order_by` can be changed at runtime using `push_back` member
     *  function Example:
     *  auto orderBy = dynamic_order_by(storage);
     *  if(someCondition) {
     *    orderBy.push_back(&User::id);
     *  } else {
     *    orderBy.push_back(&User::name);
     *    orderBy.push_back(&User::birthDate);
     *  }
     */
    template<class S>
    internal::dynamic_order_by_t<internal::serializer_context<typename S::db_objects_type>>
    dynamic_order_by(const S& storage) {
        return {obtain_db_objects(storage)};
    }
}
