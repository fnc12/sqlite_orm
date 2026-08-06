#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_standard_layout
#endif

namespace sqlite_orm::internal {
    template<class F, class O>
    size_t offsetof_member(F O::* member) {
        static_assert(std::is_standard_layout_v<O>);
        return &reinterpret_cast<const unsigned char&>(((O*)nullptr)->*member) - (const unsigned char*)nullptr;
    }

    template<class Nested, class Enclosing>
    const Enclosing* addressof_enclosing(const Nested* _this, Nested Enclosing::* member) {
        return reinterpret_cast<const Enclosing*>(reinterpret_cast<const unsigned char*>(_this) -
                                                  offsetof_member(member));
    }
}
