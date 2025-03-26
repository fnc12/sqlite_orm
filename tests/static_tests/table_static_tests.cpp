#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::is_column;
using internal::is_primary_key;

template<class Elements>
using dedicated_pk_columns_count_t =
    internal::nested_tuple_size_for_t<internal::columns_tuple_t,
                                      Elements,
                                      internal::filter_tuple_sequence_t<Elements, is_primary_key>>;

TEST_CASE("table static count_of<is_column>()") {
    struct User {
        int id = 0;
        std::string name;
    };
    {  //  1 column no pk
        auto table = make_table("users", make_column("id", &User::id));
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  1 column with 1 inline pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()));
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  1 column with 1 inline pk autoincrement
        auto table = make_table("users", make_column("id", &User::id, primary_key().autoincrement()));
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  1 column with 1 dedicated pk
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id));
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 1);
    }
    {  //  1 column with 1 dedicated pk autoincrement
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id).autoincrement());
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 1);
    }
    {  //  2 columns no pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  2 columns with 1 inline id pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("id", &User::name));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  2 columns with 1 inline name pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name, primary_key()));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 0);
    }
    {  //  2 columns with 1 dedicated id pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::id));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 1);
    }
    {  //  2 columns with 1 dedicated name pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::name));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 1);
    }
    {  //  2 columns with 2 dedicated pks
        auto table = make_table("users",
                                make_column("id", &User::id),
                                make_column("id", &User::name),
                                primary_key(&User::id, &User::name));
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<decltype(table.elements)>::value == 2);
    }
}
