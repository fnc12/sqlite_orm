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
        REQUIRE(idInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(idInfo.field_index == std::type_index{typeid(int)});
        REQUIRE(idInfo.t == member_pointer_info::type::member);

        member_pointer_info firstNameInfo{&User::firstName};
        REQUIRE(firstNameInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(firstNameInfo.field_index == std::type_index{typeid(std::string)});
        REQUIRE(firstNameInfo.t == member_pointer_info::type::member);

        member_pointer_info lastNameInfo{&User::lastName};
        REQUIRE(lastNameInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(lastNameInfo.field_index == std::type_index{typeid(std::string)});
        REQUIRE(lastNameInfo.t == member_pointer_info::type::member);

        member_pointer_info birthDateInfo{&User::birthDate};
        REQUIRE(birthDateInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(birthDateInfo.field_index == std::type_index{typeid(time_t)});
        REQUIRE(birthDateInfo.t == member_pointer_info::type::member);

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
        REQUIRE(idGetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(idGetterInfo.field_index == std::type_index{typeid(int)});
        REQUIRE(idGetterInfo.t == member_pointer_info::type::getter);

        member_pointer_info idSetterInfo{&User::setId};
        REQUIRE(idSetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(idSetterInfo.field_index == std::type_index{typeid(int)});
        REQUIRE(idSetterInfo.t == member_pointer_info::type::setter);

        member_pointer_info nameGetterInfo{&User::name};
        REQUIRE(nameGetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(nameGetterInfo.field_index == std::type_index{typeid(std::string)});
        REQUIRE(nameGetterInfo.t == member_pointer_info::type::getter);

        member_pointer_info nameSetterInfo{&User::setName};
        REQUIRE(nameSetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(nameSetterInfo.field_index == std::type_index{typeid(std::string)});
        REQUIRE(nameSetterInfo.t == member_pointer_info::type::setter);

        member_pointer_info ageGetterInfo{&User::age};
        REQUIRE(ageGetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(ageGetterInfo.field_index == std::type_index{typeid(float)});
        REQUIRE(ageGetterInfo.t == member_pointer_info::type::getter);

        member_pointer_info ageSetterInfo{&User::setAge};
        REQUIRE(ageSetterInfo.type_index == std::type_index{typeid(User)});
        REQUIRE(ageSetterInfo.field_index == std::type_index{typeid(float)});
        REQUIRE(ageSetterInfo.t == member_pointer_info::type::setter);

        REQUIRE(idGetterInfo != idSetterInfo);
        REQUIRE(idGetterInfo != nameGetterInfo);
        REQUIRE(idGetterInfo != nameSetterInfo);
        REQUIRE(idGetterInfo != ageGetterInfo);
        REQUIRE(idGetterInfo != ageSetterInfo);
    }
}
