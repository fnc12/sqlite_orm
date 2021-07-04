#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("select constraints") {
    using Catch::Matchers::UnorderedEquals;

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
            std::vector<Employee> expected;
            expected.push_back(paul);
            expected.push_back(allen);
            expected.push_back(teddy);
            expected.push_back(mark);
            expected.push_back(david);
            expected.push_back(kim);
            expected.push_back(james);
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

TEST_CASE("explicit from") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.replace(User{1, "Bebe Rexha"});

    std::vector<decltype(User::id)> rows;
    decltype(rows) expected;
    SECTION("without conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>());
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id), from<als>());
        }
        expected.push_back(1);
    }
    SECTION("with real conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>(), where(is_equal(&User::name, "Bebe Rexha")));
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id),
                                  from<als>(),
                                  where(is_equal(alias_column<als>(&User::name), "Bebe Rexha")));
        }
        expected.push_back(1);
    }
    SECTION("with unreal conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>(), where(is_equal(&User::name, "Zara Larsson")));
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id),
                                  from<als>(),
                                  where(is_equal(alias_column<als>(&User::name), "Zara Larsson")));
        }
    }
    REQUIRE(expected == rows);
}
