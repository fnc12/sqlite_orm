#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using namespace internal;

template<class Tpl>
using insertable_index_sequence = filter_tuple_sequence_t<Tpl,
                                                          is_primary_key_insertable,
                                                          polyfill::type_identity_t,
                                                          col_index_sequence_with<Tpl, is_primary_key>>;
template<class Tpl>
using noninsertable_index_sequence = filter_tuple_sequence_t<Tpl,
                                                             check_if_not<is_primary_key_insertable>::template fn,
                                                             polyfill::type_identity_t,
                                                             col_index_sequence_with<Tpl, is_primary_key>>;

TEST_CASE("is_column_with_insertable_primary_key") {
    struct User {
        int id;
        std::string username;
        std::string password;
        bool isActive;
    };

    auto insertable = std::make_tuple(  ///
        make_column("", &User::id, primary_key()),
        make_column("", &User::username, primary_key(), default_value("Clint Eastwood")),
        make_column("", &User::username, primary_key(), default_value(std::vector<int>{})),
        make_column("", &User::username, primary_key().autoincrement()));

    auto noninsertable = std::make_tuple(  ///
        make_column("", &User::username, primary_key()),
        make_column("", &User::password, primary_key()));

    auto outside = std::make_tuple(  ///
        make_column("", &User::id),  ///< not a primary key
        std::make_shared<int>()  ///< not a column
    );

    STATIC_REQUIRE(insertable_index_sequence<decltype(insertable)>::size() == 4);
    STATIC_REQUIRE(noninsertable_index_sequence<decltype(noninsertable)>::size() == 2);
    STATIC_REQUIRE(noninsertable_index_sequence<decltype(outside)>::size() == 0);
}