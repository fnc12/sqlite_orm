#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <memory>  //  std::default_delete

#if SQLITE_VERSION_NUMBER >= 3020000
using namespace sqlite_orm;
using internal::is_pointer_arg;
using internal::is_pointer_arg_v;
using internal::is_pointer_binding;
using internal::is_pointer_binding_v;

TEST_CASE("is_pointer_value") {
    using arg_type = carray_pointer_arg<int>;
    using binding_type = carray_pointer_binding<int, std::default_delete<int>>;
    using static_binding_type = static_carray_pointer_binding<int>;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    using null_binding_type = static_pointer_binding_t<std::nullptr_t, carray_pointer_tag>;
#else
    using null_binding_type = static_pointer_binding<std::nullptr_t, carray_pointer_type>;
#endif

    SECTION("pointer_arg") {
        STATIC_REQUIRE(is_pointer_arg_v<arg_type>);
        STATIC_REQUIRE(is_pointer_arg<arg_type>::value);
        STATIC_REQUIRE_FALSE(is_pointer_arg_v<binding_type>);
        STATIC_REQUIRE_FALSE(is_pointer_arg_v<int*>);
        STATIC_REQUIRE_FALSE(is_pointer_arg_v<std::nullptr_t>);
    }
    SECTION("pointer_binding") {
        STATIC_REQUIRE(is_pointer_binding_v<binding_type>);
        STATIC_REQUIRE(is_pointer_binding_v<static_binding_type>);
        STATIC_REQUIRE(is_pointer_binding_v<null_binding_type>);
        STATIC_REQUIRE(is_pointer_binding<binding_type>::value);
        STATIC_REQUIRE_FALSE(is_pointer_binding_v<arg_type>);
        STATIC_REQUIRE_FALSE(is_pointer_binding_v<int*>);
        STATIC_REQUIRE_FALSE(is_pointer_binding_v<std::nullptr_t>);
    }
}
#endif
