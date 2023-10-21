#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("table static columns_count") {
    struct User {
        int id = 0;
        std::string name;
    };
    {  //  1 column no pk
        auto table = make_table("users", make_column("id", &User::id));
        STATIC_REQUIRE(table.columns_count == 1);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  1 column with 1 inline pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()));
        STATIC_REQUIRE(table.columns_count == 1);
        STATIC_REQUIRE(table.primary_key_columns_count == 1);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  1 column with 1 inline pk autoincrement
        auto table = make_table("users", make_column("id", &User::id, primary_key().autoincrement()));
        STATIC_REQUIRE(table.columns_count == 1);
        STATIC_REQUIRE(table.primary_key_columns_count == 1);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  1 column with 1 dedicated pk
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id));
        STATIC_REQUIRE(table.columns_count == 1);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 1);
    }
    {  //  2 columns no pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  2 columns with 1 inline id pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("id", &User::name));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 1);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  2 columns with 1 inline name pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name, primary_key()));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 1);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 0);
    }
    {  //  2 columns with 1 dedicated id pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::id));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 1);
    }
    {  //  2 columns with 1 dedicated name pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::name));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 1);
    }
    {  //  2 columns with 2 dedicated pks
        auto table = make_table("users",
                                make_column("id", &User::id),
                                make_column("id", &User::name),
                                primary_key(&User::id, &User::name));
        STATIC_REQUIRE(table.columns_count == 2);
        STATIC_REQUIRE(table.primary_key_columns_count == 0);
        STATIC_REQUIRE(table.dedicated_primary_key_columns_count == 2);
    }
}
