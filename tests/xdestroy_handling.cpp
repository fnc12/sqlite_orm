#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <memory>  // std::default_delete
#include <stdlib.h>  // free()

using namespace sqlite_orm;
using std::default_delete;
using std::unique_ptr;

// Wrap std::default_delete in a function
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
template<typename T>
void delete_default(std::conditional_t<std::is_array<T>::value, std::decay_t<T>, T*> o) noexcept(
    noexcept(std::default_delete<T>{}(o))) {
    std::default_delete<T>{}(o);
}

// Integral function constant for default deletion
template<typename T>
using delete_default_t = std::integral_constant<decltype(&delete_default<T>), delete_default<T>>;
// Integral function constant variable for default deletion
template<typename T>
SQLITE_ORM_INLINE_VAR constexpr delete_default_t<T> delete_default_f{};
#endif

using free_t = std::integral_constant<decltype(&free), free>;
SQLITE_ORM_INLINE_VAR constexpr free_t free_f{};

TEST_CASE("obtain_xdestroy_for") {

    using internal::xdestroy_proxy;

    // class yielding a 'xDestroy' function pointer
    struct xdestroy_holder {
        xdestroy_fn_t xDestroy = free;

        constexpr operator xdestroy_fn_t() const noexcept {
            return xDestroy;
        }
    };

    // class yielding a function pointer not of type xdestroy_fn_t
    struct int_destroy_holder {
        using destroy_fn_t = void (*)(int*);

        destroy_fn_t destroy = nullptr;

        constexpr operator destroy_fn_t() const noexcept {
            return destroy;
        }
    };

    {
        constexpr int* int_nullptr = nullptr;
#if !defined(SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION) ||                                                             \
    (__cpp_constexpr >= 201907L)  //  Trivial default initialization in constexpr functions
        constexpr const int* const_int_nullptr = nullptr;
#endif

        // null_xdestroy_f(int*)
        constexpr xdestroy_fn_t xDestroy1 = obtain_xdestroy_for(null_xdestroy_f, int_nullptr);
        STATIC_REQUIRE(xDestroy1 == nullptr);
        REQUIRE(xDestroy1 == nullptr);

        // free(int*)
        constexpr xdestroy_fn_t xDestroy2 = obtain_xdestroy_for(free, int_nullptr);
        STATIC_REQUIRE(xDestroy2 == &free);
        REQUIRE(xDestroy2 == &free);

        // free_f(int*)
        constexpr xdestroy_fn_t xDestroy3 = obtain_xdestroy_for(free_f, int_nullptr);
        STATIC_REQUIRE(xDestroy3 == &free);
        REQUIRE(xDestroy3 == &free);

#if __cpp_constexpr >= 201603L  //  constexpr lambda
        // [](void* p){}
        constexpr auto lambda4_1 = [](void*) {};
        constexpr xdestroy_fn_t xDestroy4_1 = obtain_xdestroy_for(lambda4_1, int_nullptr);
        STATIC_REQUIRE(xDestroy4_1 == lambda4_1);
        REQUIRE(xDestroy4_1 == lambda4_1);
#else
#if !defined(_MSC_VER) || (_MSC_VER >= 1914)  //  conversion of lambda closure to function pointer using `+`
        // [](void* p){}
        auto lambda4_1 = [](void*) {};
        xdestroy_fn_t xDestroy4_1 = obtain_xdestroy_for(lambda4_1, int_nullptr);
        REQUIRE(xDestroy4_1 == lambda4_1);
#endif
#endif

        // [](int* p) { delete p; }
#if __cplusplus >= 202002L  //  default-constructible non-capturing lambdas
        constexpr auto lambda4_2 = [](int* p) {
            delete p;
        };
        using lambda4_2_t = std::remove_const_t<decltype(lambda4_2)>;
        constexpr xdestroy_fn_t xDestroy4_2 = obtain_xdestroy_for(lambda4_2, int_nullptr);
        STATIC_REQUIRE(xDestroy4_2 == &xdestroy_proxy<lambda4_2_t, int>);
        REQUIRE((xDestroy4_2 == &xdestroy_proxy<lambda4_2_t, int>));
#endif

        // default_delete<int>(int*)
        constexpr xdestroy_fn_t xDestroy5 = obtain_xdestroy_for(default_delete<int>{}, int_nullptr);
        STATIC_REQUIRE(xDestroy5 == &xdestroy_proxy<default_delete<int>, int>);
        REQUIRE((xDestroy5 == &xdestroy_proxy<default_delete<int>, int>));

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        // delete_default_f<int>(int*)
        constexpr xdestroy_fn_t xDestroy6 = obtain_xdestroy_for(delete_default_f<int>, int_nullptr);
        STATIC_REQUIRE(xDestroy6 == &xdestroy_proxy<delete_default_t<int>, int>);
        REQUIRE((xDestroy6 == &xdestroy_proxy<delete_default_t<int>, int>));

        // delete_default_f<int>(const int*)
        constexpr xdestroy_fn_t xDestroy7 = obtain_xdestroy_for(delete_default_f<int>, const_int_nullptr);
        STATIC_REQUIRE(xDestroy7 == &xdestroy_proxy<delete_default_t<int>, const int>);
        REQUIRE((xDestroy7 == &xdestroy_proxy<delete_default_t<int>, const int>));
#endif

#if __cpp_constexpr >= 201907L  //  Trivial default initialization in constexpr functions
        // xdestroy_holder{ free }(int*)
        constexpr xdestroy_fn_t xDestroy8 = obtain_xdestroy_for(xdestroy_holder{free}, int_nullptr);
        STATIC_REQUIRE(xDestroy8 == &free);
        REQUIRE(xDestroy8 == &free);

        // xdestroy_holder{ free }(const int*)
        constexpr xdestroy_fn_t xDestroy9 = obtain_xdestroy_for(xdestroy_holder{free}, const_int_nullptr);
        STATIC_REQUIRE(xDestroy9 == &free);
        REQUIRE(xDestroy9 == &free);

        // xdestroy_holder{ nullptr }(const int*)
        constexpr xdestroy_fn_t xDestroy10 = obtain_xdestroy_for(xdestroy_holder{nullptr}, const_int_nullptr);
        STATIC_REQUIRE(xDestroy10 == nullptr);
        REQUIRE(xDestroy10 == nullptr);
#endif

        // expressions that do not work
#if 0
    // can't use functions that differ from xdestroy_fn_t
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(delete_default<int>, int_nullptr);
    // can't use object yielding a function pointer that differs from xdestroy_fn_t
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(int_destroy_holder{}, int_nullptr);
    // successfully takes default_delete<void>, but default_delete statically asserts on a non-complete type `void*`
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(default_delete<void>{}, int_nullptr);
    // successfully takes default_delete<int>, but xdestroy_proxy can't call the deleter with a `const int*`
    constexpr xdestroy_fn_t xDestroy = obtain_xdestroy_for(default_delete<int>{}, const_int_nullptr);
#endif
    }
}
