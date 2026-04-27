#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <numeric>  //  std::iota

using namespace sqlite_orm;

TEST_CASE("Iterate mapped") {
    struct Test {
        int64_t id = 0;
        std::vector<char> key;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const Test&) const = default;
#else
        bool operator==(const Test& rhs) const {
            return this->id == rhs.id && this->key == rhs.key;
        }
#endif
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
        for (Test& obj: db.iterate<Test>()) {
            REQUIRE(obj == expected);
        }
    }
    SECTION("from iterator range") {
        auto view = db.iterate<Test>();
        REQUIRE(std::vector<Test>{view.begin(), view.end()} == expected_vec);
    }
    SECTION("borrowed iterator") {
        auto [begin, end] = [](auto view) {
            return std::make_pair(view.begin(), view.end());
        }(db.iterate<Test>());
        REQUIRE(*begin == expected);
        REQUIRE(++begin == end);
    }

#if __cpp_lib_containers_ranges >= 202202L
    SECTION("from range") {
        auto view = db.iterate<Test>();
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif

    SECTION("with conditions") {
        auto view = db.iterate<Test>(where(c(&Test::id) == 5), order_by(&Test::id));
        REQUIRE(std::vector<Test>{view.begin(), view.end()} == expected_vec);
    }
}

TEST_CASE("Iterate select statement") {
    struct Test {
        int64_t id = 0;
        std::vector<char> key;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const Test&) const = default;
#else
        bool operator==(const Test& right) const {
            return this->id == right.id && this->key == right.key;
        }
#endif
    };
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    constexpr orm_table_reference auto test_table = c<Test>();
#endif
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_alias auto test_alias = "t"_alias.for_<Test>();
#endif

    auto db =
        make_storage("",
                     make_table("Test", make_column("id", &Test::id, primary_key()), make_column("key", &Test::key)));
    db.sync_schema(true);

    std::vector<char> key(255);
    iota(key.begin(), key.end(), '\0');
    Test expected{5, key};

    db.replace(expected);
    std::vector<Test> expected_vec{expected};

    SECTION("range-based for") {
        for (Test&& obj: db.iterate(select(object<Test>()))) {
            REQUIRE(obj == expected);
        }
    }

#ifdef SQLITE_ORM_STL_HAS_DEFAULT_SENTINEL
    SECTION("borrowed iterator") {
        std::input_iterator auto begin = db.iterate(select(object<Test>())).begin();
        REQUIRE(*begin == expected);
        REQUIRE(++begin == std::default_sentinel);
    }
#endif

#if __cpp_lib_containers_ranges >= 202202L
#if !defined(__clang__) || (__clang_major__ >= 15)
    SECTION("from range") {
        std::ranges::view auto view = db.iterate(select(object<Test>()));
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif
#endif

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("with") {
        constexpr auto x = "x"_cte;
        std::input_iterator auto begin =
            db.iterate(with(x().as(select(asterisk<Test>())), select(struct_<Test>(asterisk<x>())))).begin();
        REQUIRE(*begin == expected);
    }
#endif
#endif

#ifdef SQLITE_ORM_CPP23_GENERATOR_SUPPORTED
    SECTION("object generator, classic") {
        auto view = db.yield<Test>();
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    SECTION("object generator, table reference") {
        auto view = db.yield<test_table>();
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("object generator, alias") {
        auto view = db.yield<test_alias>();
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif
    SECTION("select generator") {
        auto view = db.yield(select(object<Test>()));
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    SECTION("with generator") {
        constexpr auto x = "x"_cte;
        auto view = db.yield(with(x().as(select(asterisk<Test>())), select(struct_<Test>(asterisk<x>()))));
        REQUIRE(std::vector<Test>{std::from_range, view} == expected_vec);
    }
#endif
#endif
}
