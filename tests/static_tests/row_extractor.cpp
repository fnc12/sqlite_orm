#include <sqlite_orm/sqlite_orm.h>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#define ENABLE_THIS_UT
#endif

#ifdef ENABLE_THIS_UT
#include <catch2/catch_all.hpp>
#include <memory>  //  std::unique_ptr, std::shared_ptr
#include <string>  //  std::string
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  //  std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
#include <string_view>
#endif

using namespace sqlite_orm;

template<class T>
static void check_extractable() {
    STATIC_CHECK(orm_column_text_extractable<T>);
    STATIC_CHECK(orm_row_value_extractable<T>);
    STATIC_CHECK(orm_boxed_value_extractable<T>);
}

template<class T>
static void check_not_extractable() {
    STATIC_CHECK_FALSE(orm_column_text_extractable<T>);
    STATIC_CHECK_FALSE(orm_row_value_extractable<T>);
    STATIC_CHECK_FALSE(orm_boxed_value_extractable<T>);
}

namespace {
    enum class custom_enum { custom };

    template<class Elem>
    class StringVeneer : public std::basic_string<Elem> {};

    struct User {
        int id;
    };
}

template<>
struct sqlite_orm::row_extractor<custom_enum> {
    custom_enum extract(const char* /*columnText*/) const {
        return custom_enum::custom;
    }
    custom_enum extract(sqlite3_stmt*, int /*columnIndex*/) const {
        return custom_enum::custom;
    }
    custom_enum extract(sqlite3_value*) const {
        return custom_enum::custom;
    }
};

TEST_CASE("is_extractable") {
    check_extractable<bool>();
    check_extractable<char>();
    check_extractable<signed char>();
    check_extractable<unsigned char>();
    check_extractable<short>();
    check_extractable<unsigned short>();
    check_extractable<int>();
    check_extractable<unsigned int>();
    check_extractable<long>();
    check_extractable<unsigned long>();
    check_extractable<float>();
    check_extractable<long long>();
    check_extractable<unsigned long long>();
    check_extractable<double>();
    check_not_extractable<const char*>();
    check_extractable<std::string>();
    check_extractable<StringVeneer<char>>();
#ifndef SQLITE_ORM_OMITS_CODECVT
    check_not_extractable<const wchar_t*>();
    check_extractable<std::wstring>();
    check_not_extractable<StringVeneer<wchar_t>>();
#endif
    check_extractable<std::nullptr_t>();
    check_extractable<std::unique_ptr<int>>();
    check_extractable<std::shared_ptr<int>>();
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    check_not_extractable<std::string_view>();
#ifndef SQLITE_ORM_OMITS_CODECVT
    check_not_extractable<std::wstring_view>();
#endif
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    check_not_extractable<std::nullopt_t>();
    check_extractable<std::optional<int>>();
    check_extractable<std::optional<custom_enum>>();
    check_not_extractable<std::optional<User>>();
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    check_not_extractable<static_pointer_binding_t<std::nullptr_t, carray_pointer_tag>>();
#else
    check_not_extractable<static_pointer_binding<std::nullptr_t, carray_pointer_type>>();
#endif
    // pointer arguments are special: they can only be passed to and from functions, but casting is prohibited
    {
        using int64_pointer_arg = carray_pointer_arg<int64>;
        STATIC_CHECK_FALSE(orm_column_text_extractable<int64_pointer_arg>);
        STATIC_CHECK_FALSE(orm_row_value_extractable<int64_pointer_arg>);
        STATIC_CHECK(orm_boxed_value_extractable<int64_pointer_arg>);
    }
#endif

    check_extractable<custom_enum>();
    check_extractable<std::unique_ptr<custom_enum>>();

    check_not_extractable<void>();
    check_not_extractable<internal::literal_holder<int>>();
    check_not_extractable<User>();
    check_not_extractable<std::unique_ptr<User>>();
}
#endif
