#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using std::tuple;
using namespace sqlite_orm;
using internal::structure;
using internal::table_reference;

template<class Type, class E>
void do_assert() {
    STATIC_REQUIRE(std::is_same<Type, E>::value);
}

template<class DBOs, class E, class V>
void runTest(V /*value*/) {
    do_assert<internal::column_result_of_t<DBOs, V>, E>();
}

TEST_CASE("column_result_of_t") {
    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        void setId(int value) {
            this->id = value;
        }

        int getId() const {
            return this->id;
        }

        void setComment(std::string comment) {
            this->comment = std::move(comment);
        }

        const std::string& getComment() const {
            return this->comment;
        }

      private:
        int id = 0;
        std::string comment;
    };
    auto dbObjects =
        std::make_tuple(make_table("users", make_column("id", &User::id), make_column("name", &User::name)));
    using db_objects_t = decltype(dbObjects);

    runTest<db_objects_t, int>(&User::id);
    runTest<db_objects_t, std::string>(&User::name);
    {
        int n = 0;
        runTest<db_objects_t, int>(std::ref(n));
    }
    runTest<db_objects_t, bool>(in(&User::id, {1, 2, 3}));
    {
        std::vector<int> vector;
        vector.insert(vector.cend(), {1, 2, 3});
        runTest<db_objects_t, bool>(in(&User::id, vector));
    }
    runTest<db_objects_t, bool>(in(&User::id, select(&User::id)));
    runTest<db_objects_t, bool>(c(&User::id).in(1, 2, 3));
    runTest<db_objects_t, int>(&Visit::getId);
    runTest<db_objects_t, std::string>(&Visit::getComment);
    runTest<db_objects_t, int>(&Visit::setId);
    runTest<db_objects_t, std::string>(&Visit::setComment);
    runTest<db_objects_t, std::unique_ptr<double>>(sqlite_orm::abs(&User::id));
    runTest<db_objects_t, int>(sqlite_orm::length(&User::id));
    runTest<db_objects_t, int>(sqlite_orm::unicode(&User::id));
    runTest<db_objects_t, std::string>(sqlite_orm::typeof_(&User::id));
    runTest<db_objects_t, std::string>(sqlite_orm::lower(&User::id));
    runTest<db_objects_t, std::string>(sqlite_orm::upper(&User::id));
    runTest<db_objects_t, std::unique_ptr<int>>(max(&User::id, 4));
    runTest<db_objects_t, std::unique_ptr<int>>(min(&User::id, 4));
    runTest<db_objects_t, std::unique_ptr<int>>(max(&User::id));
    runTest<db_objects_t, std::unique_ptr<std::string>>(max(&User::name));
    runTest<db_objects_t, std::unique_ptr<int>>(min(&User::id));
    runTest<db_objects_t, std::unique_ptr<std::string>>(min(&User::name));
    runTest<db_objects_t, int>(count<User>());
    runTest<db_objects_t, int>(count());
    {
        struct RandomFunc {
            static const char* name();
            int operator()() const {
                return 4;
            }
        };
        runTest<db_objects_t, int>(func<RandomFunc>());
    }
    runTest<db_objects_t, int>(distinct(&User::id));
    runTest<db_objects_t, std::string>(distinct(&User::name));
    runTest<db_objects_t, int>(all(&User::id));
    runTest<db_objects_t, std::string>(all(&User::name));
    runTest<db_objects_t, std::string>(conc(&User::name, &User::id));
    runTest<db_objects_t, std::string>(c(&User::name) || &User::id);
    runTest<db_objects_t, double>(minus(&User::id));
    runTest<db_objects_t, double>(-c(&User::id));
    runTest<db_objects_t, double>(add(&User::id, 5));
    runTest<db_objects_t, double>(c(&User::id) + 5);
    runTest<db_objects_t, double>(sub(&User::id, 5));
    runTest<db_objects_t, double>(c(&User::id) - 5);
    runTest<db_objects_t, double>(mul(&User::id, 5));
    runTest<db_objects_t, double>(c(&User::id) * 5);
    runTest<db_objects_t, double>(sqlite_orm::div(&User::id, 5));
    runTest<db_objects_t, double>(c(&User::id) / 5);
    runTest<db_objects_t, double>(mod(&User::id, 5));
    runTest<db_objects_t, double>(c(&User::id) % 5);
    runTest<db_objects_t, int>(bitwise_not(&User::id));
    runTest<db_objects_t, int>(~c(&User::id));
    runTest<db_objects_t, int>(bitwise_shift_left(&User::id, 4));
    runTest<db_objects_t, int>(c(&User::id) << 4);
    runTest<db_objects_t, int>(bitwise_shift_right(&User::id, 4));
    runTest<db_objects_t, int>(c(&User::id) >> 4);
    runTest<db_objects_t, int>(bitwise_and(&User::id, 4));
    runTest<db_objects_t, int>(c(&User::id) & 4);
    runTest<db_objects_t, int>(bitwise_or(&User::id, 4));
    runTest<db_objects_t, int>(c(&User::id) | 4);
    runTest<db_objects_t, int64>(rowid());
    runTest<db_objects_t, int64>(oid());
    runTest<db_objects_t, int64>(_rowid_());
    runTest<db_objects_t, int64>(rowid<User>());
    runTest<db_objects_t, int64>(oid<User>());
    runTest<db_objects_t, int64>(_rowid_<User>());
    runTest<db_objects_t, tuple<int, std::string>>(columns(&User::id, &User::name));
    runTest<db_objects_t, tuple<int, std::string>>(asterisk<User>());
    runTest<db_objects_t, tuple<int, std::string>>(asterisk<alias_a<User>>());
    runTest<db_objects_t, tuple<int, std::string, int, std::string>>(columns(asterisk<User>(), asterisk<User>()));
    runTest<db_objects_t, int>(column<User>(&User::id));
    runTest<db_objects_t, int>(alias_column<alias_a<User>>(&User::id));
    runTest<db_objects_t, table_reference<User>>(object<User>());
    runTest<db_objects_t, tuple<table_reference<User>, table_reference<User>>>(columns(object<User>(), object<User>()));
    runTest<db_objects_t, structure<User, tuple<int, std::string>>>(struct_<User>(&User::id, &User::name));
    runTest<db_objects_t, structure<User, tuple<int, std::string>>>(struct_<User>(asterisk<User>()));
    runTest<db_objects_t, tuple<structure<User, tuple<int, std::string>>, structure<User, tuple<int, std::string>>>>(
        columns(struct_<User>(asterisk<User>()), struct_<User>(asterisk<User>())));
    runTest<db_objects_t, int>(union_all(select(1), select(2)));
    runTest<db_objects_t, int64>(union_all(select(1ll), select(2)));
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    using cte_1 = decltype(1_ctealias);
    // note: even though used with the CTE, &User::id doesn't need to be mapped into the CTE to make column results work;
    //       this is because the result type is taken from the member pointer just because we can't look it up in the storage definition
    auto dbObjects2 =
        internal::db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cte<cte_1>().as(select(1))));
    using db_objects2_t = decltype(dbObjects2);
    runTest<db_objects_t, int>(column<cte_1>(&User::id));
    runTest<db_objects2_t, int>(column<cte_1>(1_colalias));
    runTest<db_objects2_t, int>(column<cte_1>(get<internal::column_alias<'1'>>()));
    runTest<db_objects_t, int>(alias_column<alias_a<cte_1>>(&User::id));
    runTest<db_objects2_t, int>(alias_column<alias_a<cte_1>>(1_colalias));
    runTest<db_objects2_t, tuple<int>>(asterisk<cte_1>());
    runTest<db_objects2_t, tuple<int>>(asterisk<alias_a<cte_1>>());
    runTest<db_objects_t, int>(count<cte_1>());
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr auto cte1 = 1_ctealias;
    runTest<db_objects_t, int>(column<cte1>(&User::id));
    runTest<db_objects2_t, int>(column<cte1>(1_colalias));
    runTest<db_objects2_t, int>(column<cte1>(get<1_colalias>()));
    runTest<db_objects_t, int>(cte1->*&User::id);
    runTest<db_objects_t, int>(count<cte1>());
#endif
#endif
}
