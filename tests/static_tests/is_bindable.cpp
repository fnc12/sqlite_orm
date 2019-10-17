#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <memory>   //  std::unique_ptr, std::shared_ptr
#include <string>   //  std::string

using namespace sqlite_orm;

TEST_CASE("is_bindable") {
    struct User {
        int id;
    };
    static_assert(internal::is_bindable<bool>::value, "bool must be bindable");
    static_assert(internal::is_bindable<int>::value, "int must be bindable");
    static_assert(internal::is_bindable<long>::value, "long must be bindable");
    static_assert(internal::is_bindable<float>::value, "float must be bindable");
    static_assert(internal::is_bindable<double>::value, "double must be bindable");
    static_assert(internal::is_bindable<std::string>::value, "string must be bindable");
    static_assert(internal::is_bindable<std::nullptr_t>::value, "null must be bindable");
    static_assert(internal::is_bindable<std::unique_ptr<int>>::value, "unique_ptr must be bindable");
    static_assert(internal::is_bindable<std::shared_ptr<int>>::value, "shared_ptr must be bindable");
    
    static_assert(!internal::is_bindable<void>::value, "void cannot be bindable");
    auto isEqual = is_equal(&User::id, 5);
    static_assert(!internal::is_bindable<decltype(isEqual)>::value, "is_equal cannot be bindable");
    auto notEqual = is_not_equal(&User::id, 10);
    static_assert(!internal::is_bindable<decltype(notEqual)>::value, "is_not_equal cannot be bindable");
    auto lesserThan = lesser_than(&User::id, 10);
    static_assert(!internal::is_bindable<decltype(lesserThan)>::value, "lesser_than cannot be bindable");
    auto lesserOrEqual = lesser_or_equal(&User::id, 5);
    static_assert(!internal::is_bindable<decltype(lesserOrEqual)>::value, "lesser_or_equal cannot be bindable");
    auto greaterThan = greater_than(&User::id, 5);
    static_assert(!internal::is_bindable<decltype(greaterThan)>::value, "greater_than cannot be bindable");
    auto greaterOrEqual = greater_or_equal(&User::id, 5);
    static_assert(!internal::is_bindable<decltype(greaterOrEqual)>::value, "greater_or_equal cannot be bindable");
}
