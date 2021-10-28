#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

TEST_CASE("tuple_filter") {
    struct User {
        int id = 0;
    };
    SECTION("single") {
        SECTION("is_bindable") {
            SECTION("int") {
                using Arg = int;
                using Expected = std::tuple<int>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_bindable>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
            SECTION("std::string") {
                using Arg = std::string;
                using Expected = std::tuple<std::string>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_bindable>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
            SECTION("where_t") {
                using Arg = internal::where_t<bool>;
                using Expected = std::tuple<>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_bindable>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
            SECTION("order_by_t") {
                using Arg = internal::order_by_t<decltype(&User::id)>;
                using Expected = std::tuple<>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_bindable>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
        }
        SECTION("is_column") {
            SECTION("column_t") {
                auto column = make_column({}, &User::id);
                using Arg = decltype(column);
                using Expected = std::tuple<Arg>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_column>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
            SECTION("order_by_t") {
                using Arg = internal::order_by_t<decltype(&User::id)>;
                using Expected = std::tuple<>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_column>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
            SECTION("unique_t") {
                using Arg = decltype(unique(&User::id));
                using Expected = std::tuple<>;
                using ResultType = internal::tuple_filter_single<Arg, internal::is_column>::type;
                static_assert(std::is_same<ResultType, Expected>::value, "");
            }
        }
    }
    SECTION("multiple") {
        SECTION("is_bindable") {
            using Arg =
                std::tuple<int, std::string, internal::where_t<bool>, internal::order_by_t<decltype(&User::id)>>;
            using Expected = std::tuple<int, std::string>;
            using ResultType = internal::tuple_filter<Arg, internal::is_bindable>::type;
            static_assert(std::is_same<ResultType, Expected>::value, "");
        }
        SECTION("is_column") {
            auto column = make_column({}, &User::id);
            using Column = decltype(column);
            using OrderBy = internal::order_by_t<decltype(&User::id)>;
            using Unique = decltype(unique(&User::id));
            using Arg = std::tuple<Column, OrderBy, Unique>;
            using Expected = std::tuple<Column>;
            using ResultType = internal::tuple_filter<Arg, internal::is_column>::type;
            static_assert(std::is_same<ResultType, Expected>::value, "");
        }
    }
}
