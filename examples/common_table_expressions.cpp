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
    // variant 1, where-clause
    {
        //WITH RECURSIVE
        //    cnt(x) AS(VALUES(1) UNION ALL SELECT x + 1 FROM cnt WHERE x < 1000000)
        //    SELECT x FROM cnt;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr auto cnt_cte = "cnt"_cte;
        auto ast =
            with(cte<cnt_cte>()(
                     union_all(select(from), select(cnt_cte->*1_colalias + c(1), where(cnt_cte->*1_colalias < end)))),
                 select(cnt_cte->*1_colalias));
#else
        auto ast =
            with(cte<cte_1>()(union_all(select(from),
                                        select(1_ctealias->*1_colalias + c(1), where(1_ctealias->*1_colalias < end)))),
                 select(1_ctealias->*1_colalias));
#endif
// alternative
#if 0
        auto ast = with(
            cte<cte_1>()(union_all(select(from >>= colalias_f{}),
                                   select(1_ctealias->*colalias_f{} + c(1), where(1_ctealias->*colalias_f{} < end)))),
            select(1_ctealias->*colalias_f{}));
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

    // variant 2, limit-clause
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
        auto ast = with(cte<"cnt"_cte>("y"_col)(
                            union_all(select(from >>= "x"_col), select("cnt"_cte->*"y"_col + c(1), limit(end)))),
                        select("cnt"_cte->*"y"_col));
#else
        auto ast = with(cte<cte_1>()(union_all(select(as<colalias_f>(from)),
                                               select(column<cte_1>->*colalias_f{} + c(1), limit(end)))),
                        select(column<cte_1>->*colalias_f{}));
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

    // variant 3, limit-clause, explicit as() call
    {
        cout << "Integer range (limit-clause) [" << from << ", " << end
             << "]"
                "\n";
        for(int n: storage.with(cte<cte_1>().as(union_all(select(as<colalias_f>(from)),
                                                          select(column<cte_1>->*colalias_f{} + c(1), limit(end)))),
                                select(column<cte_1>->*colalias_f{}))) {
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
        auto ast = with(cte<"chain"_cte>()(union_all(
                            select(asterisk<Org>(), where(&Org::name == c("Fred"))),
                            select(asterisk<alias_a<Org>>(),
                                   where(c(alias_column<alias_a<Org>>(&Org::name)) == "chain"_cte->*&Org::boss)))),
                        select("chain"_cte->*&Org::name));
#else
        auto ast = with(cte<cte_1>()(union_all(
                            select(asterisk<Org>(), where(&Org::name == c("Fred"))),
                            select(asterisk<alias_a<Org>>(),
                                   where(c(alias_column<alias_a<Org>>(&Org::name)) == column<cte_1>->*&Org::boss)))),
                        select(column<cte_1>->*&Org::name));
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
        auto ast =
            with(cte<works_for_alice>("n"_col)(
                     union_(select("Alice"), select(&Org::name, where(c(&Org::boss) == works_for_alice->*"n"_col)))),
                 select(avg(&Org::height), from<Org>(), where(in(&Org::name, select(works_for_alice->*"n"_col)))));
#else
        auto ast =
            with(cte<cte_1>()(
                     union_(select("Alice"), select(&Org::name, where(c(&Org::boss) == column<cte_1>->*1_colalias)))),
                 select(avg(&Org::height), from<Org>(), where(in(&Org::name, select(column<cte_1>->*1_colalias)))));
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
    auto ast =
        with(make_tuple(cte<parent_of>(&Family::name, parent)(union_(select(columns(&Family::name, &Family::mom)),
                                                                     select(columns(&Family::name, &Family::dad)))),
                        cte<ancestor_of_alice>(name)(union_all(
                            select(parent_of->*parent, where(parent_of->*&Family::name == "Alice")),
                            select(parent_of->*parent, join<ancestor_of_alice>(using_(parent_of->*&Family::name)))))),
             select(&Family::name,
                    where(is_equal(ancestor_of_alice->*name, &Family::name) && is_null(&Family::died)),
                    order_by(&Family::born)));
#else
    auto ast = with(
        make_tuple(cte<cte_1>("name", "parent")(union_(select(columns(&Family::name, &Family::mom >>= colalias_h{})),
                                                       select(columns(&Family::name, &Family::dad)))),
                   cte<cte_2>("name")(union_all(
                       select(column<cte_1>->*colalias_h{}, where(column<cte_1>->*&Family::name == "Alice")),
                       select(column<cte_1>->*colalias_h{}, join<cte_2>(using_(column<cte_1>->*&Family::name)))))),
        select(&Family::name,
               where(is_equal(column<cte_2>->*colalias_h{}, &Family::name) && is_null(&Family::died)),
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
        auto ast = with(cte<under_alice>(&Org::name, level)(
                            union_all(select(columns("Alice", 0)),
                                      select(columns(&Org::name, under_alice->*level + c(1)),
                                             join<under_alice>(on(under_alice->*&Org::name == &Org::boss)),
                                             order_by(2)))),
                        select(substr("..........", 1, under_alice->*level * c(3)) || c(under_alice->*&Org::name)));
#else
        auto ast =
            with(cte<cte_1>("name", "level")(union_all(select(columns("Alice", 0)),
                                                       select(columns(&Org::name, column<cte_1>->*2_colalias + c(1)),
                                                              join<cte_1>(on(column<cte_1>->*1_colalias == &Org::boss)),
                                                              order_by(2)))),
                 select(substr("..........", 1, column<cte_1>->*2_colalias * c(3)) || c(column<cte_1>->*1_colalias)));
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
        auto ast = with(cte<under_alice>(&Org::name, level)(
                            union_all(select(columns("Alice", 0)),
                                      select(columns(&Org::name, under_alice->*level + c(1)),
                                             join<under_alice>(on(under_alice->*&Org::name == &Org::boss)),
                                             order_by(2).desc()))),
                        select(substr("..........", 1, under_alice->*level * c(3)) || c(under_alice->*&Org::name)));
#else
        auto ast =
            with(cte<cte_1>("name", "level")(union_all(select(columns("Alice", 0)),
                                                       select(columns(&Org::name, column<cte_1>->*2_colalias + c(1)),
                                                              join<cte_1>(on(column<cte_1>->*1_colalias == &Org::boss)),
                                                              order_by(2).desc()))),
                 select(substr("..........", 1, column<cte_1>->*2_colalias * c(3)) || c(column<cte_1>->*1_colalias)));
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

    // SELECT * FROM (SELECT salary, comm AS commmission FROM emp) WHERE salary < 5000
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto sub = "sub"_cte;
    auto expression = with(cte<sub>()(select(columns(&Employee::m_salary, &Employee::m_commission))),
                           select(asterisk<sub>(), where(c(sub->*&Employee::m_salary) < 5000)));
#else
    auto expression = with(cte<cte_1>()(select(columns(&Employee::m_salary, &Employee::m_commission))),
                           select(asterisk<cte_1>(), where(c(1_ctealias->*&Employee::m_salary) < 5000)));
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
    auto ast = with(
        make_tuple(cte<xaxis>(x)(union_all(select(-2.0), select(xaxis->*x + c(0.05), where(xaxis->*x < 1.2)))),
                   cte<yaxis>(y)(union_all(select(-1.0), select(yaxis->*y + c(0.10), where(yaxis->*y < 1.0)))),
                   cte<m>(iter, cx, cy, x, y)(
                       union_all(select(columns(0, xaxis->*x, yaxis->*y, 0.0, 0.0)),
                                 select(columns(m->*iter + c(1),
                                                m->*cx,
                                                m->*cy,
                                                m->*x * c(m->*x) - m->*y * c(m->*y) + m->*cx,
                                                c(2.0) * m->*x * m->*y + m->*cy),
                                        where((m->*x * c(m->*x) + m->*y * c(m->*y)) < c(4.0) && m->*iter < 28)))),
                   cte<m2>(iter, cx, cy)(select(columns(max<>(m->*iter), m->*cx, m->*cy), group_by(m->*cx, m->*cy))),
                   cte<a>(t)(select(group_concat(substr(" .+*#", 1 + min<>(m2->*iter / c(7.0), 4.0), 1), ""),
                                    group_by(m2->*cy)))),
        select(group_concat(rtrim(a->*t), "\n")));
#else
    using cte_xaxis = decltype(1_ctealias);
    using cte_yaxis = decltype(2_ctealias);
    using cte_m = decltype(3_ctealias);
    using cte_m2 = decltype(4_ctealias);
    using cte_a = decltype(5_ctealias);
    constexpr cte_xaxis xaxis{};
    constexpr cte_yaxis yaxis{};
    constexpr cte_m m{};
    constexpr cte_m2 m2{};
    constexpr cte_a a{};
    constexpr internal::column_alias<'x'> x;
    constexpr internal::column_alias<'y'> y;
    constexpr internal::column_alias<'i', 't', 'e', 'r'> iter;
    constexpr internal::column_alias<'c', 'x'> cx;
    constexpr internal::column_alias<'c', 'y'> cy;
    constexpr internal::column_alias<'t'> t;
    auto ast = with(
        make_tuple(
            cte<cte_xaxis>("x")(union_all(select(-2.0 >>= x), select(xaxis->*x + c(0.05), where(xaxis->*x < 1.2)))),
            cte<cte_yaxis>("y")(union_all(select(-1.0 >>= y), select(yaxis->*y + c(0.10), where(yaxis->*y < 1.0)))),
            cte<cte_m>("iter", "cx", "cy", "x", "y")(
                union_all(select(columns(0 >>= iter, xaxis->*x >>= cx, yaxis->*y >>= cy, 0.0 >>= x, 0.0 >>= y)),
                          select(columns(m->*iter + c(1),
                                         m->*cx,
                                         m->*cy,
                                         m->*x * c(m->*x) - m->*y * c(m->*y) + m->*cx,
                                         c(2.0) * m->*x * m->*y + m->*cy),
                                 where((m->*x * c(m->*x) + m->*y * c(m->*y)) < c(4.0) && m->*iter < 28)))),
            cte<cte_m2>("iter", "cx", "cy")(
                select(columns(max<>(m->*iter) >>= iter, m->*cx, m->*cy), group_by(m->*cx, m->*cy))),
            cte<cte_a>("t")(select(group_concat(substr(" .+*#", 1 + min<>(m2->*iter / c(7.0), 4.0), 1), "") >>= t,
                                   group_by(m2->*cy)))),
        select(group_concat(rtrim(a->*t), "\n")));
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
    auto ast = with(
        make_tuple(
            cte<input>(sud)(
                select("53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79")),
            cte<digits>(z, lp)(union_all(
                select(columns("1", 1)),
                select(columns(cast<string>(digits->*lp + c(1)), digits->*lp + c(1)), where(digits->*lp < 9)))),
            cte<x>(s, ind)(union_all(
                select(columns(input->*sud, instr(input->*sud, "."))),
                select(columns(substr(x->*s, 1, x->*ind - c(1)) || c(z) || substr(x->*s, x->*ind + c(1)),
                               instr(substr(x->*s, 1, x->*ind - c(1)) || c(z) || substr(x->*s, x->*ind + c(1)), ".")),
                       where(x->*ind > 0 and
                             not exists(select(
                                 1 >>= lp,
                                 from<digits>(),
                                 where(is_equal(z_alias->*z, substr(x->*s, ((x->*ind - c(1)) / 9) * 9 + lp, 1)) or
                                       is_equal(z_alias->*z,
                                                substr(x->*s, ((x->*ind - c(1)) % 9) + (lp - c(1)) * 9 + 1, 1)) or
                                       is_equal(z_alias->*z,
                                                substr(x->*s,
                                                       (((x->*ind - c(1)) / 3) % 3) * 3 + ((x->*ind - c(1)) / 27) * 27 +
                                                           lp + ((lp - c(1)) / 3) * 6,
                                                       1)))))))))),
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

void show_mapping_and_backreferencing() {
    struct Object {
        int64 id;
    };

    // column alias
    struct cnt : alias_tag {
        static constexpr std::string_view get() {
            return "counter";
        }
    };

    auto storage = make_storage("", make_table("object", make_column("id", &Object::id)));
    storage.sync_schema();

    // back-reference via `column_pointer<cte_1, F O::*>`
    {
        auto ast = with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>->*&Object::id));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // map column via alias_holder into cte,
    // back-reference via `column_pointer<cte_1, alias_holder>`
    {
        auto ast =
            with(cte<cte_1>("x")(union_all(select(as<cnt>(1)), select(column<cte_1>(get<cnt>()) + c(1), limit(10)))),
                 select(column<cte_1>(get<cnt>())));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }
    // map column via alias_holder into cte,
    // back-reference via `column_pointer<cte_1, alias_holder>`
    {
        auto ast = with(cte<cte_1>("x")(union_all(select(as<cnt>(1)), select(column<cte_1>->*cnt{} + c(1), limit(10)))),
                        select(column<cte_1>->*cnt{}));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }
    // map column via alias_holder into cte,
    // back-reference via `column_pointer<cte_1, alias_holder>`
    {
        auto ast = with(cte<cte_1>("x")(union_all(select(as<cnt>(1)), select(cte_1{}->*cnt{} + c(1), limit(10)))),
                        select(cte_1{}->*cnt{}));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // implicitly remap column into cte
    {
        auto ast = with(cte<cte_1>(std::ignore)(select(&Object::id)), select(column<cte_1>(&Object::id)));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly remap column into cte (independent of subselect)
    {
        auto ast = with(cte<cte_1>(&Object::id)(select(&Object::id)), select(column<cte_1>(&Object::id)));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly remap column as an alias into cte (independent of subselect)
    {
        auto ast = with(cte<cte_1>(cnt{})(select(&Object::id)), select(column<cte_1>(get<cnt>())));

        string sql = storage.dump(ast);
        auto stmt = storage.prepare(ast);
    }

    // explicitly state that column name should be taken from subselect
    {
        struct CTEObject : alias_tag {
            // a CTE object is its own table alias
            using type = CTEObject;

            static std::string get() {
                return "CTEObj";
            }

            int64 xyz;
        };
        auto ast =
            with(cte<CTEObject>(make_column("xyz", &CTEObject::xyz))(select(&Object::id)), select(&CTEObject::xyz));

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
        __time64_t timestamp;
    };

    auto storage = make_storage("",
                                make_table("user_activity",
                                           make_column("id", &user_activity::id, primary_key().autoincrement()),
                                           make_column("uid", &user_activity::uid),
                                           make_column("timestamp", &user_activity::timestamp)));
    storage.sync_schema();
    storage.transaction([&storage]() {
        __time64_t now = _time64(nullptr);
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
            cte<register_user>()(select(
                columns(&user_activity::uid, min(date(&user_activity::timestamp, "unixepoch")) >>= register_date),
                group_by(&user_activity::uid).having(greater_or_equal(register_date, date("now", "-7 days"))))),
            cte<registered_cnt>()(select(columns(register_user->*register_date,
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
    // This is possible using a (self) join and filtering by aggregated value (in case it is unique), or using window functions.
    //
    // In this example, we select the most recent (successful) result per item

    struct some_result {
        int64 result_id;
        int64 item_id;
        __time64_t timestamp;
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
        __time64_t now = _time64(nullptr);
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
    auto expression =
        with(cte<wnd>(&some_result::item_id, max_date)(
                 select(columns(&some_result::item_id, max(&some_result::timestamp)), group_by(&some_result::item_id))),
             select(object<some_result>(),
                    inner_join<wnd>(on(c(&some_result::item_id) == wnd->*&some_result::item_id and
                                       c(&some_result::timestamp) == wnd->*max_date)),
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
        show_mapping_and_backreferencing();
    } catch(const system_error& e) {
        cout << "[" << e.code() << "] " << e.what();
    }
#endif

    return 0;
}
