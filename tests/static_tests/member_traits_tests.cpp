#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("member_traits_tests") {
    using internal::field_member_traits;
    using internal::getter_traits;
    using internal::is_field_member_pointer;
    using internal::is_getter;
    using internal::is_setter;
    using internal::member_traits;
    using internal::setter_traits;
    using std::is_same;

    struct User {
        int id;

        const int& getIdByRefConst() const {
            return this->id;
        }

        const int& getIdByRef() {
            return this->id;
        }

        int getIdByValConst() const {
            return this->id;
        }

        void setIdByVal(int id) {
            this->id = id;
        }

        void setIdByConstRef(const int& id) {
            this->id = id;
        }

        void setIdByRef(int& id) {
            this->id = id;
        }
    };

    static_assert(is_field_member_pointer<decltype(&User::id)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByValConst)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByVal)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByConstRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByRef)>::value, "");

    static_assert(!is_getter<decltype(&User::id)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRef)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByValConst)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByVal)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByConstRef)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByRef)>::value, "");

    static_assert(!is_setter<decltype(&User::id)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRef)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByValConst)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByVal)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByConstRef)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByRef)>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConst)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConst)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByVal)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByVal)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRef)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename field_member_traits<decltype(&User::id)>::object_type, User>::value, "");
    static_assert(is_same<typename field_member_traits<decltype(&User::id)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::id)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::id)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConst)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConst)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByVal)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByVal)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRef)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByRef)>::field_type, int>::value, "");
}
