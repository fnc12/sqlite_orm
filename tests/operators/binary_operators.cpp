#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("binary operators") {
    using Catch::Matchers::UnorderedEquals;

    struct User {
        int id = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id) : id{id} {}
#endif
    };
    auto storage = make_storage({}, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();

    storage.replace(User{1});
    storage.replace(User{2});
    storage.replace(User{3});

    std::vector<bool> rows;
    decltype(rows) expected;

    SECTION("is_equal") {
        SECTION("is_equal") {
            rows = storage.select(is_equal(&User::id, 1));
        }
        SECTION("eq") {
            rows = storage.select(eq(&User::id, 1));
        }
        SECTION("==") {
            SECTION("left") {
                rows = storage.select(c(&User::id) == 1);
            }
            SECTION("right") {
                rows = storage.select(&User::id == c(1));
            }
            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) == 1);
            }
        }
        expected.push_back(true);
        expected.push_back(false);
        expected.push_back(false);
    }
    SECTION("is_not_equal") {
        SECTION("is_not_equal") {
            rows = storage.select(is_not_equal(&User::id, 1));
        }
        SECTION("ne") {
            rows = storage.select(ne(&User::id, 1));
        }
        SECTION("!=") {
            SECTION("left") {
                rows = storage.select(c(&User::id) != 1);
            }
            SECTION("right") {
                rows = storage.select(&User::id != c(1));
            }
            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) != 1);
            }
        }
        expected.push_back(false);
        expected.push_back(true);
        expected.push_back(true);
    }
    SECTION("greater_than") {
        SECTION("greater_than") {
            rows = storage.select(greater_than(&User::id, 2));
        }
        SECTION("gt") {
            rows = storage.select(gt(&User::id, 2));
        }
        SECTION(">") {
            SECTION("left") {
                rows = storage.select(c(&User::id) > 2);
            }
            SECTION("right") {
                rows = storage.select(&User::id > c(2));
            }
            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) > 2);
            }
        }
        expected.push_back(true);
        expected.push_back(false);
        expected.push_back(false);
    }
    SECTION("greater_or_equal") {
        SECTION("greater_or_equal") {
            rows = storage.select(greater_or_equal(&User::id, 2));
        }
        SECTION("ge") {
            rows = storage.select(ge(&User::id, 2));
        }
        SECTION(">=") {
            SECTION("left") {
                rows = storage.select(c(&User::id) >= 2);
            }
            SECTION("right") {
                rows = storage.select(&User::id >= c(2));
            }

            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) >= 2);
            }
        }
        expected.push_back(true);
        expected.push_back(true);
        expected.push_back(false);
    }
    SECTION("lesser_than") {
        SECTION("lesser_than") {
            rows = storage.select(lesser_than(&User::id, 2));
        }
        SECTION("lt") {
            rows = storage.select(lt(&User::id, 2));
        }
        SECTION("<") {
            SECTION("left") {
                rows = storage.select(c(&User::id) < 2);
            }
            SECTION("right") {
                rows = storage.select(&User::id < c(2));
            }
            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) < 2);
            }
        }
        expected.push_back(true);
        expected.push_back(false);
        expected.push_back(false);
    }
    SECTION("lesser_or_equal") {
        SECTION("lesser_or_equal") {
            rows = storage.select(lesser_or_equal(&User::id, 2));
        }
        SECTION("le") {
            rows = storage.select(le(&User::id, 2));
        }
        SECTION("<=") {
            SECTION("left") {
                rows = storage.select(c(&User::id) <= 2);
            }
            SECTION("right") {
                rows = storage.select(&User::id <= c(2));
            }
            SECTION("explicit column") {
                rows = storage.select(column<User>(&User::id) <= 2);
            }
        }
        expected.push_back(true);
        expected.push_back(true);
        expected.push_back(false);
    }
    SECTION("and") {
        rows = storage.select(greater_than(&User::id, 1) and lesser_than(&User::id, 3));
        expected.push_back(false);
        expected.push_back(true);
        expected.push_back(false);
    }
    SECTION("or") {
        rows = storage.select(lesser_than(&User::id, 2) or greater_than(&User::id, 2));
        expected.push_back(true);
        expected.push_back(false);
        expected.push_back(true);
    }

    REQUIRE_THAT(rows, UnorderedEquals(expected));
}
