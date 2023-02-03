#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <algorithm>  //  std::count_if
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

using namespace sqlite_orm;

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
        storage.update_all(set(assign(&User::name, std::move(ptr))));
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

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };

    struct Visit {
        int id = 0;
        int userId = 0;
        time_t date = 0;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        Visit() = default;
        Visit(int id, int userId, time_t date) : id{id}, userId{userId}, date{date} {}
#endif
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
        int fkey_account_other = 0;  // Account optional

        long line_date;
        std::string descripcion;
        int fkey_category;
        int fkey_concepto;
        int fkey_statement;
        int row;  // fkey_statement + row is unique
    };

    struct Categoria {
        int id_categoria;
        std::string name;
        bool is_expense_or_income;
    };

    auto storage = make_storage(
        {},
        make_table("Statement",
                   make_column("id_statement", &Statement::id_statement, primary_key().autoincrement()),
                   make_column("date", &Statement::date)),
        make_table("Categoria",
                   make_column("id_category", &Categoria::id_categoria, primary_key().autoincrement()),
                   make_column("name", &Categoria::name, collate_nocase()),
                   make_column("is_expense_or_income", &Categoria::is_expense_or_income)),
        make_table("Concepto",
                   make_column("id_concepto", &Concepto::id_concepto, primary_key().autoincrement()),
                   make_column("name", &Concepto::name, collate_nocase()),
                   make_column("fkey_account", &Concepto::fkey_account),
                   foreign_key(&Concepto::fkey_account).references(&Account::id_account)),
        make_table("Account",
                   make_column("id_account", &Account::id_account, primary_key().autoincrement()),
                   make_column("number", &Account::number, collate_nocase()),
                   make_column("fkey_bank", &Account::fkey_bank),
                   make_column("fkey_account_owner", &Account::fkey_account_owner),
                   make_column("description", &Account::description, collate_nocase()),
                   make_column("is_tarjeta", &Account::is_tarjeta),
                   foreign_key(&Account::fkey_account_owner).references(&AccountOwner::id_owner),
                   foreign_key(&Account::fkey_bank).references(&Banco::id_bank)),
        make_table("Banco",
                   make_column("id_bank", &Banco::id_bank, primary_key().autoincrement()),
                   make_column("nombre", &Banco::nombre, collate_nocase()),
                   make_column("ubicacion", &Banco::ubicacion, collate_nocase()),
                   make_column("fkey_Pais", &Banco::fkey_pais),
                   foreign_key(&Banco::fkey_pais).references(&Pais::id_pais)),
        make_table("AccountOwner",
                   make_column("id_owner", &AccountOwner::id_owner, primary_key().autoincrement()),
                   make_column("name", &AccountOwner::name, collate_nocase())),
        make_table("Transaccion",
                   make_column("id_transaccion", &Transaccion::id_transaccion, primary_key().autoincrement()),
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
                   make_column("id_pais", &Pais::id_pais, primary_key().autoincrement()),
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
