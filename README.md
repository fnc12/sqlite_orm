# SQLite ORM
SQLite ORM light header only library for modern C++

# Advantages

* **Intuitive syntax**
* **Built with modern C++14 features (no macros)**
* **CRUD support**
* **Transactions support**
* **Migrations functionality**
* **Powerful conditions**
* **Follows single responsibility principle** - no need write code inside your data model classes
* **Easy integration** - single header only lib.
* **The only dependency** - libsqlite3
* **C++ standart code style**
* **No undefined behaviour** - if something goes wrong lib throws an exeption

`sqlite_orm` library allows to create easy data model mappings to your database schema. It is built to manage (CRUD) objects with a single column with primary key and without it. It also allows you to specify table names and column names explicitly no matter how your classes actually named. Take a look at example:

```c++

struct User{
    int id;
    std::string firstName;
    std::string lastName;
    int birthDate;
    std::shared_ptr<std::string> imageUrl;      
    int typeId;
};

struct UserType {
    int id;
    std::string name;
};

```

So we have database with predefined schema like 

`CREATE TABLE users (id integer primary key autoincrement, first_name text not null, last_name text not null, birth_date integer not null, image_url text, type_id integer not null)`

`CREATE TABLE user_types (id integer primary key autoincrement, name text not null DEFAULT 'name_placeholder')`

Now we tell `sqlite_orm` library about schema and provide database filename. We create `storage` service object that has CRUD interface. Also we create every table and every column. All code is intuitive and minimalistic.

```c++

using namespace sqlite_orm;
auto storage = make_storage("db.sqlite",
                            make_table("users",
                                       make_column("id",
                                                   &User::id,
                                                   autoincrement(),
                                                   primary_key()),
                                       make_column("first_name",
                                                   &User::firstName),
                                       make_column("last_name",
                                                   &User::lastName),
                                       make_column("birth_date",
                                                   &User::birthDate),
                                       make_column("image_url",
                                                   &User::imageUrl),
                                       make_column("type_id",
                                                   &User::typeId)),
                            make_table("user_types",
                                       make_column("id",
                                                   &UserType::id,
                                                   autoincrement(),
                                                   primary_key()),
                                       make_column("name",
                                                   &UserType::name,
                                                   default_value("name_placeholder"))));
```

Too easy isn't it? You do not have to specify mapped type expllicitly - it is deduced from your member pointers you pass during making a column (for example: `&User::id`). To create a column you have to pass two arguments at least: its name in the table and your mapped class member pointer. You can also add extra arguments to tell your storage about schema options like ~~`not_null`~~ (deduced from type), `primary_key` or `autoincrement` (order isn't important).

# CRUD

Let's create and insert new `User` into database. First we need to create a `User` object with any id and call `insert` function. It will return id of just created user or throw exeption if something goes wrong.

```c++
User user{-1, "Jonh", "Doe", 664416000, std::make_shared<std::string>("url_to_heaven"), 3 };
    
auto insertedId = storage.insert(user);
cout << "insertedId = " << insertedId << endl;      //  insertedId = 8
user.id = insertedId;

User secondUser{-1, "Alice", "Inwonder", 831168000, {} , 2};
insertedId = storage.insert(secondUser);
secondUser.id = insertedId;

```

Next let's get our user by id.

```c++
try{
    auto user = storage.get<User>(insertedId);
    cout << "user = " << user.firstName << " " << user.lastName << endl;
}catch(sqlite_orm::not_found_exception) {
    cout << "user not found with id " << insertedId << endl;
}catch(...){
    cout << "unknown exeption" << endl;
}
```

Probably you may not like throwing exeptions. Me too. Exeption `not_found_exception` is thrown because return type in `get` function is not nullable. You can use alternative version `get_no_throw` which returns `std::shared_ptr` and doesn't throw `not_found_exception` if nothing found - just returns `nullptr`.

```c++
if(auto user = storage.get_no_throw<User>(insertedId)){
    cout << "user = " << user->firstName << " " << user->lastName << endl;
}else{
    cout << "no user with id " << insertedId << endl;
}
```

`std::shared_ptr` is used as optional in `sqlite_orm`. Of course there is class optional in C++14 located at `std::experimental::optional`. But we don't want to use it until it is `experimental`.

We can also update our user. It updates row by id provided in `user` object and sets all other non `primary_key` fields to values stored in the passed `user` object. So you can just assign members to `user` object you want and call `update`

```c++
user.firstName = "Nicholas";
user.imageUrl = "https://cdn1.iconfinder.com/data/icons/man-icon-set/100/man_icon-21-512.png"
storage.update(user);
```

And delete. To delete you have to pass id only, not whole object. Also we need to explicitly tell which class of object we want to delete. Function name is `remove` not `delete` cause `delete` is a reseved word in C++.

```c++
storage.remove<User>(insertedId)
```

Also we can extract all objects into `std::vector`.

```c++
auto allUsers = storage.get_all<User>();
cout << "allUsers (" << allUsers.size() << "):" << endl;
for(auto &user : allUsers) {
    cout << storage.dump(user) << endl; //  dump returns std::string with json-like style object info. For example: { id : '1', first_name : 'Jonh', last_name : 'Doe', birth_date : '664416000', image_url : '0x10090c3d8', type_id : '3' }
}
```

CRUD functions `get`, `get_no_throw`, `remove`, `update` (not `insert`) work only if your type has a primary key column. If you try to `get` an object that is mapped to your storage but has no primary key column a `std::runtime_error` will be thrown cause `sqlite_orm` cannot detect an id. If you want to know how to perform a storage without primary key take a look at `key_value.cpp` example in `examples` folder.

# Aggregate Functions

```c++
auto averageId = storage.avg(&User::id);    //  maps to 'select avg(id) from users'
cout << "averageId = " << averageId << endl;        //  averageId = 4.5
    
auto averageBirthDate = storage.avg(&User::birthDate);  //  maps into 'select avg(birth_date) from users'
cout << "averageBirthDate = " << averageBirthDate << endl;      //  averageBirthDate = 6.64416e+08
  
auto usersCount = storage.count<User>();    //  maps to 'select count(*) from users'
cout << "users count = " << usersCount << endl;     //  users count = 8
    
auto countId = storage.count(&User::id);    //  maps to 'select count(id) from users'
cout << "countId = " << countId << endl;        //  countId = 8

auto countImageUrl = storage.count(&User::imageUrl);   //  maps to 'select count(image_url) from users'
cout << "countImageUrl = " << countImageUrl << endl;      //  countImageUrl = 5

auto concatedUserId = storage.group_concat(&User::id);      //  maps to 'select group_concat(id) from users'
cout << "concatedUserId = " << concatedUserId << endl;      //  concatedUserId = 1,2,3,4,5,6,7,8
    
auto concatedUserIdWithDashes = storage.group_concat(&User::id, "---");     //  maps to 'select group_concat(id, "---") from users'
cout << "concatedUserIdWithDashes = " << concatedUserIdWithDashes << endl;      //  concatedUserIdWithDashes = 1---2---3---4---5---6---7---8

if(auto maxId = storage.max(&User::id)){    //  maps to 'select max(id) from users'
    cout << "maxId = " << *maxId <<endl;    //  maxId = 12  (maxId is std::shared_ptr<int>)
}else{
    cout << "maxId is null" << endl;
}
    
if(auto maxFirstName = storage.max(&User::firstName)){  //  maps to 'select max(first_name) from users'
    cout << "maxFirstName = " << *maxFirstName << endl; //  maxFirstName = Jonh (maxFirstName is std::shared_ptr<std::string>)
}else{
    cout << "maxFirstName is null" << endl;
}

if(auto minId = storage.min(&User::id)){    //  maps to 'select min(id) from users'
    cout << "minId = " << *minId << endl;   //  minId = 1 (minId is std::shared_ptr<int>)
}else{
    cout << "minId is null" << endl;
}

if(auto minLastName = storage.min(&User::lastName)){    //  maps to 'select min(last_name) from users'
    cout << "minLastName = " << *minLastName << endl;   //  minLastName = Doe (minLastName is std::shared_ptr<std::string>)
}else{
    cout << "minLastName is null" << endl;
}

```

# Where conditions

You also can select objects with custom where conditions with `=`, `!=`, `>`, `>=`, `<`, `<=`, `IN`.

For example: let's select users with id lesser then 10:

```c++
auto idLesserThan10 = storage.get_all<User>(where(lesser_than(&User::id, 10))); //  maps to 'select * from users where ( id < 10 )`
cout << "idLesserThan10 count = " << idLesserThan10.size() << endl;
for(auto &user : idLesserThan10) {
    cout << storage.dump(user) << endl;
}
```

Or select all users who's first name is not equal "John":

```c++
auto notJohn = storage.get_all<User>(where(is_not_equal(&User::firstName, "John")));    //  maps to 'select * from users where ( first_name != 'John' ) '
cout << "notJohn count = " << notJohn.size() << endl;
for(auto &user : notJohn) {
    cout << storage.dump(user) << endl;
}
```

By the way one can implement not equal in a different way using C++ negation operator:

```c++
auto notJohn2 = storage.get_all<User>(where(not is_equal(&User::firstName, "John")));
```

You can use `!` and `not` in this case cause they are equal. Also you can chain several conditions with `and` and `or` operators. Let's try to get users with query with conditions like `where id >= 5 and id <= 7 and not id = 6`:

```c++
auto id5and7 = storage.get_all<User>(where(lesser_or_equal(&User::id, 7) and greater_or_equal(&User::id, 5) and not is_equal(&User::id, 6)));
cout << "id5and7 count = " << id5and7.size() << endl;
for(auto &user : id5and7) {
    cout << storage.dump(user) << endl;
}
```

Or let's just export two users with id 10 or id 16 (of course if these users exist):

```c++
auto id10or16 = storage.get_all<User>(where(is_equal(&User::id, 10) or is_equal(&User::id, 16)));
cout << "id10or16 count = " << id10or16.size() << endl;
for(auto &user : id10or16) {
    cout << storage.dump(user) << endl;
}
```

In fact you can chain together any number of different conditions with any operator from `and`, `or` and `not`. All conditions are templated so there is no runtime overhead. And this makes `sqlite_orm` the most powerful **sqlite** C++ ORM library.

Moreover you can use parentheses to set the priority of query conditions:

```c++
auto cuteConditions = storage.get_all<User>(where((is_equal(&User::firstName, "John") or is_equal(&User::firstName, "Alex")) and is_equal(&User::id, 4)));  //  where (first_name = 'John' or first_name = 'Alex') and id = 4
cout << "cuteConditions count = " << cuteConditions.size() << endl; //  cuteConditions count = 1
cuteConditions = storage.get_all<User>(where(is_equal(&User::firstName, "John") or (is_equal(&User::firstName, "Alex") and is_equal(&User::id, 4))));   //  where first_name = 'John' or (first_name = 'Alex' and id = 4)
cout << "cuteConditions count = " << cuteConditions.size() << endl; //  cuteConditions count = 2
```

Also we can implement `get` by id with `get_all` and `where` like this:

```c++
auto idEquals2 = storage.get_all<User>(where(is_equal(2, &User::id)));  //  maps to 'select * from users where ( 2 = id )'
cout << "idEquals2 count = " << idEquals2.size() << endl;
if(idEquals2.size()){
    cout << storage.dump(idEquals2.front()) << endl;
}else{
    cout << "user with id 2 doesn't exist" << endl;
}
```

Lets try the `IN` operator:

```c++
auto evenLesserTen10 = storage.get_all<User>(where(in(&User::id, {2, 4, 6, 8, 10})));   //  maps to select * from users where id in (2, 4, 6, 8, 10)
cout << "evenLesserTen10 count = " << evenLesserTen10.size() << endl;
for(auto &user : evenLesserTen10) {
    cout << storage.dump(user) << endl;
}

auto doesAndWhites = storage.get_all<User>(where(in(&User::lastName, {"Doe", "White"})));   //  maps to select * from users where last_name in ("Doe", "White")
cout << "doesAndWhites count = " << doesAndWhites.size() << endl;
for(auto &user : doesAndWhites) {
    cout << storage.dump(user) << endl;
}
```

Looks like magic but it works very simple. Functions

* is_equal
* is_not_equal
* greater_than
* greater_or_equal
* lesser_than
* lesser_or_equal

simulate binary comparison operator so they take 2 arguments: left hand side and right hand side. Arguments may be either member pointer of mapped class or literal. Binary comparison functions map arguments to text to be passed to sqlite engine to process query. Member pointers are being mapped to column names and literals to literals (numbers to raw numbers and string to quoted strings). Next `where` function places brackets around condition and adds "where" keyword before condition text. Next resulted string appends to query string and is being processed further.

If you omit `where` function in `get_all` it will return all objects from a table:

```c++
auto allUsers = storage.get_all<User>();
```

Also you can use `remove_all` function to perform `DELETE FROM ... WHERE` query with the same type of conditions.

```c++
storage.remove_all<User>(where(lesser_than(&User::id, 100)));
```

# Migrations functionality

There are no explicit `up` and `down` functions that are used to be used in migrations. Instead `sqlite_orm` offers `sync_schema` function that takes responsibility of comparing actual db file schema with one you specified in `make_storage` call and if something is not equal it alters or drops/creates schema.

```c++

storage.sync_schema();
```

Please beware that `sync_schema` doesn't guarantee that data will be saved. It *tries* to save it only. Below you can see rules list that `sync_schema` follows during call:
* if there are excess tables exist in db they are ignored (not dropped)
* every table from storage is compared with it's db analog and 
    * if table doesn't exist it is created
    * if table exists its colums are being compared with table_info from db and
        * if there are columns in db that do not exist in storage (excess) table will be dropped and recreated
        * if there are columns in storage that do not exist in db they will be added using 'ALTER TABLE ... ADD COLUMN ...' command and table data will not be dropped but if any of added columns is null but has not default value table will be dropped and recreated
        * if there is any column existing in both db and storage but differs by any of properties (type, pk, notnull) table will be dropped and recreated (dflt_value isn't checked cause there can be ambiguity in default values, please beware).

The reason of this kinda weird behaviour is the fact that `sqlite3` doesn't have `DROP COLUMN` query but has `ADD COLUMN`. Of course one can use temporary table to save all data. Probably it will be implemented in next versions of `sqlite_orm` lib (with `bool preserve = false` argument probably).

The best practice is to call this function right after storage creation.

# Transactions

There are two ways to begin and commit/rollback transactions:
* explicitly call `begin_transaction();`, `rollback();` or `commit();` functions
* use `transaction` function which begins transaction implicitly and takes a lambda argument which returns true for commit and false for rollback.

Example for explicit call:

```c++
auto secondUser = storage.get<User>(2);

storage.begin_transaction();
secondUser.typeId = 3;
storage.update(secondUser);
storage.rollback(); //  or storage.commit();

secondUser = storage.get<decltype(secondUser)>(secondUser.id);
assert(secondUser.typeId != 3);
```

Example for implicit call:

```c++
storage.transaction([&] () mutable {    //  mutable keyword allows make non-const function calls
    auto secondUser = storage.get<User>(2);
    secondUser.typeId = 1;
    storage.update(secondUser);
    auto gottaRollback = bool(rand() % 2);
    if(gottaRollback){  //  dummy condition for test
        return false;   //  exits lambda and calls ROLLBACK
    }
    return true;        //  exits lambda and calls COMMIT
});
```

The second way guarantess that `commit` or `rollback` will be called. You can use either way.

Trancations are useful with `changes` sqlite function that returns number of rows modified.

```c++
storage.transaction([&] () mutable {
    storage.remove_all<User>(where(lesser_than(&User::id, 100)));
    auto usersRemoved = storage.changes();
    cout << "usersRemoved = " << usersRemoved << endl;
    return true;
});
```

It will print a number of deleted users (rows). But if you call `changes` without a transaction and your database is located in file not in RAM the result will be 0 always cause `sqlite_orm` opens and closes connection every time you call a function without a transaction.

# Notes

To work well your data model class must be default constructable and must not have const fields mapped to database cause they are assigned during queries. Otherwise code won't compile on line with member assignment operator.

# Installation

Just put `include/sqlite_orm/sqlite_orm.h` into you folder with headers. Also it is recommended to keep project libraries' sources in separate folders cause there is no normal dependency manager for C++ yet.

# Requirements

* C++14 compatible compiler (not C++11 cause of templated lambdas in the lib).
* libsqlite3 linked to your binary
