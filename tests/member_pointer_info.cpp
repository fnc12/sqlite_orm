#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("member_pointer_info") {
    using internal::member_pointer_info;
    {
        struct User {
            int id = 0;
            std::string firstName;
            std::string lastName;
            time_t birthDate = 0;
        };
        member_pointer_info idInfo{&User::id};
        member_pointer_info firstNameInfo{&User::firstName};
        member_pointer_info lastNameInfo{&User::lastName};
        member_pointer_info birthDateInfo{&User::birthDate};
        REQUIRE(idInfo != firstNameInfo);
        REQUIRE(idInfo != lastNameInfo);
        REQUIRE(idInfo != birthDateInfo);
    }
    {
        struct User {

            int id() const {
                return this->_id;
            }

            void setId(int value) {
                this->_id = value;
            }

            const std::string &name() const {
                return this->_name;
            }

            void setName(std::string value) {
                this->_name = move(value);
            }

            float age() const {
                return this->_age;
            }

            void setAge(float value) {
                this->_age = value;
            }

          private:
            int _id = 0;
            std::string _name;
            float _age = 0;
        };
        member_pointer_info idGetterInfo{&User::id};
        member_pointer_info idSetterInfo{&User::setId};
        member_pointer_info nameGetterInfo{&User::name};
        member_pointer_info nameSetterInfo{&User::setName};
        member_pointer_info ageGetterInfo{&User::age};
        member_pointer_info ageSetterInfo{&User::setAge};
        REQUIRE(idGetterInfo != idSetterInfo);
        REQUIRE(idGetterInfo != nameGetterInfo);
        REQUIRE(idGetterInfo != nameSetterInfo);
        REQUIRE(idGetterInfo != ageGetterInfo);
        REQUIRE(idGetterInfo != ageSetterInfo);
    }
}
