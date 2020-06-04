#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Type parsing") {
    using namespace sqlite_orm::internal;

    //  int
    REQUIRE(*to_sqlite_type("INT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("integeer") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INTEGER") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("TINYINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("SMALLINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("MEDIUMINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("BIGINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("UNSIGNED BIG INT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INT2") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INT8") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("UNSIGNED INT(6)") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("UNSIGNED  INT(4)") == sqlite_type::INTEGER);

    //  text
    REQUIRE(*to_sqlite_type("TEXT") == sqlite_type::TEXT);
    REQUIRE(*to_sqlite_type("CLOB") == sqlite_type::TEXT);
    for(auto i = 0; i < 255; ++i) {
        REQUIRE(*to_sqlite_type("CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("VARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("VARYING CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NATIVE CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NVARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
    }

    //  blob..
    REQUIRE(*to_sqlite_type("BLOB") == sqlite_type::BLOB);

    //  real
    REQUIRE(*to_sqlite_type("REAL") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DOUBLE") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DOUBLE PRECISION") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("FLOAT") == sqlite_type::REAL);

    REQUIRE(*to_sqlite_type("NUMERIC") == sqlite_type::REAL);
    for(auto i = 0; i < 255; ++i) {
        for(auto j = 0; j < 10; ++j) {
            REQUIRE(*to_sqlite_type("DECIMAL(" + std::to_string(i) + "," + std::to_string(j) + ")") ==
                    sqlite_type::REAL);
        }
    }
    REQUIRE(*to_sqlite_type("BOOLEAN") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DATE") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DATETIME") == sqlite_type::REAL);

    REQUIRE(type_is_nullable<bool>::value == false);
    REQUIRE(type_is_nullable<char>::value == false);
    REQUIRE(type_is_nullable<unsigned char>::value == false);
    REQUIRE(type_is_nullable<signed char>::value == false);
    REQUIRE(type_is_nullable<short>::value == false);
    REQUIRE(type_is_nullable<unsigned short>::value == false);
    REQUIRE(type_is_nullable<int>::value == false);
    REQUIRE(type_is_nullable<unsigned int>::value == false);
    REQUIRE(type_is_nullable<long>::value == false);
    REQUIRE(type_is_nullable<unsigned long>::value == false);
    REQUIRE(type_is_nullable<long long>::value == false);
    REQUIRE(type_is_nullable<unsigned long long>::value == false);
    REQUIRE(type_is_nullable<float>::value == false);
    REQUIRE(type_is_nullable<double>::value == false);
    REQUIRE(type_is_nullable<long double>::value == false);
    REQUIRE(type_is_nullable<long double>::value == false);
    REQUIRE(type_is_nullable<std::string>::value == false);
    REQUIRE(type_is_nullable<std::unique_ptr<int>>::value == true);
    REQUIRE(type_is_nullable<std::unique_ptr<std::string>>::value == true);
    REQUIRE(type_is_nullable<std::shared_ptr<int>>::value == true);
    REQUIRE(type_is_nullable<std::shared_ptr<std::string>>::value == true);

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    REQUIRE(type_is_nullable<std::optional<int>>::value == true);
    REQUIRE(type_is_nullable<std::optional<std::string>>::value == true);
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
}
