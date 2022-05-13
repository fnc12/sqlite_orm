#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::default_t;
using internal::is_primary_key;
using internal::mpl_tuple_has_template;
using internal::mpl_tuple_has_trait;
using internal::mpl_tuple_has_type;
using internal::primary_key_t;

TEST_CASE("has_some_type") {
    using empty_tuple_type = std::tuple<>;
    using tuple_type = std::tuple<int, char, default_t<int>, primary_key_t<>, std::string>;

    STATIC_REQUIRE(mpl::invoke_t<internal::mpl_tuple_has_trait<is_primary_key>, tuple_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<internal::mpl_tuple_has_type<int>, tuple_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<internal::mpl_tuple_has_template<default_t>, tuple_type>::value);
    STATIC_REQUIRE(!mpl::invoke_t<internal::mpl_tuple_has_template<std::shared_ptr>, tuple_type>::value);
    STATIC_REQUIRE(!mpl::invoke_t<internal::mpl_tuple_has_template<default_t>, empty_tuple_type>::value);
}
