#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

#if SQLITE_VERSION_NUMBER >= 3006019
TEST_CASE("Prepared select") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);

#define JD_AUDITING_SETTINGS
#ifdef JD_AUDITING_SETTINGS
    sql_auditor_settings::set_destination_file("log.txt");
    sql_auditor_settings::set_behavior(auditing_behavior::OFF);

#endif
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key().autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time, default_value(defaultVisitTime)),
                                           foreign_key(&Visit::userId).references(&User::id)),
                                make_table("users_and_visits",
                                           make_column("user_id", &UserAndVisit::userId),
                                           make_column("visit_id", &UserAndVisit::visitId),
                                           make_column("description", &UserAndVisit::description),
                                           primary_key(&UserAndVisit::userId, &UserAndVisit::visitId)));
    storage.sync_schema();

    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});

    storage.replace(UserAndVisit{2, 1, "Glad you came"});
    storage.replace(UserAndVisit{3, 1, "Shine on"});

    SECTION("const access to bindable") {
        auto statement = storage.prepare(select(10));
        REQUIRE(get<0>(static_cast<const decltype(statement)&>(statement)) == 10);
    }
    SECTION("one simple argument") {
        SECTION("by val") {
            SECTION("int") {
                auto statement = storage.prepare(select(10));
                auto str = storage.dump(statement);
                REQUIRE(get<0>(statement) == 10);
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({10}));
                get<0>(statement) = 20;
                REQUIRE(get<0>(statement) == 20);
                auto rows2 = storage.execute(statement);
                REQUIRE_THAT(rows2, UnorderedEquals<int>({20}));
            }
            SECTION("null") {
                auto statement = storage.prepare(select(nullptr));
                REQUIRE(get<0>(statement) == nullptr);
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<std::nullptr_t>({nullptr}));
            }
            SECTION("optional") {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                auto statement = storage.prepare(select(std::optional<int>()));
                REQUIRE(get<0>(statement) == std::nullopt);
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<std::optional<int>>({std::nullopt}));
#endif
            }
        }
        SECTION("by ref") {
            auto id = 10;
            auto statement = storage.prepare(select(std::ref(id)));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == 10);
            REQUIRE(&get<0>(statement) == &id);
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals<int>({10}));
            id = 20;
            REQUIRE(get<0>(statement) == 20);
            str = storage.dump(statement);
            REQUIRE(&get<0>(statement) == &id);
            auto rows2 = storage.execute(statement);
            REQUIRE_THAT(rows2, UnorderedEquals<int>({20}));
        }
    }
    SECTION("two simple arguments") {
        SECTION("by val") {
            auto statement = storage.prepare(select(columns("ototo", 25)));
            auto str = storage.dump(statement);
            REQUIRE(strcmp(get<0>(statement), "ototo") == 0);
            REQUIRE(get<1>(statement) == 25);
            auto rows = storage.execute(statement);
            REQUIRE(rows == decltype(rows){std::make_tuple("ototo", 25)});
            get<0>(statement) = "Rock";
            get<1>(statement) = -15;
            str = storage.dump(statement);
            auto rows2 = storage.execute(statement);
            REQUIRE(rows2 == decltype(rows2){std::make_tuple("Rock", -15)});
        }
        SECTION("by ref") {
            std::string ototo = "ototo";
            auto id = 25;
            auto statement = storage.prepare(select(columns(std::ref(ototo), std::ref(id))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == "ototo");
            REQUIRE(&get<0>(statement) == &ototo);
            REQUIRE(get<1>(statement) == 25);
            REQUIRE(&get<1>(statement) == &id);
            auto rows = storage.execute(statement);
            REQUIRE(rows == decltype(rows){std::make_tuple("ototo", 25)});
            ototo = "Rock";
            REQUIRE(get<0>(statement) == ototo);
            REQUIRE(&get<0>(statement) == &ototo);
            id = -15;
            REQUIRE(get<1>(statement) == id);
            REQUIRE(&get<1>(statement) == &id);
            auto rows2 = storage.execute(statement);
            REQUIRE(rows2 == decltype(rows2){std::make_tuple("Rock", -15)});
        }
    }
    SECTION("three columns, aggregate func and where") {
        SECTION("by val") {
            auto statement =
                storage.prepare(select(columns(5.0, &User::id, count(&User::name)), where(less_than(&User::id, 10))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == 5.0);
            REQUIRE(get<1>(statement) == 10);
            auto rows = storage.execute(statement);
            REQUIRE(rows == decltype(rows){std::make_tuple(5.0, 1, 3)});
            get<0>(statement) = 4;
            get<1>(statement) = 2;
            str = storage.dump(statement);
            auto rows2 = storage.execute(statement);
            REQUIRE(rows2 == decltype(rows2){std::make_tuple(4.0, 1, 1)});
        }
        SECTION("by ref") {
            auto first = 5.0;
            auto id = 10;
            auto statement = storage.prepare(select(columns(std::ref(first), &User::id, count(&User::name)),
                                                    where(less_than(&User::id, std::ref(id)))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == 5.0);
            REQUIRE(&get<0>(statement) == &first);
            REQUIRE(get<1>(statement) == 10);
            REQUIRE(&get<1>(statement) == &id);
            auto rows = storage.execute(statement);
            REQUIRE(rows == decltype(rows){std::make_tuple(5.0, 1, 3)});
            first = 4;
            REQUIRE(&get<0>(statement) == &first);
            id = 2;
            str = storage.dump(statement);
            auto rows2 = storage.execute(statement);
            REQUIRE(rows2 == decltype(rows2){std::make_tuple(4.0, 1, 1)});
        }
    }
    SECTION("serialize one column") {
        for(auto i = 0; i < 2; ++i) {
            auto statement = storage.prepare(select(&User::id));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
            }
        }
    }
    SECTION("serialize one column with order by") {
        for(auto i = 0; i < 2; ++i) {
            auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
            auto str = storage.dump(statement);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
            }
        }
    }
    SECTION("serialize one column with where") {
        SECTION("by val") {
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == 5);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
            }
        }
        SECTION("by ref") {
            auto len = 5;
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > std::ref(len))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == len);
            REQUIRE(&get<0>(statement) == &len);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
            }
        }
    }
    SECTION("one column with where and") {
        SECTION("by val") {
            auto statement =
                storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == 5);
            REQUIRE(strcmp(get<1>(statement), "T%") == 0);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
            }
        }
        SECTION("by ref") {
            auto len = 5;
            std::string pattern = "T%";
            auto statement = storage.prepare(
                select(&User::id, where(length(&User::name) > std::ref(len) and like(&User::name, std::ref(pattern)))));
            auto str = storage.dump(statement);
            REQUIRE(get<0>(statement) == len);
            REQUIRE(&get<0>(statement) == &len);
            REQUIRE(get<1>(statement) == pattern);
            REQUIRE(&get<1>(statement) == &pattern);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
            }
        }
    }
    SECTION("two columns") {
        auto statement = storage.prepare(select(columns(&User::id, &User::name)));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<int, std::string>> expected;
            expected.push_back(std::make_tuple(1, "Team BS"));
            expected.push_back(std::make_tuple(2, "Shy'm"));
            expected.push_back(std::make_tuple(3, "Maître Gims"));
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    SECTION("two columns with where + order by") {
        SECTION("by val") {
            auto statement = storage.prepare(
                select(columns(&User::name, &User::id), where(is_equal(mod(&User::id, 2), 0)), order_by(&User::name)));
            auto str = storage.dump(statement);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                std::vector<std::tuple<std::string, int>> expected;
                expected.push_back(std::make_tuple("Shy'm", 2));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
        SECTION("by ref") {
            auto m = 2;
            auto v = 0;
            auto statement = storage.prepare(select(columns(&User::name, &User::id),
                                                    where(is_equal(mod(&User::id, std::ref(m)), std::ref(v))),
                                                    order_by(&User::name)));
            auto str = storage.dump(statement);
            testSerializing(statement);
            REQUIRE(get<0>(statement) == m);
            REQUIRE(&get<0>(statement) == &m);
            REQUIRE(get<1>(statement) == v);
            REQUIRE(&get<1>(statement) == &v);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                std::vector<std::tuple<std::string, int>> expected;
                expected.push_back(std::make_tuple("Shy'm", 2));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
    }
    SECTION("object") {
        auto statement = storage.prepare(select(object<User>(true)));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<User> expected{{1, "Team BS"}, {2, "Shy'm"}, {3, "Maître Gims"}};
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    SECTION("multi object") {
        auto statement = storage.prepare(select(columns(object<User>(true), object<User>(true))));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<User, User>> expected;
            expected.push_back({User{1, "Team BS"}, User{1, "Team BS"}});
            expected.push_back({User{2, "Shy'm"}, User{2, "Shy'm"}});
            expected.push_back({User{3, "Maître Gims"}, User{3, "Maître Gims"}});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    SECTION("multi object 2") {
        auto statement = storage.prepare(select(columns(object<User>(true), asterisk<User>(true), object<User>(true))));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<User, int, std::string, User>> expected;
            expected.push_back({User{1, "Team BS"}, 1, "Team BS", User{1, "Team BS"}});
            expected.push_back({User{2, "Shy'm"}, 2, "Shy'm", User{2, "Shy'm"}});
            expected.push_back({User{3, "Maître Gims"}, 3, "Maître Gims", User{3, "Maître Gims"}});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    SECTION("struct") {
        using Z = User;  // for the unit test it is fine to just reuse `User` as an unmapped struct
        constexpr auto z_struct = struct_<Z>(&User::id, &User::name);
        auto statement = storage.prepare(select(z_struct));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<Z> expected{{1, "Team BS"}, {2, "Shy'm"}, {3, "Maître Gims"}};
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
    SECTION("non-aggregate struct") {
        // testing `struct_extractor` with a non-aggregate type,
        // which is expected to extract sql result values in the order of arguments.
        // This was not the case with msvc, which would extract the name before the id.

        struct Z {
            decltype(User::id) id = 0;
            decltype(User::name) name;

            Z(decltype(id) id, decltype(name) name) : id{id}, name{std::move(name)} {}

            bool operator==(const Z& z) const {
                return this->id == z.id && this->name == z.name;
            }
        };
        constexpr auto z_struct = struct_<Z>(&User::id, &User::name);
        auto statement = storage.prepare(select(z_struct));
        auto rows = storage.execute(statement);
        std::vector<Z> expected{{1, "Team BS"}, {2, "Shy'm"}, {3, "Maître Gims"}};
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    SECTION("multi struct") {
        using Z = User;  // for the unit test it is fine to just reuse `User` as an unmapped struct
        constexpr auto z_struct = struct_<Z>(&User::id, &User::name);
        auto statement = storage.prepare(select(columns(z_struct, z_struct)));
        auto str = storage.dump(statement);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            std::vector<std::tuple<Z, Z>> expected;
            expected.push_back({Z{1, "Team BS"}, Z{1, "Team BS"}});
            expected.push_back({Z{2, "Shy'm"}, Z{2, "Shy'm"}});
            expected.push_back({Z{3, "Maître Gims"}, Z{3, "Maître Gims"}});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
    }
}
#endif

TEST_CASE("dumping") {
    auto storage = make_storage("");

    std::string value, expected;

    SECTION("expression") {
        auto expression = select(1);
        SECTION("default") {
            value = storage.dump(expression);
            expected = "SELECT 1";
        }
        SECTION("parametrized") {
            value = storage.dump(expression, false);
            expected = "SELECT 1";
        }
        SECTION("dump") {
            value = storage.dump(expression, true);
            expected = "SELECT ?";
        }
    }
    SECTION("statement") {
        auto statement = storage.prepare(select(1));
        SECTION("default") {
            value = storage.dump(statement);
            expected = "SELECT ?";
        }
        SECTION("parametrized") {
            value = storage.dump(statement, true);
            expected = "SELECT ?";
        }
        SECTION("dump") {
            value = storage.dump(statement, false);
            expected = "SELECT 1";
        }
    }
#if SQLITE_VERSION_NUMBER >= 3020000
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
    SECTION("with bound pointer") {
        int64 lastSelectedId;
        auto statement = storage.prepare(select(bind_carray_pointer_statically(&lastSelectedId)));
        SECTION("default") {
            value = storage.dump(statement);
            expected = "SELECT ?";
        }
        SECTION("parametrized") {
            value = storage.dump(statement, true);
            expected = "SELECT ?";
        }
        SECTION("dump") {
            value = storage.dump(statement, false);
            expected = "SELECT NULL";
        }
    }
#endif
#endif
    REQUIRE(value == expected);
}
