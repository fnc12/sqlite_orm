#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

namespace {
    struct Employee {
        int id;
        std::string name;
        int age;
        std::string address;  //  optional
        double salary;  //  optional

        bool operator==(const Employee& other) const {
            return this->id == other.id && this->name == other.name && this->age == other.age &&
                   this->address == other.address && this->salary == other.salary;
        }
    };
}
TEST_CASE("select constraints") {
    using Catch::Matchers::UnorderedEquals;

    auto storage = make_storage({},
                                make_table("COMPANY",
                                           make_column("ID", &Employee::id, primary_key()),
                                           make_column("NAME", &Employee::name),
                                           make_column("AGE", &Employee::age),
                                           make_column("ADDRESS", &Employee::address),
                                           make_column("SALARY", &Employee::salary)));
    storage.sync_schema();

    //  create employees..
    Employee paul{-1, "Paul", 32, "California", 20000.0};
    Employee allen{-1, "Allen", 25, "Texas", 15000.0};
    Employee teddy{-1, "Teddy", 23, "Norway", 20000.0};
    Employee mark{-1, "Mark", 25, "Rich-Mond", 65000.0};
    Employee david{-1, "David", 27, "Texas", 85000.0};
    Employee kim{-1, "Kim", 22, "South-Hall", 45000.0};
    Employee james{-1, "James", 24, "Houston", 10000.0};

    //  insert employees. `insert` function returns id of inserted object..
    paul.id = storage.insert(paul);
    allen.id = storage.insert(allen);
    teddy.id = storage.insert(teddy);
    mark.id = storage.insert(mark);
    david.id = storage.insert(david);
    kim.id = storage.insert(kim);
    james.id = storage.insert(james);

    SECTION("select asterisk") {
        SECTION("asterisk") {
            auto allEmployeesTuples = storage.select(asterisk<Employee>());

            std::vector<std::tuple<int, std::string, int, std::string, double>> expected;

            expected.push_back(std::make_tuple(paul.id, "Paul", 32, "California", 20000.0));
            expected.push_back(std::make_tuple(allen.id, "Allen", 25, "Texas", 15000.0));
            expected.push_back(std::make_tuple(teddy.id, "Teddy", 23, "Norway", 20000.0));
            expected.push_back(std::make_tuple(mark.id, "Mark", 25, "Rich-Mond", 65000.0));
            expected.push_back(std::make_tuple(david.id, "David", 27, "Texas", 85000.0));
            expected.push_back(std::make_tuple(kim.id, "Kim", 22, "South-Hall", 45000.0));
            expected.push_back(std::make_tuple(james.id, "James", 24, "Houston", 10000.0));
            REQUIRE_THAT(allEmployeesTuples, UnorderedEquals(expected));
        }
        SECTION("object") {
            auto allEmployees = storage.select(object<Employee>());
            std::vector<Employee> expected{paul, allen, teddy, mark, david, kim, james};
            REQUIRE_THAT(allEmployees, UnorderedEquals(expected));
        }
    }
    SECTION("distinct") {
        storage.insert(Employee{-1, "Paul", 24, "Houston", 20000.0});
        storage.insert(Employee{-1, "James", 44, "Norway", 5000.0});
        storage.insert(Employee{-1, "James", 45, "Texas", 5000.0});

        std::vector<std::string> names;
        decltype(names) expected;
        SECTION("without distinct") {
            SECTION("without prepared statement") {
                names = storage.select(&Employee::name);
            }
            SECTION("with prepared statement") {
                auto statement = storage.prepare(select(&Employee::name));
                names = storage.execute(statement);
            }
            expected.push_back("Paul");
            expected.push_back("Allen");
            expected.push_back("Teddy");
            expected.push_back("Mark");
            expected.push_back("David");
            expected.push_back("Kim");
            expected.push_back("James");
            expected.push_back("Paul");
            expected.push_back("James");
            expected.push_back("James");
        }
        SECTION("with distinct") {
            SECTION("without prepared statement") {
                names = storage.select(distinct(&Employee::name));
            }
            SECTION("with prepared statement") {
                auto statement = storage.prepare(select(distinct(&Employee::name)));
                names = storage.execute(statement);
            }
            expected.push_back("Paul");
            expected.push_back("Allen");
            expected.push_back("Teddy");
            expected.push_back("Mark");
            expected.push_back("David");
            expected.push_back("Kim");
            expected.push_back("James");
        }
        REQUIRE_THAT(names, UnorderedEquals(expected));
    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    SECTION("as_optional") {
        SECTION("prepared statements bindings") {
            auto statement = storage.prepare(select(as_optional(5)));
            auto rows = storage.execute(statement);
            decltype(rows) expected;
            expected.push_back(5);
            REQUIRE(rows == expected);

            get<0>(statement) = 10;
            expected.clear();
            expected.push_back(10);
            rows = storage.execute(statement);
            REQUIRE(rows == expected);
        }
        SECTION("just names") {
            auto rows = storage.select(as_optional(&Employee::name));
            decltype(rows) expected;
            expected.push_back("Paul");
            expected.push_back("Allen");
            expected.push_back("Teddy");
            expected.push_back("Mark");
            expected.push_back("David");
            expected.push_back("Kim");
            expected.push_back("James");
            REQUIRE_THAT(rows, UnorderedEquals(expected));
        }
        SECTION("left join") {
            struct Author {
                int id = 0;
                std::string name;
            };
            struct Book {
                int id = 0;
                std::string title;
                int authorId = 0;
            };
            auto storage2 = make_storage({},
                                         make_table("Author",
                                                    make_column("id", &Author::id),
                                                    make_column("name", &Author::name),
                                                    primary_key(&Author::id)),
                                         make_table("Book",
                                                    make_column("id", &Book::id),
                                                    make_column("title", &Book::title),
                                                    make_column("author_id", &Book::authorId),
                                                    primary_key(&Book::id),
                                                    foreign_key(&Book::authorId).references(&Author::id)));
            storage2.sync_schema();
            storage2.replace(Author{1, "Dostoevsky"});
            storage2.replace(Author{2, "Tolstoy"});
            storage2.replace(Author{3, "Chekhov"});
            storage2.replace(Author{4, "Joanne Rowling"});
            storage2.replace(Book{1, "War and Peace", 2});
            storage2.replace(Book{2, "Crime and Punishment", 1});
            storage2.replace(Book{3, "Harry Potter", 4});
            SECTION("without optional") {
                auto rows =
                    storage2.select(columns(&Author::id, &Author::name, &Book::id, &Book::title, &Book::authorId),
                                    left_join<Book>(on(c(&Author::id) == &Book::authorId)));
                decltype(rows) expected;
                expected.push_back(std::make_tuple(1, "Dostoevsky", 2, "Crime and Punishment", 1));
                expected.push_back(std::make_tuple(2, "Tolstoy", 1, "War and Peace", 2));
                expected.push_back(std::make_tuple(3, "Chekhov", 0, std::string(), 0));
                expected.push_back(std::make_tuple(4, "Joanne Rowling", 3, "Harry Potter", 4));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            SECTION("with optional") {
                using Rows = std::vector<
                    std::tuple<int, std::string, std::optional<int>, std::optional<std::string>, std::optional<int>>>;
                Rows rows;
                SECTION("without prepared statement") {
                    rows = storage2.select(columns(&Author::id,
                                                   &Author::name,
                                                   as_optional(&Book::id),
                                                   as_optional(&Book::title),
                                                   as_optional(&Book::authorId)),
                                           left_join<Book>(on(c(&Author::id) == &Book::authorId)));
                }
                SECTION("with prepared statement") {
                    auto statement = storage2.prepare(select(columns(&Author::id,
                                                                     &Author::name,
                                                                     as_optional(&Book::id),
                                                                     as_optional(&Book::title),
                                                                     as_optional(&Book::authorId)),
                                                             left_join<Book>(on(c(&Author::id) == &Book::authorId))));
                    rows = storage2.execute(statement);
                }
                decltype(rows) expected;
                expected.push_back(std::make_tuple(1, "Dostoevsky", 2, "Crime and Punishment", 1));
                expected.push_back(std::make_tuple(2, "Tolstoy", 1, "War and Peace", 2));
                expected.push_back(std::make_tuple(3, "Chekhov", std::nullopt, std::nullopt, std::nullopt));
                expected.push_back(std::make_tuple(4, "Joanne Rowling", 3, "Harry Potter", 4));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}

namespace {
    struct User1 {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User1() = default;
        User1(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };

    struct Visit1 {
        int id = 0;
        int userId = 0;
        time_t time = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Visit1() = default;
        Visit1(int id, int userId, time_t time) : id{id}, userId{userId}, time{time} {}
#endif
    };
}
TEST_CASE("Exists") {
    auto storage = make_storage(
        "",
        make_table("users", make_column("id", &User1::id, primary_key()), make_column("name", &User1::name)),
        make_table("visits",
                   make_column("id", &Visit1::id, primary_key()),
                   make_column("userId", &Visit1::userId),
                   make_column("time", &Visit1::time),
                   foreign_key(&Visit1::userId).references(&User1::id)));
    storage.sync_schema();

    storage.replace(User1{1, "Daddy Yankee"});
    storage.replace(User1{2, "Don Omar"});

    storage.replace(Visit1{1, 1, 100000});
    storage.replace(Visit1{2, 1, 100001});
    storage.replace(Visit1{3, 1, 100002});
    storage.replace(Visit1{4, 1, 200000});
    storage.replace(Visit1{5, 2, 100000});

    auto rows = storage.select(
        &User1::id,
        where(exists(select(&Visit1::id, where(c(&Visit1::time) == 200000 and eq(&Visit1::userId, &User1::id))))));
    REQUIRE(rows.empty() == false);
}

namespace {
    struct User2 {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string country;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User2() = default;
        User2(int id, std::string firstName, std::string lastName, std::string country) :
            id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, country{country} {}
#endif
    };

    struct Track2 {
        int id = 0;
        std::string name;
        long milliseconds = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Track2() = default;
        Track2(int id, std::string name, long milliseconds) :
            id{id}, name{std::move(name)}, milliseconds{milliseconds} {}
#endif
    };

}
TEST_CASE("Case") {
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User2::id, primary_key().autoincrement()),
                                           make_column("first_name", &User2::firstName),
                                           make_column("last_name", &User2::lastName),
                                           make_column("country", &User2::country)),
                                make_table("tracks",
                                           make_column("trackid", &Track2::id, primary_key().autoincrement()),
                                           make_column("name", &Track2::name),
                                           make_column("milliseconds", &Track2::milliseconds)));
    storage.sync_schema();

    struct GradeAlias : alias_tag {
        static const std::string& get() {
            static const std::string res = "Grade";
            return res;
        }
    };

    {
        storage.insert(User2{0, "Roberto", "Almeida", "Mexico"});
        storage.insert(User2{0, "Julia", "Bernett", "USA"});
        storage.insert(User2{0, "Camille", "Bernard", "Argentina"});
        storage.insert(User2{0, "Michelle", "Brooks", "USA"});
        storage.insert(User2{0, "Robet", "Brown", "USA"});

        auto rows = storage.select(
            columns(case_<std::string>(&User2::country).when("USA", then("Dosmetic")).else_("Foreign").end()),
            multi_order_by(order_by(&User2::lastName), order_by(&User2::firstName)));
        auto verifyRows = [&storage](auto& rows) {
            REQUIRE(rows.size() == storage.count<User2>());
            REQUIRE(std::get<0>(rows[0]) == "Foreign");
            REQUIRE(std::get<0>(rows[1]) == "Foreign");
            REQUIRE(std::get<0>(rows[2]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[3]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[4]) == "Dosmetic");
        };
        verifyRows(rows);

        rows = storage.select(
            columns(as<GradeAlias>(
                case_<std::string>(&User2::country).when("USA", then("Dosmetic")).else_("Foreign").end())),
            multi_order_by(order_by(&User2::lastName), order_by(&User2::firstName)));

        verifyRows(rows);
    }
    {
        storage.insert(Track2{0, "For Those About To Rock", 400000});
        storage.insert(Track2{0, "Balls to the Wall", 500000});
        storage.insert(Track2{0, "Fast as a Shark", 200000});
        storage.insert(Track2{0, "Restless and Wild", 100000});
        storage.insert(Track2{0, "Princess of the Dawn", 50000});

        auto rows = storage.select(
            case_<std::string>()
                .when(c(&Track2::milliseconds) < 60000, then("short"))
                .when(c(&Track2::milliseconds) >= 60000 and c(&Track2::milliseconds) < 300000, then("medium"))
                .else_("long")
                .end(),
            order_by(&Track2::name));
        auto verifyRows = [&storage](auto& rows) {
            REQUIRE(rows.size() == storage.count<Track2>());
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
                    .when(c(&Track2::milliseconds) < 60000, then("short"))
                    .when(c(&Track2::milliseconds) >= 60000 and c(&Track2::milliseconds) < 300000, then("medium"))
                    .else_("long")
                    .end()),
            order_by(&Track2::name));
        verifyRows(rows);
    }
}

namespace {
    struct User3 {
        int id = 0;
        int age = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User3() = default;
        User3(int id, int age, std::string name) : id{id}, age{age}, name{std::move(name)} {}
#endif
    };

}
TEST_CASE("Where") {
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User3::id, primary_key()),
                                           make_column("age", &User3::age),
                                           make_column("name", &User3::name)));
    storage.sync_schema();

    storage.replace(User3{1, 4, "Jeremy"});
    storage.replace(User3{2, 18, "Nataly"});

    auto users = storage.get_all<User3>();
    REQUIRE(users.size() == 2);

    auto users2 = storage.get_all<User3>(where(true));
    REQUIRE(users2.size() == 2);

    auto users3 = storage.get_all<User3>(where(false));
    REQUIRE(users3.size() == 0);

    auto users4 = storage.get_all<User3>(where(true and c(&User3::id) == 1));
    REQUIRE(users4.size() == 1);
    REQUIRE(users4.front().id == 1);

    auto users5 = storage.get_all<User3>(where(false and c(&User3::id) == 1));
    REQUIRE(users5.size() == 0);

    auto users6 = storage.get_all<User3>(where((false or c(&User3::id) == 4) and (false or c(&User3::age) == 18)));
    REQUIRE(users6.empty());
}

namespace {
    struct User4 {
        int id = 0;
        std::string firstName;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User4() = default;
        User4(int id, std::string firstName) : id{id}, firstName{std::move(firstName)} {}
#endif

        bool operator==(const User4& user) const {
            return this->id == user.id && this->firstName == user.firstName;
        }
    };
}
TEST_CASE("collate") {
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User4::id, primary_key()),
                                           make_column("first_name", &User4::firstName)));
    storage.sync_schema();
    User4 user1{1, "HELLO"};
    User4 user2{2, "Hello"};
    User4 user3{3, "HEllo"};

    storage.replace(user1);
    storage.replace(user2);
    storage.replace(user3);

    auto rows = storage.get_all<User4>(where(is_equal(&User4::firstName, "hello").collate_nocase()));
    std::vector<User4> expected = {user1, user2, user3};
    REQUIRE(rows == expected);
}

namespace {
    struct User5 {
        int id = 0;
        std::string firstName;
        std::string lastName;
        long registerTime = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User5() = default;
        User5(int id, std::string firstName, std::string lastName, long registerTime) :
            id{id}, firstName{std::move(firstName)}, lastName{std::move(lastName)}, registerTime{registerTime} {}
#endif
    };

}
TEST_CASE("Dynamic order by") {
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User5::id, primary_key()),
                                           make_column("first_name", &User5::firstName),
                                           make_column("last_name", &User5::lastName),
                                           make_column("register_time", &User5::registerTime)));
    storage.sync_schema();

    storage.replace(User5{1, "Jack", "Johnson", 100});
    storage.replace(User5{2, "John", "Jackson", 90});
    storage.replace(User5{3, "Elena", "Alexandra", 80});
    storage.replace(User5{4, "Kaye", "Styles", 70});

    auto orderBy = dynamic_order_by(storage);
    std::vector<decltype(User5::id)> expectedIds;

    SECTION("id") {
        auto ob = order_by(&User5::id);
        orderBy.push_back(ob);
        expectedIds = {
            1,
            2,
            3,
            4,
        };
    }

    SECTION("id desc") {
        orderBy.push_back(order_by(&User5::id).desc());
        expectedIds = {
            4,
            3,
            2,
            1,
        };
    }

    SECTION("firstName") {
        orderBy.push_back(order_by(&User5::firstName));
        expectedIds = {
            3,
            1,
            2,
            4,
        };
    }

    SECTION("firstName asc") {
        orderBy.push_back(order_by(&User5::firstName).asc());
        expectedIds = {
            3,
            1,
            2,
            4,
        };
    }

    SECTION("firstName desc") {
        orderBy.push_back(order_by(&User5::firstName).desc());
        expectedIds = {
            4,
            2,
            1,
            3,
        };
    }

    SECTION("firstName asc + id desc") {
        orderBy.push_back(order_by(&User5::firstName).asc());
        orderBy.push_back(order_by(&User5::id).desc());
        expectedIds = {
            3,
            1,
            2,
            4,
        };
    }

    SECTION("lastName + firstName + id") {
        orderBy.push_back(order_by(&User5::lastName));
        orderBy.push_back(order_by(&User5::firstName));
        orderBy.push_back(order_by(&User5::id));
        expectedIds = {
            3,
            2,
            1,
            4,
        };
    }

    SECTION("lastName + firstName desc + id") {
        orderBy.push_back(order_by(&User5::lastName));
        orderBy.push_back(order_by(&User5::firstName).desc());
        orderBy.push_back(order_by(&User5::id));
        expectedIds = {
            3,
            2,
            1,
            4,
        };
    }

    auto rows = storage.get_all<User5>(orderBy);
    REQUIRE(rows.size() == 4);
    for(size_t i = 0; i < rows.size(); ++i) {
        auto& row = rows[i];
        REQUIRE(row.id == expectedIds[i]);
    }
    orderBy.clear();
}

TEST_CASE("rows") {
    //  https://www.sqlite.org/rowvalue.html
    auto storage = make_storage({});

    auto rows = storage.select(is_equal(std::make_tuple(1, 2, 3), std::make_tuple(1, 2, 3)));
    decltype(rows) expected{true};
    REQUIRE(rows == expected);
}
