#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using std::index_sequence;
using namespace sqlite_orm;
using internal::flatten_idxseq_t;
using internal::index_sequence_value_at;

TEST_CASE("index sequence value at") {
    STATIC_REQUIRE(index_sequence_value_at<0>(index_sequence<1, 3>{}) == 1);
#if defined(SQLITE_ORM_PACK_INDEXING_SUPPORTED) || defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
    STATIC_REQUIRE(index_sequence_value_at<1>(index_sequence<1, 3>{}) == 3);
#endif
}

TEST_CASE("flatten index sequence") {
    STATIC_REQUIRE(std::is_same<flatten_idxseq_t<>, index_sequence<>>::value);
    STATIC_REQUIRE(std::is_same<flatten_idxseq_t<index_sequence<1, 3>>, index_sequence<1, 3>>::value);
    STATIC_REQUIRE(
        std::is_same<flatten_idxseq_t<index_sequence<1, 3>, index_sequence<1, 3>>, index_sequence<1, 3, 1, 3>>::value);
    STATIC_REQUIRE(std::is_same<flatten_idxseq_t<index_sequence<1, 3>, index_sequence<1, 3>, index_sequence<1, 3>>,
                                index_sequence<1, 3, 1, 3, 1, 3>>::value);
}
