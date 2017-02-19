

/******
 This example is not about CRUD. This is an implementation of key-value storage just like 
 NSUserDefaults on iOS or SharedPreferences on Android.
 Here is the deal: we need to have `setValueForKey` and `getValueByKey` interface. All data must ba saved in sqlite 
 database.
 To perform this we create schema with table `key_value` and two columns: key:string and value:string. There is no
 primary key so we are unable to use CRUD functions like `get`, `get_no_throw`, `remove`, `update`. And there is no need to use
 these functions actually. We use non-CRUD functions `remove_all`, `insert` (it is non-CRUD cause it doesn't require a table to have a
 primary key column) and `get_all`. `setValue` function takes two string arguments: key and value. It creates a KeyValue
 object (KeyValue is a class mapped to the storage) with arguments and performs a transaction. Within this transaction
 first we call `REMOVE FROM key_value WHERE key = '%key%'` and next `INSERT INTO key_value values (%key%, %value%)`.
 After this there is a row in the `key_value` table with key and value passed into `setValue` function.
 `getValue` performs `get_all` with specific condition: `SELECT * FROM key_value WHERE key = '%key%'` and returns a front value if result
 (std::vector<KeyValue>) is not empty or empty string if there is no row with specified key.
 ******/

#include "sqlite_orm.h"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

/**
 *  This is just a mapped type. User doesn't have to interact with it directly.
 */
struct KeyValue {
    std::string key;
    std::string value;
};

auto& getStorage() {
    using namespace sqlite_orm;
    static auto storage = make_storage("key_value_example.sqlite",
                                       make_table("key_value",
                                                  make_column("key",
                                                              &KeyValue::key),
                                                  make_column("value",
                                                              &KeyValue::value)));
    return storage;
}

void setValue(const std::string &key, const std::string &value) {
    using namespace sqlite_orm;
    KeyValue kv{key, value};
    getStorage().transaction([&] {
        getStorage().remove_all<KeyValue>(where(is_equal(&KeyValue::key, key)));
        getStorage().insert(kv);
        return true;
    });
}

std::string getValue(const std::string &key) {
    using namespace sqlite_orm;
    auto kvs = getStorage().get_all<KeyValue>(where(is_equal(&KeyValue::key, key)));
    if(kvs.size()){
        if(kvs.size() == 1){
            return kvs.front().value;
        }else{
            cerr << "kvs for key *" << key << "* is more that 1 (" << kvs.size() << ")" << endl;
            return kvs.front().value;
        }
    }else{
        return {};
    }
}

int storedKeysCount() {
    return getStorage().count<KeyValue>();
}

int main(int argc, char **argv) {
    
    cout << argv[0] << endl;    //  to know executable path in case if you need to access sqlite directly from sqlite client
    
    getStorage().sync_schema(); //  to create table if it doesn't exist
    
    struct {
        std::string userId = "userId";
        std::string userName = "userName";
        std::string userGender = "userGender";  //  this key will be missing
    } keys; //  to keep keys in one place
    
    setValue(keys.userId, "6");
    setValue(keys.userName, "Peter");
    
    auto userId = getValue(keys.userId);
    cout << "userId = " << userId << endl;  //  userId = 6
    
    auto userName = getValue(keys.userName);
    cout << "userName = " << userName << endl;  //  userName = Peter
    
    auto userGender = getValue(keys.userGender);
    cout << "userGender = " << userGender << endl;  //  userGender =
    
    auto kvsCount = storedKeysCount();
    cout << "kvsCount = " << kvsCount << endl;  //  kvsCount = 2
    
}
