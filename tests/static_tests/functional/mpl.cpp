#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>
#include <vector>
#include <functional>  //  std::less

using namespace sqlite_orm;

template<class TransparentFunction>
using transparent_of_t = typename TransparentFunction::is_transparent;

TEST_CASE("mpl") {
    using mpl_is_same = mpl::quote_fn<std::is_same>;

    STATIC_REQUIRE_FALSE(mpl::is_metafunction_class_v<std::true_type>);
    STATIC_REQUIRE(mpl::is_metafunction_class_v<mpl_is_same>);
    STATIC_REQUIRE_FALSE(mpl::is_alias_template_v<std::common_type, int>);
    STATIC_REQUIRE(mpl::is_alias_template_v<std::common_type_t, int>);
    STATIC_REQUIRE(std::is_same<mpl::invoke_fn_t<std::common_type, int>, int>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_op_t<std::common_type_t, int>, int>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_meta_t<std::common_type, int>, int>::value);
    STATIC_REQUIRE(std::is_same<mpl::invoke_meta_t<std::common_type_t, int>, int>::value);
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
    using check_if_same_type = mpl::bind_front_fn<std::is_same, int>;
    STATIC_REQUIRE(mpl::invoke_t<check_if_same_type, int>::value);
    using check_if_same_template =
        mpl::pass_extracted_fn_to<mpl::bind_front_fn<std::is_same, mpl::quote_fn<std::vector>>>;
    STATIC_REQUIRE(mpl::invoke_t<check_if_same_template, std::vector<int>>::value);
    using check_if_names_type = mpl::bind_front_higherorder_fn<polyfill::is_detected, internal::type_t>;
    STATIC_REQUIRE_FALSE(mpl::invoke_t<check_if_names_type, int>::value);
    STATIC_REQUIRE(mpl::invoke_t<check_if_names_type, std::true_type>::value);
    using predicate_type = std::less<void>;
    STATIC_REQUIRE(std::is_same<mpl::invoke_t<mpl::pass_result_to<transparent_of_t, mpl::identity>, predicate_type>,
                                predicate_type::is_transparent>::value);

    STATIC_REQUIRE(mpl::invoke_t<internal::check_if_is_type<predicate_type>, predicate_type>::value);
    using check_if_is_projected_type = internal::check_if_is_type<predicate_type::is_transparent, transparent_of_t>;
    STATIC_REQUIRE(mpl::invoke_t<check_if_is_projected_type, predicate_type>::value);
}
