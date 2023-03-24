#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using std::unique_ptr;

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
namespace {
    struct delete_int64 {
        static int64 lastSelectedId;
        static bool deleted;

        void operator()(int64* p) const {
            // must not double-delete
            REQUIRE(!deleted);

            lastSelectedId = *p;
            delete p;
            deleted = true;
        }
    };

    int64 delete_int64::lastSelectedId = -1;
    bool delete_int64::deleted = false;

    struct Object {
        int64 id = 0;
    };
}

TEST_CASE("pointer-passing") {
    // accept and return a pointer of type "carray"
    struct pass_thru_pointer_fn {
        using bindable_carray_ptr_t = static_carray_pointer_binding<int64>;

        bindable_carray_ptr_t operator()(carray_pointer_arg<int64> pv) const {
            return rebind_statically(pv);
        }

        static const char* name() {
            return "pass_thru_pointer";
        }
    };

    // return a pointer of type "carray"
    struct make_pointer_fn {
        using bindable_carray_ptr_t = carray_pointer_binding<int64, delete_int64>;

        bindable_carray_ptr_t operator()() const {
            return bindable_pointer<bindable_carray_ptr_t>(new int64{-1});
            // outline: low-level; must compile
            return bindable_carray_pointer(new int64{-1}, delete_int64{});
        }

        static const char* name() {
            return "make_pointer";
        }
    };

    // return value from a pointer of type "carray"
    struct fetch_from_pointer_fn {
        int64 operator()(carray_pointer_arg<const int64> pv) const {
            if(const int64* v = pv) {
                return *v;
            }
            return 0;
        }

        static const char* name() {
            return "fetch_from_pointer";
        }
    };

    auto storage =
        make_storage("", make_table("objects", make_column("id", &Object::id, primary_key().autoincrement())));
    storage.sync_schema();

    storage.insert(Object{});

    storage.create_scalar_function<note_value_fn<int64>>();
    storage.create_scalar_function<make_pointer_fn>();
    storage.create_scalar_function<fetch_from_pointer_fn>();
    storage.create_scalar_function<pass_thru_pointer_fn>();

    // test the note_value function
    SECTION("note_value, statically_bindable_pointer") {
        int64 lastUpdatedId = -1;
        storage.update_all(set(
            c(&Object::id) =
                add(1ll,
                    func<note_value_fn<int64>>(&Object::id, statically_bindable_pointer<carray_pvt>(&lastUpdatedId)))));
        REQUIRE(lastUpdatedId == 1);
        storage.update_all(set(
            c(&Object::id) =
                add(1ll, func<note_value_fn<int64>>(&Object::id, statically_bindable_carray_pointer(&lastUpdatedId)))));
        REQUIRE(lastUpdatedId == 2);
    }

    // test passing a pointer into another function
    SECTION("test_pass_thru, statically_bindable_pointer") {
        int64 lastSelectedId = -1;
        auto v = storage.select(func<note_value_fn<int64>>(
            &Object::id,
            func<pass_thru_pointer_fn>(statically_bindable_carray_pointer(&lastSelectedId))));
        REQUIRE(v.back() == lastSelectedId);
    }

    SECTION("bindable_pointer") {
        delete_int64::lastSelectedId = -1;
        delete_int64::deleted = false;

        SECTION("unbound is deleted") {
            try {
                unique_ptr<int64, delete_int64> x{new int64(42)};
                auto ast = select(func<fetch_from_pointer_fn>(bindable_pointer<carray_pvt>(std::move(x))));
                auto stmt = storage.prepare(std::move(ast));
                throw std::system_error{0, std::system_category()};
            } catch(const std::system_error&) {
            }
            // unbound pointer value must be deleted in face of exceptions (unregistered sql function)
            REQUIRE(delete_int64::deleted == true);
        }

        SECTION("deleted with prepared statement") {
            {
                unique_ptr<int64, delete_int64> x{new int64(42)};
                auto ast = select(func<fetch_from_pointer_fn>(bindable_pointer<carray_pvt>(std::move(x))));
                auto stmt = storage.prepare(std::move(ast));

                storage.execute(stmt);
                // bound pointer value must not be deleted while executing statements
                REQUIRE(delete_int64::deleted == false);
                storage.execute(stmt);
                REQUIRE(delete_int64::deleted == false);
            }
            // bound pointer value must be deleted when prepared statement is going out of scope
            REQUIRE(delete_int64::deleted == true);
        }

        SECTION("ownership transfer") {
            auto ast = select(func<note_value_fn<int64>>(&Object::id, func<make_pointer_fn>()));
            auto stmt = storage.prepare(std::move(ast));

            auto results = storage.execute(stmt);
            // returned pointers must be deleted by sqlite after executing the statement
            REQUIRE(delete_int64::deleted == true);
            REQUIRE(results.back() == delete_int64::lastSelectedId);
        }

        // test passing a pointer into another function
        SECTION("test_pass_thru") {
            auto v = storage.select(func<note_value_fn<int64>>(
                &Object::id,
                func<pass_thru_pointer_fn>(bindable_carray_pointer(new int64{-1}, delete_int64{}))));
            REQUIRE(delete_int64::deleted == true);
            REQUIRE(v.back() == delete_int64::lastSelectedId);
        }
    }
}
#endif
