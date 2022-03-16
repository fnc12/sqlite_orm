#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator conditions") {
    internal::serializator_context_base context;

    SECTION("static assertions") {
#if __cplusplus >= 201703L  // use of C++17 or higher
        // must assert
        //constexpr auto n = 0_nth_col;

        STATIC_REQUIRE(std::is_same_v<internal::nth_constant<1u>, decltype(1_nth_col)>);
        STATIC_REQUIRE(std::is_same_v<internal::nth_constant<10u>, decltype(10_nth_col)>);
#endif
    }
    SECTION("literals") {
        SECTION("dump") {
            context.replace_bindable_with_question = false;
            auto expr = 2_nth_col;
            auto value = serialize(expr, context);
            REQUIRE(value == "2");
        }
        SECTION("bindable") {
            context.replace_bindable_with_question = true;
            auto expr = 2_nth_col;
            auto value = serialize(expr, context);
            REQUIRE(value == "2");
        }
    }
#if 0  // outline; order_by statically asserts when passed bindable values
    SECTION("order by bindable") {
        order_by(std::wstring_view{L""});
        SECTION("dump") {
            context.replace_bindable_with_question = false;
            auto expr = order_by(2);
            auto value = serialize(expr, context);
            REQUIRE(value == "ORDER BY 2");
        }
        SECTION("bindable") {
            context.replace_bindable_with_question = true;
            auto expr = order_by(2);
            auto value = serialize(expr, context);
            REQUIRE(value == "ORDER BY ?");
        }
    }
#endif
    SECTION("order by kth column") {
        SECTION("dump") {
            context.replace_bindable_with_question = false;
            auto expr = order_by(2_nth_col);
            auto value = serialize(expr, context);
            REQUIRE(value == "ORDER BY 2");
        }
        SECTION("bindable") {
            context.replace_bindable_with_question = true;
            auto expr = order_by(2_nth_col);
            auto value = serialize(expr, context);
            REQUIRE(value == "ORDER BY 2");
        }
    }
    SECTION("using") {
        struct User {
            int64 id;
        };

        auto t1 = make_table("user", make_column("id", &User::id));
        auto storage = internal::storage_impl<decltype(t1)>{t1};
        using storage_impl = decltype(storage);

        internal::serializator_context<storage_impl> ctx{storage};

        SECTION("using column") {
            auto expr = using_(&User::id);
            auto value = serialize(expr, ctx);
            REQUIRE(value == R"(USING ("id"))");
        }
        SECTION("using explicit column") {
            auto expr = using_(column<User>(&User::id));
            auto value = serialize(expr, ctx);
            REQUIRE(value == R"(USING ("id"))");
        }
    }
}
