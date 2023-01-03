#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("tuple_helper static") {
    SECTION("tuple_transformer") {
        struct Table {
            int64_t id;
            std::string a;
            std::string b;
            std::string c;
        };
        auto column1 = make_column("id", &Table::id);
        auto column2 = make_column("a", &Table::a);
        auto column3 = make_column("b", &Table::b);
        auto column4 = make_column("c", &Table::c);

        using Column1 = decltype(column1);
        using Column2 = decltype(column2);
        using Column3 = decltype(column3);
        using Column4 = decltype(column4);
        using ColumnsTuple = std::tuple<Column1, Column2, Column3, Column4>;
        using ColumnsMappedTypes = internal::transform_tuple_t<ColumnsTuple, internal::field_type_t>;
        using Expected = std::tuple<int64_t, std::string, std::string, std::string>;
        STATIC_REQUIRE(std::is_same<ColumnsMappedTypes, Expected>::value);
    }
}
