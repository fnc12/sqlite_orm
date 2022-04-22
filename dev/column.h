#pragma once

#include <tuple>  //  std::tuple
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::true_type, std::false_type, std::is_same, std::enable_if, std::decay

#include "cxx_polyfill.h"
#include "type_traits.h"
#include "type_is_nullable.h"
#include "tuple_helper/tuple_helper.h"
#include "constraints.h"
#include "member_traits/member_traits.h"

namespace sqlite_orm {

    namespace internal {

        struct basic_column {

            /**
             *  Column name. Specified during construction in `make_column()`.
             */
            const std::string name;
        };

        struct empty_setter {};

        template<class G, class S>
        struct field_access_closure {
            using member_pointer_t = G;
            using setter_type = S;

            /**
             *  Member pointer used to read a field value.
             *  If it is a object member pointer it is also used to write a field value.
             */
            const member_pointer_t member_pointer;

            /**
             *  Setter member function to write a field value
             */
            SQLITE_ORM_NOUNIQUEADDRESS
            const setter_type setter;
        };

        /**
         *  This class stores information about a single column.
         *  column_t is a pair of [column_name:member_pointer] mapped to a storage.
         *  
         *  O is a mapped class, e.g. User
         *  T is a mapped class'es field type, e.g. &User::name
         *  Op... is a constraints pack, e.g. primary_key_t, autoincrement_t etc
         */
        template<class O, class T, class G, class S, class... Op>
        struct column_t : basic_column, field_access_closure<G, S> {
            using object_type = O;
            using field_type = T;
            using constraints_type = std::tuple<Op...>;

            /**
             *  Constraints tuple
             */
            constraints_type constraints;

#if __cplusplus < 201703L  // C++14 or earlier
            column_t(std::string name, G memberPointer, S setter, std::tuple<Op...> op) :
                basic_column{move(name)}, field_access_closure<G, S>{memberPointer, setter}, constraints{move(op)} {}
#endif

            /**
             *  Simplified interface for `NOT NULL` constraint
             */
            bool not_null() const {
                return !type_is_nullable<field_type>::value;
            }

            template<class Opt>
            constexpr bool has() const {
                return tuple_helper::tuple_contains_type<Opt, constraints_type>::value;
            }

            /**
             *  Simplified interface for `DEFAULT` constraint
             *  @return string representation of default value if it exists otherwise nullptr
             */
            std::unique_ptr<std::string> default_value() const;

            bool is_generated() const {
#if SQLITE_VERSION_NUMBER >= 3031000
                auto res = false;
                iterate_tuple(this->constraints, [&res](auto& constraint) {
                    using constraint_type = std::decay_t<decltype(constraint)>;
                    if(!res) {
                        res = is_generated_always<constraint_type>::value;
                    }
                });
                return res;
#else
                return false;
#endif
            }
        };

        // we are compelled to wrap all sfinae-implemented traits to prevent "error: type/value mismatch at argument 2 in template parameter list"
        namespace sfinae {
            /**
             *  Column with insertable primary key traits. Common case.
             */
            template<class T, class SFINAE = void>
            struct is_column_with_insertable_primary_key : public std::false_type {};

            /**
             *  Column with insertable primary key traits. Specialized case case.
             */
            template<class O, class T, class... Op>
            struct is_column_with_insertable_primary_key<
                column_t<O, T, Op...>,
                std::enable_if_t<(
                    tuple_helper::tuple_contains_type<primary_key_t<>,
                                                      typename column_t<O, T, Op...>::constraints_type>::value)>> {
                using column_type = column_t<O, T, Op...>;
                static constexpr bool value = is_primary_key_insertable<column_type>::value;
            };

            /**
             *  Column with noninsertable primary key traits. Common case.
             */
            template<class T, class SFINAE = void>
            struct is_column_with_noninsertable_primary_key : public std::false_type {};

            /**
             *  Column with noninsertable primary key traits. Specialized case case.
             */
            template<class O, class T, class... Op>
            struct is_column_with_noninsertable_primary_key<
                column_t<O, T, Op...>,
                std::enable_if_t<(
                    tuple_helper::tuple_contains_type<primary_key_t<>,
                                                      typename column_t<O, T, Op...>::constraints_type>::value)>> {
                using column_type = column_t<O, T, Op...>;
                static constexpr bool value = !is_primary_key_insertable<column_type>::value;
            };

        }

        /**
         *  Column traits. Common case.
         */
        template<class T>
        struct is_column : public std::false_type {};

        /**
         *  Column traits. Specialized case case.
         */
        template<class O, class T, class... Op>
        struct is_column<column_t<O, T, Op...>> : public std::true_type {};

        /**
         *  Column with insertable primary key traits.
         */
        template<class T>
        struct is_column_with_insertable_primary_key : public sfinae::is_column_with_insertable_primary_key<T> {};

        /**
         *  Column with noninsertable primary key traits.
         */
        template<class T>
        struct is_column_with_noninsertable_primary_key : public sfinae::is_column_with_noninsertable_primary_key<T> {};

        template<class T>
        struct column_field_type {
            using type = void;
        };

        template<class O, class T, class... Op>
        struct column_field_type<column_t<O, T, Op...>> {
            using type = typename column_t<O, T, Op...>::field_type;
        };

        template<class T>
        struct column_constraints_type {
            using type = std::tuple<>;
        };

        template<class O, class T, class... Op>
        struct column_constraints_type<column_t<O, T, Op...>> {
            using type = typename column_t<O, T, Op...>::constraints_type;
        };

    }

    /**
     *  Column builder function. You should use it to create columns instead of constructor
     */
    template<class O, class T, internal::satisfies<std::is_member_object_pointer, T O::*> = true, class... Op>
    internal::column_t<O, T, T O::*, internal::empty_setter, Op...>
    make_column(std::string name, T O::*m, Op... constraints) {
        static_assert(internal::constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        return {move(name), m, {}, std::make_tuple(constraints...)};
    }

    /**
     *  Column builder function with setter and getter. You should use it to create columns instead of constructor
     */
    template<class G,
             class S,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true,
             class... Op>
    internal::column_t<typename internal::setter_traits<S>::object_type,
                       typename internal::setter_traits<S>::field_type,
                       G,
                       S,
                       Op...>
    make_column(std::string name, S setter, G getter, Op... constraints) {
        static_assert(std::is_same<typename internal::setter_traits<S>::field_type,
                                   typename internal::getter_traits<G>::field_type>::value,
                      "Getter and setter must get and set same data type");
        static_assert(internal::constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        return {move(name), getter, setter, std::make_tuple(constraints...)};
    }

    /**
     *  Column builder function with getter and setter (reverse order). You should use it to create columns instead of
     * constructor
     */
    template<class G,
             class S,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true,
             class... Op>
    internal::column_t<typename internal::setter_traits<S>::object_type,
                       typename internal::setter_traits<S>::field_type,
                       G,
                       S,
                       Op...>
    make_column(std::string name, G getter, S setter, Op... constraints) {
        static_assert(std::is_same<typename internal::setter_traits<S>::field_type,
                                   typename internal::getter_traits<G>::field_type>::value,
                      "Getter and setter must get and set same data type");
        static_assert(internal::constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        return {move(name), getter, setter, std::make_tuple(constraints...)};
    }
}
