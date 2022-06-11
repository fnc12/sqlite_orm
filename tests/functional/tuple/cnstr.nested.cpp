#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

// See this bug: https://llvm.org/bugs/show_bug.cgi?id=24173

namespace {
    template<typename... X>
    constexpr mpl::tuple<std::decay_t<X>...> f(X&&... xs) {
        return mpl::tuple<std::decay_t<X>...>{std::forward<X>(xs)...};
    }

    template<typename... X>
    constexpr mpl::uple<std::decay_t<X>...> g(X&&... xs) {
        return mpl::uple<std::decay_t<X>...>{std::forward<X>(xs)...};
    }
}

TEST_CASE("tuple - nested") {
    f(f(f(f(f(f(f(f(f(f(f(f(1))))))))))));
    g(g(g(g(g(g(g(g(g(g(g(g(1))))))))))));
}
