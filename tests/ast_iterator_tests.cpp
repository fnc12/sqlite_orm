#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <typeindex>  //  std::type_index

using namespace sqlite_orm;
using internal::alias_column_t;
using internal::alias_holder;
using internal::column_alias;
using internal::column_pointer;
using internal::iterate_ast;

TEST_CASE("ast_iterator") {
    struct User {
        int id = 0;
        std::string name;
    };
    std::vector<std::type_index> typeIndexes;
    decltype(typeIndexes) expected;
    auto lambda = [&typeIndexes](auto& value) {
        typeIndexes.push_back(typeid(value));
    };
    SECTION("bindables") {
        auto node = select(1);
        expected.push_back(typeid(int));
        iterate_ast(node, lambda);
    }
    SECTION("aggregate functions") {
        SECTION("avg") {
            auto node = avg(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("avg filter") {
            auto node = avg(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("count(*)") {
            auto node = count<User>();
            expected.push_back(typeid(node));
            iterate_ast(node, lambda);
        }
        SECTION("count(*) filter") {
            auto node = count<User>().filter(where(length(&User::name) > 5));
            expected.push_back(typeid(decltype(count<User>())));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("count(X)") {
            auto node = count(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("count(X) filter") {
            auto node = count(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("group_concat(X)") {
            auto node = group_concat(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("group_concat(X) filter") {
            auto node = group_concat(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("group_concat(X,Y)") {
            auto node = group_concat(&User::id, std::string("-"));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(std::string));
            iterate_ast(node, lambda);
        }
        SECTION("group_concat(X,Y) filter") {
            auto node = group_concat(&User::id, std::string("-")).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(std::string));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("max(X)") {
            auto node = max(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("max(X) filter") {
            auto node = max(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("min(X)") {
            auto node = min(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("min(X) filter") {
            auto node = min(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("sum(X)") {
            auto node = sum(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("sum(X) filter") {
            auto node = sum(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("total(X)") {
            auto node = total(&User::id);
            expected.push_back(typeid(&User::id));
            iterate_ast(node, lambda);
        }
        SECTION("total(X) filter") {
            auto node = total(&User::id).filter(where(length(&User::name) > 5));
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(&User::name));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
    }
    SECTION("scalar functions") {
        SECTION("max(X,Y)") {
            auto node = max(&User::id, 4);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("min(X,Y)") {
            auto node = min(&User::id, 4);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
    }
    SECTION("on") {
        auto node = on(&User::id == c(0));
        expected.push_back(typeid(&User::id));
        expected.push_back(typeid(int));
        iterate_ast(node, lambda);
    }
    SECTION("using") {
        auto node = using_(&User::id);
        expected.push_back(typeid(column_pointer<User, decltype(&User::id)>));
        iterate_ast(node, lambda);
    }
    SECTION("exists") {
        auto node = exists(select(5));
        expected.push_back(typeid(int));
        iterate_ast(node, lambda);
    }
    SECTION("order_by") {
        SECTION("expression") {
            auto node = order_by(c(&User::id) == 0);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("bindable") {
            auto node = order_by("");
            expected.push_back(typeid(const char*));
            iterate_ast(node, lambda);
        }
        SECTION("positional ordinal") {
            auto node = order_by(1);
            iterate_ast(node, lambda);
        }
        SECTION("sole column alias") {
            auto node = order_by(get<colalias_a>());
            iterate_ast(node, lambda);
        }
        SECTION("direct sole column alias") {
            auto node = order_by(colalias_a{});
            iterate_ast(node, lambda);
        }
        SECTION("column alias in expression") {
            auto node = order_by(get<colalias_a>() > 1);
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
    }
    SECTION("group_by") {
        auto node = group_by(&User::id);
        expected.push_back(typeid(&User::id));
        iterate_ast(node, lambda);
    }
    SECTION("excluded") {
        auto node = excluded(&User::id);
        expected.push_back(typeid(&User::id));
        iterate_ast(node, lambda);
    }
#if SQLITE_VERSION_NUMBER >= 3024000
    SECTION("upsert_clause") {
        auto node = on_conflict(&User::id).do_update(set(c(&User::name) = excluded(&User::name)));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(&User::name));
        iterate_ast(node, lambda);
    }
#endif
    SECTION("into") {
        auto node = into<User>();
        iterate_ast(node, lambda);
    }
    SECTION("match") {
        auto node = match<User>(std::string("Plazma"));
        expected.push_back(typeid(std::string));
        iterate_ast(node, lambda);
    }
    SECTION("replace") {
        auto node =
            replace(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        expected.push_back(typeid(&User::id));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        iterate_ast(node, lambda);
    }
    SECTION("insert") {
        auto node =
            insert(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, std::string("Ellie"))));
        expected.push_back(typeid(&User::id));
        expected.push_back(typeid(&User::name));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        iterate_ast(node, lambda);
    }
    SECTION("values") {
        auto node = values(std::make_tuple(1, std::string("hi")));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        iterate_ast(node, lambda);
    }
    SECTION("tuple") {
        auto node = std::make_tuple(1, std::string("hi"));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        iterate_ast(node, lambda);
    }
    SECTION("in") {
        SECTION("static") {
            auto node = c(&User::id).in(1, 2, 3);
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
        SECTION("dynamic") {
            auto node = in(&User::id, {1, 2, 3});
            expected.push_back(typeid(&User::id));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            expected.push_back(typeid(int));
            iterate_ast(node, lambda);
        }
    }
    SECTION("function_call") {
        struct Func {
            static const char* name();
            bool operator()(int value) const {
                return value % 2 == 0;
            }
        };
        auto node = func<Func>(&User::id);
        expected.push_back(typeid(&User::id));
        iterate_ast(node, lambda);
    }
    SECTION("aliases") {
        SECTION("holder") {
            auto expression = get<colalias_a>() > 0;
            expected.push_back(typeid(int));
            iterate_ast(expression, lambda);
        }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        {
            SECTION("direct") {
                auto expression = "a"_col > 0;
                expected.push_back(typeid(int));
                iterate_ast(expression, lambda);
            }
        }
#endif
    }
    SECTION("is_equal_with_table_t") {
        auto expression = is_equal<User>(std::string("Claude"));
        expected.push_back(typeid(std::string));
        iterate_ast(expression, lambda);
    }
    SECTION("aliased regular column") {
        using als = alias_z<User>;
        auto expression = alias_column<als>(&User::id);
        expected.push_back(typeid(alias_column_t<alias_z<User>, decltype(&User::id)>));
        iterate_ast(expression, lambda);
    }
    SECTION("aliased regular column pointer") {
        using als = alias_z<User>;
        auto expression = alias_column<als>(column<User>(&User::id));
        expected.push_back(typeid(alias_column_t<alias_z<User>, column_pointer<User, decltype(&User::id)>>));
        iterate_ast(expression, lambda);
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("aliased regular column 2") {
        constexpr auto z_alias = "z"_alias.for_<User>();
        auto expression = z_alias->*&User::id;
        expected.push_back(typeid(alias_column_t<alias_z<User>, decltype(&User::id)>));
        iterate_ast(expression, lambda);
    }
    SECTION("aliased regular column pointer 2") {
        constexpr auto z_alias = "z"_alias.for_<User>();
        auto expression = z_alias->*column<User>(&User::id);
        expected.push_back(typeid(alias_column_t<alias_z<User>, column_pointer<User, decltype(&User::id)>>));
        iterate_ast(expression, lambda);
    }
#endif
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    SECTION("with ordinary") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with(cte<cte_1>().as(select(1)), select(column<cte_1>(1_colalias)));
        expected.insert(expected.cend(), {typeid(int), typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>)});
        iterate_ast(expression, lambda);
    }
    SECTION("with not enforced recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with_recursive(cte<cte_1>().as(select(1)), select(column<cte_1>(1_colalias)));
        expected.insert(expected.cend(), {typeid(int), typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>)});
        iterate_ast(expression, lambda);
    }
    SECTION("with optional recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with(
            cte<cte_1>().as(
                union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
            select(column<cte_1>(1_colalias)));
        expected.insert(expected.cend(),
                        {typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>),
                         typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>),
                         typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>)});
        iterate_ast(expression, lambda);
    }
    SECTION("with recursive") {
        using cte_1 = decltype(1_ctealias);
        auto expression = with_recursive(
            cte<cte_1>().as(
                union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
            select(column<cte_1>(1_colalias)));
        expected.insert(expected.cend(),
                        {typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>),
                         typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>),
                         typeid(int),
                         typeid(column_pointer<cte_1, alias_holder<column_alias<'1'>>>)});
        iterate_ast(expression, lambda);
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("aliased CTE column pointer") {
        constexpr auto c = "1"_cte;
        using cte_1 = decltype("1"_cte);
        constexpr auto z_alias = "z"_alias.for_<c>();
        auto expression = z_alias->*&User::id;
        expected.push_back(typeid(alias_column_t<alias_z<cte_1>, column_pointer<cte_1, decltype(&User::id)>>));
        iterate_ast(expression, lambda);
    }
    SECTION("aliased CTE column alias") {
        constexpr auto c = "1"_cte;
        using cte_1 = decltype("1"_cte);
        constexpr auto z_alias = "z"_alias.for_<c>();
        auto expression = z_alias->*1_colalias;
        expected.push_back(
            typeid(alias_column_t<alias_z<cte_1>, column_pointer<cte_1, alias_holder<column_alias<'1'>>>>));
        iterate_ast(expression, lambda);
    }
#endif
#endif
    SECTION("highlight") {
        auto expression = highlight<User>(0, std::string("<b>"), std::string("</b>"));
        expected.push_back(typeid(expression));
        expected.push_back(typeid(int));
        expected.push_back(typeid(std::string));
        expected.push_back(typeid(std::string));
        iterate_ast(expression, lambda);
    }
    REQUIRE(typeIndexes == expected);
}
