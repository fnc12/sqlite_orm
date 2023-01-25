#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("member_traits_tests") {
    using internal::getter_field_type_t;
    using internal::is_getter;
    using internal::is_setter;
    using internal::member_field_type_t;
    using internal::member_object_type_t;
    using internal::object_field_type_t;
    using internal::setter_field_type_t;
    using std::is_same;
#if __cpp_lib_type_trait_variable_templates >= 201510L
    using std::is_same_v;
#endif

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

    STATIC_REQUIRE(!is_getter<decltype(&User::id)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByValConst)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByRefConst)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByRef)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByConstRefConst)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByConstRef)>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByValConstNoexcept)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByValNoexcept)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByRefConstNoexcept)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByRefNoexcept)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByConstRefConstNoexcept)>::value);
    STATIC_REQUIRE(is_getter<decltype(&User::getIdByConstRefNoExcept)>::value);
#endif
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByVal)>::value);
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByRef)>::value);
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByConstRef)>::value);
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByValueNoexcept)>::value);
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByRefNoExcept)>::value);
    STATIC_REQUIRE(!is_getter<decltype(&User::setIdByConstRefNoexcept)>::value);

    STATIC_REQUIRE(!is_setter<decltype(&User::id)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByValConst)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByRefConst)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByRef)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByConstRefConst)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByConstRef)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByValConstNoexcept)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByValNoexcept)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByRefConstNoexcept)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByRefNoexcept)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByConstRefConstNoexcept)>::value);
    STATIC_REQUIRE(!is_setter<decltype(&User::getIdByConstRefNoExcept)>::value);
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByVal)>::value);
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByRef)>::value);
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByConstRef)>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByValueNoexcept)>::value);
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByRefNoExcept)>::value);
    STATIC_REQUIRE(is_setter<decltype(&User::setIdByConstRefNoexcept)>::value);
#endif

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByValConst)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByValConst)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByValConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByRefConst)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByRefConst)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByRefConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByRef)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByRef)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByRef)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByConstRefConst)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByConstRefConst)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByConstRefConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByConstRef)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByConstRef)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByConstRef)>, int>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByValConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByValConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByValConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByValNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByValNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByValNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByRefConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByRefConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByRefConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByRefNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByRefNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByRefNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByConstRefNoExcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByConstRefNoExcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByConstRefNoExcept)>, int>);
#endif
    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByVal)>, User>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByVal)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByVal)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByRef)>, User>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByRef)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByRef)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByConstRef)>, User>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByConstRef)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByConstRef)>, int>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByValueNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByValueNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByValueNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByRefNoExcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByRefNoExcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByRefNoExcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByConstRefNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByConstRefNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByConstRefNoexcept)>, int>);
#endif
    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::id)>, User>::value);
    STATIC_REQUIRE(is_same<object_field_type_t<decltype(&User::id)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::id)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::id)>, User>::value);
    STATIC_REQUIRE(is_same<object_field_type_t<decltype(&User::id)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::id)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByValConst)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByValConst)>, int>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByValConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByRefConst)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByRefConst)>, int>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByRefConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByRef)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByRef)>, int>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByRef)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByConstRefConst)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByConstRefConst)>, int>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByConstRefConst)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByConstRef)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByConstRef)>, int>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByConstRef)>, int>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByValConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByValConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByValConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByValNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByValNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByValNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByRefConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByRefConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByRefConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByRefNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByRefNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByRefNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<getter_field_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::getIdByConstRefConstNoexcept)>, int>);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::getIdByConstRefNoExcept)>, User>::value);
    STATIC_REQUIRE(is_same<getter_field_type_t<decltype(&User::getIdByConstRefNoExcept)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::getIdByConstRefNoExcept)>, int>::value);
#endif
    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByVal)>, User>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByVal)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByVal)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByRef)>, User>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByRef)>, int>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByRef)>, int>::value);

    STATIC_REQUIRE(is_same<member_object_type_t<decltype(&User::setIdByConstRef)>, User>::value);
    STATIC_REQUIRE(is_same<member_field_type_t<decltype(&User::setIdByConstRef)>, int>::value);
    STATIC_REQUIRE(is_same<setter_field_type_t<decltype(&User::setIdByConstRef)>, int>::value);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByValueNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByValueNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByValueNoexcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByRefNoExcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByRefNoExcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByRefNoExcept)>, int>);

    STATIC_REQUIRE(is_same_v<member_object_type_t<decltype(&User::setIdByConstRefNoexcept)>, User>);
    STATIC_REQUIRE(is_same_v<setter_field_type_t<decltype(&User::setIdByConstRefNoexcept)>, int>);
    STATIC_REQUIRE(is_same_v<member_field_type_t<decltype(&User::setIdByConstRefNoexcept)>, int>);
#endif
}
