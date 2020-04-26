#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator check") {
    SECTION("greater than") {
        struct Table {
            int col1 = 0;
            std::string col2;
            int col3 = 0;
        };
        auto ch = check(greater_than(&Table::col3, 0));
        auto table = make_table("tablename",
                                make_column("col1", &Table::col1, primary_key()),
                                make_column("col2", &Table::col2),
                                make_column("col3", &Table::col3, ch));

        using storage_impl_t = internal::storage_impl<decltype(table)>;

        storage_impl_t storageImpl{table};

        using context_t = internal::serializator_context<storage_impl_t>;

        context_t context{storageImpl};
        std::string value;
        std::string expected;
        SECTION("with parentheses") {
            value = serialize(ch, context);
            expected = "CHECK (\"col3\" > 0)";
        }
        SECTION("without parentheses") {
            context.use_parentheses = false;
            value = serialize(ch, context);
            expected = "CHECK \"col3\" > 0";
        }
        REQUIRE(value == expected);
    }
    SECTION("lesser than") {
        struct Book {
            int id = 0;
            std::string name;
            std::string pubName;
            int price = 0;
        };
        auto ch = check(lesser_than(0, &Book::price));
        auto table = make_table("BOOK",
                                make_column("Book_id", &Book::id, primary_key()),
                                make_column("Book_name", &Book::name),
                                make_column("Pub_name", &Book::pubName),
                                make_column("PRICE", &Book::price, ch));
        using storage_impl_t = internal::storage_impl<decltype(table)>;

        storage_impl_t storageImpl{table};

        using context_t = internal::serializator_context<storage_impl_t>;

        context_t context{storageImpl};
        std::string value;
        std::string expected;
        SECTION("with parentheses") {
            value = serialize(ch, context);
            expected = "CHECK (0 < \"PRICE\")";
        }
        SECTION("without parentheses") {
            context.use_parentheses = false;
            value = serialize(ch, context);
            expected = "CHECK 0 < \"PRICE\"";
        }
        REQUIRE(value == expected);
    }
}
