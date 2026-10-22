// note: importing `std` directly also makes CMake 4.5.0-rc1 link its object file with MSVC and the Ninja generators,
// which it omits when `std` is imported only through `std.compat` [see https://gitlab.kitware.com/cmake/cmake/-/work_items/28185]
import std;
import sqlite_orm;

using namespace sqlite_orm;

int main() {
    auto storage = make_storage("");
    storage.sync_schema();

    constexpr orm_cte_moniker auto c = "cte"_cte;
    for (auto& [value]: storage.with(c().as(select(1)), select(asterisk<c>()))) {
        std::println("cte value: {}", value);
    }
}
