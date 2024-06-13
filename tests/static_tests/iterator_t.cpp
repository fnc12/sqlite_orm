#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>
#include <utility>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>
#endif

using namespace sqlite_orm;
using internal::iterator_t;
using internal::view_t;

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
    { view.begin() } -> std::same_as<iterator_t<O, DBOs>>;
    { view.end() } -> std::same_as<iterator_t<O, DBOs>>;
};

template<class S, class O, class DBOs = typename S::db_objects_type>
concept storage_iterate_mapped = requires(S& storage_type) {
    { storage_type.iterate<O>() } -> std::same_as<view_t<O, S>>;
    { storage_type.iterate<O>() } -> can_view_mapped<O, DBOs>;
};
#endif

namespace {
    struct Object {};
}

TEST_CASE("can view and iterate mapped") {
    using storage_type = decltype(make_storage("", make_table<Object>("")));

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    using iter = iterator_t<Object, storage_type::db_objects_type>;
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
