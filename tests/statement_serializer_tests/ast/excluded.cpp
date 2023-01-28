#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("excluded") {
    using internal::serialize;
    struct Vocabulary {
        std::string word;
        int count = 0;
    };
    auto table = make_table("vocabulary",
                            make_column("word", &Vocabulary::word, primary_key()),
                            make_column("count", &Vocabulary::count, default_value(1)));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    decltype(value) expected;
    SECTION("word") {
        auto statement = excluded(&Vocabulary::word);
        value = serialize(statement, context);
        expected = R"(excluded."word")";
    }
    SECTION("count") {
        auto statement = excluded(&Vocabulary::count);
        value = serialize(statement, context);
        expected = R"(excluded."count")";
    }

    REQUIRE(value == expected);
}
