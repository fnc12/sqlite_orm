#include <type_traits>
#include <array>
#include <tuple>
#include <algorithm>  //  std::fill_n
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using std::array;
using std::index_sequence;
using std::index_sequence_for;
using std::make_index_sequence;
using std::nullptr_t;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::tuple_element_t;
using std::tuple_size;
using std::unique_ptr;
using std::vector;
using std::wstring;
using namespace sqlite_orm;

template<typename T>
constexpr T get_default() {
    return T{};
}

template<>
constexpr auto get_default<const char*>() -> const char* {
    return "";
}

template<>
constexpr auto get_default<const wchar_t*>() -> const wchar_t* {
    return L"";
}

template<>
constexpr auto get_default<internal::literal_holder<const char*>>() -> internal::literal_holder<const char*> {
    return {""};
}

template<>
constexpr auto get_default<internal::literal_holder<const wchar_t*>>() -> internal::literal_holder<const wchar_t*> {
    return {L""};
}

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
template<>
constexpr auto get_default<std::nullopt_t>() -> std::nullopt_t {
    return std::nullopt;
}

template<>
constexpr auto get_default<internal::literal_holder<std::nullopt_t>>() -> internal::literal_holder<std::nullopt_t> {
    return {std::nullopt};
}
#endif

template<class Tpl, size_t... Idx>
constexpr Tpl make_default_tuple(index_sequence<Idx...>) {
    return {get_default<tuple_element_t<Idx, Tpl>>()...};
}

template<class Tpl>
constexpr Tpl make_default_tuple() {
    return make_default_tuple<Tpl>(make_index_sequence<tuple_size<Tpl>::value>{});
}

template<size_t N>
array<string, N> single_value_array(const char* s) {
    array<string, N> a;
    std::fill_n(a.data(), a.size(), s);
    return a;
}

template<class T>
using wrap_in_literal = internal::literal_holder<T>;

inline void require_string(const string& value, const string& expected) {
    REQUIRE(value == expected);
}

template<size_t... Idx>
void require_strings(const array<string, sizeof...(Idx)>& values,
                     const array<string, sizeof...(Idx)>& expected,
                     index_sequence<Idx...>) {
    for(size_t i = 0; i < sizeof...(Idx); ++i) {
        require_string(values[i], expected[i]);
    }
}

template<typename Ctx, typename... Ts>
void test_tuple(const tuple<Ts...>& t, const Ctx& ctx, const array<string, sizeof...(Ts)>& expected) {
    require_strings({internal::serialize(get<Ts>(t), ctx)...}, expected, index_sequence_for<Ts...>{});
}

namespace {
    struct Custom {};
    template<class Elem>
    class StringVeneer : public std::basic_string<Elem> {
      public:
        using std::basic_string<Elem>::basic_string;
    };
}

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
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

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

        constexpr Tuple tpl = make_default_tuple<Tuple>();

        array<string, tuple_size<Tuple>::value> e{"0",
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

        SECTION("dump") {
            context.replace_bindable_with_question = false;
            test_tuple(tpl, context, e);
        }
        SECTION("parametrized") {
            context.replace_bindable_with_question = true;
            test_tuple(tpl, context, single_value_array<tuple_size<Tuple>::value>("?"));
        }
        SECTION("non-bindable literals") {
            context.replace_bindable_with_question = true;
            constexpr auto t = make_default_tuple<internal::transform_tuple_t<Tuple, wrap_in_literal>>();
            test_tuple(t, context, e);
        }
    }

    SECTION("bindable_types") {
        using Tuple = tuple<string,
#ifndef SQLITE_ORM_OMITS_CODECVT
                            wstring,
                            StringVeneer<wchar_t>,
#endif
                            unique_ptr<int>,
                            shared_ptr<int>,
                            vector<char>,
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                            std::optional<int>,
                            std::optional<Custom>,
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                            std::string_view,
#ifndef SQLITE_ORM_OMITS_CODECVT
                            std::wstring_view,
#endif
#endif
                            StringVeneer<char>,
                            Custom,
                            unique_ptr<Custom>>;

        Tuple tpl = make_default_tuple<Tuple>();

        array<string, tuple_size<Tuple>::value> e{"''",
#ifndef SQLITE_ORM_OMITS_CODECVT
                                                  "''",
                                                  "''",
#endif
                                                  "null",
                                                  "null",
                                                  "x''",
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                                  "null",
                                                  "null",
#endif
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
                                                  "''",
#ifndef SQLITE_ORM_OMITS_CODECVT
                                                  "''",
#endif
#endif
                                                  "''",
                                                  "custom",
                                                  "null"};

        SECTION("dump") {
            context.replace_bindable_with_question = false;
            test_tuple(tpl, context, e);
        }
        SECTION("parametrized") {
            context.replace_bindable_with_question = true;
            test_tuple(tpl, context, single_value_array<tuple_size<Tuple>::value>("?"));
        }
        SECTION("non-bindable literals") {
            context.replace_bindable_with_question = true;
            auto t = make_default_tuple<internal::transform_tuple_t<Tuple, wrap_in_literal>>();
            test_tuple(t, context, e);
        }
    }

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
    SECTION("bindable_pointer") {
        string value, expected;
        context.replace_bindable_with_question = false;

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
#endif
}
