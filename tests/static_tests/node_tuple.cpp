#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

template<class T>
struct is_pair : std::false_type {};

template<class L, class R>
struct is_pair<std::pair<L, R>> : std::true_type {};

template<class T>
struct is_tuple : std::false_type {};

template<class... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type {};

TEST_CASE("Node tuple") {
    using internal::bindable_filter;
    using internal::node_tuple;
    using std::is_same;

    struct User {
        int id = 0;
        std::string name;
    };

    //  simple
    {
        using Tuple = node_tuple<int>::type;
        using Expected = std::tuple<int>;
        static_assert(is_same<Tuple, Expected>::value, "int");
        static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int>>::value, "");
    }
    {
        using Tuple = node_tuple<float>::type;
        using Expected = std::tuple<float>;
        static_assert(is_same<Tuple, Expected>::value, "float");
        static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<float>>::value, "");
    }

    {  //  binary_condition
        using namespace internal;
        {  //  5 < 6.0f
            auto c = lesser_than(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "lesser_than_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  id < 10
            auto c = lesser_than(&User::id, 10);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "lesser_than_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int>>::value, "");
        }
        {  //  5 <= 6.0f
            auto c = lesser_or_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "lesser_or_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  id <= 10.0
            auto c = lesser_or_equal(&User::id, 10.0);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<decltype(&User::id), double>;
            static_assert(is_same<Tuple, Expected>::value, "lesser_or_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<double>>::value, "");
        }
        {  //  5 > 6.0f
            auto c = greater_than(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "greater_than_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  id > 20
            auto c = greater_than(&User::id, 20);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<decltype(&User::id), int>;
            static_assert(is_same<Tuple, Expected>::value, "greater_than_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int>>::value, "");
        }
        {  //  5 >= 6.0f
            auto c = greater_or_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "greater_or_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  5 >= id
            auto c = greater_or_equal(5, &User::id);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, decltype(&User::id)>;
            static_assert(is_same<Tuple, Expected>::value, "greater_or_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int>>::value, "");
        }
        {  //  5 == 6.0f
            auto c = is_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "is_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  "ototo" == name
            auto c = is_equal("ototo", &User::name);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<const char*, decltype(&User::name)>;
            static_assert(is_same<Tuple, Expected>::value, "is_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<const char*>>::value, "");
        }
        {  //  5 != 6.0f
            auto c = is_not_equal(5, 6.0f);
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<int, float>;
            static_assert(is_same<Tuple, Expected>::value, "is_not_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<int, float>>::value, "");
        }
        {  //  name != std::string("ototo")
            auto c = is_not_equal(&User::name, std::string("ototo"));
            using C = decltype(c);
            using Tuple = node_tuple<C>::type;
            using Expected = std::tuple<decltype(&User::name), std::string>;
            static_assert(is_same<Tuple, Expected>::value, "is_not_equal_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<std::string>>::value, "");
        }
        {  //  bool and int
            using Tuple = node_tuple<and_condition_t<bool, int>>::type;
            using Expected = std::tuple<bool, int>;
            static_assert(is_same<Tuple, Expected>::value, "and_condition_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<bool, int>>::value, "");
        }
        {  //  bool or int
            using Tuple = node_tuple<or_condition_t<bool, int>>::type;
            using Expected = std::tuple<bool, int>;
            static_assert(is_same<Tuple, Expected>::value, "or_condition_t");
            static_assert(is_same<bindable_filter<Tuple>::type, std::tuple<bool, int>>::value, "");
        }
    }
    {  //  binary_operator
        using namespace internal;

        using CondTuple = node_tuple<conc_t<std::string, decltype(&User::name)>>::type;
        static_assert(is_same<CondTuple, std::tuple<std::string, decltype(&User::name)>>::value, "conc_t");

        using AddTuple = node_tuple<add_t<int, decltype(&User::id)>>::type;
        static_assert(is_same<AddTuple, std::tuple<int, decltype(&User::id)>>::value, "add_t");

        using SubTuple = node_tuple<sub_t<float, double>>::type;
        static_assert(is_same<SubTuple, std::tuple<float, double>>::value, "sub_t");

        using MulTuple = node_tuple<mul_t<double, decltype(&User::id)>>::type;
        static_assert(is_same<MulTuple, std::tuple<double, decltype(&User::id)>>::value, "mul_t");

        using DivTuple = node_tuple<sqlite_orm::internal::div_t<int, float>>::type;
        static_assert(is_same<DivTuple, std::tuple<int, float>>::value, "div_t");

        using ModTuple = node_tuple<mod_t<decltype(&User::id), int>>::type;
        static_assert(is_same<ModTuple, std::tuple<decltype(&User::id), int>>::value, "mod_t");

        using AssignTuple = node_tuple<assign_t<decltype(&User::name), std::string>>::type;
        static_assert(is_same<AssignTuple, std::tuple<decltype(&User::name), std::string>>::value, "assign_t");
    }
    {  // columns
        auto cols = columns(&User::id, &User::name);
        using Cols = decltype(cols);
        using ColsTuple = node_tuple<Cols>::type;
        static_assert(is_same<ColsTuple, std::tuple<decltype(&User::id), decltype(&User::name)>>::value, "columns_t");
    }
    {  // in
        auto inValue = in(&User::id, {1, 2, 3});
        using In = decltype(inValue);
        using InTuple = node_tuple<In>::type;
        static_assert(is_same<InTuple, std::tuple<decltype(&User::id), std::vector<int>>>::value, "in_t");
    }
    {//  compound operator...
     {// union_(select(1), select(2))
      auto un = union_(select(1), select(2));
    using Union = decltype(un);
    using Tuple = node_tuple<Union>::type;
    static_assert(is_same<Tuple, std::tuple<int, int>>::value, "union_(select(1), select(2))");
}
{  // union_all
    auto un = union_all(select(&User::id, where(is_equal(&User::name, "Alice"))),
                        select(&User::id, where(is_equal(std::string("Bob"), &User::name))));
    using Union = decltype(un);
    using Tuple = node_tuple<Union>::type;
    using Expected = std::tuple<decltype(&User::id),
                                decltype(&User::name),
                                const char*,
                                decltype(&User::id),
                                std::string,
                                decltype(&User::name)>;
    static_assert(is_same<Tuple, Expected>::value, "union_all");
}
{  // except
    auto un = except(select(columns(&User::id, &User::name), where(is_equal(&User::id, 10L))),
                     select(columns(&User::id, &User::name), where(is_equal(&User::id, 15L))));
    using Union = decltype(un);
    using Tuple = node_tuple<Union>::type;
    using Expected = std::tuple<decltype(&User::id),
                                decltype(&User::name),
                                decltype(&User::id),
                                long,
                                decltype(&User::id),
                                decltype(&User::name),
                                decltype(&User::id),
                                long>;
    static_assert(is_same<Tuple, Expected>::value, "except");
}
{  // intersect
    auto un = intersect(select(&User::name), select(&User::name, where(is_equal(&User::name, "Anny"))));
    using Union = decltype(un);
    using Tuple = node_tuple<Union>::type;
    using Expected = std::tuple<decltype(&User::name), decltype(&User::name), decltype(&User::name), const char*>;
    static_assert(is_same<Tuple, Expected>::value, "intersect");
}
}
{// select
 {// select(&User::id)
  auto sel = select(&User::id);
using Sel = decltype(sel);
using Tuple = node_tuple<Sel>::type;
static_assert(is_same<Tuple, std::tuple<decltype(&User::id)>>::value, "select(&User::id)");
}
{  // select(&User::name)
    auto sel = select(&User::name);
    using Sel = decltype(sel);
    using Tuple = node_tuple<Sel>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::name)>>::value, "select(&User::name)");
}
{  // select(&User::id, where(is_equal(&User::id, 5)))
    auto sel = select(&User::id, where(is_equal(&User::id, 5)));
    using Sel = decltype(sel);
    using Tuple = node_tuple<Sel>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::id), decltype(&User::id), int>>::value,
                  "select(&User::id, where(is_equal(&User::id, 5)))");
}
{  // select(&User::name, where(lesser_than(&User::id, 10)))
    auto sel = select(&User::name, where(lesser_than(&User::id, 10)));
    using Sel = decltype(sel);
    using Tuple = node_tuple<Sel>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::name), decltype(&User::id), int>>::value,
                  "select(&User::name, where(lesser_than(&User::id, 10)))");
}
{  // select(columns(&User::id, &User::name), where(greater_or_equal(&User::id, 10) and lesser_or_equal(&User::id, 20)))
    auto sel = select(columns(&User::id, &User::name),
                      where(greater_or_equal(&User::id, 10) and lesser_or_equal(&User::id, 20)));
    using Sel = decltype(sel);
    using Tuple = node_tuple<Sel>::type;
    using Expected =
        std::tuple<decltype(&User::id), decltype(&User::name), decltype(&User::id), int, decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value,
                  "select(columns(&User::id, &User::name), where(greater_or_equal(&User::id, 10) and "
                  "lesser_or_equal(&User::id, 20)))");
}
}
{// get_all_t
 {// get_all<User>()
  auto getAll = get_all<User>();
using GetAll = decltype(getAll);
using Tuple = node_tuple<GetAll>::type;
static_assert(is_same<Tuple, std::tuple<>>::value, "get_all<User>()");
}
{  // get_all<User>(where(is_equal(5.0, &User::id)))
    auto getAll = get_all<User>(where(is_equal(5.0, &User::id)));
    using GetAll = decltype(getAll);
    using Tuple = node_tuple<GetAll>::type;
    static_assert(is_same<Tuple, std::tuple<double, decltype(&User::id)>>::value,
                  "get_all<User>(where(is_equal(5.0, &User::id)))");
}
{  // get_all<User>(where(is_equal(5.0, &User::id)))
    auto getAll = get_all<User>(where(is_equal(&User::id, 1) or is_equal(std::string("Alex"), &User::name)));
    using GetAll = decltype(getAll);
    using Tuple = node_tuple<GetAll>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::id), int, std::string, decltype(&User::name)>>::value,
                  "get_all<User>(where(is_equal(5.0, &User::id)))");
}
}
{  // having_t
    using namespace internal;
    auto hav = having(greater_or_equal(&User::id, 10));
    using Having = decltype(hav);
    using Tuple = node_tuple<Having>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::id), int>>::value,
                  "having(greater_or_equal(&User::id, 10))");
}
{  // cast_t
    auto sel = select(columns(cast<int>(&User::id), cast<int>(&User::name)));
    using Select = decltype(sel);
    using Tuple = node_tuple<Select>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::id), decltype(&User::name)>>::value,
                  "select(columns(cast<int>(&User::id), cast<int>(&User::name)))");
}
{  // optional_container
    using namespace internal;
    {
        using Op = optional_container<int>;
        using Tuple = node_tuple<Op>::type;
        static_assert(is_same<Tuple, std::tuple<int>>::value, "optional_container<int>");
    }
    {
        using Op = optional_container<void>;
        using Tuple = node_tuple<Op>::type;
        static_assert(is_same<Tuple, std::tuple<>>::value, "optional_container<void>");
    }
}
{// like_t
 {// like(&User::name, "S%")
  auto lk = like(&User::name, "S%");
using Like = decltype(lk);
using NodeTuple = node_tuple<Like>;
using ArgTuple = NodeTuple::arg_tuple;
static_assert(is_same<ArgTuple, std::tuple<decltype(&User::name)>>::value, "arg_tuple");
using PatternTuple = NodeTuple::pattern_tuple;
static_assert(is_same<PatternTuple, std::tuple<const char*>>::value, "pattern_tuple");
using EscapeTuple = NodeTuple::escape_tuple;
static_assert(is_same<EscapeTuple, std::tuple<>>::value, "escape_tuple");
using Tuple = NodeTuple::type;
static_assert(std::tuple_size<Tuple>::value == 2, "like(&User::name, \"S%\") size");
using Tuple0 = std::tuple_element<0, Tuple>::type;
static_assert(is_same<Tuple0, decltype(&User::name)>::value, "like(&User::name, \"S%\") type 0");
static_assert(is_same<Tuple, std::tuple<decltype(&User::name), const char*>>::value, "like(&User::name, \"S%\")");
}
{  // like(&User::name, std::string("pattern"), "%")
    auto lk = like(&User::name, std::string("pattern"), "%");
    using Like = decltype(lk);
    using NodeTuple = node_tuple<Like>::type;
    using Expected = std::tuple<decltype(&User::name), std::string, const char*>;
    static_assert(is_same<NodeTuple, Expected>::value, "like(&User::name, std::string(\"pattern\"), \"%\")");
}
{  // like(&User::name, std::string("pattern")).escape("%")
    auto lk = like(&User::name, std::string("pattern")).escape("%");
    using Like = decltype(lk);
    using NodeTuple = node_tuple<Like>::type;
    using Expected = std::tuple<decltype(&User::name), std::string, const char*>;
    static_assert(is_same<NodeTuple, Expected>::value, "like(&User::name, std::string(\"pattern\")).escape(\"%\")");
}
}
{  // glob_t
    auto gl = glob(&User::name, "H*");
    using Glob = decltype(gl);
    using Tuple = node_tuple<Glob>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::name), const char*>>::value, "glob(&User::name, \"H*\")");
}
{  // between_t
    auto bet = between(&User::id, 10, 20);
    using Between = decltype(bet);
    using Tuple = node_tuple<Between>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::id), int, int>>::value, "between(&User::id, 10, 20)");
}
{  // named_collate
    auto sel = select(&User::name, where(is_equal(&User::name, "Mercury").collate("ototo")));
    using Select = decltype(sel);
    using Tuple = node_tuple<Select>::type;
    using Expected = std::tuple<decltype(&User::name), decltype(&User::name), const char*>;
    static_assert(is_same<Tuple, Expected>::value, "named_collate");
}
{// negated_condition_t
 {// not is_equal(20, "20");
  auto c = not is_equal(20, "20");
using Con = decltype(c);
using Tuple = node_tuple<Con>::type;
using Expected = std::tuple<int, const char*>;
static_assert(is_same<Tuple, Expected>::value, "not is_equal(20, \"20\")");
}
{  // not is_not_equal(&User::id, 15.0)
    auto c = not is_not_equal(&User::id, 15.0);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::id), double>;
    static_assert(is_same<Tuple, Expected>::value, "not is_not_equal(&User::id, 15.0)");
}
{  // not greater_than(20.0f, &User::id)
    auto c = not greater_than(20.0f, &User::id);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<float, decltype(&User::id)>;
    static_assert(is_same<Tuple, Expected>::value, "not greater_than(20.0f, &User::id)");
}
{  // not greater_or_equal(&User::id, 5)
    auto c = not greater_or_equal(&User::id, 5);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value, "not greater_or_equal(&User::id, 5)");
}
{  // not lesser_than(&User::id, std::string("6"))
    auto c = not lesser_than(&User::id, std::string("6"));
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::id), std::string>;
    static_assert(is_same<Tuple, Expected>::value, "not lesser_than(&User::id, std::string(\"6\"))");
}
{  // not lesser_or_equal(&User::id, 10)
    auto c = not lesser_or_equal(&User::id, 10);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value, "not lesser_or_equal(&User::id, 10)");
}
{  // not in(&User::id, {1, 2, 3})
    auto c = not in(&User::id, {1, 2, 3});
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::id), std::vector<int>>;
    static_assert(is_same<Tuple, Expected>::value, "not in(&User::id, {1, 2, 3})");
}
{  // not is_null(&User::name)
    auto c = not is_null(&User::name);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::name)>;
    static_assert(is_same<Tuple, Expected>::value, "not is_null(&User::name)");
}
{  // not is_not_null(&User::name)
    auto c = not is_not_null(&User::name);
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::name)>;
    static_assert(is_same<Tuple, Expected>::value, "not is_not_null(&User::name)");
}
{  // not like(&User::name, "*D*")
    auto c = not like(&User::name, "*D*");
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::name), const char*>;
    static_assert(is_same<Tuple, Expected>::value, "not like(&User::name, \"*D*\")");
}
{  // not glob(&User::name, std::string("_A_"))
    auto c = not glob(&User::name, std::string("_A_"));
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::name), std::string>;
    static_assert(is_same<Tuple, Expected>::value, "not glob(&User::name, std::string(\"_A_\"))");
}
{  // not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))
    auto c = not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))));
    using Con = decltype(c);
    using Tuple = node_tuple<Con>::type;
    using Expected = std::tuple<decltype(&User::name), decltype(&User::id), std::vector<int>>;
    static_assert(is_same<Tuple, Expected>::value, "not exists(select(&User::name, where(in(&User::id, {6, 7, 9}))))");
}
}
{  // core_function_t
    {  // lower
        auto f = lower(&User::name);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name)>;
        static_assert(is_same<Tuple, Expected>::value, "lower");
    }
    {  // upper
        auto f = upper("hi");
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using ArgType = std::tuple_element<0, Fun::args_type>::type;
        static_assert(is_same<ArgType, const char*>::value, "upper arg[0]");
        using Expected = std::tuple<const char*>;
        static_assert(is_same<Tuple, Expected>::value, "upper");
    }
    {  // total_changes
        auto f = total_changes();
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<>;
        static_assert(is_same<Tuple, Expected>::value, "total_changes");
    }
    {  // changes
        auto f = changes();
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<>;
        static_assert(is_same<Tuple, Expected>::value, "changes");
    }
    {  // trim(1)
        auto f = trim(&User::name);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name)>;
        static_assert(is_same<Tuple, Expected>::value, "trim(1)");
    }
    {  // trim(2)
        auto f = trim(&User::name, std::string("pay"));
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name), std::string>;
        static_assert(is_same<Tuple, Expected>::value, "trim(2)");
    }
    {  // ltrim(1)
        auto f = ltrim(&User::id);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::id)>;
        static_assert(is_same<Tuple, Expected>::value, "ltrim(1)");
    }
    {  // ltrim(2)
        auto f = ltrim(&User::id, "see");
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::id), const char*>;
        static_assert(is_same<Tuple, Expected>::value, "ltrim(2)");
    }
    {  // rtrim(1)
        auto f = rtrim(&User::name);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name)>;
        static_assert(is_same<Tuple, Expected>::value, "rtrim(1)");
    }
    {  // rtrim(2)
        auto f = rtrim(&User::name, &User::id);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name), decltype(&User::id)>;
        static_assert(is_same<Tuple, Expected>::value, "rtrim(2)");
    }
#if SQLITE_VERSION_NUMBER >= 3007016
    {  // char_
        auto f = char_(100, 20.0);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<int, double>;
        static_assert(is_same<Tuple, Expected>::value, "char_");
    }
#endif
    {  // coalesce
        auto f = char_(10, 20);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<int, int>;
        static_assert(is_same<Tuple, Expected>::value, "coalesce");
    }
    {  // date
        auto f =
            date(std::string("now"), std::string("start of month"), std::string("+1 month"), std::string("-1 day"));
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<std::string, std::string, std::string, std::string>;
        static_assert(is_same<Tuple, Expected>::value, "date");
    }
    {  // datetime
        auto f = datetime("now");
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<const char*>;
        static_assert(is_same<Tuple, Expected>::value, "datetime");
    }
    {  // julianday
        auto f = julianday("now");
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<const char*>;
        static_assert(is_same<Tuple, Expected>::value, "julianday");
    }
    {  // zeroblob
        auto f = zeroblob(10);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<int>;
        static_assert(is_same<Tuple, Expected>::value, "zeroblob");
    }
    {  // substr(2)
        auto f = substr(&User::name, 7);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name), int>;
        static_assert(is_same<Tuple, Expected>::value, "substr");
    }
    {  // substr(3)
        auto f = substr(&User::name, 7, 20.f);
        using Fun = decltype(f);
        using Tuple = node_tuple<Fun>::type;
        using Expected = std::tuple<decltype(&User::name), int, float>;
        static_assert(is_same<Tuple, Expected>::value, "substr");
    }
}
{// join
 {// left_join
  auto j = left_join<User>(on(is_equal(&User::id, 2)));
using Join = decltype(j);
using Tuple = node_tuple<Join>::type;
using Expected = std::tuple<decltype(&User::id), int>;
static_assert(is_same<Tuple, Expected>::value, "left_join");
}
{  // join
    auto j = join<User>(on(is_equal(&User::id, 2)));
    using Join = decltype(j);
    using Tuple = node_tuple<Join>::type;
    using Expected = std::tuple<decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value, "join");
}
{  // left_outer_join
    auto j = left_outer_join<User>(on(is_equal(&User::id, 2)));
    using Join = decltype(j);
    using Tuple = node_tuple<Join>::type;
    using Expected = std::tuple<decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value, "left_outer_join");
}
{  // inner_join
    auto j = inner_join<User>(on(is_equal(&User::id, 2)));
    using Join = decltype(j);
    using Tuple = node_tuple<Join>::type;
    using Expected = std::tuple<decltype(&User::id), int>;
    static_assert(is_same<Tuple, Expected>::value, "inner_join");
}
}
{  // case
    auto c = case_<std::string>(&User::name).when("USA", then("Dosmetic")).else_("Foreign").end();
    using Case = decltype(c);
    using CaseExpressionTuple = node_tuple<Case::case_expression_type>::type;
    static_assert(is_same<CaseExpressionTuple, std::tuple<decltype(&User::name)>>::value, "");

    static_assert(is_tuple<std::tuple<>>::value, "");
    static_assert(is_tuple<std::tuple<int, std::string>>::value, "");
    static_assert(!is_tuple<int>::value, "");
    static_assert(is_pair<std::pair<int, std::string>>::value, "");
    static_assert(!is_pair<int>::value, "");

    using ArgsType = Case::args_type;
    static_assert(is_tuple<ArgsType>::value, "");
    static_assert(std::tuple_size<ArgsType>::value == 1, "");

    using Arg0 = std::tuple_element<0, ArgsType>::type;
    static_assert(is_pair<Arg0>::value, "");
    using Arg0First = Arg0::first_type;
    static_assert(is_same<Arg0First, const char*>::value, "");
    using Arg0Second = Arg0::second_type;
    static_assert(is_same<Arg0Second, const char*>::value, "");
    static_assert(is_same<ArgsType, std::tuple<std::pair<const char*, const char*>>>::value, "");

    using ElseExpressionTuple = node_tuple<Case::else_expression_type>::type;
    static_assert(is_same<ElseExpressionTuple, std::tuple<const char*>>::value, "");
}
{  // as
    struct GradeAlias : alias_tag {
        static const std::string& get() {
            static const std::string res = "Grade";
            return res;
        }
    };
    auto a = as<GradeAlias>(&User::name);
    using A = decltype(a);
    using Tuple = node_tuple<A>::type;
    static_assert(is_same<Tuple, std::tuple<decltype(&User::name)>>::value, "");
}
{
    auto statement = select(columns("ototo", 25));
    using Statement = decltype(statement);
    using Tuple = node_tuple<Statement>::type;
    static_assert(std::tuple_size<Tuple>::value == 2, "");
    {
        using Arg0 = std::tuple_element<0, Tuple>::type;
        static_assert(is_same<Arg0, const char*>::value, "");
    }
    {
        using Arg1 = std::tuple_element<1, Tuple>::type;
        static_assert(is_same<Arg1, int>::value, "");
    }
}
}
