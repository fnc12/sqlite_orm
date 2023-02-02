#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <list>
#include <deque>

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };

    struct Comparator {

        bool operator()(const User& lhs, const User& rhs) const {
            return lhs.id == rhs.id && lhs.name == rhs.name;
        }

        bool operator()(const std::unique_ptr<User>& lhs, const User& rhs) const {
            if(lhs) {
                return this->operator()(*lhs, rhs);
            } else {
                return false;
            }
        }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        bool operator()(const std::optional<User>& lhs, const User& rhs) const {
            if(lhs.has_value()) {
                return this->operator()(*lhs, rhs);
            } else {
                return false;
            }
        }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
    };
}

struct Tester {
    const std::vector<User>& expected;

    template<class E, class T>
    void testContainer(const T& users) const {
        REQUIRE(std::equal(users.begin(), users.end(), this->expected.begin(), this->expected.end(), Comparator{}));
        STATIC_REQUIRE(std::is_same<T, E>::value);
    }

    template<class E, class S, class T>
    void testPreparedStatement(S& storage, const T& statement) const {
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
    {
        using UserP = std::unique_ptr<User>;
        tester.testContainer<std::vector<UserP>>(storage.get_all_pointer<User>());
        {
            using Container = std::vector<UserP>;
            tester.testContainer<Container>(storage.get_all_pointer<User, Container>());
        }
        {
            using Container = std::deque<UserP>;
            tester.testContainer<Container>(storage.get_all_pointer<User, Container>());
        }
        {
            using Container = std::list<UserP>;
            tester.testContainer<Container>(storage.get_all_pointer<User, Container>());
        }
        tester.testPreparedStatement<std::vector<UserP>>(storage, storage.prepare(get_all_pointer<User>()));
        {
            using Container = std::vector<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_pointer<User, Container>()));
        }
        {
            using Container = std::deque<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_pointer<User, Container>()));
        }
        {
            using Container = std::list<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_pointer<User, Container>()));
        }
    }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    //  get_all_optional
    {
        using UserP = std::optional<User>;
        tester.testPreparedStatement<std::vector<UserP>>(storage, storage.prepare(get_all_optional<User>()));
        {
            using Container = std::vector<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_optional<User, Container>()));
        }
        {
            using Container = std::deque<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_optional<User, Container>()));
        }
        {
            using Container = std::list<UserP>;
            tester.testPreparedStatement<Container>(storage, storage.prepare(get_all_optional<User, Container>()));
        }
    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
}
