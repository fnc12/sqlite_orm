/**
 *  Examples partially from https://sqlite.org/lang_with.html
 */

#include <sqlite_orm/sqlite_orm.h>
#ifdef SQLITE_ORM_WITH_CTE
#define ENABLE_THIS_EXAMPLE
#endif

#ifdef ENABLE_THIS_EXAMPLE
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>
#endif
#include <tuple>
#include <iostream>
#include <iomanip>
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <ctime>
#endif

using std::cout;
using std::endl;
using std::make_tuple;
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
using std::nullopt;
#endif
using std::string;
using std::system_error;
using namespace sqlite_orm;

void all_integers_between(int from, int end) {
    auto storage = make_storage("");
    // variant 1, where-clause, implicitly numbered column
    {
        //WITH RECURSIVE
        //    cnt(x) AS(VALUES(1) UNION ALL SELECT x + 1 FROM cnt WHERE x < 1000000)
        //    SELECT x FROM cnt;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto cnt = "cnt"_cte;
        auto ast = with_recursive(
            cnt().as(union_all(select(from), select(cnt->*1_colalias + 1, where(cnt->*1_colalias < end)))),
            select(cnt->*1_colalias));
#else
        using cnt = decltype(1_ctealias);
        auto ast = with_recursive(
            cte<cnt>().as(
                union_all(select(from), select(column<cnt>(1_colalias) + 1, where(column<cnt>(1_colalias) < end)))),
            select(column<cnt>(1_colalias)));
#endif

        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        cout << "Integer range (where-clause) [" << from << ", " << end
             << "]"
                "\n";
        for(int n: storage.execute(stmt)) {
            cout << n << ", ";
        }
        cout << endl;
    }

    // variant 2, limit-clause, implicit column
    {
        //WITH RECURSIVE
        //    cnt(x) AS(
        //        SELECT 1
        //        UNION ALL
        //        SELECT x + 1 FROM cnt
        //        LIMIT 1000000
        //    )
        //    SELECT x FROM cnt;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto cnt = "cnt"_cte;
        constexpr auto x = "x"_col;
        auto ast =
            with_recursive(cnt().as(union_all(select(from >>= x), select(cnt->*x + 1, limit(end)))), select(cnt->*x));
#else
        using cnt = decltype(1_ctealias);
        constexpr auto x = colalias_i{};
        auto ast = with_recursive(cte<cnt>().as(union_all(select(from >>= x), select(column<cnt>(x) + 1, limit(end)))),
                                  select(column<cnt>(x)));
#endif

        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        cout << "Integer range (limit-clause) [" << from << ", " << end
             << "]"
                "\n";
        for(int n: storage.execute(stmt)) {
            cout << n << ", ";
        }
        cout << endl;
    }

    // variant 3, limit-clause, explicit column spec
    {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto cnt = "cnt"_cte;
        constexpr auto x = "x"_col;
        auto ast = with_recursive(cnt(x).as(union_all(select(from), select(cnt->*x + 1, limit(end)))), select(cnt->*x));
#else
        using cnt = decltype(1_ctealias);
        constexpr auto x = colalias_i{};
        auto ast = with_recursive(cte<cnt>(x).as(union_all(select(from), select(column<cnt>(x) + 1, limit(end)))),
                                  select(column<cnt>(x)));
#endif

        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        cout << "Integer range (limit-clause) [" << from << ", " << end
             << "]"
                "\n";
        for(int n: storage.execute(stmt)) {
            cout << n << ", ";
        }
        cout << endl;
    }

    cout << endl;
}

void supervisor_chain() {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    //CREATE TABLE org(
    //    name TEXT PRIMARY KEY,
    //    boss TEXT REFERENCES org
    //) WITHOUT ROWID;
    //INSERT INTO org VALUES('Alice', NULL);
    //INSERT INTO org VALUES('Bob', 'Alice');
    //INSERT INTO org VALUES('Cindy', 'Alice');
    //INSERT INTO org VALUES('Dave', 'Bob');
    //INSERT INTO org VALUES('Emma', 'Bob');
    //INSERT INTO org VALUES('Fred', 'Cindy');
    //INSERT INTO org VALUES('Gail', 'Cindy');
    struct Org {
        std::string name;
        std::optional<std::string> boss;
    };

    auto storage = make_storage("",
                                make_table<Org>("org",
                                                make_column("name", &Org::name, primary_key()),
                                                make_column("boss", &Org::boss),
                                                foreign_key(&Org::boss).references(&Org::name)));
    storage.sync_schema();

    storage.replace<Org>({"Alice", nullopt});
    storage.replace<Org>({"Bob", "Alice"});
    storage.replace<Org>({"Cindy", "Alice"});
    storage.replace<Org>({"Dave", "Bob"});
    storage.replace<Org>({"Emma", "Bob"});
    storage.replace<Org>({"Fred", "Cindy"});
    storage.replace<Org>({"Gail", "Cindy"});

    // supervisor chain of Fred
    {
        //WITH RECURSIVE
        //    chain AS(
        //        SELECT * from org WHERE name = 'Fred'
        //        UNION ALL
        //        SELECT parent.* FROM org parent, chain
        //        WHERE parent.name = chain.boss
        //    )
        //    SELECT name FROM chain;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto chain = "chain"_cte;
        constexpr auto parent = "parent"_alias.for_<Org>();
        auto ast = with_recursive(
            chain().as(union_all(select(asterisk<Org>(), where(&Org::name == c("Fred"))),
                                 select(asterisk<parent>(), where(parent->*&Org::name == chain->*&Org::boss)))),
            select(chain->*&Org::name));
#else
        using chain = decltype(1_ctealias);
        auto ast = with_recursive(cte<chain>().as(union_all(select(asterisk<Org>(), where(&Org::name == c("Fred"))),
                                                            select(asterisk<alias_a<Org>>(),
                                                                   where(alias_column<alias_a<Org>>(&Org::name) ==
                                                                         column<chain>(&Org::boss))))),
                                  select(column<chain>(&Org::name)));
#endif
        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        auto results = storage.execute(stmt);
        cout << "Hierarchy chain of Fred:\n";
        for(const string& name: results) {
            cout << name << ", ";
        }
        cout << endl;
    }
    cout << endl;
#endif
}

void works_for_alice() {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    //CREATE TABLE org(
    //  name TEXT PRIMARY KEY,
    //  boss TEXT REFERENCES org,
    //  height INT,
    //  -- other content omitted
    //);
    struct Org {
        std::string name;
        std::optional<std::string> boss;
        double height;
    };

    auto storage = make_storage("",
                                make_table<Org>("org",
                                                make_column("name", &Org::name, primary_key()),
                                                make_column("boss", &Org::boss),
                                                make_column("height", &Org::height),
                                                foreign_key(&Org::boss).references(&Org::name)));
    storage.sync_schema();

    storage.replace<Org>({"Alice", nullopt, 160.});
    storage.replace<Org>({"Bob", nullopt, 177.});
    storage.replace<Org>({"Dave", "Alice", 169.});
    storage.replace<Org>({"Cindy", "Dave", 165.});
    storage.replace<Org>({"Bar", "Bob", 159.});

    // average height of Alice's team
    {
        //WITH RECURSIVE
        //    works_for_alice(n) AS(
        //        VALUES('Alice')
        //        UNION
        //        SELECT name FROM org, works_for_alice
        //        WHERE org.boss = works_for_alice.n
        //    )
        //    SELECT avg(height) FROM org
        //    WHERE org.name IN works_for_alice;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto works_for_alice = "works_for_alice"_cte;
        auto ast = with_recursive(
            works_for_alice(&Org::name)
                .as(union_(select("Alice"), select(&Org::name, where(&Org::boss == works_for_alice->*&Org::name)))),
            select(avg(&Org::height), from<Org>(), where(in(&Org::name, select(works_for_alice->*&Org::name)))));
#else
        using works_for_alice = decltype(1_ctealias);
        auto ast = with_recursive(
            cte<works_for_alice>(&Org::name)
                .as(union_(select("Alice"),
                           select(&Org::name, where(&Org::boss == column<works_for_alice>(&Org::name))))),
            select(avg(&Org::height), from<Org>(), where(in(&Org::name, select(column<works_for_alice>(&Org::name))))));
#endif

        //WITH cte_1("n")
        //    AS(
        //        SELECT 'Alice'
        //        UNION
        //        SELECT "org"."name" FROM 'cte_1', 'org'
        //        WHERE("org"."boss" = 'cte_1'."n")
        //    )
        //    SELECT AVG("org"."height")
        //    FROM 'org'
        //    WHERE(
        //        "name" IN(
        //            SELECT 'cte_1'."n" FROM 'cte_1'
        //        )
        //    )
        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        auto results = storage.execute(stmt);
        cout << "Average height of Alice's team: " << results.at(0) << endl;
    }
    cout << endl;
#endif
}

void family_tree() {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    //CREATE TABLE family(
    //    name TEXT PRIMARY KEY,
    //    mom TEXT REFERENCES family,
    //    dad TEXT REFERENCES family,
    //    born DATETIME,
    //    died DATETIME, --NULL if still alive
    //    -- other content
    //);
    struct Family {
        std::string name;
        std::optional<std::string> mom;
        std::optional<std::string> dad;
        time_t born;
        std::optional<time_t> died;
    };

    auto storage = make_storage("",
                                make_table<Family>("family",
                                                   make_column("name", &Family::name, primary_key()),
                                                   make_column("mom", &Family::mom),
                                                   make_column("dad", &Family::dad),
                                                   make_column("born", &Family::born),
                                                   make_column("died", &Family::died),
                                                   foreign_key(&Family::mom).references(&Family::name),
                                                   foreign_key(&Family::dad).references(&Family::name)));
    storage.sync_schema();

    storage.replace<Family>({"Grandma (Mom)", nullopt, nullopt, 0, nullopt});
    storage.replace<Family>({"Granddad (Mom)", nullopt, nullopt, 1, 1});
    storage.replace<Family>({"Grandma (Dad)", nullopt, nullopt, 0, 0});
    storage.replace<Family>({"Granddad (Dad)", nullopt, nullopt, 1, nullopt});
    storage.replace<Family>({"Mom", "Grandma (Mom)", "Granddad (Mom)", 2, nullopt});
    storage.replace<Family>({"Dad", "Grandma (Dad)", "Granddad (Dad)", 3, nullopt});
    storage.replace<Family>({"Alice", "Mom", "Dad", 4, nullopt});

    //WITH RECURSIVE
    //    parent_of(name, parent) AS
    //    (SELECT name, mom FROM family UNION SELECT name, dad FROM family),
    //    ancestor_of_alice(name) AS
    //    (SELECT parent FROM parent_of WHERE name = 'Alice'
    //        UNION ALL
    //        SELECT parent FROM parent_of JOIN ancestor_of_alice USING(name))
    //    SELECT family.name FROM ancestor_of_alice, family
    //    WHERE ancestor_of_alice.name = family.name
    //    AND died IS NULL
    //    ORDER BY born;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto parent_of = "parent_of"_cte;
    constexpr auto ancestor_of_alice = "ancestor_of_alice"_cte;
    constexpr auto parent = "parent"_col;
    constexpr auto name = "name"_col;
    auto ast = with_recursive(
        make_tuple(
            parent_of(&Family::name, parent)
                .as(union_(select(columns(&Family::name, &Family::mom)), select(columns(&Family::name, &Family::dad)))),
            ancestor_of_alice(name).as(
                union_all(select(parent_of->*parent, where(parent_of->*&Family::name == "Alice")),
                          select(parent_of->*parent, join<ancestor_of_alice>(using_(parent_of->*&Family::name)))))),
        select(&Family::name,
               where(ancestor_of_alice->*name == &Family::name && is_null(&Family::died)),
               order_by(&Family::born)));
#else
    using parent_of = decltype(1_ctealias);
    using ancestor_of_alice = decltype(2_ctealias);
    constexpr auto parent = colalias_h{};
    constexpr auto name = colalias_f{};
    auto ast = with_recursive(
        make_tuple(
            cte<parent_of>("name", "parent")
                .as(union_(select(columns(&Family::name, &Family::mom >>= parent)),
                           select(columns(&Family::name, &Family::dad)))),
            cte<ancestor_of_alice>("name").as(union_all(
                select(column<parent_of>(parent) >>= name, where(column<parent_of>(&Family::name) == "Alice")),
                select(column<parent_of>(parent), join<ancestor_of_alice>(using_(column<parent_of>(&Family::name))))))),
        select(&Family::name,
               where(column<ancestor_of_alice>(name) == &Family::name && is_null(&Family::died)),
               order_by(&Family::born)));
#endif

    //WITH cte_1("name", "parent")
    //    AS(
    //        SELECT "family"."name", "family"."mom"
    //        FROM 'family'
    //        UNION
    //        SELECT "family"."name", "family"."dad"
    //        FROM 'family'
    //    ),
    //    cte_2("name")
    //    AS(
    //        SELECT 'cte_1'."parent"
    //        FROM 'cte_1'
    //        WHERE('cte_1'."name" = 'Alice')
    //        UNION ALL
    //        SELECT 'cte_1'."name"
    //        FROM 'cte_1'
    //        JOIN 'cte_2' USING("name")
    //    )
    //    SELECT "family"."name"
    //    FROM 'cte_2',
    //    'family'
    //    WHERE(
    //        (
    //            ('cte_2'."name" = 'family'."name")
    //            AND "family"."died" IS NULL
    //            )
    //    )
    //    ORDER BY "family"."born"
    string sql = storage.dump(ast);

    auto stmt = storage.prepare(ast);
    auto results = storage.execute(stmt);

    cout << "Living ancestor's of Alice:\n";
    for(const string& name: results) {
        cout << name << '\n';
    }
    cout << endl;
#endif
}

void depth_or_breadth_first() {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    //CREATE TABLE org(
    //    name TEXT PRIMARY KEY,
    //    boss TEXT REFERENCES org
    //) WITHOUT ROWID;
    //INSERT INTO org VALUES('Alice', NULL);
    //INSERT INTO org VALUES('Bob', 'Alice');
    //INSERT INTO org VALUES('Cindy', 'Alice');
    //INSERT INTO org VALUES('Dave', 'Bob');
    //INSERT INTO org VALUES('Emma', 'Bob');
    //INSERT INTO org VALUES('Fred', 'Cindy');
    //INSERT INTO org VALUES('Gail', 'Cindy');
    //CREATE TABLE org(
    //  name TEXT PRIMARY KEY,
    //  boss TEXT REFERENCES org,
    //  height INT,
    //  -- other content omitted
    //);
    struct Org {
        std::string name;
        std::optional<std::string> boss;
        double height;
    };

    auto storage = make_storage("",
                                make_table<Org>("org",
                                                make_column("name", &Org::name, primary_key()),
                                                make_column("boss", &Org::boss),
                                                foreign_key(&Org::boss).references(&Org::name)));
    storage.sync_schema();

    storage.replace<Org>({"Alice", nullopt});
    storage.replace<Org>({"Bob", "Alice"});
    storage.replace<Org>({"Cindy", "Alice"});
    storage.replace<Org>({"Dave", "Bob"});
    storage.replace<Org>({"Emma", "Bob"});
    storage.replace<Org>({"Fred", "Cindy"});
    storage.replace<Org>({"Gail", "Cindy"});

    // breadth-first pattern
    {
        //WITH RECURSIVE
        //    under_alice(name, level) AS(
        //        VALUES('Alice', 0)
        //        UNION ALL
        //        SELECT org.name, under_alice.level + 1
        //        FROM org JOIN under_alice ON org.boss = under_alice.name
        //        ORDER BY 2
        //    )
        //    SELECT substr('..........', 1, level * 3) || name FROM under_alice;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto under_alice = "under_alice"_cte;
        constexpr auto level = "level"_col;
        auto ast =
            with_recursive(under_alice(&Org::name, level)
                               .as(union_all(select(columns("Alice", 0)),
                                             select(columns(&Org::name, under_alice->*level + 1),
                                                    join<under_alice>(on(under_alice->*&Org::name == &Org::boss)),
                                                    order_by(2)))),
                           select(substr("..........", 1, under_alice->*level * 3) || under_alice->*&Org::name));
#else
        using under_alice = decltype(1_ctealias);
        auto ast = with_recursive(
            cte<under_alice>("name", "level")
                .as(union_all(select(columns("Alice", 0)),
                              select(columns(&Org::name, column<under_alice>(2_colalias) + 1),
                                     join<under_alice>(on(column<under_alice>(1_colalias) == &Org::boss)),
                                     order_by(2)))),
            select(substr("..........", 1, column<under_alice>(2_colalias) * 3) || column<under_alice>(1_colalias)));
#endif

        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        auto results = storage.execute(stmt);

        cout << "List of organization members, breadth-first:\n";
        for(const string& name: results) {
            cout << name << '\n';
        }
    }
    cout << endl;

    // depth-first pattern
    {
        //WITH RECURSIVE
        //    under_alice(name, level) AS(
        //        VALUES('Alice', 0)
        //        UNION ALL
        //        SELECT org.name, under_alice.level + 1
        //        FROM org JOIN under_alice ON org.boss = under_alice.name
        //        ORDER BY 2
        //    )
        //    SELECT substr('..........', 1, level * 3) || name FROM under_alice;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto under_alice = "under_alice"_cte;
        constexpr auto level = "level"_col;
        auto ast =
            with_recursive(under_alice(&Org::name, level)
                               .as(union_all(select(columns("Alice", 0)),
                                             select(columns(&Org::name, under_alice->*level + 1),
                                                    join<under_alice>(on(under_alice->*&Org::name == &Org::boss)),
                                                    order_by(2).desc()))),
                           select(substr("..........", 1, under_alice->*level * 3) || under_alice->*&Org::name));
#else
        using under_alice = decltype(1_ctealias);
        auto ast = with_recursive(
            cte<under_alice>("name", "level")
                .as(union_all(select(columns("Alice", 0)),
                              select(columns(&Org::name, column<under_alice>(2_colalias) + 1),
                                     join<under_alice>(on(column<under_alice>(1_colalias) == &Org::boss)),
                                     order_by(2).desc()))),
            select(substr("..........", 1, column<under_alice>(2_colalias) * 3) || column<under_alice>(1_colalias)));
#endif

        string sql = storage.dump(ast);

        auto stmt = storage.prepare(ast);
        auto results = storage.execute(stmt);

        cout << "List of organization members, depth-first:\n";
        for(const string& name: results) {
            cout << name << '\n';
        }
    }
    cout << endl;
#endif
}

void select_from_subselect() {
    struct Employee {
        int m_empno;
        std::string m_ename;
        double m_salary;
        std::optional<double> m_commission;
    };

    auto storage = make_storage("",
                                make_table("Emp",
                                           make_column("empno", &Employee::m_empno, primary_key().autoincrement()),
                                           make_column("ename", &Employee::m_ename),
                                           make_column("salary", &Employee::m_salary),
                                           make_column("comm", &Employee::m_commission)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        storage.insert<Employee>({1, "Patel", 4000, nullopt});
        storage.insert<Employee>({2, "Jariwala", 19300, nullopt});
        return true;
    });

    // alternative way of writing a subselect by using a CTE.
    //
    // original select from subquery:
    // SELECT * FROM (SELECT salary, comm AS commmission FROM emp) WHERE salary < 5000
    //
    // with CTE:
    // WITH
    //     sub AS(
    //         SELECT salary, comm AS commmission FROM emp) WHERE salary < 5000
    //     )
    //  SELECT * from sub;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto sub = "sub"_cte;
    auto expression = with(sub().as(select(columns(&Employee::m_salary, &Employee::m_commission))),
                           select(asterisk<sub>(), where(sub->*&Employee::m_salary < 5000)));
#else
    using sub = decltype(1_ctealias);
    auto expression = with(cte<sub>().as(select(columns(&Employee::m_salary, &Employee::m_commission))),
                           select(asterisk<sub>(), where(column<sub>(&Employee::m_salary) < 5000)));
#endif

    string sql = storage.dump(expression);

    auto stmt = storage.prepare(expression);
    auto results = storage.execute(stmt);

    cout << "List of employees with a salary less than 5000:\n";
    for(auto& result: results) {
        cout << get<0>(result) << '\n';
    }
    cout << endl;
}

void apfelmaennchen() {
    auto storage = make_storage("");

    //WITH RECURSIVE
    //    xaxis(x) AS(VALUES(-2.0) UNION ALL SELECT x + 0.05 FROM xaxis WHERE x < 1.2),
    //    yaxis(y) AS(VALUES(-1.0) UNION ALL SELECT y + 0.1 FROM yaxis WHERE y < 1.0),
    //    m(iter, cx, cy, x, y) AS(
    //        SELECT 0, x, y, 0.0, 0.0 FROM xaxis, yaxis
    //        UNION ALL
    //        SELECT iter + 1, cx, cy, x * x - y * y + cx, 2.0 * x * y + cy FROM m
    //        WHERE(x * x + y * y) < 4.0 AND iter < 28
    //    ),
    //    m2(iter, cx, cy) AS(
    //        SELECT max(iter), cx, cy FROM m GROUP BY cx, cy
    //    ),
    //    a(t) AS(
    //        SELECT group_concat(substr(' .+*#', 1 + min(iter / 7, 4), 1), '')
    //        FROM m2 GROUP BY cy
    //    )
    //    SELECT group_concat(rtrim(t), x'0a') FROM a;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto xaxis = "xaxis"_cte;
    constexpr auto yaxis = "yaxis"_cte;
    constexpr auto m = "m"_cte;
    constexpr auto m2 = "m2"_cte;
    constexpr auto a = "string"_cte;
    constexpr auto x = "x"_col;
    constexpr auto y = "y"_col;
    constexpr auto iter = "iter"_col;
    constexpr auto cx = "cx"_col;
    constexpr auto cy = "cy"_col;
    constexpr auto t = "t"_col;
    auto ast = with_recursive(
        make_tuple(
            xaxis(x).as(union_all(select(-2.0), select(xaxis->*x + 0.05, where(xaxis->*x < 1.2)))),
            yaxis(y).as(union_all(select(-1.0), select(yaxis->*y + 0.10, where(yaxis->*y < 1.0)))),
            m(iter, cx, cy, x, y)
                .as(union_all(select(columns(0, xaxis->*x, yaxis->*y, 0.0, 0.0)),
                              select(columns(m->*iter + 1,
                                             m->*cx,
                                             m->*cy,
                                             m->*x * m->*x - m->*y * m->*y + m->*cx,
                                             2.0 * m->*x * m->*y + m->*cy),
                                     where((m->*x * m->*x + m->*y * m->*y) < 4.0 && m->*iter < 28)))),
            m2(iter, cx, cy).as(select(columns(max<>(m->*iter), m->*cx, m->*cy), group_by(m->*cx, m->*cy))),
            a(t).as(select(group_concat(substr(" .+*#", 1 + min<>(m2->*iter / 7.0, 4.0), 1), ""), group_by(m2->*cy)))),
        select(group_concat(rtrim(a->*t), "\n")));
#else
    using cte_xaxis = decltype(1_ctealias);
    using cte_yaxis = decltype(2_ctealias);
    using cte_m = decltype(3_ctealias);
    using cte_m2 = decltype(4_ctealias);
    using cte_a = decltype(5_ctealias);
    constexpr auto x = colalias_a{};
    constexpr auto y = colalias_b{};
    constexpr auto iter = colalias_c{};
    constexpr auto cx = colalias_d{};
    constexpr auto cy = colalias_e{};
    constexpr auto t = colalias_f{};
    auto ast = with_recursive(
        make_tuple(
            cte<cte_xaxis>("x").as(
                union_all(select(-2.0 >>= x), select(column<cte_xaxis>(x) + 0.05, where(column<cte_xaxis>(x) < 1.2)))),
            cte<cte_yaxis>("y").as(
                union_all(select(-1.0 >>= y), select(column<cte_yaxis>(y) + 0.10, where(column<cte_yaxis>(y) < 1.0)))),
            cte<cte_m>("iter", "cx", "cy", "x", "y")
                .as(union_all(
                    select(columns(0 >>= iter,
                                   column<cte_xaxis>(x) >>= cx,
                                   column<cte_yaxis>(y) >>= cy,
                                   0.0 >>= x,
                                   0.0 >>= y)),
                    select(columns(column<cte_m>(iter) + 1,
                                   column<cte_m>(cx),
                                   column<cte_m>(cy),
                                   column<cte_m>(x) * column<cte_m>(x) - column<cte_m>(y) * column<cte_m>(y) +
                                       column<cte_m>(cx),
                                   2.0 * column<cte_m>(x) * column<cte_m>(y) + column<cte_m>(cy)),
                           where((column<cte_m>(x) * column<cte_m>(x) + column<cte_m>(y) * column<cte_m>(y)) < 4.0 &&
                                 column<cte_m>(iter) < 28)))),
            cte<cte_m2>("iter", "cx", "cy")
                .as(select(columns(max<>(column<cte_m>(iter)) >>= iter, column<cte_m>(cx), column<cte_m>(cy)),
                           group_by(column<cte_m>(cx), column<cte_m>(cy)))),
            cte<cte_a>("t").as(
                select(group_concat(substr(" .+*#", 1 + min<>(column<cte_m2>(iter) / 7.0, 4.0), 1), "") >>= t,
                       group_by(column<cte_m2>(cy))))),
        select(group_concat(rtrim(column<cte_a>(t)), "\n")));
#endif

    string sql = storage.dump(ast);

    auto stmt = storage.prepare(ast);
    auto results = storage.execute(stmt);

    cout << "Apfelmaennchen (Mandelbrot set):\n";
    for(const string& rowString: results) {
        cout << rowString << '\n';
    }
    cout << endl;
}

void sudoku() {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    auto storage = make_storage("");

    //WITH RECURSIVE
    //    input(sud) AS(
    //        VALUES('53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79')
    //    ),
    //    digits(z, lp) AS(
    //        VALUES('1', 1)
    //        UNION ALL SELECT
    //        CAST(lp + 1 AS TEXT), lp + 1 FROM digits WHERE lp < 9
    //    ),
    //    x(s, ind) AS(
    //        SELECT sud, instr(sud, '.') FROM input
    //        UNION ALL
    //        SELECT
    //        substr(s, 1, ind - 1) || z || substr(s, ind + 1),
    //        instr(substr(s, 1, ind - 1) || z || substr(s, ind + 1), '.')
    //        FROM x, digits AS z
    //        WHERE ind > 0
    //        AND NOT EXISTS(
    //            SELECT 1
    //            FROM digits AS lp
    //            WHERE z.z = substr(s, ((ind - 1) / 9) * 9 + lp, 1)
    //            OR z.z = substr(s, ((ind - 1) % 9) + (lp - 1) * 9 + 1, 1)
    //            OR z.z = substr(s, (((ind - 1) / 3) % 3) * 3
    //                + ((ind - 1) / 27) * 27 + lp
    //                + ((lp - 1) / 3) * 6, 1)
    //        )
    //    )
    //    SELECT s FROM x WHERE ind = 0;

    constexpr auto input = "input"_cte;
    constexpr auto digits = "digits"_cte;
    constexpr auto z_alias = "z"_alias.for_<digits>();
    constexpr auto x = "x"_cte;
    constexpr auto sud = "sud"_col;
    constexpr auto z = "z"_col;
    constexpr auto lp = "lp"_col;
    constexpr auto s = "s"_col;
    constexpr auto ind = "ind"_col;
    auto ast = with_recursive(
        make_tuple(
            cte<input>(sud).as(
                select("53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79")),
            cte<digits>(z, lp).as(
                union_all(select(columns("1", 1)),
                          select(columns(cast<string>(digits->*lp + 1), digits->*lp + 1), where(digits->*lp < 9)))),
            cte<x>(s, ind).as(union_all(
                select(columns(input->*sud, instr(input->*sud, "."))),
                select(columns(substr(x->*s, 1, x->*ind - 1) || z || substr(x->*s, x->*ind + 1),
                               instr(substr(x->*s, 1, x->*ind - 1) || z || substr(x->*s, x->*ind + 1), ".")),
                       where(x->*ind > 0 and
                             not exists(select(
                                 1 >>= lp,
                                 from<digits>(),
                                 where(z_alias->*z == substr(x->*s, ((x->*ind - 1) / 9) * 9 + lp, 1) or
                                       z_alias->*z == substr(x->*s, ((x->*ind - 1) % 9) + (lp - 1) * 9 + 1, 1) or
                                       z_alias->*z == substr(x->*s,
                                                             (((x->*ind - 1) / 3) % 3) * 3 + ((x->*ind - 1) / 27) * 27 +
                                                                 lp + ((lp - 1) / 3) * 6,
                                                             1))))))))),
        select(x->*s, where(x->*ind == 0)));

    string sql = storage.dump(ast);

    auto stmt = storage.prepare(ast);
    auto results = storage.execute(stmt);

    cout << "Sudoku solution:\n";
    for(const string& answer: results) {
        cout << answer << '\n';
    }
    cout << endl;
#endif
}

void show_optimization_fence() {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    auto storage = make_storage("");

    {
        //WITH
        //    cnt(x) AS MATERIALIZED(VALUES(1))
        //    SELECT x FROM cnt;
        constexpr auto cnt = "cnt"_cte;
        auto ast = with(cnt().as<materialized()>(select(1)), select(cnt->*1_colalias));

        [[maybe_unused]] string sql = storage.dump(ast);

        [[maybe_unused]] auto stmt = storage.prepare(ast);
    }

    {
        //WITH
        //    cnt(x) AS NOT MATERIALIZED(VALUES(1))
        //    SELECT x FROM cnt;
        constexpr auto cnt = "cnt"_cte;
        auto ast = with(cnt().as<not_materialized()>(select(1)), select(cnt->*1_colalias));

        [[maybe_unused]] string sql = storage.dump(ast);

        [[maybe_unused]] auto stmt = storage.prepare(ast);
    }
#endif
}

void show_mapping_and_backreferencing() {
    struct Object {
        int64 id = 0;
    };

    // column alias
    struct cnt : alias_tag {
        static constexpr std::string_view get() {
            return "counter";
        }
    };
    using cte_1 = decltype(1_ctealias);

    auto storage = make_storage("", make_table("object", make_column("id", &Object::id)));
    storage.sync_schema();

    // back-reference via `column_pointer<cte_1, F O::*>`;
    // WITH "1"("id") AS (SELECT "object"."id" FROM "object") SELECT "1"."id" FROM "1"
    {
        auto ast = with(cte<cte_1>().as(select(&Object::id)), select(column<cte_1>(&Object::id)));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // map column via alias_holder into cte,
    // back-reference via `column_pointer<cte_1, alias_holder>`;
    // WITH "1"("x") AS (SELECT 1 AS "counter" UNION ALL SELECT "1"."x" + 1 FROM "1" LIMIT 10) SELECT "1"."x" FROM "1"
    {
        auto ast =
            with(cte<cte_1>("x").as(union_all(select(as<cnt>(1)), select(column<cte_1>(get<cnt>()) + 1, limit(10)))),
                 select(column<cte_1>(get<cnt>())));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }
    // map column via alias_holder into cte,
    // back-reference via `column_pointer<cte_1, alias_holder>`;
    // WITH "1"("x") AS (SELECT 1 AS "counter" UNION ALL SELECT "1"."x" + 1 FROM "1" LIMIT 10) SELECT "1"."x" FROM "1"
    {
        auto ast = with(cte<cte_1>("x").as(union_all(select(as<cnt>(1)), select(column<cte_1>(cnt{}) + 1, limit(10)))),
                        select(column<cte_1>(cnt{})));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // implicitly remap column into cte;
    // WITH "1"("id") AS (SELECT "object"."id" FROM "object") SELECT "1"."id" FROM "1"
    {
        auto ast = with(cte<cte_1>(std::ignore).as(select(&Object::id)), select(column<cte_1>(&Object::id)));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly remap column into cte (independent of subselect);
    // WITH "1"("id") AS (SELECT "object"."id" FROM "object") SELECT "1"."id" FROM "1"
    {
        auto ast = with(cte<cte_1>(&Object::id).as(select(&Object::id)), select(column<cte_1>(&Object::id)));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly remap column as an alias into cte (independent of subselect);
    // WITH "1"("counter") AS (SELECT "object"."id" FROM "object") SELECT "1"."counter" FROM "1"
    {
        auto ast = with(cte<cte_1>(cnt{}).as(select(&Object::id)), select(column<cte_1>(get<cnt>())));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly state that column name should be taken from subselect;
    // WITH "CTEObj"("xyz") AS (SELECT "object"."id" FROM "object") SELECT "CTEObj"."xyz" FROM "CTEObj"
    {
        struct CTEObject : alias_tag {
            // a CTE object is its own table alias
            using type = CTEObject;

            static std::string get() {
                return "CTEObj";
            }

            int64 xyz = 0;
        };
        auto ast =
            with(cte<CTEObject>(make_column("xyz", &CTEObject::xyz)).as(select(&Object::id)), select(&CTEObject::xyz));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }
}

void neevek_issue_222() {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    //WITH
    //    register_user AS(
    //        SELECT
    //        uid, MIN(DATE(user_activity.timestamp)) register_date
    //        FROM user_activity
    //        GROUP BY uid
    //        HAVING register_date >= DATE('now', '-7 days')
    //    ),
    //    register_user_count AS(
    //        SELECT
    //        R.register_date,
    //        COUNT(DISTINCT R.uid) AS user_count
    //        FROM register_user R
    //        GROUP BY R.register_date
    //    )
    //    SELECT
    //    R.register_date,
    //    CAST(julianday(DATE(A.timestamp)) AS INT) - CAST(julianday(R.register_date) AS INT) AS ndays,
    //    COUNT(DISTINCT A.uid) AS retention,
    //    C.user_count
    //    FROM user_activity A
    //    LEFT JOIN register_user R ON A.uid = R.uid
    //    LEFT JOIN register_user_count C ON R.register_date = C.register_date
    //    GROUP BY R.register_date, ndays
    //    HAVING DATE(A.timestamp) >= DATE('now', '-7 days');

    struct user_activity {
        int64 id;
        int64 uid;
        time_t timestamp;
    };

    auto storage = make_storage("",
                                make_table("user_activity",
                                           make_column("id", &user_activity::id, primary_key().autoincrement()),
                                           make_column("uid", &user_activity::uid),
                                           make_column("timestamp", &user_activity::timestamp)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        time_t now = std::time(nullptr);
        auto values = {user_activity{0, 1, now - 86400 * 3},
                       user_activity{0, 1, now - 86400 * 2},
                       user_activity{0, 1, now},
                       user_activity{0, 2, now}};
        storage.insert_range(values.begin(), values.end());
        return true;
    });

    constexpr auto register_user = "register_user"_cte;
    constexpr auto registered_cnt = "registered_cnt"_cte;
    constexpr auto register_date = "register_date"_col;
    constexpr auto user_count = "user_count"_col;
    constexpr auto ndays = "ndays"_col;
    auto expression = with(
        make_tuple(
            register_user().as(select(
                columns(&user_activity::uid, min(date(&user_activity::timestamp, "unixepoch")) >>= register_date),
                group_by(&user_activity::uid).having(greater_or_equal(register_date, date("now", "-7 days"))))),
            registered_cnt().as(select(columns(register_user->*register_date,
                                               count(distinct(register_user->*&user_activity::uid)) >>= user_count),
                                       group_by(register_user->*register_date)))),
        select(columns(register_user->*register_date,
                       c(cast<int>(julianday(date(&user_activity::timestamp, "unixepoch")))) -
                           cast<int>(julianday(register_user->*register_date)) >>= ndays,
                       count(distinct(&user_activity::uid)) >>= "retention"_col,
                       registered_cnt->*user_count),
               left_join<register_user>(using_(&user_activity::uid)),
               left_join<registered_cnt>(using_(registered_cnt->*register_date)),
               group_by(register_user->*register_date, ndays)
                   .having(date(&user_activity::timestamp, "unixepoch") >= date("now", "-7 days"))));

    string sql = storage.dump(expression);

    auto stmt = storage.prepare(expression);
    auto results = storage.execute(stmt);

    cout << "User Retention:\n";
    for(const char* colName: {"register_date", "ndays", "retention", "user_count"}) {
        cout << " " << std::setw(13) << colName;
    }
    cout << '\n';
    for(int i = 0; i < 4; ++i) {
        cout << std::setfill(' ') << " " << std::setw(13) << std::setfill('_') << "";
    }
    cout << std::setfill(' ') << '\n';
    for(auto& result: results) {
        cout << " " << std::setw(13) << *get<0>(result);
        cout << " " << std::setw(13) << get<1>(result);
        cout << " " << std::setw(13) << get<2>(result);
        cout << " " << std::setw(13) << get<3>(result);
        cout << '\n';
    }
    cout << endl;
#endif
}

void greatest_n_per_group() {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    // Having a table consisting of multiple results for items,
    // I want to select a single result row per item, based on a condition like an aggregated value.
    // This is possible using a (self) join and filtering by aggregated value (in case it is unique) or using window functions.
    //
    // In this example, we select the most recent (successful) result per item

    struct some_result {
        int64 result_id;
        int64 item_id;
        time_t timestamp;
        bool flag;
    };

    auto storage =
        make_storage("",
                     make_table("some_result",
                                make_column("result_id", &some_result::result_id, primary_key().autoincrement()),
                                make_column("item_id", &some_result::item_id) /*foreign key*/,
                                make_column("timestamp", &some_result::timestamp),
                                make_column("flag", &some_result::flag)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        time_t now = std::time(nullptr);
        auto values = std::initializer_list<some_result>{{-1, 1, now - 86400 * 3, false},
                                                         {-1, 1, now - 86400 * 2, true},
                                                         {-1, 1, now, true},
                                                         {-1, 2, now, false},
                                                         {-1, 3, now - 86400 * 2, true}};
        storage.insert_range(values.begin(), values.end());
        return true;
    });

    //select r.*
    //    from some_result r
    //    inner join(
    //        select item_id,
    //        max(timestamp) as max_date
    //        from some_result
    //        group by item)id
    //    ) wnd
    //    on wnd.item_id = r.item_id
    //    and r.timestamp = wnd.max_date
    //    -- other conditions
    //where r.flag = 1
    constexpr auto wnd = "wnd"_cte;
    constexpr auto max_date = "max_date"_col;
    auto expression = with(
        wnd(&some_result::item_id, max_date)
            .as(select(columns(&some_result::item_id, max(&some_result::timestamp)), group_by(&some_result::item_id))),
        select(object<some_result>(),
               inner_join<wnd>(on(&some_result::item_id == wnd->*&some_result::item_id and
                                  &some_result::timestamp == wnd->*max_date)),
               // additional conditions
               where(c(&some_result::flag) == true)));
    string sql = storage.dump(expression);

    auto stmt = storage.prepare(expression);
    auto results = storage.execute(stmt);

    cout << "most recent (successful) result per item:\n";
    for(const char* colName: {"id", "item_id", "timestamp", "flag"}) {
        cout << "\t" << colName;
    }
    cout << '\n';
    for(auto& result: results) {
        cout << "\t" << result.result_id << "\t" << result.item_id << "\t" << result.timestamp << "\t" << result.flag
             << '\n';
    }
    cout << endl;
#endif
}
#endif

int main() {
#ifdef ENABLE_THIS_EXAMPLE
    try {
        all_integers_between(1, 10);
        supervisor_chain();
        works_for_alice();
        family_tree();
        depth_or_breadth_first();
        apfelmaennchen();
        sudoku();
        neevek_issue_222();
        select_from_subselect();
        greatest_n_per_group();
        show_optimization_fence();
        show_mapping_and_backreferencing();
    } catch(const system_error& e) {
        cout << "[" << e.code() << "] " << e.what();
    }
#endif

    return 0;
}
