#pragma once

/** @file Closed accessors of the object a field-level holder type holds.
 *
 *  A holder hands out an object as is, owned by a `std::unique_ptr` or held by a `std::optional`;
 *  a get statement's result type is one. These are keyed on a raw C++ type, not on a DSL node,
 *  which is what separates them from the node accessors in `accessors.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr, std::make_unique
#include <optional>  //  std::optional
#endif

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    /*
     *  The object held by a holder type:
     *  the object owned by a `std::unique_ptr` or held by a `std::optional`, or the object itself.
     */
    template<class Holder>
    struct held_object : polyfill::type_identity<Holder> {};

    template<class O>
    struct held_object<std::unique_ptr<O>> : polyfill::type_identity<O> {};

    template<class O>
    struct held_object<std::optional<O>> : polyfill::type_identity<O> {};

    template<class Holder>
    using held_object_t = typename held_object<Holder>::type;

    /*
     *  Put a default-constructed object into the given holder - owned by a `std::unique_ptr`,
     *  held by a `std::optional`, or the holder itself -, and return a reference to it.
     */
    template<class Holder>
    held_object_t<Holder>& emplace_held_object(Holder& holder) {
        if constexpr (polyfill::is_specialization_of_v<Holder, std::unique_ptr>) {
            holder = std::make_unique<held_object_t<Holder>>();
            return *holder;
        } else if constexpr (polyfill::is_specialization_of_v<Holder, std::optional>) {
            return holder.emplace();
        } else {
            return holder;
        }
    }
}
