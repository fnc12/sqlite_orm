#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::asterisk_t;
using internal::count_tuple;
using internal::default_t;
using internal::find_tuple_type;
using internal::is_primary_key;
using internal::primary_key_t;
using internal::tuple_has;
using internal::tuple_has_template;
using internal::tuple_has_type;

TEST_CASE("tuple traits") {
    using empty_tuple_type = std::tuple<>;
    using tuple_type = std::tuple<int, char, default_t<int>, primary_key_t<>, std::string>;

    STATIC_REQUIRE(internal::tuple_has<tuple_type, is_primary_key>::value);
    STATIC_REQUIRE_FALSE(internal::tuple_has<tuple_type, std::is_null_pointer>::value);
    STATIC_REQUIRE(internal::tuple_has_type<tuple_type, char>::value);
    STATIC_REQUIRE(internal::tuple_has_type<tuple_type, char*, std::add_pointer_t>::value);
    STATIC_REQUIRE_FALSE(internal::tuple_has_type<tuple_type, double>::value);
    STATIC_REQUIRE(internal::tuple_has_template<tuple_type, default_t>::value);
    STATIC_REQUIRE_FALSE(internal::tuple_has_template<tuple_type, asterisk_t>::value);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, char>::value == 1);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, char*, std::add_pointer_t>::value == 1);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, double>::value == std::tuple_size<tuple_type>::value);
    STATIC_REQUIRE(internal::find_tuple_template<tuple_type, default_t>::value == 2);
    STATIC_REQUIRE(internal::count_tuple<tuple_type, is_primary_key>::value == 1);
}
