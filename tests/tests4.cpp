#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <numeric>
#include <algorithm>  //  std::count_if
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

using namespace sqlite_orm;

TEST_CASE("Case") {

    struct User {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string country;
    };

    struct Track {
        int id = 0;
        std::string name;
        long milliseconds = 0;
    };

    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, autoincrement(), primary_key()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("country", &User::country)),
                                make_table("tracks",
                                           make_column("trackid", &Track::id, autoincrement(), primary_key()),
                                           make_column("name", &Track::name),
                                           make_column("milliseconds", &Track::milliseconds)));
    storage.sync_schema();

    struct GradeAlias : alias_tag {
        static const std::string &get() {
            static const std::string res = "Grade";
            return res;
        }
    };

    {
        storage.insert(User{0, "Roberto", "Almeida", "Mexico"});
        storage.insert(User{0, "Julia", "Bernett", "USA"});
        storage.insert(User{0, "Camille", "Bernard", "Argentina"});
        storage.insert(User{0, "Michelle", "Brooks", "USA"});
        storage.insert(User{0, "Robet", "Brown", "USA"});

        auto rows = storage.select(
            columns(case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end()),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<User>());
            REQUIRE(std::get<0>(rows[0]) == "Foreign");
            REQUIRE(std::get<0>(rows[1]) == "Foreign");
            REQUIRE(std::get<0>(rows[2]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[3]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[4]) == "Dosmetic");
        };
        verifyRows(rows);

        rows = storage.select(
            columns(as<GradeAlias>(
                case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end())),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));

        verifyRows(rows);
    }
    {
        storage.insert(Track{0, "For Those About To Rock", 400000});
        storage.insert(Track{0, "Balls to the Wall", 500000});
        storage.insert(Track{0, "Fast as a Shark", 200000});
        storage.insert(Track{0, "Restless and Wild", 100000});
        storage.insert(Track{0, "Princess of the Dawn", 50000});

        auto rows = storage.select(
            case_<std::string>()
                .when(c(&Track::milliseconds) < 60000, then("short"))
                .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                .else_("long")
                .end(),
            order_by(&Track::name));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<Track>());
            REQUIRE(rows[0] == "long");
            REQUIRE(rows[1] == "medium");
            REQUIRE(rows[2] == "long");
            REQUIRE(rows[3] == "short");
            REQUIRE(rows[4] == "medium");
        };
        verifyRows(rows);

        rows = storage.select(
            as<GradeAlias>(
                case_<std::string>()
                    .when(c(&Track::milliseconds) < 60000, then("short"))
                    .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                    .else_("long")
                    .end()),
            order_by(&Track::name));
        verifyRows(rows);
    }
}

TEST_CASE("Unique ptr in update") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});

    {
        storage.update_all(set(assign(&User::name, std::make_unique<std::string>("Nick"))));
        REQUIRE(storage.count<User>(where(is_null(&User::name))) == 0);
    }
    {
        std::unique_ptr<std::string> ptr;
        storage.update_all(set(assign(&User::name, move(ptr))));
        REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 0);
    }
}

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("Optional in update") {

    struct User {
        int id = 0;
        std::optional<int> carYear;  // will be empty if user takes the bus.
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("car_year", &User::carYear)));
    storage.sync_schema();

    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{0, 2006});

    REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 1);

    {
        storage.update_all(set(assign(&User::carYear, std::optional<int>{})));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, 1994)));
        REQUIRE(storage.count<User>(where(is_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, nullptr)));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
}
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

TEST_CASE("Join") {

    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        int id = 0;
        int userId = 0;
        time_t date = 0;
    };

    auto storage =
        make_storage({},
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_table("visits",
                                make_column("id", &Visit::id, primary_key()),
                                make_column("user_id", &Visit::userId),
                                make_column("date", &Visit::date)));
    storage.sync_schema();

    int id = 1;
    User will{id++, "Will"};
    User smith{id++, "Smith"};
    User nicole{id++, "Nicole"};

    storage.replace(will);
    storage.replace(smith);
    storage.replace(nicole);

    id = 1;
    storage.replace(Visit{id++, will.id, 10});
    storage.replace(Visit{id++, will.id, 20});
    storage.replace(Visit{id++, will.id, 30});

    storage.replace(Visit{id++, smith.id, 25});
    storage.replace(Visit{id++, smith.id, 35});

    {
        auto rows = storage.get_all<User>(left_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(left_outer_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(inner_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(cross_join<Visit>());
        REQUIRE(rows.size() == 15);
    }
    {
        auto rows = storage.get_all<User>(natural_join<Visit>());
        REQUIRE(rows.size() == 3);
    }
}

TEST_CASE("Storage copy") {
    struct User {
        int id = 0;
    };

    int calledCount = 0;

    auto storage = make_storage({}, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();
    storage.remove_all<User>();

    storage.on_open = [&calledCount](sqlite3 *) {
        ++calledCount;
    };

    storage.on_open(nullptr);
    REQUIRE(calledCount == 1);

    auto storageCopy = storage;
    REQUIRE(storageCopy.on_open);
    REQUIRE(calledCount == 2);
    storageCopy.on_open(nullptr);
    REQUIRE(calledCount == 3);

    storageCopy.sync_schema();
    storageCopy.remove_all<User>();
}

TEST_CASE("Set null") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.replace(User{1, std::make_unique<std::string>("Ototo")});
    REQUIRE(storage.count<User>() == 1);

    auto rows = storage.get_all<User>();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front().name);

    storage.update_all(set(assign(&User::name, nullptr)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }

    storage.update_all(set(assign(&User::name, "ototo")));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
        REQUIRE(*rows.front().name == "ototo");
    }

    storage.update_all(set(assign(&User::name, nullptr)), where(is_equal(&User::id, 1)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }
}
