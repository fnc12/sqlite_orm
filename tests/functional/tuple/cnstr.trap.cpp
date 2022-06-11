#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

// Make sure that the tuple(Yn&&...) is not preferred over copy constructors
// in single-element cases and other similar cases.

namespace {
    struct Trap1 {
        Trap1() = default;
        Trap1(Trap1 const&) = default;
#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
        Trap1(Trap1&) = default;
#endif
        Trap1(Trap1&&) = default;

        template<typename X>
        Trap1(X&&) {
            static_assert(sizeof(X) && false, "this constructor must not be instantiated");
        }
    };

    struct Trap2 {
        Trap2() = default;
        Trap2(Trap2 const&) = default;
#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
        Trap2(Trap2&) = default;
#endif
        Trap2(Trap2&&) = default;

        template<typename X>
        Trap2(X) {  // not by reference
            static_assert(sizeof(X) && false, "this constructor must not be instantiated");
        }
    };

    struct Trap3 {
        Trap3() = default;
        Trap3(Trap3 const&) = default;
#ifndef SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
        Trap3(Trap3&) = default;
#endif
        Trap3(Trap3&&) = default;

        template<typename X>
        constexpr explicit Trap3(X&&) {  // explicit, and constexpr
            static_assert(sizeof(X) && false, "this constructor must not be instantiated");
        }
    };

    struct Trap4 {
        Trap4() = default;
        template<typename Args>
        constexpr explicit Trap4(Args&&) {
            static_assert(sizeof(Args) && false, "must never be instantiated");
        }

        Trap4(Trap4 const&) = default;
        Trap4(Trap4&&) = default;
    };
}

TEMPLATE_PRODUCT_TEST_CASE("tuple - constructors trap",
                           "[tuple][trap]",
                           (mpl::tuple, mpl::uple),
                           (Trap1, Trap2, Trap3)) {
    TestType tuple{};
    TestType implicit_copy = tuple;
    TestType explicit_copy(tuple);
    TestType implicit_move = std::move(tuple);
    TestType explicit_move(std::move(tuple));

    (void)implicit_copy;
    (void)explicit_copy;
    (void)implicit_move;
    (void)explicit_move;
}

TEMPLATE_PRODUCT_TEST_CASE("tuple - element constructor trap", "[tuple]", (mpl::tuple, mpl::uple), (Trap4)) {
    struct Foo {
        Foo() = default;
        Foo(Foo const&) = default;
        Foo(Foo&&) = default;
        TestType t;
    };
}
