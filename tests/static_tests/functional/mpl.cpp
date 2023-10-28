#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>
#include <vector>
#include <functional>  //  std::less

using namespace sqlite_orm;
using internal::literal_holder;

template<class TransparentFunction>
using transparent_of_t = typename TransparentFunction::is_transparent;

template<class T>
using make_literal_holder = literal_holder<T>;

template<class T>
using value_type_t = typename T::value_type;

TEST_CASE("mpl") {
    using mpl_is_same = mpl::quote_fn<std::is_same>;
    using check_if_same_type = mpl::bind_front_fn<std::is_same, int>;
    using check_if_same_template =
        mpl::pass_extracted_fn_to<mpl::bind_front_fn<std::is_same, mpl::quote_fn<std::vector>>>;
    using check_if_names_value_type = mpl::bind_front_higherorder_fn<polyfill::is_detected, value_type_t>;
    using predicate_type = std::less<void>;
    using check_if_is_projected_type = internal::check_if_is_type<predicate_type::is_transparent, transparent_of_t>;

    STATIC_REQUIRE_FALSE(mpl::is_metafunction_class_v<std::true_type>);
    STATIC_REQUIRE(mpl::is_metafunction_class_v<mpl_is_same>);
    STATIC_REQUIRE(std::is_same<mpl::invoke_fn_t<std::common_type, int>, int>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_op_t<std::common_type_t, int>, int>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_op_t<make_literal_holder, int>, literal_holder<int>>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_op_t<mpl::as_op<mpl::bind_front_fn<make_literal_holder, int>>>,
                                literal_holder<int>>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl_is_same, int, int>::value);
    STATIC_REQUIRE_FALSE(mpl::invoke_t<mpl::not_<mpl_is_same>, int, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::bind_front_fn<std::is_constructible, std::vector<int>>, size_t, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::bind_back_fn<std::is_constructible, size_t, int>, std::vector<int>>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::conjunction_fn<>>::value);
    STATIC_REQUIRE_FALSE(mpl::invoke_t<mpl::disjunction_fn<>>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::conjunction_fn<std::is_same, std::is_convertible>, int, int>::value);
    STATIC_REQUIRE_FALSE(
        mpl::invoke_t<mpl::not_<mpl::disjunction_fn<std::is_same, std::is_convertible>>, int, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::identity, std::true_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::always<std::true_type>>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_t<mpl::pass_extracted_fn_to<mpl::identity>, std::is_void<int>>,
                                mpl::quote_fn<std::is_void>>::value);
    STATIC_REQUIRE(mpl::invoke_t<mpl::pass_extracted_fn_to<mpl_is_same>,
                                 mpl::quote_fn<std::is_void>,
                                 mpl::quote_fn<std::is_void>>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_same_type, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_same_template, std::vector<int>>::value);
    STATIC_REQUIRE_FALSE(mpl::invoke_t<check_if_names_value_type, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_names_value_type, std::true_type>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_t<mpl::pass_result_to<transparent_of_t, mpl::identity>, predicate_type>,
                                predicate_type::is_transparent>::value);

    STATIC_REQUIRE(mpl::invoke_t<internal::check_if_is_type<predicate_type>, predicate_type>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_is_projected_type, predicate_type>::value);
}
