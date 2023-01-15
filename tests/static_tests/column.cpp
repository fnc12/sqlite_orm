#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Column") {
    {
        using column_type = decltype(make_column("id", &User::id));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, int User::*>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, internal::empty_setter>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRefConst, &User::setIdByVal));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, const int& (User::*)() const>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(int)>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByVal, &User::getIdByRefConst));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, const int& (User::*)() const>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(int)>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRef, &User::setIdByConstRef));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, const int& (User::*)()>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(const int&)>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByConstRef, &User::getIdByRef));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, const int& (User::*)()>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(const int&)>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByValConst, &User::setIdByRef));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, int (User::*)() const>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(int&)>::value);
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByRef, &User::getIdByValConst));
        STATIC_REQUIRE(std::tuple_size<column_type::constraints_type>::value == 0);
        STATIC_REQUIRE(std::is_same<column_type::object_type, User>::value);
        STATIC_REQUIRE(std::is_same<column_type::field_type, int>::value);
        STATIC_REQUIRE(std::is_same<column_type::member_pointer_t, int (User::*)() const>::value);
        STATIC_REQUIRE(std::is_same<column_type::setter_type, void (User::*)(int&)>::value);
    }
    {
        using column_type = decltype(column<Token>(&Token::id));
        STATIC_REQUIRE(std::is_same<column_type::type, Token>::value);
        using field_type = column_type::field_type;
        STATIC_REQUIRE(std::is_same<field_type, decltype(&Object::id)>::value);
        STATIC_REQUIRE(std::is_same<internal::table_type_of<column_type>::type, Token>::value);
        STATIC_REQUIRE(std::is_same<internal::table_type_of<field_type>::type, Object>::value);
        STATIC_REQUIRE(std::is_same<internal::column_result_t<internal::storage_t<>, field_type>::type, int>::value);
        STATIC_REQUIRE(std::is_member_pointer<field_type>::value);
        STATIC_REQUIRE(!std::is_member_function_pointer<field_type>::value);
    }
}
