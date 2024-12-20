#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;
using internal::alias_holder;
using internal::column_alias;
using internal::column_pointer;

template<class T>
struct is_pair : std::false_type {};

template<class L, class R>
struct is_pair<std::pair<L, R>> : std::true_type {};

template<class T>
struct is_tuple : std::false_type {};

template<class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type {};

TEST_CASE("Node tuple") {
    using internal::bindable_filter_t;
    using internal::node_tuple;
    using internal::node_tuple_t;
    using std::is_same;
    using std::tuple;

    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("wrapper types") {
        SECTION("reference wrapper") {
            using Tuple = node_tuple_t<std::reference_wrapper<int>>;
            STATIC_REQUIRE(is_same<Tuple, tuple<int>>::value);
        }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        {
            SECTION("as_optional") {
                using Tuple = node_tuple_t<internal::as_optional_t<int>>;
                STATIC_REQUIRE(is_same<Tuple, tuple<int>>::value);
            }
        }
#endif
    }
    SECTION("bindables") {
        SECTION("int") {
            using Tuple = node_tuple_t<int>;
            using Expected = tuple<int>;
            static_assert(is_same<Tuple, Expected>::value, "bindable int");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int>>::value);
        }
        SECTION("float") {
            using Tuple = node_tuple_t<float>;
            using Expected = tuple<float>;
            static_assert(is_same<Tuple, Expected>::value, "bindable float");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<float>>::value);
        }
    }
    SECTION("non-bindable literals") {
        using namespace internal;
        using Tuple = node_tuple_t<literal_holder<int>>;
        using Expected = tuple<>;
        static_assert(is_same<Tuple, Expected>::value, "literal int");
        STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<>>::value);
    }
    SECTION("is_equal_with_table_t") {
        auto node = is_equal<User>(std::string("Claude"));
        using Node = decltype(node);
        using Tuple = node_tuple_t<Node>;
        using Expected = tuple<std::string>;
        static_assert(is_same<Tuple, Expected>::value, "is_equal_with_table_t");
        STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<std::string>>::value);
    }
    SECTION("binary_condition") {
        using namespace internal;
        SECTION("5 < 6.0f") {
            auto c = less_than(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "less_than_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("id < 10") {
            auto c = less_than(&User::id, 10);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "less_than_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int>>::value);
        }
        SECTION("5 <= 6.0f") {
            auto c = less_or_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "less_or_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("id <= 10.0") {
            auto c = less_or_equal(&User::id, 10.0);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<decltype(&User::id), double>;
            static_assert(is_same<Tuple, Expected>::value, "less_or_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<double>>::value);
        }
        SECTION("5 > 6.0f") {
            auto c = greater_than(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "greater_than_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("id > 20") {
            auto c = greater_than(&User::id, 20);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "greater_than_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int>>::value);
        }
        SECTION("5 >= 6.0f") {
            auto c = greater_or_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "greater_or_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("5 >= id") {
            auto c = greater_or_equal(5, &User::id);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "greater_or_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int>>::value);
        }
        SECTION("5 == 6.0f") {
            auto c = is_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "is_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("'ototo' == name") {
            auto c = is_equal("ototo", &User::name);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<const char*, decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "is_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<const char*>>::value);
        }
        SECTION("5 != 6.0f") {
            auto c = is_not_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "is_not_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<int, float>>::value);
        }
        SECTION("name != std::string('ototo')") {
            auto c = is_not_equal(&User::name, std::string("ototo"));
            using C = decltype(c);
            using Tuple = node_tuple_t<C>;
            using Expected = tuple<decltype(&User::name), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "is_not_equal_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<std::string>>::value);
        }
        SECTION("bool and int") {
            using Tuple = node_tuple_t<and_condition_t<bool, int>>;
            using Expected = tuple<bool, int>;
            static_assert(is_same<Tuple, Expected>::value, "and_condition_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<bool, int>>::value);
        }
        SECTION("bool or int") {
            using Tuple = node_tuple_t<or_condition_t<bool, int>>;
            using Expected = tuple<bool, int>;
            static_assert(is_same<Tuple, Expected>::value, "or_condition_t");
            STATIC_REQUIRE(is_same<bindable_filter_t<Tuple>, tuple<bool, int>>::value);
        }
    }
    SECTION("binary_operator") {
        using namespace internal;

        using CondTuple = node_tuple_t<conc_t<std::string, decltype(&User::name)>>;
        STATIC_REQUIRE(is_same<CondTuple, tuple<std::string, decltype(&User::name)>>::value);

        using MinusTuple = node_tuple_t<unary_minus_t<decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<MinusTuple, tuple<decltype(&User::id)>>::value);

        using AddTuple = node_tuple_t<add_t<int, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<AddTuple, tuple<int, decltype(&User::id)>>::value);

        using SubTuple = node_tuple_t<sub_t<float, double>>;
        STATIC_REQUIRE(is_same<SubTuple, tuple<float, double>>::value);

        using MulTuple = node_tuple_t<mul_t<double, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<MulTuple, tuple<double, decltype(&User::id)>>::value);

        using DivTuple = node_tuple_t<sqlite_orm::internal::div_t<int, float>>;
        STATIC_REQUIRE(is_same<DivTuple, tuple<int, float>>::value);

        using ModTuple = node_tuple_t<mod_t<decltype(&User::id), int>>;
        STATIC_REQUIRE(is_same<ModTuple, tuple<decltype(&User::id), int>>::value);

        using AssignTuple = node_tuple_t<assign_t<decltype(&User::name), std::string>>;
        STATIC_REQUIRE(is_same<AssignTuple, tuple<decltype(&User::name), std::string>>::value);
    }
    SECTION("bitwise operator") {
        using namespace internal;

        using BitwiseNotTuple = node_tuple_t<bitwise_not_t<decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<BitwiseNotTuple, tuple<decltype(&User::id)>>::value);

        using BitwiseShiftLeftTuple = node_tuple_t<bitwise_shift_left_t<int, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<BitwiseShiftLeftTuple, tuple<int, decltype(&User::id)>>::value);

        using BitwiseShiftRightTuple = node_tuple_t<bitwise_shift_right_t<int, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<BitwiseShiftRightTuple, tuple<int, decltype(&User::id)>>::value);

        using BitwiseAndTuple = node_tuple_t<bitwise_and_t<int, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<BitwiseAndTuple, tuple<int, decltype(&User::id)>>::value);

        using BitwiseOrTuple = node_tuple_t<bitwise_or_t<int, decltype(&User::id)>>;
        STATIC_REQUIRE(is_same<BitwiseOrTuple, tuple<int, decltype(&User::id)>>::value);
    }
    SECTION("columns") {
        auto cols = columns(&User::id, &User::name);
        using Cols = decltype(cols);
        using ColsTuple = node_tuple_t<Cols>;
        static_assert(is_same<ColsTuple, tuple<decltype(&User::id), decltype(&User::name)>>::value, "columns_t");
    }
    SECTION("in") {
        auto inValue = in(&User::id, {1, 2, 3});
        using In = decltype(inValue);
        using InTuple = node_tuple_t<In>;
        static_assert(is_same<InTuple, tuple<decltype(&User::id), std::vector<int>>>::value, "in_t");
    }
    SECTION("exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))") {
        auto c = exists(select(&User::name, where(in(&User::id, {6, 7, 9}))));
        using Con = decltype(c);
        using Tuple = node_tuple_t<Con>;
        using Expected = tuple<decltype(&User::name), decltype(&User::id), std::vector<int>>;
        static_assert(is_same<Tuple, Expected>::value, "exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))");
    }
    SECTION("aggregate functions") {
        SECTION("avg") {
            auto node = avg(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "avg");
        }
        SECTION("avg filter") {
            auto node = avg(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "avg filter");
        }
        SECTION("count(*)") {
            auto node = count<User>();
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(node)>;
            static_assert(is_same<Tuple, Expected>::value, "count(*)");
        }
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
        SECTION("count(*) cte") {
            auto node = count<decltype(1_ctealias)>();
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<Node>;
            static_assert(is_same<Tuple, Expected>::value, "count(*) cte");
        }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("count(*) cte 2") {
            auto node = count<1_ctealias>();
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<Node>;
            static_assert(is_same<Tuple, Expected>::value, "count(*) cte 2");
        }
#endif
#endif
        SECTION("count(*) filter") {
            auto node = count<User>().filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(count<User>()), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "count(*) filter");
        }
        SECTION("count(X)") {
            auto node = count(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "count(X)");
        }
        SECTION("count(X) filter") {
            auto node = count(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "count(X) filter");
        }
        SECTION("group_concat(X)") {
            auto node = group_concat(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "group_concat(X)");
        }
        SECTION("group_concat(X) filter") {
            auto node = group_concat(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "group_concat(X) filter");
        }
        SECTION("group_concat(X,Y)") {
            auto node = group_concat(&User::id, std::string("-"));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "group_concat(X,Y)");
        }
        SECTION("group_concat(X,Y) filter") {
            auto node = group_concat(&User::id, std::string("-")).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), std::string, decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "group_concat(X,Y) filter");
        }
        SECTION("max(X)") {
            auto node = max(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "max(X)");
        }
        SECTION("max(X) filter") {
            auto node = max(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "max(X) filter");
        }
        SECTION("min(X)") {
            auto node = min(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "min(X)");
        }
        SECTION("min(X) filter") {
            auto node = min(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "min(X) filter");
        }
        SECTION("sum(X)") {
            auto node = sum(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "sum(X)");
        }
        SECTION("sum(X) filter") {
            auto node = sum(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "sum(X) filter");
        }
        SECTION("total(X)") {
            auto node = total(&User::id);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "total(X)");
        }
        SECTION("total(X) filter") {
            auto node = total(&User::id).filter(where(length(&User::name) > 5));
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "total(X) filter");
        }
    }
    SECTION("scalar functions") {
        SECTION("max(X,Y)") {
            auto node = max(&User::id, 4);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "max(X,Y)");
        }
        SECTION("min(X,Y)") {
            auto node = min(&User::id, 4);
            using Node = decltype(node);
            using Tuple = node_tuple_t<Node>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "min(X,Y)");
        }
    }
    SECTION("compound operator") {
        SECTION("union_(select(1), select(2))") {
            auto un = union_(select(1), select(2));
            using Union = decltype(un);
            using Tuple = node_tuple_t<Union>;
            static_assert(is_same<Tuple, tuple<int, int>>::value, "union_(select(1), select(2))");
        }
        SECTION("union_all") {
            auto un = union_all(select(&User::id, where(is_equal(&User::name, "Alice"))),
                                select(&User::id, where(is_equal(std::string("Bob"), &User::name))));
            using Union = decltype(un);
            using Tuple = node_tuple_t<Union>;
            using Expected = tuple<decltype(&User::id),
                                   decltype(&User::name),
                                   const char*,
                                   decltype(&User::id),
                                   std::string,
                                   decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "union_all");
        }
        SECTION("except") {
            auto un = except(select(columns(&User::id, &User::name), where(is_equal(&User::id, 10L))),
                             select(columns(&User::id, &User::name), where(is_equal(&User::id, 15L))));
            using Union = decltype(un);
            using Tuple = node_tuple_t<Union>;
            using Expected = tuple<decltype(&User::id),
                                   decltype(&User::name),
                                   decltype(&User::id),
                                   long,
                                   decltype(&User::id),
                                   decltype(&User::name),
                                   decltype(&User::id),
                                   long>;
            static_assert(is_same<Tuple, Expected>::value, "except");
        }
        SECTION("intersect") {
            auto un = intersect(select(&User::name), select(&User::name, where(is_equal(&User::name, "Anny"))));
            using Union = decltype(un);
            using Tuple = node_tuple_t<Union>;
            using Expected = tuple<decltype(&User::name), decltype(&User::name), decltype(&User::name), const char*>;
            static_assert(is_same<Tuple, Expected>::value, "intersect");
        }
    }
    SECTION("replace_raw_t") {
        auto expression =
            replace(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        using Expression = decltype(expression);
        using Tuple = node_tuple_t<Expression>;
        STATIC_REQUIRE(is_same<Tuple, tuple<decltype(&User::id), decltype(&User::name), int, std::string>>::value);
    }
    SECTION("insert_raw_t") {
        auto expression =
            insert(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        using Expression = decltype(expression);
        using Tuple = node_tuple_t<Expression>;
        STATIC_REQUIRE(is_same<Tuple, tuple<decltype(&User::id), decltype(&User::name), int, std::string>>::value);
    }
    SECTION("tuple") {
        auto expression = std::make_tuple(1, std::string("hi"));
        using Expression = decltype(expression);
        using Tuple = node_tuple_t<Expression>;
        STATIC_REQUIRE(is_same<Tuple, tuple<int, std::string>>::value);
    }
    SECTION("values") {
        SECTION("int + string") {
            auto expression = values(1, std::string("hi"));
            using Expression = decltype(expression);
            using Tuple = node_tuple_t<Expression>;
            STATIC_REQUIRE(is_same<Tuple, tuple<int, std::string>>::value);
        }
        SECTION("tuple") {
            auto expression = values(std::make_tuple(1, std::string("hi")));
            using Expression = decltype(expression);
            using Tuple = node_tuple_t<Expression>;
            STATIC_REQUIRE(is_same<Tuple, tuple<int, std::string>>::value);
        }
    }
    SECTION("into") {
        auto expression = into<User>();
        using Expression = decltype(expression);
        using Tuple = node_tuple_t<Expression>;
        STATIC_REQUIRE(is_same<Tuple, tuple<>>::value);
    }
    SECTION("match") {
        auto expression = match<User>(std::string("Plazma"));
        using Expression = decltype(expression);
        using Tuple = node_tuple_t<Expression>;
        STATIC_REQUIRE(is_same<Tuple, tuple<std::string>>::value);
    }
    SECTION("select") {
        SECTION("select(&User::id)") {
            auto sel = select(&User::id);
            using Sel = decltype(sel);
            using Tuple = node_tuple_t<Sel>;
            static_assert(is_same<Tuple, tuple<decltype(&User::id)>>::value, "select(&User::id)");
        }
        SECTION("select(&User::name)") {
            auto sel = select(&User::name);
            using Sel = decltype(sel);
            using Tuple = node_tuple_t<Sel>;
            static_assert(is_same<Tuple, tuple<decltype(&User::name)>>::value, "select(&User::name)");
        }
        SECTION("select(&User::id, where(is_equal(&User::id, 5)))") {
            auto sel = select(&User::id, where(is_equal(&User::id, 5)));
            using Sel = decltype(sel);
            using Tuple = node_tuple_t<Sel>;
            static_assert(is_same<Tuple, tuple<decltype(&User::id), decltype(&User::id), int>>::value,
                          "select(&User::id, where(is_equal(&User::id, 5)))");
        }
        SECTION("select(&User::name, where(less_than(&User::id, 10)))") {
            auto sel = select(&User::name, where(less_than(&User::id, 10)));
            using Sel = decltype(sel);
            using Tuple = node_tuple_t<Sel>;
            static_assert(is_same<Tuple, tuple<decltype(&User::name), decltype(&User::id), int>>::value,
                          "select(&User::name, where(less_than(&User::id, 10)))");
        }
        SECTION("select(columns(&User::id, &User::name), where(greater_or_equal(&User::id, 10) and "
                "less_or_equal(&User::id, 20)))") {
            auto sel = select(columns(&User::id, &User::name),
                              where(greater_or_equal(&User::id, 10) and less_or_equal(&User::id, 20)));
            using Sel = decltype(sel);
            using Tuple = node_tuple_t<Sel>;
            using Expected = std::
                tuple<decltype(&User::id), decltype(&User::name), decltype(&User::id), int, decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value,
                          "select(columns(&User::id, &User::name), where(greater_or_equal(&User::id, 10) and "
                          "less_or_equal(&User::id, 20)))");
        }
        SECTION("select(columns('ototo', 25))") {
            auto statement = select(columns("ototo", 25));
            using Statement = decltype(statement);
            using Tuple = node_tuple_t<Statement>;
            using ExpectedTuple = tuple<const char*, int>;
            STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
        }
    }
    SECTION("get_all_t") {
        SECTION("get_all<User>()") {
            auto getAll = get_all<User>();
            using GetAll = decltype(getAll);
            using Tuple = node_tuple_t<GetAll>;
            static_assert(is_same<Tuple, tuple<>>::value, "get_all<User>()");
        }
        SECTION("get_all<User>(where(is_equal(5.0, &User::id)))") {
            auto getAll = get_all<User>(where(is_equal(5.0, &User::id)));
            using GetAll = decltype(getAll);
            using Tuple = node_tuple_t<GetAll>;
            static_assert(is_same<Tuple, tuple<double, decltype(&User::id)>>::value,
                          "get_all<User>(where(is_equal(5.0, &User::id)))");
        }
        SECTION("get_all<User>(where(is_equal(5.0, &User::id)))") {
            auto getAll = get_all<User>(where(is_equal(&User::id, 1) or is_equal(std::string("Alex"), &User::name)));
            using GetAll = decltype(getAll);
            using Tuple = node_tuple_t<GetAll>;
            static_assert(is_same<Tuple, tuple<decltype(&User::id), int, std::string, decltype(&User::name)>>::value,
                          "get_all<User>(where(is_equal(5.0, &User::id)))");
        }
    }
    SECTION("cast_t") {
        auto sel = select(columns(cast<int>(&User::id), cast<int>(&User::name)));
        using Select = decltype(sel);
        using Tuple = node_tuple_t<Select>;
        static_assert(is_same<Tuple, tuple<decltype(&User::id), decltype(&User::name)>>::value,
                      "select(columns(cast<int>(&User::id), cast<int>(&User::name)))");
    }
    SECTION("optional_container") {
        using namespace internal;
        SECTION("int") {
            using Op = optional_container<int>;
            using Tuple = node_tuple_t<Op>;
            static_assert(is_same<Tuple, tuple<int>>::value, "optional_container<int>");
        }
        SECTION("void") {
            using Op = optional_container<void>;
            using Tuple = node_tuple_t<Op>;
            static_assert(is_same<Tuple, tuple<>>::value, "optional_container<void>");
        }
    }
    SECTION("like_t") {
        SECTION("like(&User::name, 'S%')") {
            auto lk = like(&User::name, "S%");
            using Like = decltype(lk);
            using NodeTuple = node_tuple_t<Like>;
            using Expected = tuple<decltype(&User::name), const char*>;
            static_assert(is_same<NodeTuple, Expected>::value, "like(&User::name, \"S%\") type 0");
        }
        SECTION("like(&User::name, std::string('pattern'), '%')") {
            auto lk = like(&User::name, std::string("pattern"), "%");
            using Like = decltype(lk);
            using NodeTuple = node_tuple_t<Like>;
            using Expected = tuple<decltype(&User::name), std::string, const char*>;
            static_assert(is_same<NodeTuple, Expected>::value, "like(&User::name, std::string(\"pattern\"), \"%\")");
        }
        SECTION("like(&User::name, std::string('pattern')).escape('%')") {
            auto lk = like(&User::name, std::string("pattern")).escape("%");
            using Like = decltype(lk);
            using NodeTuple = node_tuple_t<Like>;
            using Expected = tuple<decltype(&User::name), std::string, const char*>;
            static_assert(is_same<NodeTuple, Expected>::value,
                          "like(&User::name, std::string(\"pattern\")).escape(\"%\")");
        }
    }
    SECTION("order_by_t") {
        SECTION("expression") {
            STATIC_REQUIRE(is_same<node_tuple_t<decltype(order_by(&User::name == c(5)))>,
                                   tuple<decltype(&User::name), int>>::value);
        }
        SECTION("bindable") {
            STATIC_REQUIRE(is_same<node_tuple_t<decltype(order_by(""))>, tuple<const char*>>::value);
        }
        SECTION("positional ordinal") {
            STATIC_REQUIRE(is_same<node_tuple_t<decltype(order_by(1))>, tuple<>>::value);
        }
        SECTION("sole column alias") {
            STATIC_REQUIRE(is_same<node_tuple_t<decltype(order_by(get<colalias_a>()))>, tuple<>>::value);
        }
        SECTION("column alias in expression") {
            STATIC_REQUIRE(is_same<node_tuple_t<decltype(order_by(get<colalias_a>() > 1))>, tuple<int>>::value);
        }
    }
    SECTION("glob_t") {
        auto gl = glob(&User::name, "H*");
        using Glob = decltype(gl);
        using Tuple = node_tuple_t<Glob>;
        static_assert(is_same<Tuple, tuple<decltype(&User::name), const char*>>::value, "glob(&User::name, \"H*\")");
    }
    SECTION("between_t") {
        auto bet = between(&User::id, 10, 20);
        using Between = decltype(bet);
        using Tuple = node_tuple_t<Between>;
        static_assert(is_same<Tuple, tuple<decltype(&User::id), int, int>>::value, "between(&User::id, 10, 20)");
    }
    SECTION("named_collate") {
        auto sel = select(&User::name, where(is_equal(&User::name, "Mercury").collate("ototo")));
        using Select = decltype(sel);
        using Tuple = node_tuple_t<Select>;
        using Expected = tuple<decltype(&User::name), decltype(&User::name), const char*>;
        static_assert(is_same<Tuple, Expected>::value, "named_collate");
    }
    SECTION("negated_condition_t") {
        SECTION("not is_equal(20, '20')") {
            auto c = not is_equal(20, "20");
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<int, const char*>;
            static_assert(is_same<Tuple, Expected>::value, "not is_equal(20, \"20\")");
        }
        SECTION("not is_not_equal(&User::id, 15.0)") {
            auto c = not is_not_equal(&User::id, 15.0);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::id), double>;
            static_assert(is_same<Tuple, Expected>::value, "not is_not_equal(&User::id, 15.0)");
        }
        SECTION("not greater_than(20.0f, &User::id)") {
            auto c = not greater_than(20.0f, &User::id);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<float, decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "not greater_than(20.0f, &User::id)");
        }
        SECTION("not greater_or_equal(&User::id, 5)") {
            auto c = not greater_or_equal(&User::id, 5);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "not greater_or_equal(&User::id, 5)");
        }
        SECTION("not less_than(&User::id, std::string('6'))") {
            auto c = not less_than(&User::id, std::string("6"));
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::id), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "not less_than(&User::id, std::string(\"6\"))");
        }
        SECTION("not less_or_equal(&User::id, 10)") {
            auto c = not less_or_equal(&User::id, 10);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "not less_or_equal(&User::id, 10)");
        }
        SECTION("not in(&User::id, {1, 2, 3})") {
            auto c = not in(&User::id, {1, 2, 3});
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::id), std::vector<int>>;
            static_assert(is_same<Tuple, Expected>::value, "not in(&User::id, {1, 2, 3})");
        }
        SECTION("not is_null(&User::name)") {
            auto c = not is_null(&User::name);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "not is_null(&User::name)");
        }
        SECTION("not is_not_null(&User::name)") {
            auto c = not is_not_null(&User::name);
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "not is_not_null(&User::name)");
        }
        SECTION("not like(&User::name, '*D*')") {
            auto c = not like(&User::name, "*D*");
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::name), const char*>;
            static_assert(is_same<Tuple, Expected>::value, "not like(&User::name, \"*D*\")");
        }
        SECTION("not glob(&User::name, std::string('_A_'))") {
            auto c = not glob(&User::name, std::string("_A_"));
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::name), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "not glob(&User::name, std::string(\"_A_\"))");
        }
        SECTION("not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))") {
            auto c = not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))));
            using Con = decltype(c);
            using Tuple = node_tuple_t<Con>;
            using Expected = tuple<decltype(&User::name), decltype(&User::id), std::vector<int>>;
            static_assert(is_same<Tuple, Expected>::value,
                          "not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))");
        }
    }
    SECTION("core_function_t") {
        SECTION("lower") {
            auto f = lower(&User::name);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "lower");
        }
        SECTION("upper") {
            auto f = upper("hi");
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using ArgType = std::tuple_element_t<0, Fun::args_type>;
            static_assert(is_same<ArgType, const char*>::value, "upper arg[0]");
            using Expected = tuple<const char*>;
            static_assert(is_same<Tuple, Expected>::value, "upper");
        }
        SECTION("total_changes") {
            auto f = total_changes();
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<>;
            static_assert(is_same<Tuple, Expected>::value, "total_changes");
        }
        SECTION("changes") {
            auto f = changes();
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<>;
            static_assert(is_same<Tuple, Expected>::value, "changes");
        }
        SECTION("trim(1)") {
            auto f = trim(&User::name);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "trim(1)");
        }
        SECTION("trim(2)") {
            auto f = trim(&User::name, std::string("pay"));
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "trim(2)");
        }
        SECTION("ltrim(1)") {
            auto f = ltrim(&User::id);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "ltrim(1)");
        }
        SECTION("ltrim(2)") {
            auto f = ltrim(&User::id, "see");
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::id), const char*>;
            static_assert(is_same<Tuple, Expected>::value, "ltrim(2)");
        }
        SECTION("rtrim(1)") {
            auto f = rtrim(&User::name);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "rtrim(1)");
        }
        SECTION("rtrim(2)") {
            auto f = rtrim(&User::name, &User::id);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name), decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "rtrim(2)");
        }
#if SQLITE_VERSION_NUMBER >= 3007016
        SECTION("char_") {
            auto f = char_(100, 20.0);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<int, double>;
            static_assert(is_same<Tuple, Expected>::value, "char_");
        }
#endif
        SECTION("coalesce") {
            auto f = coalesce(10, 20);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<int, int>;
            static_assert(is_same<Tuple, Expected>::value, "coalesce");
        }
        SECTION("date") {
            auto f =
                date(std::string("now"), std::string("start of month"), std::string("+1 month"), std::string("-1 day"));
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<std::string, std::string, std::string, std::string>;
            static_assert(is_same<Tuple, Expected>::value, "date");
        }
        SECTION("datetime") {
            auto f = datetime("now");
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<const char*>;
            static_assert(is_same<Tuple, Expected>::value, "datetime");
        }
        SECTION("julianday") {
            auto f = julianday("now");
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<const char*>;
            static_assert(is_same<Tuple, Expected>::value, "julianday");
        }
        SECTION("zeroblob") {
            auto f = zeroblob(10);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<int>;
            static_assert(is_same<Tuple, Expected>::value, "zeroblob");
        }
        SECTION("substr(2)") {
            auto f = substr(&User::name, 7);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name), int>;
            static_assert(is_same<Tuple, Expected>::value, "substr");
        }
        SECTION("substr(3)") {
            auto f = substr(&User::name, 7, 20.f);
            using Fun = decltype(f);
            using Tuple = node_tuple_t<Fun>;
            using Expected = tuple<decltype(&User::name), int, float>;
            static_assert(is_same<Tuple, Expected>::value, "substr");
        }
    }
    SECTION("join") {
        SECTION("left_join") {
            auto j = left_join<User>(on(is_equal(&User::id, 2)));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "left_join");
        }
        SECTION("join on") {
            auto j = join<User>(on(is_equal(&User::id, 2)));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "join on");
        }
        SECTION("join using column") {
            auto j = join<User>(using_(&User::id));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<column_pointer<User, decltype(&User::id)>>;
            static_assert(is_same<Tuple, Expected>::value, "join using");
        }
        SECTION("join using explicit column") {
            struct Derived : User {};
            auto j = join<User>(using_(column<Derived>(&User::id)));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<column_pointer<Derived, decltype(&User::id)>>;
            static_assert(is_same<Tuple, Expected>::value, "join using explicit column");
        }
        SECTION("left_outer_join") {
            auto j = left_outer_join<User>(on(is_equal(&User::id, 2)));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "left_outer_join");
        }
        SECTION("inner_join") {
            auto j = inner_join<User>(on(is_equal(&User::id, 2)));
            using Join = decltype(j);
            using Tuple = node_tuple_t<Join>;
            using Expected = tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "inner_join");
        }
    }
    SECTION("case") {
        auto c = case_<std::string>(&User::name).when("USA", then("Dosmetic")).else_("Foreign").end();
        using Case = decltype(c);
        using CaseExpressionTuple = node_tuple<Case::case_expression_type>::type;
        STATIC_REQUIRE(is_same<CaseExpressionTuple, tuple<decltype(&User::name)>>::value);

        STATIC_REQUIRE(is_tuple<tuple<>>::value);
        STATIC_REQUIRE(is_tuple<tuple<int, std::string>>::value);
        STATIC_REQUIRE_FALSE(is_tuple<int>::value);
        STATIC_REQUIRE(is_pair<std::pair<int, std::string>>::value);
        STATIC_REQUIRE_FALSE(is_pair<int>::value);

        using ArgsType = Case::args_type;
        STATIC_REQUIRE(is_tuple<ArgsType>::value);
        STATIC_REQUIRE(std::tuple_size<ArgsType>::value == 1);

        using Arg0 = std::tuple_element_t<0, ArgsType>;
        STATIC_REQUIRE(is_pair<Arg0>::value);
        using Arg0First = Arg0::first_type;
        STATIC_REQUIRE(is_same<Arg0First, const char*>::value);
        using Arg0Second = Arg0::second_type;
        STATIC_REQUIRE(is_same<Arg0Second, const char*>::value);
        STATIC_REQUIRE(is_same<ArgsType, tuple<std::pair<const char*, const char*>>>::value);

        using ElseExpressionTuple = node_tuple<Case::else_expression_type>::type;
        STATIC_REQUIRE(is_same<ElseExpressionTuple, tuple<const char*>>::value);
    }
    SECTION("as") {
        struct GradeAlias : alias_tag {
            static const std::string& get() {
                static const std::string res = "Grade";
                return res;
            }
        };
        auto a = as<GradeAlias>(&User::name);
        using A = decltype(a);
        using Tuple = node_tuple_t<A>;
        using ExpectedTuple = tuple<decltype(&User::name)>;
        STATIC_REQUIRE(is_same<Tuple, ExpectedTuple>::value);
    }
    SECTION("function_call") {
        struct Func {
            static const char* name();
            bool operator()(int value) const {
                return value % 2;
            }
        };
        auto statement = func<Func>(8);
        using Statement = decltype(statement);
        using Tuple = node_tuple_t<Statement>;
        using ExpectedTuple = tuple<int>;
        STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
    }
    SECTION("excluded") {
        auto statement = excluded(&User::id);
        using Statement = decltype(statement);
        using Tuple = node_tuple_t<Statement>;
        using ExpectedTuple = tuple<decltype(&User::id)>;
        STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
    }
#if SQLITE_VERSION_NUMBER >= 3024000
    SECTION("upsert_clause") {
        auto statement = on_conflict(&User::id).do_update(set(c(&User::name) = excluded(&User::name)));
        using Statement = decltype(statement);
        using Tuple = node_tuple_t<Statement>;
        using ExpectedTuple = tuple<decltype(&User::name), decltype(&User::name)>;
        STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
    }
#endif
    SECTION("group_by") {
        auto statement = group_by(&User::id);
        using Statement = decltype(statement);
        using Tuple = node_tuple_t<Statement>;
        using ExpectedTuple = tuple<decltype(&User::id)>;
        STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
    }
    SECTION("set assign") {
        auto statement = set(assign(&User::name, conc(&User::name, "_")));
        using Tuple = node_tuple_t<decltype(statement)>;
        using ExpectedTuple = tuple<decltype(&User::name), decltype(&User::name), const char*>;
        STATIC_REQUIRE(std::is_same<Tuple, ExpectedTuple>::value);
    }
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    SECTION("with ordinary") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with(1_ctealias().as(select(1)), select(column<cte_1>(1_colalias)));
        using Tuple = node_tuple_t<decltype(expression)>;
        using ExpectedTuple = tuple<int, column_pointer<cte_1, alias_holder<column_alias<'1'>>>>;
        STATIC_REQUIRE(std::is_same_v<Tuple, ExpectedTuple>);
    }
    SECTION("with not enforced recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with(1_ctealias().as(select(1)), select(column<cte_1>(1_colalias)));
        using Tuple = node_tuple_t<decltype(expression)>;
        using ExpectedTuple = tuple<int, column_pointer<cte_1, alias_holder<column_alias<'1'>>>>;
        STATIC_REQUIRE(std::is_same_v<Tuple, ExpectedTuple>);
    }
    SECTION("with optional recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with_recursive(
            1_ctealias().as(
                union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
            select(column<cte_1>(1_colalias)));
        using Tuple = node_tuple_t<decltype(expression)>;
        using ExpectedTuple = tuple<int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>,
                                    int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>,
                                    int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>>;
        STATIC_REQUIRE(std::is_same_v<Tuple, ExpectedTuple>);
    }
    SECTION("with recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with_recursive(
            1_ctealias().as(
                union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
            select(column<cte_1>(1_colalias)));
        using Tuple = node_tuple_t<decltype(expression)>;
        using ExpectedTuple = tuple<int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>,
                                    int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>,
                                    int,
                                    column_pointer<cte_1, alias_holder<column_alias<'1'>>>>;
        STATIC_REQUIRE(std::is_same_v<Tuple, ExpectedTuple>);
    }
#endif
}
