#pragma once

#include <type_traits>  //  std::is_member_object_pointer

namespace sqlite_orm {
    namespace internal {

        template<class T>
        using is_field_member_pointer = std::is_member_object_pointer<T>;
    }
}
