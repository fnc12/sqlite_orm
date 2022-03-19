#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

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

        void setNameByConstRef(const std::string& name_) {
            this->name = name_;
        }

        const int& getConstIdByRefConst() const {
            return this->id;
        }

        void setIdByRef(int& id_) {
            this->id = id_;
        }

        const std::string& getConstNameByRefConst() const {
            return this->name;
        }

        void setNameByRef(std::string& name_) {
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
    STATIC_REQUIRE(std::is_same<decltype(storage0.max(&User::id))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.max(&User::getIdByValConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.max(&User::setIdByVal))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.max(&User::getConstIdByRefConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.max(&User::setIdByRef))::element_type, int>::value);

    STATIC_REQUIRE(
        std::is_same<decltype(storage0.max(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.max(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.max(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.max(&User::getConstIdByRefConst,
                                                      where(lesser_than(&User::id, 10))))::element_type,
                                int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage2.max(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);

    STATIC_REQUIRE(std::is_same<decltype(storage0.min(&User::id))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.min(&User::getIdByValConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.min(&User::setIdByVal))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.min(&User::getConstIdByRefConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.min(&User::setIdByRef))::element_type, int>::value);

    STATIC_REQUIRE(
        std::is_same<decltype(storage0.min(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.min(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.min(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.min(&User::getConstIdByRefConst,
                                                      where(lesser_than(&User::id, 10))))::element_type,
                                int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage2.min(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);

    STATIC_REQUIRE(std::is_same<decltype(storage0.sum(&User::id))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.sum(&User::getIdByValConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage1.sum(&User::setIdByVal))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.sum(&User::getConstIdByRefConst))::element_type, int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.sum(&User::setIdByRef))::element_type, int>::value);

    STATIC_REQUIRE(
        std::is_same<decltype(storage0.sum(&User::id, where(lesser_than(&User::id, 10))))::element_type, int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.sum(&User::getIdByValConst, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage1.sum(&User::setIdByVal, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
    STATIC_REQUIRE(std::is_same<decltype(storage2.sum(&User::getConstIdByRefConst,
                                                      where(lesser_than(&User::id, 10))))::element_type,
                                int>::value);
    STATIC_REQUIRE(
        std::is_same<decltype(storage2.sum(&User::setIdByRef, where(lesser_than(&User::id, 10))))::element_type,
                     int>::value);
}
