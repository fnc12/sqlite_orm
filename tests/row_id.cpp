#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Row id") {

    struct SimpleTable {
        std::string letter;
        std::string desc;
    };

    auto storage = make_storage(
        "rowid.sqlite",
        make_table("tbl1", make_column("letter", &SimpleTable::letter), make_column("desc", &SimpleTable::desc)));
    storage.sync_schema();
    storage.remove_all<SimpleTable>();

    storage.insert(SimpleTable{"A", "first letter"});
    storage.insert(SimpleTable{"B", "second letter"});
    storage.insert(SimpleTable{"C", "third letter"});
    SECTION("select everything") {
        auto rows = storage.select(columns(rowid(),
                                           oid(),
                                           _rowid_(),
                                           rowid<SimpleTable>(),
                                           oid<SimpleTable>(),
                                           _rowid_<SimpleTable>(),
                                           &SimpleTable::letter,
                                           &SimpleTable::desc));
        for(size_t i = 0; i < rows.size(); ++i) {
            auto& row = rows[i];
            REQUIRE(std::get<0>(row) == std::get<1>(row));
            REQUIRE(std::get<1>(row) == std::get<2>(row));
            REQUIRE(std::get<2>(row) == static_cast<int>(i + 1));
            REQUIRE(std::get<2>(row) == std::get<3>(row));
            REQUIRE(std::get<3>(row) == std::get<4>(row));
            REQUIRE(std::get<4>(row) == std::get<5>(row));
        }
    }
    SECTION("select single") {
        std::vector<std::unique_ptr<int64>> rows;
        SECTION("rowid") {
            rows = storage.select(max(rowid<SimpleTable>()));
        }
        SECTION("oid") {
            rows = storage.select(max(oid<SimpleTable>()));
        }
        SECTION("_rowid_") {
            rows = storage.select(max(_rowid_<SimpleTable>()));
        }
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0]);
        REQUIRE(*rows[0] == 3);
    }
}
