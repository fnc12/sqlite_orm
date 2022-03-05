#include <type_traits>
#include <array>
#include <tuple>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using std::array;
using std::nullptr_t;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::vector;
using namespace sqlite_orm;

inline void require_string(const std::string& value, const std::string& expected) {
    REQUIRE(value == expected);
}

template<size_t... Idx>
void require_strings(const array<string, sizeof...(Idx)>& values,
                     const array<string, sizeof...(Idx)>& expected,
                     std::index_sequence<Idx...>) {
    for(size_t i = 0; i < sizeof...(Idx); ++i) {
        require_string(values[i], expected[i]);
    }
}

template<typename Ctx, typename... Ts>
void test_tuple(const tuple<Ts...>& t, const Ctx& ctx, const array<string, sizeof...(Ts)>& expected) {
    require_strings({internal::serialize(get<Ts>(t), ctx)...}, expected, std::index_sequence_for<Ts...>{});
}

struct Custom {};
template<class Elem>
class StringVeneer : public std::basic_string<Elem> {
  public:
    using std::basic_string<Elem>::basic_string;
};

namespace sqlite_orm {
    template<>
    struct statement_binder<Custom> {};
    template<>
    struct field_printer<Custom> {
        std::string operator()(const Custom&) const {
            return "custom";
        }
    };
}

TEST_CASE("bindables") {
    using internal::serialize;

    struct Dummy {};
    auto table = make_table<Dummy>("dummy");
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    SECTION("bindable_builtin_types") {
        using Tuple = tuple<bool,
                            char,
                            unsigned char,
                            signed char,
                            short,
                            unsigned short,
                            int,
                            unsigned int,
                            long,
                            unsigned long,
                            long long,
                            unsigned long long,
                            float,
                            double,
                            long double,
                            const char*,
                            nullptr_t
#ifndef SQLITE_ORM_OMITS_CODECVT
                            ,
                            const wchar_t*
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                            ,
                            std::nullopt_t
#endif
                            >;

        Tuple t{false,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0.f,
                0.,
                0.,
                "",
                nullptr
#ifndef SQLITE_ORM_OMITS_CODECVT
                ,
                L""
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                ,
                std::nullopt
#endif
        };
        array<string, std::tuple_size<Tuple>::value> e{"0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "0",
                                                       "''",
                                                       "null"
#ifndef SQLITE_ORM_OMITS_CODECVT
                                                       ,
                                                       "''"
#endif
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                                       ,
                                                       "null"
#endif
        };

        test_tuple(t, context, e);
    }

    SECTION("bindable_types") {
        using Tuple = tuple<string,
#ifndef SQLITE_ORM_OMITS_CODECVT
                            std::wstring,
                            StringVeneer<wchar_t>,
#endif
                            unique_ptr<int>,
                            shared_ptr<int>,
                            vector<char>,
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                            std::optional<int>,
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                            std::string_view,
#ifndef SQLITE_ORM_OMITS_CODECVT
                            std::wstring_view,
#endif
#endif
                            StringVeneer<char>,
                            Custom>;

        Tuple t{"",
#ifndef SQLITE_ORM_OMITS_CODECVT
                L"",
                L"",
#endif
                nullptr,
                nullptr,
                vector<char>{},
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                std::nullopt,
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                "",
#ifndef SQLITE_ORM_OMITS_CODECVT
                L"",
#endif
#endif
                "",
                Custom{}};
        array<string, std::tuple_size<Tuple>::value> e{"''",
#ifndef SQLITE_ORM_OMITS_CODECVT
                                                       "''",
                                                       "''",
#endif
                                                       "null",
                                                       "null",
                                                       "x''",
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                                       "null",
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                                                       "''",
#ifndef SQLITE_ORM_OMITS_CODECVT
                                                       "''",
#endif
#endif
                                                       "''",
                                                       "custom"};

        test_tuple(t, context, e);
    }

    SECTION("bindable_pointer") {
        string value;
        decltype(value) expected;

        SECTION("null by itself") {
            auto v = statically_bindable_pointer<carray_pvt, nullptr_t>(nullptr);
            value = serialize(v, context);
            expected = "null";
        }
        SECTION("null by itself 2") {
            auto v = statically_bindable_pointer<carray_pvt>(&value);
            value = serialize(v, context);
            expected = "null";
        }
        SECTION("null in select") {
            auto ast = select(statically_bindable_pointer<carray_pvt, nullptr_t>(nullptr));
            ast.highest_level = true;
            value = serialize(ast, context);
            expected = "SELECT null";
        }
        SECTION("null as function argument") {
            auto ast = func<remember_fn>(1, statically_bindable_pointer<carray_pvt, nullptr_t>(nullptr));
            value = serialize(ast, context);
            expected = "remember(1, null)";
        }
        SECTION("null as function argument 2") {
            auto ast = func<remember_fn>(1, nullptr);
            value = serialize(ast, context);
            expected = "remember(1, null)";
        }

        REQUIRE(value == expected);
    }
}
