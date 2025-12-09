import sqlite_orm;

using namespace sqlite_orm;

int main() {
    auto storage = make_storage("");
    storage.sync_schema();

    constexpr orm_cte_moniker auto c = "cte"_cte;
    storage.with(c().as(select(1)), select(asterisk<c>()));
}
