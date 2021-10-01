#pragma once

#include <tuple>  //  std::tuple
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::true_type, std::false_type, std::is_same, std::enable_if, std::is_member_pointer, std::is_member_function_pointer

#include "type_is_nullable.h"
#include "tuple_helper/tuple_helper.h"
#include "default_value_extractor.h"
#include "constraints.h"
#include "member_traits/member_traits.h"

namespace sqlite_orm {

    namespace internal {

        struct column_base {

            /**
             *  Column name. Specified during construction in `make_column`.
             */
            const std::string name;
        };

        /**
         *  This class stores single column info. column_t is a pair of [column_name:member_pointer] mapped to a storage
         *  O is a mapped class, e.g. User
         *  T is a mapped class'es field type, e.g. &User::name
         *  Op... is a constraints pack, e.g. primary_key_t, autoincrement_t etc
         */
        template<class O, class T, class G /* = const T& (O::*)() const*/, class S /* = void (O::*)(T)*/, class... Op>
        struct column_t : column_base {
            using object_type = O;
            using field_type = T;
            using constraints_type = std::tuple<Op...>;
            using member_pointer_t = field_type object_type::*;
            using getter_type = G;
            using setter_type = S;

            /**
             *  Member pointer used to read/write member
             */
            member_pointer_t member_pointer /* = nullptr*/;

            /**
             *  Getter member function pointer to get a value. If member_pointer is null than
             *  `getter` and `setter` must be not null
             */
            getter_type getter /* = nullptr*/;

            /**
             *  Setter member function
             */
            setter_type setter /* = nullptr*/;

            /**
             *  Constraints tuple
             */
            constraints_type constraints;

            column_t(std::string name_,
                     member_pointer_t member_pointer_,
                     getter_type getter_,
                     setter_type setter_,
                     constraints_type constraints_) :
                column_base{std::move(name_)},
                member_pointer(member_pointer_), getter(getter_), setter(setter_), constraints(move(constraints_)) {}

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

            template<class O1, class O2, class... Opts>
            constexpr bool has_every() const {
                if(has<O1>() && has<O2>()) {
                    return true;
                } else {
                    return has_every<Opts...>();
                }
            }

            template<class O1>
            constexpr bool has_every() const {
                return has<O1>();
            }

            /**
             *  Simplified interface for `DEFAULT` constraint
             *  @return string representation of default value if it exists otherwise nullptr
             */
            std::unique_ptr<std::string> default_value() const {
                std::unique_ptr<std::string> res;
                iterate_tuple(this->constraints, [&res](auto& v) {
                    auto dft = internal::default_value_extractor()(v);
                    if(dft) {
                        res = std::move(dft);
                    }
                });
                return res;
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
                typename std::enable_if<(tuple_helper::tuple_contains_type<
                                         primary_key_t<>,
                                         typename column_t<O, T, Op...>::constraints_type>::value)>::type> {
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
                typename std::enable_if<(tuple_helper::tuple_contains_type<
                                         primary_key_t<>,
                                         typename column_t<O, T, Op...>::constraints_type>::value)>::type> {
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
    template<class O,
             class T,
             typename = typename std::enable_if<!std::is_member_function_pointer<T O::*>::value>::type,
             class... Op>
    internal::column_t<O, T, const T& (O::*)() const, void (O::*)(T), Op...>
    make_column(const std::string& name, T O::*m, Op... constraints) {
        static_assert(internal::template constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        static_assert(internal::is_field_member_pointer<T O::*>::value,
                      "second argument expected as a member field pointer, not member function pointer");
        return {name, m, nullptr, nullptr, std::make_tuple(constraints...)};
    }

    /**
     *  Column builder function with setter and getter. You should use it to create columns instead of constructor
     */
    template<class G,
             class S,
             typename = typename std::enable_if<internal::is_getter<G>::value>::type,
             typename = typename std::enable_if<internal::is_setter<S>::value>::type,
             class... Op>
    internal::column_t<typename internal::setter_traits<S>::object_type,
                       typename internal::setter_traits<S>::field_type,
                       G,
                       S,
                       Op...>
    make_column(const std::string& name, S setter, G getter, Op... constraints) {
        static_assert(std::is_same<typename internal::setter_traits<S>::field_type,
                                   typename internal::getter_traits<G>::field_type>::value,
                      "Getter and setter must get and set same data type");
        static_assert(internal::template constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }

    /**
     *  Column builder function with getter and setter (reverse order). You should use it to create columns instead of
     * constructor
     */
    template<class G,
             class S,
             typename = typename std::enable_if<internal::is_getter<G>::value>::type,
             typename = typename std::enable_if<internal::is_setter<S>::value>::type,
             class... Op>
    internal::column_t<typename internal::setter_traits<S>::object_type,
                       typename internal::setter_traits<S>::field_type,
                       G,
                       S,
                       Op...>
    make_column(const std::string& name, G getter, S setter, Op... constraints) {
        static_assert(std::is_same<typename internal::setter_traits<S>::field_type,
                                   typename internal::getter_traits<G>::field_type>::value,
                      "Getter and setter must get and set same data type");
        static_assert(internal::template constraints_size<Op...>::value == std::tuple_size<std::tuple<Op...>>::value,
                      "Incorrect constraints pack");
        return {name, nullptr, getter, setter, std::make_tuple(constraints...)};
    }

}
