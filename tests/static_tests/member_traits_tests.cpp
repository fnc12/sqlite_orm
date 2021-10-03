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
        mutable int id = 0;

        //  getter_by_value
        int getIdByValConst() const {
            return this->id;
        }

        //  getter_by_ref_const
        int& getIdByRefConst() const {
            return this->id;
        }

        //  getter_by_ref_const
        int& getIdByRef() {
            return this->id;
        }

        //  getter_by_const_ref_const
        const int& getIdByConstRefConst() const {
            return this->id;
        }

        //  getter_by_const_ref
        const int& getIdByConstRef() {
            return this->id;
        }

        //  getter_by_value_const_noexcept
        int getIdByValConstNoexcept() const noexcept {
            return this->id;
        }

        //  getter_by_value_noexcept
        int getIdByValNoexcept() noexcept {
            return this->id;
        }

        //  getter_by_ref_const_noexcept
        int& getIdByRefConstNoexcept() const noexcept {
            return this->id;
        }

        //  getter_by_ref_noexcept
        int& getIdByRefNoexcept() noexcept {
            return this->id;
        }

        //  getter_by_const_ref_const_noexcept
        const int& getIdByConstRefConstNoexcept() const noexcept {
            return this->id;
        }

        //  getter_by_const_ref_noexcept
        const int& getIdByConstRefNoExcept() noexcept {
            return this->id;
        }

        //  setter_by_value
        void setIdByVal(int id) {
            this->id = id;
        }

        //  setter_by_ref
        void setIdByRef(int& id) {
            this->id = id;
        }

        //  setter_by_const_ref
        void setIdByConstRef(const int& id) {
            this->id = id;
        }

        //  setter_by_value_noexcept
        void setIdByValueNoexcept(int id) noexcept {
            this->id = id;
        }

        //  setter_by_ref_noexcept
        void setIdByRefNoExcept(int& id) noexcept {
            this->id = id;
        }

        //  setter_by_const_ref_noexcept
        void setIdByConstRefNoexcept(const int& id) noexcept {
            this->id = id;
        }
    };

    static_assert(is_field_member_pointer<decltype(&User::id)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByValConst)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByConstRefConst)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByConstRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByValConstNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByValNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRefConstNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByRefNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByConstRefConstNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::getIdByConstRefNoExcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByVal)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByConstRef)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByValueNoexcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByRefNoExcept)>::value, "");
    static_assert(!is_field_member_pointer<decltype(&User::setIdByConstRefNoexcept)>::value, "");

    static_assert(!is_getter<decltype(&User::id)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByValConst)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRef)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByConstRefConst)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByConstRef)>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_getter<decltype(&User::getIdByValConstNoexcept)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByValNoexcept)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRefConstNoexcept)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByRefNoexcept)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByConstRefConstNoexcept)>::value, "");
    static_assert(is_getter<decltype(&User::getIdByConstRefNoExcept)>::value, "");
#endif
    static_assert(!is_getter<decltype(&User::setIdByVal)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByRef)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByConstRef)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByValueNoexcept)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByRefNoExcept)>::value, "");
    static_assert(!is_getter<decltype(&User::setIdByConstRefNoexcept)>::value, "");

    static_assert(!is_setter<decltype(&User::id)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByValConst)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRefConst)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRef)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByConstRefConst)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByConstRef)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByValConstNoexcept)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByValNoexcept)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRefConstNoexcept)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByRefNoexcept)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByConstRefConstNoexcept)>::value, "");
    static_assert(!is_setter<decltype(&User::getIdByConstRefNoExcept)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByVal)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByRef)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByConstRef)>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_setter<decltype(&User::setIdByValueNoexcept)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByRefNoExcept)>::value, "");
    static_assert(is_setter<decltype(&User::setIdByConstRefNoexcept)>::value, "");
#endif

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConst)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConst)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRef)>::field_type, int>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConstNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValConstNoexcept)>::field_type, int>::value,
                  "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByValNoexcept)>::field_type, int>::value, "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConstNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefConstNoexcept)>::field_type, int>::value,
                  "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByRefNoexcept)>::field_type, int>::value, "");

    static_assert(
        is_same<typename getter_traits<decltype(&User::getIdByConstRefConstNoexcept)>::object_type, User>::value,
        "");
    static_assert(
        is_same<typename getter_traits<decltype(&User::getIdByConstRefConstNoexcept)>::field_type, int>::value,
        "");

    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRefNoExcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename getter_traits<decltype(&User::getIdByConstRefNoExcept)>::field_type, int>::value,
                  "");
#endif
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByVal)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByVal)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRef)>::field_type, int>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByValueNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByValueNoexcept)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRefNoExcept)>::object_type, User>::value, "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByRefNoExcept)>::field_type, int>::value, "");

    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRefNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename setter_traits<decltype(&User::setIdByConstRefNoexcept)>::field_type, int>::value,
                  "");
#endif
    static_assert(is_same<typename field_member_traits<decltype(&User::id)>::object_type, User>::value, "");
    static_assert(is_same<typename field_member_traits<decltype(&User::id)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::id)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::id)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConst)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConst)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRefConst)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRefConst)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRef)>::field_type, int>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConstNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByValConstNoexcept)>::field_type, int>::value,
                  "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByValNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByValNoexcept)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConstNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefConstNoexcept)>::field_type, int>::value,
                  "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByRefNoexcept)>::field_type, int>::value, "");

    static_assert(
        is_same<typename member_traits<decltype(&User::getIdByConstRefConstNoexcept)>::object_type, User>::value,
        "");
    static_assert(
        is_same<typename member_traits<decltype(&User::getIdByConstRefConstNoexcept)>::field_type, int>::value,
        "");

    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRefNoExcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename member_traits<decltype(&User::getIdByConstRefNoExcept)>::field_type, int>::value,
                  "");
#endif
    static_assert(is_same<typename member_traits<decltype(&User::setIdByVal)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByVal)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByRef)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRef)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRef)>::field_type, int>::value, "");
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    static_assert(is_same<typename member_traits<decltype(&User::setIdByValueNoexcept)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByValueNoexcept)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByRefNoExcept)>::object_type, User>::value, "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByRefNoExcept)>::field_type, int>::value, "");

    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRefNoexcept)>::object_type, User>::value,
                  "");
    static_assert(is_same<typename member_traits<decltype(&User::setIdByConstRefNoexcept)>::field_type, int>::value,
                  "");
#endif
}
