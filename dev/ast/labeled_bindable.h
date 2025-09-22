#pragma once
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <utility>
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cstring_literal.h"
#include "../statement_binding_traits.h"

namespace sqlite_orm::internal {
    template<class T>
    using access_bindable_t = polyfill::detected_or_t<T, type_t, T>;
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    template<class Moniker, class T>
    struct labeled_bindable {
        using name_constant_type = Moniker;
        using type = T;

        type value;
    };

    template<class Moniker>
    struct bindable_label : Moniker {
        using name_constant_type = Moniker;
    };

    // note: not in use because AST iteration walks through to the value leaf
    template<class Moniker, class T>
    T& access_bindable(const labeled_bindable<Moniker, T>& t) = delete;

    template<class T>
    using name_constant_type_t = typename T::name_constant_type;

    template<class T>
    using name_constant_type_or_none_t = polyfill::detected_t<name_constant_type_t, T>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    // Intentionally place operators for types classified as arithmetic or general operator arguments in the internal namespace
    // to facilitate ADL (Argument Dependent Lookup)
    namespace internal {
        /*  
            Associates a bindable value with a moniker.
         */
        template<class T, class Moniker>
            requires is_bindable_v<T>
        constexpr labeled_bindable<Moniker, T> operator>>=(T bindable, const bindable_label<Moniker>&) {
            return {std::move(bindable)};
        }

        /*  
            Associates a referenced bindable value with a moniker.
         */
        template<class T, class Moniker>
            requires is_bindable_v<T>
        constexpr labeled_bindable<Moniker, T> operator>>=(std::reference_wrapper<T> bindable,
                                                           const bindable_label<Moniker>&) {
            return {bindable};
        }
    }

    inline namespace literals {
        /*  
         *  Make a label for a bindable from a string literal.
         *  E.g. "myparam"_bindable
         */
        template<internal::cstring_literal name>
        [[nodiscard]] consteval auto operator"" _bindable() {
            using name_constant_type = std::integral_constant<decltype(name), name>;
            return internal::bindable_label<name_constant_type>{};
        }
    }

    /** @short Specifies that a type is an integral constant C-string usable as a label for a bindable.
     */
    template<class T>
    concept orm_bindable_label = polyfill::is_specialization_of_v<T, internal::bindable_label>;
}
#endif
