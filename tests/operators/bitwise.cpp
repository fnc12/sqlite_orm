#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("bitwise operators") {
    struct Entry {
        int lhs = 0;
        int rhs = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Entry() = default;
        Entry(int lhs, int rhs) : lhs{lhs}, rhs{rhs} {}
#endif
    };
    auto storage =
        make_storage({}, make_table("entries", make_column("lhs", &Entry::lhs), make_column("rhs", &Entry::rhs)));
    storage.sync_schema();

    {
        auto rows = storage.select(bitwise_or(60, 13));
        REQUIRE(rows == std::vector<int>{61});
    }
    {
        auto rows = storage.select(bitwise_and(60, 13));
        REQUIRE(rows == std::vector<int>{12});
    }
    {
        auto rows = storage.select(bitwise_shift_left(60, 2));
        REQUIRE(rows == std::vector<int>{240});
    }
    {
        auto rows = storage.select(bitwise_shift_right(60, 2));
        REQUIRE(rows == std::vector<int>{15});
    }
    {
        auto rows = storage.select(bitwise_not(60));
        REQUIRE(rows == std::vector<int>{-61});
    }
    storage.insert(Entry{60, 13});
    {
        auto rows = storage.select(bitwise_or(&Entry::lhs, &Entry::rhs));
        REQUIRE(rows == std::vector<int>{61});
    }
    {
        auto rows = storage.select(bitwise_and(&Entry::lhs, &Entry::rhs));
        REQUIRE(rows == std::vector<int>{12});
    }
    storage.remove_all<Entry>();
    storage.insert(Entry{60, 2});
    {
        auto rows = storage.select(bitwise_shift_left(&Entry::lhs, &Entry::rhs));
        REQUIRE(rows == std::vector<int>{240});
    }
    {
        auto rows = storage.select(bitwise_shift_right(&Entry::lhs, &Entry::rhs));
        REQUIRE(rows == std::vector<int>{15});
    }
    {
        auto rows = storage.select(bitwise_not(&Entry::lhs));
        REQUIRE(rows == std::vector<int>{-61});
    }
}
