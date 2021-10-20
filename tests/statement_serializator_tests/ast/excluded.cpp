#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

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
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;
    SECTION("word") {
        auto statement = excluded(&Vocabulary::word);
        value = serialize(statement, context);
        expected = "excluded.\"word\"";
    }
    SECTION("count") {
        auto statement = excluded(&Vocabulary::count);
        value = serialize(statement, context);
        expected = "excluded.\"count\"";
    }

    REQUIRE(value == expected);
}
