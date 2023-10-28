#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same
#include <array>

using namespace sqlite_orm;
using internal::create_from_tuple;
using internal::field_type_t;
using internal::literal_holder;
using internal::transform_tuple_t;
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
using internal::nested_tuple_size_for_t;
using internal::recombine_tuple;
#endif

template<class T>
using make_literal_holder = literal_holder<T>;

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
struct tuple_maker {
    template<class... Types>
    constexpr auto operator()(Types&&... types) const {
        return std::make_tuple(std::forward<Types>(types)...);
    }
};
#endif

TEST_CASE("tuple_helper static") {
    SECTION("tuple_transformer") {
        struct Table {
            int64_t id;
            std::string a;
            std::string b;
            std::string c;
        };

        auto columnsTuple = std::make_tuple(make_column("id", &Table::id),
                                            make_column("a", &Table::a),
                                            make_column("b", &Table::b),
                                            make_column("c", &Table::c));
        using ColumnsTuple = decltype(columnsTuple);
        using ColumnsMappedTypes = transform_tuple_t<ColumnsTuple, field_type_t>;
        using Expected = std::tuple<int64_t, std::string, std::string, std::string>;
        STATIC_REQUIRE(std::is_same<ColumnsMappedTypes, Expected>::value);

        STATIC_REQUIRE(std::is_same<transform_tuple_t<std::tuple<bool, int>, make_literal_holder>,
                                    std::tuple<literal_holder<bool>, literal_holder<int>>>::value);

#if __cpp_lib_constexpr_algorithms >= 201806L
        STATIC_REQUIRE(create_from_tuple<std::array<int, 2>>(std::make_tuple(1, 2), polyfill::identity{}) ==
                       std::array<int, 2>{1, 2});
#endif
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
        STATIC_REQUIRE(recombine_tuple(tuple_maker{}, std::make_tuple(1, 2), polyfill::identity{}, 3) ==
                       std::make_tuple(3, 1, 2));

        // make tuples out of tuple elements, and sump up the size of the first 2 of 3 resulting tuples
        STATIC_REQUIRE(
            std::is_same<nested_tuple_size_for_t<std::tuple, std::tuple<int, int, int>, std::make_index_sequence<2>>,
                         std::integral_constant<size_t, 2>>::value);
#endif
    }
}
