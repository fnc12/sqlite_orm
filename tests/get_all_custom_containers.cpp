#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

#include <list>
#include <deque>

using namespace sqlite_orm;

struct User {
    int id = 0;
    std::string name;
};

struct Comparator {

    bool operator()(const User &lhs, const User &rhs) const {
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }

    bool operator()(const std::unique_ptr<User> &lhs, const User &rhs) const {
        if(lhs) {
            return this->operator()(*lhs, rhs);
        } else {
            return false;
        }
    }
};

struct Tester {
    const std::vector<User> &expected;

    template<class E, class T>
    void testContainer(const T &users) const {
        REQUIRE(std::equal(users.begin(), users.end(), this->expected.begin(), this->expected.end(), Comparator{}));
        static_assert(std::is_same<T, E>::value, "");
    }

    template<class E, class S, class T>
    void testPreparedStatement(S &storage, const T &statement) const {
        this->testContainer<E>(storage.execute(statement));
    }
};

TEST_CASE("get_all deque") {
    using Catch::Matchers::UnorderedEquals;

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    User user1{1, "Nicki"};
    User user2{2, "Karol"};
    storage.replace(user1);
    storage.replace(user2);

    std::vector<User> expected;
    expected.push_back(user1);
    expected.push_back(user2);

    Tester tester{expected};

    //  get_all
    tester.testContainer<std::vector<User>>(storage.get_all<User>());
    tester.testContainer<std::vector<User>>(storage.get_all<User, std::vector<User>>());
    tester.testContainer<std::deque<User>>(storage.get_all<User, std::deque<User>>());
    tester.testContainer<std::list<User>>(storage.get_all<User, std::list<User>>());
    tester.testPreparedStatement<std::vector<User>>(storage, storage.prepare(get_all<User>()));
    tester.testPreparedStatement<std::vector<User>>(storage, storage.prepare(get_all<User, std::vector<User>>()));
    tester.testPreparedStatement<std::deque<User>>(storage, storage.prepare(get_all<User, std::deque<User>>()));
    tester.testPreparedStatement<std::list<User>>(storage, storage.prepare(get_all<User, std::list<User>>()));

    //  get_all_pointer
    tester.testContainer<std::vector<std::unique_ptr<User>>>(storage.get_all_pointer<User>());
    tester.testContainer<std::vector<std::unique_ptr<User>>>(
        storage.get_all_pointer<User, std::vector<std::unique_ptr<User>>>());
    tester.testContainer<std::deque<std::unique_ptr<User>>>(
        storage.get_all_pointer<User, std::deque<std::unique_ptr<User>>>());
    tester.testContainer<std::list<std::unique_ptr<User>>>(
        storage.get_all_pointer<User, std::list<std::unique_ptr<User>>>());
    tester.testPreparedStatement<std::vector<std::unique_ptr<User>>>(storage, storage.prepare(get_all_pointer<User>()));
    tester.testPreparedStatement<std::vector<std::unique_ptr<User>>>(
        storage,
        storage.prepare(get_all_pointer<User, std::vector<std::unique_ptr<User>>>()));
    tester.testPreparedStatement<std::deque<std::unique_ptr<User>>>(
        storage,
        storage.prepare(get_all_pointer<User, std::deque<std::unique_ptr<User>>>()));
    tester.testPreparedStatement<std::list<std::unique_ptr<User>>>(
        storage,
        storage.prepare(get_all_pointer<User, std::list<std::unique_ptr<User>>>()));
}
