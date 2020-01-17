#pragma once

#include <typeindex>  //  std::type_index

#include "getter_traits.h"
#include "error_code.h"

namespace sqlite_orm {

    namespace internal {

        struct member_pointer_info {
            using value_t = std::ptrdiff_t;

            enum class type {
                member,
                getter,
                setter,
            };

            template<class T>
            static type type_from_value(T value) {
                if(is_field_member_pointer<T>::value) {
                    return type::member;
                } else if(is_getter<T>::value) {
                    return type::getter;
                } else if(is_setter<T>::value) {
                    return type::setter;
                } else {
                    throw std::system_error(std::make_error_code(orm_error_code::unknown_member_value));
                }
            }

            template<class F, class T>
            static value_t value_from_member_pointer(F T::*member_pointer) {
                union {
                    F T::*pointer;
                    value_t value;
                } un;
                un.pointer = member_pointer;
                return un.value;
            }

            template<class F, class T>
            member_pointer_info(F T::*member) :
                type_index{typeid(T)},
                field_index{typeid(F)}, t{type_from_value(member)}, value{value_from_member_pointer(member)} {}

            const std::type_index type_index;
            const std::type_index field_index;
            const type t;
            const std::ptrdiff_t value;
        };

        inline bool operator==(const member_pointer_info &lhs, const member_pointer_info &rhs) {
            return lhs.type_index == rhs.type_index && lhs.field_index == rhs.field_index && lhs.t == rhs.t &&
                   lhs.value == rhs.value;
        }

        inline bool operator!=(const member_pointer_info &lhs, const member_pointer_info &rhs) {
            return !(lhs == rhs);
        }

    }

}
