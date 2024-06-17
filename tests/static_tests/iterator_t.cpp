#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>
#include <utility>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>
#endif

using namespace sqlite_orm;
using internal::mapped_iterator;
using internal::mapped_view;
using internal::structure;
#if defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)
using internal::result_set_iterator;
using internal::result_set_sentinel_t;
using internal::result_set_view;
#endif
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
using internal::table_reference;
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
template<class T>
using with_reference = T&;

template<class T>
concept can_reference = requires { typename with_reference<T>; };

template<class T>
concept names_difference_type = requires { typename T::difference_type; };

template<class T>
concept names_value_type = requires { typename T::value_type; };

// named concept of a legacy iterator
template<class Iter>
concept LegacyIterator = requires(Iter it) {
    { *it } -> can_reference;
    { ++it } -> std::same_as<Iter&>;
    { *it++ } -> can_reference;
} && std::copyable<Iter>;

// named concept of a legacy input iterator
template<class Iter>
concept LegacyInputIterator =
    LegacyIterator<Iter> && std::equality_comparable<Iter> && names_difference_type<std::incrementable_traits<Iter>> &&
    names_value_type<std::indirectly_readable_traits<Iter>> && requires(Iter it) {
        typename std::common_reference_t<std::iter_reference_t<Iter>&&,
                                         typename std::indirectly_readable_traits<Iter>::value_type&>;
        typename std::common_reference_t<decltype(*it++)&&,
                                         typename std::indirectly_readable_traits<Iter>::value_type&>;
        requires std::signed_integral<typename std::incrementable_traits<Iter>::difference_type>;
    };

template<class Iter, class Value>
concept can_iterate_mapped = requires(Iter it) {
    requires LegacyInputIterator<Iter>;
    // explicit check of the sentinel role, since `std::ranges::borrowed_range` in `can_view_mapped`
    // would not tell us why exactly the end iterator cannot be a sentinel
    requires std::sentinel_for<Iter, Iter>;
    { *it } -> std::same_as<Value&>;
    // note: should actually be only present for contiguous iterators
    { it.operator->() } -> std::same_as<Value*>;
};

template<class V, class O, class DBOs>
concept can_view_mapped = requires(V view) {
    requires std::ranges::borrowed_range<V>;
    { view.begin() } -> std::same_as<mapped_iterator<O, DBOs>>;
    { view.end() } -> std::same_as<mapped_iterator<O, DBOs>>;
};

template<class S, class O, class DBOs = typename S::db_objects_type>
concept storage_iterate_mapped = requires(S& storage_type) {
    { storage_type.iterate<O>() } -> std::same_as<mapped_view<O, S>>;
    { storage_type.iterate<O>() } -> can_view_mapped<O, DBOs>;
};
#endif

#if(defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)) &&          \
    defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED)
template<class Iter, class Value>
concept can_iterate_result_set = requires(Iter it) {
    requires std::input_iterator<Iter>;
    // explicit check of the sentinel role, since `std::ranges::borrowed_range` in `can_view_mapped`
    // would not tell us why exactly the end iterator cannot be a sentinel
    requires std::sentinel_for<result_set_sentinel_t, Iter>;
#ifdef SQLITE_ORM_STL_HAS_DEFAULT_SENTINEL
    requires std::same_as<result_set_sentinel_t, std::default_sentinel_t>;
#endif
    { *it } -> std::same_as<Value>;
};

template<class V, class ColResult, class DBOs>
concept can_view_result_set = requires(V view) {
    requires std::ranges::view<V>;
    requires std::ranges::borrowed_range<V>;
    { view.begin() } -> std::same_as<result_set_iterator<ColResult, DBOs>>;
    { view.end() } -> std::same_as<result_set_sentinel_t>;
};

template<class S, class Select, class ColResult, class DBOs = typename S::db_objects_type>
concept storage_iterate_result_set = requires(S& storage_type, Select select) {
    { storage_type.iterate(select) } -> std::same_as<result_set_view<Select, DBOs>>;
    { storage_type.iterate(select) } -> can_view_result_set<ColResult, DBOs>;
};
#endif

namespace {
    struct Object {};
}

TEST_CASE("can view and iterate mapped") {
    using storage_type = decltype(make_storage("", make_table<Object>("")));

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    using iter = mapped_iterator<Object, storage_type::db_objects_type>;
    STATIC_REQUIRE(can_iterate_mapped<iter, Object>);
    // check default initializability at runtime
    [[maybe_unused]] const iter end;
#else
    using iter = decltype(std::declval<storage_type>().iterate<Object>().begin());
    iter it;
    const iter end;

    // LegacyInputIterator
    {
        // LegacyIterator
        {
            STATIC_REQUIRE(std::is_same<decltype(*it), Object&>::value);
            STATIC_REQUIRE(std::is_same<decltype(++it), iter&>::value);
            STATIC_REQUIRE(std::is_same<decltype(*it++), Object&>::value);
            // copyable (partially, as it is a rather extensive concept)
            { STATIC_REQUIRE(std::is_copy_constructible<iter>::value); }
        }
        // equality_comparable (sentinel)
        {
            STATIC_REQUIRE(std::is_same<decltype(it == end), bool>::value);
            STATIC_REQUIRE(std::is_same<decltype(it != end), bool>::value);
        }
        STATIC_REQUIRE(std::is_same<std::iterator_traits<iter>::iterator_category, std::input_iterator_tag>::value);
        STATIC_REQUIRE(std::is_same<std::iterator_traits<iter>::value_type, Object>::value);
        STATIC_REQUIRE(std::is_same<std::iterator_traits<iter>::difference_type, ptrdiff_t>::value);
    }
    // semiregular (actually sentinel_for, but the other concepts were verified above)
    { STATIC_REQUIRE(std::is_default_constructible<iter>::value); }
    STATIC_REQUIRE(std::is_same<std::iterator_traits<iter>::pointer, Object*>::value);
    // note: should actually be only present for contiguous iterators
    STATIC_REQUIRE(std::is_same<decltype(it.operator->()), Object*>::value);
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    STATIC_REQUIRE(storage_iterate_mapped<storage_type, Object>);
#endif
}

#if(defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)) &&          \
    defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED)
TEST_CASE("can view and iterate result set") {
    struct Object {};
    using empty_storage_type = decltype(make_storage(""));
    using empty_db_objects_type = empty_storage_type::db_objects_type;
    using storage_type = decltype(make_storage("", make_table<Object>("")));
    using db_objects_type = storage_type::db_objects_type;

    STATIC_REQUIRE(can_iterate_result_set<result_set_iterator<int, empty_db_objects_type>, int>);
    STATIC_REQUIRE(
        can_iterate_result_set<result_set_iterator<std::tuple<int, int>, empty_db_objects_type>, std::tuple<int, int>>);
    STATIC_REQUIRE(
        can_iterate_result_set<result_set_iterator<structure<Object, empty_db_objects_type>, empty_db_objects_type>,
                               Object>);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    STATIC_REQUIRE(can_iterate_result_set<result_set_iterator<table_reference<Object>, db_objects_type>, Object>);
#endif

    STATIC_REQUIRE(storage_iterate_result_set<empty_storage_type, decltype(select(42)), int>);
    STATIC_REQUIRE(
        storage_iterate_result_set<empty_storage_type, decltype(select(columns(1, 42))), std::tuple<int, int>>);
    STATIC_REQUIRE(storage_iterate_result_set<empty_storage_type,
                                              decltype(select(struct_<Object>())),
                                              structure<Object, std::tuple<>>>);
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    STATIC_REQUIRE(
        storage_iterate_result_set<storage_type, decltype(select(object<Object>())), table_reference<Object>>);
#endif

#ifdef SQLITE_ORM_WITH_CTE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_cte_moniker auto x = "x"_cte;
    constexpr orm_column_alias auto i = "i"_col;
    STATIC_REQUIRE(storage_iterate_result_set<storage_type, decltype(with(x(i).as(select(1)), select(x->*i))), int>);
#endif
#endif
}
#endif
