#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::check_if_has;
using internal::check_if_has_template;
using internal::check_if_has_type;
using internal::default_t;
using internal::is_primary_key;
using internal::primary_key_t;

TEST_CASE("tuple traits") {
    using empty_tuple_type = std::tuple<>;
    using tuple_type = std::tuple<int, char, default_t<int>, primary_key_t<>, std::string>;

    STATIC_REQUIRE(mpl::invoke_t<check_if_has<is_primary_key>, tuple_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_has_type<int>, tuple_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_has_template<default_t>, tuple_type>::value);
    STATIC_REQUIRE_FALSE(mpl::invoke_t<check_if_has_template<std::shared_ptr>, tuple_type>::value);
    STATIC_REQUIRE_FALSE(mpl::invoke_t<check_if_has_template<default_t>, empty_tuple_type>::value);

    STATIC_REQUIRE(internal::tuple_has<tuple_type, is_primary_key>::value);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, char>::value == 1);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, char*, std::add_pointer_t>::value == 1);
    STATIC_REQUIRE(internal::find_tuple_type<tuple_type, double>::value == std::tuple_size<tuple_type>::value);
    STATIC_REQUIRE(internal::count_tuple<tuple_type, is_primary_key>::value == 1);
}
