#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("upsert_clause") {
    using internal::serialize;
    struct Vocabulary {
        std::string word;
        int count = 0;
    };
    struct User {
        int id = 0;
        std::string firstname;
        std::string lastname;
    };
    auto vocabularyTable = make_table("vocabulary",
                                      make_column("word", &Vocabulary::word, primary_key()),
                                      make_column("count", &Vocabulary::count, default_value(1)));
    auto usersTable = make_table("users",
                                 make_column("id", &User::id, primary_key()),
                                 make_column("firstname", &User::firstname),
                                 make_column("lastname", &User::lastname));
    using storage_impl_t = internal::storage_impl<decltype(vocabularyTable), decltype(usersTable)>;
    auto storageImpl = storage_impl_t{vocabularyTable, usersTable};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;
    SECTION("empty") {
        auto expression = on_conflict().do_nothing();
        value = serialize(expression, context);
        expected = "ON CONFLICT DO NOTHING";
    }
    SECTION("one column") {
        SECTION("1 set") {
            SECTION("functions") {
                auto expression = on_conflict(&Vocabulary::word)
                                      .do_update(set(assign(&Vocabulary::count, add(&Vocabulary::count, 1))));
                value = serialize(expression, context);
            }
            SECTION("operators") {
                auto expression =
                    on_conflict(&Vocabulary::word).do_update(set(c(&Vocabulary::count) = c(&Vocabulary::count) + 1));
                value = serialize(expression, context);
            }
            expected = "ON CONFLICT (\"word\") DO UPDATE SET \"count\" = \"count\" + 1";
        }
        SECTION("2 sets") {
            SECTION("fuctions") {
                auto expression = on_conflict(&Vocabulary::word)
                                      .do_update(set(assign(&Vocabulary::count, add(&Vocabulary::count, 1)),
                                                     assign(&Vocabulary::word, "abc")));
                value = serialize(expression, context);
            }
            SECTION("operators") {
                auto expression = on_conflict(&Vocabulary::word)
                                      .do_update(set(c(&Vocabulary::count) = c(&Vocabulary::count) + 1,
                                                     c(&Vocabulary::word) = "abc"));
                value = serialize(expression, context);
            }
            expected = "ON CONFLICT (\"word\") DO UPDATE SET \"count\" = \"count\" + 1, \"word\" = 'abc'";
        }
    }
    SECTION("two columns") {
        SECTION("1 set") {
            SECTION("functions") {
                auto expression = on_conflict(columns(&Vocabulary::word, &Vocabulary::count))
                                      .do_update(set(assign(&Vocabulary::count, add(&Vocabulary::count, 1))));
                value = serialize(expression, context);
            }
            SECTION("operators") {
                auto expression = on_conflict(columns(&Vocabulary::word, &Vocabulary::count))
                                      .do_update(set(c(&Vocabulary::count) = c(&Vocabulary::count) + 1));
                value = serialize(expression, context);
            }
            expected = "ON CONFLICT (\"word\", \"count\") DO UPDATE SET \"count\" = \"count\" + 1";
        }
        SECTION("2 sets") {
            SECTION("fuctions") {
                auto expression = on_conflict(columns(&Vocabulary::word, &Vocabulary::count))
                                      .do_update(set(assign(&Vocabulary::count, add(&Vocabulary::count, 1)),
                                                     assign(&Vocabulary::word, "abc")));
                value = serialize(expression, context);
            }
            SECTION("operators") {
                auto expression = on_conflict(columns(&Vocabulary::word, &Vocabulary::count))
                                      .do_update(set(c(&Vocabulary::count) = c(&Vocabulary::count) + 1,
                                                     c(&Vocabulary::word) = "abc"));
                value = serialize(expression, context);
            }
            expected = "ON CONFLICT (\"word\", \"count\") DO UPDATE SET \"count\" = \"count\" + 1, \"word\" = 'abc'";
        }
    }
    SECTION("with excluded") {
        auto expression = on_conflict(&User::id).do_update(
            set(c(&User::firstname) = excluded(&User::firstname), c(&User::lastname) = excluded(&User::lastname)));
        value = serialize(expression, context);
        expected = "ON CONFLICT (\"id\") DO UPDATE SET \"firstname\" = excluded.\"firstname\", \"lastname\" = "
                   "excluded.\"lastname\"";
    }
    REQUIRE(value == expected);
}
