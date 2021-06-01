#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("storage traits") {
    SECTION("table_types") {
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
        auto uniqueC = sqlite_orm::unique(&Table::a, &Table::b, &Table::c);

        using Column1 = decltype(column1);
        using Column2 = decltype(column2);
        using Column3 = decltype(column3);
        using Column4 = decltype(column4);
        using UniqueC = decltype(uniqueC);

        auto table = make_table("table", column1, column2, column3, column4, uniqueC);
        using TableT = decltype(table);
        using TableTypes = internal::storage_traits::table_types<TableT>;

        using ArgsTuple = TableTypes::args_tuple;
        using ExpectedArgsTuple = std::tuple<Column1, Column2, Column3, Column4, UniqueC>;
        static_assert(std::is_same<ArgsTuple, ExpectedArgsTuple>::value, "");

        using ColumnsTuple = TableTypes::columns_tuple;
        using ExpectedColumnsTuple = std::tuple<Column1, Column2, Column3, Column4>;
        static_assert(std::is_same<ColumnsTuple, ExpectedColumnsTuple>::value, "");

        using ResultType = TableTypes::type;
        using ExpectedResultType = std::tuple<int64_t, std::string, std::string, std::string>;
        static_assert(std::is_same<ResultType, ExpectedResultType>::value, "");
    }
}
