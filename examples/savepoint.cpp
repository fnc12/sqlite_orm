/**
 *  Savepoints are named nested transactions: https://www.sqlite.org/lang_savepoint.html
 *
 *  SAVEPOINT name;                      -> storage.savepoint("name") / storage.savepoint_guard("name")
 *  RELEASE SAVEPOINT name;              -> storage.release_savepoint("name") / guard.release()
 *  ROLLBACK TO SAVEPOINT name;          -> storage.rollback_to_savepoint("name") / guard.rollback_to()
 */
#include <sqlite_orm/sqlite_orm.h>
#include <iostream>

using std::cout;
using std::endl;

struct Order {
    int id = 0;
    std::string item;
    int quantity = 0;
};

int main() {
    using namespace sqlite_orm;
    auto storage = make_storage("savepoint.sqlite",
                                make_table("orders",
                                           make_column("id", &Order::id, primary_key()),
                                           make_column("item", &Order::item),
                                           make_column("quantity", &Order::quantity)));
    storage.sync_schema();
    storage.remove_all<Order>();

    //  BEGIN TRANSACTION;
    //  INSERT INTO orders (item, quantity) VALUES ('keyboard', 1);
    //  SAVEPOINT "accessories";
    //  INSERT INTO orders (item, quantity) VALUES ('mouse', 1);
    //  ROLLBACK TO SAVEPOINT "accessories";     -- the mouse is out of stock
    //  RELEASE SAVEPOINT "accessories";
    //  COMMIT;
    {
        auto transactionGuard = storage.transaction_guard();
        storage.insert(Order{0, "keyboard", 1});
        {
            auto savepointGuard = storage.savepoint_guard("accessories");
            storage.insert(Order{0, "mouse", 1});
            //  the mouse turned out to be out of stock: the guard's destructor
            //  executes ROLLBACK TO SAVEPOINT + RELEASE SAVEPOINT, undoing only this insert
        }
        transactionGuard.commit();
    }
    cout << "after a partial rollback the keyboard is still ordered: " << storage.count<Order>() << " order(s)" << endl;

    //  a savepoint outside of a transaction behaves like BEGIN DEFERRED TRANSACTION;
    //  ROLLBACK TO SAVEPOINT keeps the savepoint on the stack, so a batch may be retried
    {
        auto savepointGuard = storage.savepoint_guard("bulk_order");
        for (int attempt = 1; attempt <= 2; ++attempt) {
            try {
                storage.insert(Order{0, "monitor", attempt == 1 ? -1 : 2});
                if (storage.count<Order>(where(c(&Order::quantity) < 0)) > 0) {
                    throw std::runtime_error("negative quantity");
                }
                break;
            } catch (const std::runtime_error&) {
                cout << "attempt " << attempt << " failed, retrying" << endl;
                savepointGuard.rollback_to();
            }
        }
        savepointGuard.release();
    }
    cout << "orders after the retried bulk order: " << storage.count<Order>() << endl;

    //  the functional style mirrors storage.transaction():
    //  returning true releases the savepoint, false rolls it back
    storage.savepoint("discount", [&storage] {
        storage.insert(Order{0, "cable", 3});
        return false;  //  changed our mind
    });
    cout << "orders after the declined discount purchase: " << storage.count<Order>() << endl;

    return 0;
}
