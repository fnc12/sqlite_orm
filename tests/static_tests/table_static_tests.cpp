#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::col_index_sequence_of, internal::col_index_sequence_excluding, internal::col_index_sequence_with,
    internal::col_index_sequence_with_field_type;
using internal::is_column;
using internal::is_primary_key;

template<class Elements>
using dedicated_pk_columns_count_t =
    internal::nested_tuple_size_for_t<internal::columns_tuple_t,
                                      Elements,
                                      internal::filter_tuple_sequence_t<Elements, is_primary_key>>;

TEST_CASE("table static count_of<is_column>()") {
    struct User {
        int id = 0;
        std::string name;
    };
    {  //  1 column no pk
        auto table = make_table("users", make_column("id", &User::id));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with 1 inline pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with 1 inline pk autoincrement
        auto table = make_table("users", make_column("id", &User::id, primary_key().autoincrement()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with 1 dedicated pk
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
    }
    {  //  1 column with 1 dedicated pk autoincrement
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id).autoincrement());
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
    }
    {  //  2 columns no pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with 1 inline id pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("id", &User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with 1 inline name pk
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name, primary_key()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with 1 dedicated id pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::id));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
    }
    {  //  2 columns with 1 dedicated name pk
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
    }
    {  //  2 columns with 2 dedicated pks
        auto table = make_table("users",
                                make_column("id", &User::id),
                                make_column("id", &User::name),
                                primary_key(&User::id, &User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 2);
    }
}
