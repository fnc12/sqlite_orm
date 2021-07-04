#pragma once

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct is_field_member_pointer : std::false_type {};

        template<class T>
        struct is_field_member_pointer<T,
                                       typename std::enable_if<std::is_member_pointer<T>::value &&
                                                               !std::is_member_function_pointer<T>::value>::type>
            : std::true_type {};

        template<class T, class SFINAE = void>
        struct field_member_traits;

        template<class O, class F>
        struct field_member_traits<F O::*, typename std::enable_if<is_field_member_pointer<F O::*>::value>::type> {
            using object_type = O;
            using field_type = F;
        };

        /**
     *  Getters aliases
     */
        template<class O, class T>
        using getter_by_value_const = T (O::*)() const;

        template<class O, class T>
        using getter_by_value = T (O::*)();

        template<class O, class T>
        using getter_by_ref_const = T& (O::*)() const;

        template<class O, class T>
        using getter_by_ref = T& (O::*)();

        template<class O, class T>
        using getter_by_const_ref_const = const T& (O::*)() const;

        template<class O, class T>
        using getter_by_const_ref = const T& (O::*)();

        /**
     *  Setters aliases
     */
        template<class O, class T>
        using setter_by_value = void (O::*)(T);

        template<class O, class T>
        using setter_by_ref = void (O::*)(T&);

        template<class O, class T>
        using setter_by_const_ref = void (O::*)(const T&);

        template<class T>
        struct is_getter : std::false_type {};

        template<class O, class T>
        struct is_getter<getter_by_value_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_value<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref<O, T>> : std::true_type {};

        template<class T>
        struct is_setter : std::false_type {};

        template<class O, class T>
        struct is_setter<setter_by_value<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_ref<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_const_ref<O, T>> : std::true_type {};

        template<class T>
        struct getter_traits;

        template<class O, class T>
        struct getter_traits<getter_by_value_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class T>
        struct setter_traits;

        template<class O, class T>
        struct setter_traits<setter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class T, class SFINAE = void>
        struct member_traits;

        template<class T>
        struct member_traits<T, typename std::enable_if<is_field_member_pointer<T>::value>::type> {
            using object_type = typename field_member_traits<T>::object_type;
            using field_type = typename field_member_traits<T>::field_type;
        };

        template<class T>
        struct member_traits<T, typename std::enable_if<is_getter<T>::value>::type> {
            using object_type = typename getter_traits<T>::object_type;
            using field_type = typename getter_traits<T>::field_type;
        };

        template<class T>
        struct member_traits<T, typename std::enable_if<is_setter<T>::value>::type> {
            using object_type = typename setter_traits<T>::object_type;
            using field_type = typename setter_traits<T>::field_type;
        };
    }
}
