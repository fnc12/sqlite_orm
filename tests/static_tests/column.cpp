#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include "static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Column") {
    {
        using column_type = decltype(make_column("id", &User::id));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int& (User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRefConst, &User::setIdByVal));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int& (User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByVal, &User::getIdByRefConst));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int& (User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRef, &User::setIdByConstRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int& (User::*)()>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(const int&)>::value,
                      "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByConstRef, &User::getIdByRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int& (User::*)()>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(const int&)>::value,
                      "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByValConst, &User::setIdByRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, int (User::*)() const>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int&)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByRef, &User::getIdByValConst));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, int (User::*)() const>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int&)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(column<Token>(&Token::id));
        static_assert(std::is_same<column_type::type, Token>::value, "Incorrect column type");
        using field_type = column_type::field_type;
        static_assert(std::is_same<field_type, decltype(&Object::id)>::value, "Incorrect field type");
        static_assert(std::is_same<internal::table_type<field_type>::type, Object>::value, "Incorrect mapped type");
        static_assert(std::is_same<internal::column_result_t<internal::storage_t<>, field_type>::type, int>::value,
                      "Incorrect field type");
        static_assert(std::is_member_pointer<field_type>::value, "Field type is not a member pointer");
        static_assert(!std::is_member_function_pointer<field_type>::value, "Field type is not a member pointer");
    }
}
