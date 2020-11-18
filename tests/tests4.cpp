#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <algorithm>  //  std::count_if
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

using namespace sqlite_orm;

TEST_CASE("Case") {

    struct User {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string country;
    };

    struct Track {
        int id = 0;
        std::string name;
        long milliseconds = 0;
    };

    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, autoincrement(), primary_key()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("country", &User::country)),
                                make_table("tracks",
                                           make_column("trackid", &Track::id, autoincrement(), primary_key()),
                                           make_column("name", &Track::name),
                                           make_column("milliseconds", &Track::milliseconds)));
    storage.sync_schema();

    struct GradeAlias : alias_tag {
        static const std::string &get() {
            static const std::string res = "Grade";
            return res;
        }
    };

    {
        storage.insert(User{0, "Roberto", "Almeida", "Mexico"});
        storage.insert(User{0, "Julia", "Bernett", "USA"});
        storage.insert(User{0, "Camille", "Bernard", "Argentina"});
        storage.insert(User{0, "Michelle", "Brooks", "USA"});
        storage.insert(User{0, "Robet", "Brown", "USA"});

        auto rows = storage.select(
            columns(case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end()),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<User>());
            REQUIRE(std::get<0>(rows[0]) == "Foreign");
            REQUIRE(std::get<0>(rows[1]) == "Foreign");
            REQUIRE(std::get<0>(rows[2]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[3]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[4]) == "Dosmetic");
        };
        verifyRows(rows);

        rows = storage.select(
            columns(as<GradeAlias>(
                case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end())),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));

        verifyRows(rows);
    }
    {
        storage.insert(Track{0, "For Those About To Rock", 400000});
        storage.insert(Track{0, "Balls to the Wall", 500000});
        storage.insert(Track{0, "Fast as a Shark", 200000});
        storage.insert(Track{0, "Restless and Wild", 100000});
        storage.insert(Track{0, "Princess of the Dawn", 50000});

        auto rows = storage.select(
            case_<std::string>()
                .when(c(&Track::milliseconds) < 60000, then("short"))
                .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                .else_("long")
                .end(),
            order_by(&Track::name));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<Track>());
            REQUIRE(rows[0] == "long");
            REQUIRE(rows[1] == "medium");
            REQUIRE(rows[2] == "long");
            REQUIRE(rows[3] == "short");
            REQUIRE(rows[4] == "medium");
        };
        verifyRows(rows);

        rows = storage.select(
            as<GradeAlias>(
                case_<std::string>()
                    .when(c(&Track::milliseconds) < 60000, then("short"))
                    .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                    .else_("long")
                    .end()),
            order_by(&Track::name));
        verifyRows(rows);
    }
}

TEST_CASE("Unique ptr in update") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});

    {
        storage.update_all(set(assign(&User::name, std::make_unique<std::string>("Nick"))));
        REQUIRE(storage.count<User>(where(is_null(&User::name))) == 0);
    }
    {
        std::unique_ptr<std::string> ptr;
        storage.update_all(set(assign(&User::name, move(ptr))));
        REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 0);
    }
}

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("optional in update") {

    struct User {
        int id = 0;
        std::optional<int> carYear;  // will be empty if user takes the bus.
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("car_year", &User::carYear)));
    storage.sync_schema();

    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{0, 2006});

    REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 1);

    {
        storage.update_all(set(assign(&User::carYear, std::optional<int>{})));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, 1994)));
        REQUIRE(storage.count<User>(where(is_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, nullptr)));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
}
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

TEST_CASE("join") {

    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        int id = 0;
        int userId = 0;
        time_t date = 0;
    };

    auto storage =
        make_storage({},
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_table("visits",
                                make_column("id", &Visit::id, primary_key()),
                                make_column("user_id", &Visit::userId),
                                make_column("date", &Visit::date)));
    storage.sync_schema();

    int id = 1;
    User will{id++, "Will"};
    User smith{id++, "Smith"};
    User nicole{id++, "Nicole"};

    storage.replace(will);
    storage.replace(smith);
    storage.replace(nicole);

    id = 1;
    storage.replace(Visit{id++, will.id, 10});
    storage.replace(Visit{id++, will.id, 20});
    storage.replace(Visit{id++, will.id, 30});

    storage.replace(Visit{id++, smith.id, 25});
    storage.replace(Visit{id++, smith.id, 35});

    {
        auto rows = storage.get_all<User>(left_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(left_outer_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(inner_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(cross_join<Visit>());
        REQUIRE(rows.size() == 15);
    }
    {
        auto rows = storage.get_all<User>(natural_join<Visit>());
        REQUIRE(rows.size() == 3);
    }
}

TEST_CASE("two joins") {
    struct Statement {
        int id_statement;
        long date;
    };

    struct Concepto {
        int id_concepto;
        std::string name;  // TFT-SINPE A: 15103-02**-****-8467
        int fkey_account;  // { 15103-02**-****-8467, ...}
    };

    struct Account {
        int id_account;
        std::string number;  // 15103-02**-****-8467
        int fkey_bank;  // { BAC San Jose, "Barrio Dent", { Costa Rica} }
        int fkey_account_owner;  // { Juan Dent Herrera, ... }
        std::string description;  // AMEX Cashback Premium
        bool is_tarjeta;  // true
    };

    struct Pais {
        int id_pais;
        std::string name;
    };

    struct Banco {
        int id_bank;
        std::string nombre;
        std::string ubicacion;
        int fkey_pais;

        Pais getPais() const;
    };

    struct AccountOwner {
        int id_owner;
        std::string name;
    };

    struct Transaccion {
        int id_transaccion;
        double amount_colones;
        double amount_dolares;
        int fkey_account_own;  // Account
        std::optional<int> fkey_account_other;  // Account optional

        long line_date;
        std::string descripcion;
        int fkey_category;
        int fkey_concepto;
        int fkey_statement;
        int row;  // fkey_statement + row is unique

        std::shared_ptr<Account> getAccountOrigin() const;
        std::shared_ptr<Account> getAccountDestination() const;
    };

    struct Categoria {
        int id_categoria;
        std::string name;
        bool is_expense_or_income;
    };

    auto storage = make_storage(
        {},
        make_table("Statement",
                   make_column("id_statement", &Statement::id_statement, autoincrement(), primary_key()),
                   make_column("date", &Statement::date)),
        make_table("Categoria",
                   make_column("id_category", &Categoria::id_categoria, autoincrement(), primary_key()),
                   make_column("name", &Categoria::name, collate_nocase()),
                   make_column("is_expense_or_income", &Categoria::is_expense_or_income)),
        make_table("Concepto",
                   make_column("id_concepto", &Concepto::id_concepto, autoincrement(), primary_key()),
                   make_column("name", &Concepto::name, collate_nocase()),
                   make_column("fkey_account", &Concepto::fkey_account),
                   foreign_key(&Concepto::fkey_account).references(&Account::id_account)),
        make_table("Account",
                   make_column("id_account", &Account::id_account, autoincrement(), primary_key()),
                   make_column("number", &Account::number, collate_nocase()),
                   make_column("fkey_bank", &Account::fkey_bank),
                   make_column("fkey_account_owner", &Account::fkey_account_owner),
                   make_column("description", &Account::description, collate_nocase()),
                   make_column("is_tarjeta", &Account::is_tarjeta),
                   foreign_key(&Account::fkey_account_owner).references(&AccountOwner::id_owner),
                   foreign_key(&Account::fkey_bank).references(&Banco::id_bank)),
        make_table("Banco",
                   make_column("id_bank", &Banco::id_bank, autoincrement(), primary_key()),
                   make_column("nombre", &Banco::nombre, collate_nocase()),
                   make_column("ubicacion", &Banco::ubicacion, collate_nocase()),
                   make_column("fkey_Pais", &Banco::fkey_pais),
                   foreign_key(&Banco::fkey_pais).references(&Pais::id_pais)),
        make_table("AccountOwner",
                   make_column("id_owner", &AccountOwner::id_owner, autoincrement(), primary_key()),
                   make_column("name", &AccountOwner::name, collate_nocase())),
        make_table("Transaccion",
                   make_column("id_transaccion", &Transaccion::id_transaccion, autoincrement(), primary_key()),
                   make_column("colones", &Transaccion::amount_colones),
                   make_column("dolares", &Transaccion::amount_dolares),
                   make_column("fkey_account_own", &Transaccion::fkey_account_own),
                   make_column("fkey_account_other", &Transaccion::fkey_account_other),
                   make_column("line_date", &Transaccion::line_date),
                   make_column("descripcion", &Transaccion::descripcion),
                   make_column("fkey_category", &Transaccion::fkey_category),
                   make_column("concepto", &Transaccion::fkey_concepto),
                   make_column("fkey_statement", &Transaccion::fkey_statement),
                   make_column("row", &Transaccion::row),
                   foreign_key(&Transaccion::fkey_account_own).references(&Account::id_account),
                   foreign_key(&Transaccion::fkey_account_other).references(&Account::id_account),
                   foreign_key(&Transaccion::fkey_category).references(&Categoria::id_categoria),
                   foreign_key(&Transaccion::fkey_concepto).references(&Concepto::id_concepto),
                   foreign_key(&Transaccion::fkey_statement).references(&Statement::id_statement)),
        make_table("Pais",
                   make_column("id_pais", &Pais::id_pais, autoincrement(), primary_key()),
                   make_column("name", &Pais::name, collate_nocase())));
    storage.sync_schema();

    using als_t = alias_t<Transaccion>;
    using als_a = alias_a<Account>;
    using als_b = alias_b<Account>;

    std::ignore = storage.select(columns(alias_column<als_t>(&Transaccion::fkey_account_other),
                                         alias_column<als_t>(&Transaccion::fkey_account_own),
                                         alias_column<als_a>(&Account::id_account),
                                         alias_column<als_b>(&Account::id_account)),
                                 left_outer_join<als_a>(on(c(alias_column<als_t>(&Transaccion::fkey_account_other)) ==
                                                           alias_column<als_a>(&Account::id_account))),
                                 inner_join<als_b>(on(c(alias_column<als_t>(&Transaccion::fkey_account_own)) ==
                                                      alias_column<als_b>(&Account::id_account))));

    std::ignore = storage.select(columns(alias_column<als_t>(&Transaccion::fkey_account_other),
                                         alias_column<als_t>(&Transaccion::fkey_account_own),
                                         alias_column<als_a>(&Account::id_account),
                                         alias_column<als_b>(&Account::id_account)),
                                 left_join<als_a>(on(c(alias_column<als_t>(&Transaccion::fkey_account_other)) ==
                                                     alias_column<als_a>(&Account::id_account))),
                                 inner_join<als_b>(on(c(alias_column<als_t>(&Transaccion::fkey_account_own)) ==
                                                      alias_column<als_b>(&Account::id_account))));

    std::ignore = storage.select(columns(alias_column<als_t>(&Transaccion::fkey_account_other),
                                         alias_column<als_t>(&Transaccion::fkey_account_own),
                                         alias_column<als_a>(&Account::id_account),
                                         alias_column<als_b>(&Account::id_account)),
                                 join<als_a>(on(c(alias_column<als_t>(&Transaccion::fkey_account_other)) ==
                                                alias_column<als_a>(&Account::id_account))),
                                 inner_join<als_b>(on(c(alias_column<als_t>(&Transaccion::fkey_account_own)) ==
                                                      alias_column<als_b>(&Account::id_account))));
}

TEST_CASE("update set null") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.replace(User{1, std::make_unique<std::string>("Ototo")});
    REQUIRE(storage.count<User>() == 1);
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
    }

    storage.update_all(set(assign(&User::name, nullptr)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }

    storage.update_all(set(assign(&User::name, "ototo")));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
        REQUIRE(*rows.front().name == "ototo");
    }

    storage.update_all(set(assign(&User::name, nullptr)), where(is_equal(&User::id, 1)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }
}
