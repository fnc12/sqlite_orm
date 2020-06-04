#include <sqlite_orm/sqlite_orm.h>
#include <tuple>
#include <type_traits>
#include <catch2/catch.hpp>

#include "static_tests/static_tests_common.h"

using namespace sqlite_orm;

TEST_CASE("Column") {
    {
        using column_type = decltype(make_column("id", &User::id));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int &(User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRefConst, &User::setIdByVal));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int &(User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByVal, &User::getIdByRefConst));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int &(User::*)() const>::value,
                      "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByRef, &User::setIdByConstRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int &(User::*)()>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(const int &)>::value,
                      "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByConstRef, &User::getIdByRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, const int &(User::*)()>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(const int &)>::value,
                      "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::getIdByValConst, &User::setIdByRef));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, int (User::*)() const>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int &)>::value, "Incorrect setter_type");
    }
    {
        using column_type = decltype(make_column("id", &User::setIdByRef, &User::getIdByValConst));
        static_assert(std::tuple_size<column_type::constraints_type>::value == 0, "Incorrect constraints_type size");
        static_assert(std::is_same<column_type::object_type, User>::value, "Incorrect object_type");
        static_assert(std::is_same<column_type::field_type, int>::value, "Incorrect field_type");
        static_assert(std::is_same<column_type::member_pointer_t, int User::*>::value, "Incorrect member pointer type");
        static_assert(std::is_same<column_type::getter_type, int (User::*)() const>::value, "Incorrect getter_type");
        static_assert(std::is_same<column_type::setter_type, void (User::*)(int &)>::value, "Incorrect setter_type");
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

TEST_CASE("Aggregate function return types") {
    struct User {
        int id;
        std::string name;

        int getIdByValConst() const {
            return this->id;
        }

        void setIdByVal(int id_) {
            this->id = id_;
        }

        std::string getNameByVal() {
            return this->name;
        }

        void setNameByConstRef(const std::string &name_) {
            this->name = name_;
        }

        const int &getConstIdByRefConst() const {
            return this->id;
        }

        void setIdByRef(int &id_) {
            this->id = id_;
        }

        const std::string &getConstNameByRefConst() const {
            return this->name;
        }

        void setNameByRef(std::string &name_) {
            this->name = std::move(name_);
        }
    };
    const std::string filename = "static_tests.sqlite";
    auto storage0 = make_storage(
        filename,
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    auto storage1 = make_storage(filename,
                                 make_table("users",
                                            make_column("id", &User::getIdByValConst, &User::setIdByVal, primary_key()),
                                            make_column("name", &User::setNameByConstRef, &User::getNameByVal)));
    auto storage2 =
        make_storage(filename,
                     make_table("users",
                                make_column("id", &User::getConstIdByRefConst, &User::setIdByRef, primary_key()),
                                make_column("name", &User::getConstNameByRefConst, &User::setNameByRef)));
    static_assert(std::is_same<decltype(storage0.max(&User::id))::element_type, int>::value, "Incorrect max value");
    static_assert(std::is_same<decltype(storage1.max(&User::getIdByValConst))::element_type, int>::value,
                  "Incorrect max value");
    static_assert(std::is_same<decltype(storage1.max(&User::setIdByVal))::element_type, int>::value,
                  "Incorrect max value");
    static_assert(std::is_same<decltype(storage2.max(&User::getConstIdByRefConst))::element_type, int>::value,
                  "Incorrect max value");
    static_assert(std::is_same<decltype(storage2.max(&User::setIdByRef))::element_type, int>::value,
                  "Incorrect max value");

    static_assert(
        std::is_same<decltype(storage0.max(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value,
        "Incorrect max value");
    static_assert(
        std::is_same<decltype(storage1.max(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect max value");
    static_assert(
        std::is_same<decltype(storage1.max(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect max value");
    static_assert(std::is_same<decltype(storage2.max(&User::getConstIdByRefConst,
                                                     where(lesser_than(&User::id, 10))))::element_type,
                               int>::value,
                  "Incorrect max value");
    static_assert(
        std::is_same<decltype(storage2.max(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect max value");

    static_assert(std::is_same<decltype(storage0.min(&User::id))::element_type, int>::value, "Incorrect min value");
    static_assert(std::is_same<decltype(storage1.min(&User::getIdByValConst))::element_type, int>::value,
                  "Incorrect min value");
    static_assert(std::is_same<decltype(storage1.min(&User::setIdByVal))::element_type, int>::value,
                  "Incorrect min value");
    static_assert(std::is_same<decltype(storage2.min(&User::getConstIdByRefConst))::element_type, int>::value,
                  "Incorrect min value");
    static_assert(std::is_same<decltype(storage2.min(&User::setIdByRef))::element_type, int>::value,
                  "Incorrect min value");

    static_assert(
        std::is_same<decltype(storage0.min(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value,
        "Incorrect min value");
    static_assert(
        std::is_same<decltype(storage1.min(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect min value");
    static_assert(
        std::is_same<decltype(storage1.min(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect min value");
    static_assert(std::is_same<decltype(storage2.min(&User::getConstIdByRefConst,
                                                     where(lesser_than(&User::id, 10))))::element_type,
                               int>::value,
                  "Incorrect min value");
    static_assert(
        std::is_same<decltype(storage2.min(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect min value");

    static_assert(std::is_same<decltype(storage0.sum(&User::id))::element_type, int>::value, "Incorrect sum value");
    static_assert(std::is_same<decltype(storage1.sum(&User::getIdByValConst))::element_type, int>::value,
                  "Incorrect sum value");
    static_assert(std::is_same<decltype(storage1.sum(&User::setIdByVal))::element_type, int>::value,
                  "Incorrect sum value");
    static_assert(std::is_same<decltype(storage2.sum(&User::getConstIdByRefConst))::element_type, int>::value,
                  "Incorrect sum value");
    static_assert(std::is_same<decltype(storage2.sum(&User::setIdByRef))::element_type, int>::value,
                  "Incorrect sum value");

    static_assert(
        std::is_same<decltype(storage0.sum(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value,
        "Incorrect sum value");
    static_assert(
        std::is_same<decltype(storage1.sum(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect sum value");
    static_assert(
        std::is_same<decltype(storage1.sum(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect sum value");
    static_assert(std::is_same<decltype(storage2.sum(&User::getConstIdByRefConst,
                                                     where(lesser_than(&User::id, 10))))::element_type,
                               int>::value,
                  "Incorrect sum value");
    static_assert(
        std::is_same<decltype(storage2.sum(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value,
        "Incorrect sum value");
}

TEST_CASE("Compound operators") {
    auto unionValue = union_(select(&User::id), select(&Token::id));
    static_assert(internal::is_base_of_template<decltype(unionValue), internal::compound_operator>::value,
                  "union must be base of compound_operator");
    auto exceptValue = except(select(&User::id), select(&Token::id));
    static_assert(internal::is_base_of_template<decltype(exceptValue), internal::compound_operator>::value,
                  "except must be base of compound_operator");
}
