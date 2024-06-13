#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <numeric>  //  std::iota

using namespace sqlite_orm;

TEST_CASE("Iterate mapped") {
    struct Test {
        int64_t id;
        std::vector<char> key;

        bool operator==(const Test& rhs) const {
            return this->id == rhs.id && this->key == rhs.key;
        }
    };

    auto db =
        make_storage("",
                     make_table("Test", make_column("id", &Test::id, primary_key()), make_column("key", &Test::key)));
    db.sync_schema(true);

    std::vector<char> key(255);
    iota(key.begin(), key.end(), '\0');

    Test expected{5, key};
    std::vector<Test> expected_vec{expected};

    db.replace(expected);

    SECTION("range-based for") {
        for(Test& obj: db.iterate<Test>()) {
            REQUIRE(obj == expected);
        }
    }
    SECTION("from iterator range") {
        auto view = db.iterate<Test>();
        REQUIRE(std::vector<Test>{view.begin(), view.end()} == expected_vec);
    }

#ifdef SQLITE_ORM_STRUCTURED_BINDINGS_SUPPORTED
    SECTION("borrowed iterator") {
        auto [begin, end] = [](auto view) {
            return std::make_pair(view.begin(), view.end());
        }(db.iterate<Test>());
        REQUIRE(*begin == expected);
        REQUIRE(++begin == end);
    }
#endif

#if __cpp_lib_containers_ranges >= 202202L
    SECTION("from range") {
        auto view = db.iterate<Test>();
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif
}
