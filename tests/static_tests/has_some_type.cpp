#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace {
    template<class T>
    class my_vector : public std::vector<T> {
        using super = std::vector<T>;

      public:
        using super::super;
    };
}  // end of anonymous namespace

TEST_CASE("has_some_type") {
    using empty_tuple_type = std::tuple<>;
    using tuple_type = std::tuple<int, char, my_vector<char>, std::string>;

    static_assert(tuple_helper::has_some_type<my_vector, tuple_type>::value, "");
    static_assert(!tuple_helper::has_some_type<std::shared_ptr, tuple_type>::value, "");
    static_assert(!tuple_helper::has_some_type<my_vector, empty_tuple_type>::value, "");
}