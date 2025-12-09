#pragma once
#pragma once
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <type_traits>  //  std::remove_const
#include <algorithm>
#include <utility>
#include <memory>
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cstring_literal.h"
#include "../statement_binding_traits.h"

namespace sqlite_orm::internal {
    template<class T>
    using access_bindable_t = polyfill::detected_or_t<T, type_t, T>;

    template<class T>
    T& access_bindable(T& t) {
        return t;
    }
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*  
     *  @note Named SQL parameters can be used multiple times in an SQL statement, which is the reason that we use shared_ptr here.
     */
    template<class Moniker, class T>
    struct named_bindable {
        using name_constant_type = Moniker;
        using type = T;

        std::shared_ptr<type> value;

        template<class U = type>
        void operator=(U&& other) {
            *value = std::forward<U>(other);
        }
    };

    template<class Moniker>
    struct parameter_moniker : Moniker {
        using name_constant_type = Moniker;

        /*  
         *  Create a named SQL parameter argument of type T.
         */
        template<class T, class... Args>
        [[nodiscard]] constexpr named_bindable<Moniker, T> create(Args&&... args) const {
            return {std::make_shared<T>(std::forward<Args>(args)...)};
        }
    };

    template<class Moniker, class T>
    T& access_bindable(const named_bindable<Moniker, T>& bindable) {
        return *bindable.value;
    }

    template<class T>
    using name_constant_type_t = typename T::name_constant_type;

    template<class T>
    using name_constant_type_or_none_t = polyfill::detected_t<name_constant_type_t, T>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline namespace literals {
        /*  
         *  Make a moniker for a named parameter from a string literal.
         *  E.g. "myparam"_param
         */
        template<internal::cstring_literal moniker>
        [[nodiscard]] consteval auto operator"" _param() {
            static_assert(moniker.cstr[0] == ':' || moniker.cstr[0] == '@');
            using name_constant_type = std::integral_constant<decltype(moniker), moniker>;
            return internal::parameter_moniker<name_constant_type>{};
        }
    }

    /** @short Specifies that a type is an integral constant C-string usable for a named parameter.
     */
    template<class T>
    concept orm_parameter_moniker =
        polyfill::is_specialization_of_v<std::remove_const_t<T>, internal::parameter_moniker>;
}
#endif
