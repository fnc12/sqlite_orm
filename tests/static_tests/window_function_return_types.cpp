#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same
#include <memory>  //  std::unique_ptr

using namespace sqlite_orm;

template<class Type, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<Type, E>::value);
}

TEST_CASE("window function return types") {
    struct User {
        int id = 0;
        std::string name;
        double salary = 0;
    };

    auto table = make_table("users",
                            make_column("id", &User::id, primary_key()),
                            make_column("name", &User::name),
                            make_column("salary", &User::salary));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;

    SECTION("nullary window functions") {
        do_assert<internal::column_result_of_t<db_objects_t, decltype(row_number().over(order_by(&User::id)))>, int>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(rank().over(order_by(&User::id)))>, int>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(dense_rank().over(order_by(&User::id)))>, int>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(percent_rank().over(order_by(&User::id)))>,
                  double>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(cume_dist().over(order_by(&User::id)))>,
                  double>();
    }
    SECTION("ntile") {
        do_assert<internal::column_result_of_t<db_objects_t, decltype(ntile(4).over(order_by(&User::id)))>, int>();
    }
    SECTION("lag/lead deduce type from first arg") {
        do_assert<internal::column_result_of_t<db_objects_t, decltype(lag(&User::name).over(order_by(&User::id)))>,
                  std::string>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(lag(&User::id, 2).over(order_by(&User::id)))>,
                  int>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(lead(&User::salary).over(order_by(&User::id)))>,
                  double>();
    }
    SECTION("first_value/last_value/nth_value deduce type from first arg") {
        do_assert<
            internal::column_result_of_t<db_objects_t, decltype(first_value(&User::name).over(order_by(&User::id)))>,
            std::string>();
        do_assert<
            internal::column_result_of_t<db_objects_t, decltype(last_value(&User::salary).over(order_by(&User::id)))>,
            double>();
        do_assert<
            internal::column_result_of_t<db_objects_t, decltype(nth_value(&User::id, 3).over(order_by(&User::id)))>,
            int>();
    }
    SECTION("aggregate over_t delegates to aggregate return type") {
        do_assert<internal::column_result_of_t<db_objects_t, decltype(sum(&User::id).over(order_by(&User::id)))>,
                  std::unique_ptr<double>>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(avg(&User::id).over(order_by(&User::id)))>,
                  double>();
        do_assert<internal::column_result_of_t<db_objects_t, decltype(count(&User::id).over(order_by(&User::id)))>,
                  int>();
    }
    SECTION("filtered aggregate over_t") {
        do_assert<internal::column_result_of_t<
                      db_objects_t,
                      decltype(count(&User::id).filter(where(c(&User::id) > 0)).over(order_by(&User::id)))>,
                  int>();
    }
}
