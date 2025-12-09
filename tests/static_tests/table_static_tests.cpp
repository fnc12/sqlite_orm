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
        int64 id = 0;
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
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with column pk
        auto table = make_table("users", make_column("id", &User::id, primary_key()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with column pk autoincrement
        auto table = make_table("users", make_column("id", &User::id, primary_key().autoincrement()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  1 column with a single table pk
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
    }
    {  //  1 column with a single table pk autoincrement
        auto table = make_table("users", make_column("id", &User::id), primary_key(&User::id).autoincrement());
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 1);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
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
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with a column pk (id)
        auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("id", &User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with a column pk (name)
        auto table = make_table("users", make_column("id", &User::id), make_column("id", &User::name, primary_key()));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 0);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 1);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 1);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 0);
    }
    {  //  2 columns with a single table pk (id)
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::id));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
    }
    {  //  2 columns with a single table pk (name)
        auto table =
            make_table("users", make_column("id", &User::id), make_column("id", &User::name), primary_key(&User::name));
        using elements_type = decltype(table.elements);
        STATIC_REQUIRE(table.count_of<is_column>() == 2);
        STATIC_REQUIRE(table.count_of<is_primary_key>() == 1);
        STATIC_REQUIRE(table.count_of_columns_with<is_primary_key>() == 0);
        STATIC_REQUIRE(col_index_sequence_of<elements_type>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with<elements_type, is_primary_key>::size() == 0);
        STATIC_REQUIRE(col_index_sequence_excluding<elements_type, is_primary_key>::size() == 2);
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 1);
    }
    {  //  2 columns with a composite pk
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
        STATIC_REQUIRE(col_index_sequence_with_field_type<elements_type, int64>::size() == 1);
        STATIC_REQUIRE(dedicated_pk_columns_count_t<elements_type>::value == 2);
    }
}
