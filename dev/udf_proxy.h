#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <cassert>  //  assert macro
#include <type_traits>  //  std::true_type, std::false_type
#include <new>  //  std::bad_alloc
#include <memory>  //  std::allocator, std::allocator_traits, std::unique_ptr
#include <string>  //  std::string
#include <functional>  //  std::function
#include <utility>  //  std::move, std::pair
#endif

#include "error_code.h"

namespace sqlite_orm {
    namespace internal {
        /*
         *  Returns properly allocated memory space for the specified application-defined function object
         *  paired with an accompanying deallocation function.
         */
        template<class UDF>
        std::pair<void*, xdestroy_fn_t> preallocate_udf_memory() {
            std::allocator<UDF> allocator;
            using traits = std::allocator_traits<decltype(allocator)>;

            SQLITE_ORM_CONSTEXPR_LAMBDA_CPP17 auto deallocate = [](void* location) noexcept {
                std::allocator<UDF> allocator;
                using traits = std::allocator_traits<decltype(allocator)>;
                traits::deallocate(allocator, (UDF*)location, 1);
            };

            return {traits::allocate(allocator, 1), deallocate};
        }

        /*
         *  Returns a pair of functions to allocate/deallocate properly aligned memory space for the specified application-defined function object.
         */
        template<class UDF>
        std::pair<void* (*)(), xdestroy_fn_t> obtain_udf_allocator() {
            SQLITE_ORM_CONSTEXPR_LAMBDA_CPP17 auto allocate = []() {
                std::allocator<UDF> allocator;
                using traits = std::allocator_traits<decltype(allocator)>;
                return (void*)traits::allocate(allocator, 1);
            };

            SQLITE_ORM_CONSTEXPR_LAMBDA_CPP17 auto deallocate = [](void* location) noexcept {
                std::allocator<UDF> allocator;
                using traits = std::allocator_traits<decltype(allocator)>;
                traits::deallocate(allocator, (UDF*)location, 1);
            };

            return {allocate, deallocate};
        }

        /*
         *  A deleter that only destroys the application-defined function object.
         */
        struct udf_destruct_only_deleter {
            template<class UDF>
            void operator()(UDF* f) const noexcept {
                std::allocator<UDF> allocator;
                using traits = std::allocator_traits<decltype(allocator)>;
                traits::destroy(allocator, f);
            }
        };

        /*
         *  Stores type-erased information in relation to an application-defined scalar or aggregate function object:
         *  - name and argument count
         *  - function dispatch (step, final)
         *  - either preallocated memory with a possibly a priori constructed function object [scalar],
         *  - or memory allocation/deallocation functions [aggregate]
         */
        struct udf_proxy {
            using sqlite_func_t = void (*)(sqlite3_context* context, int argsCount, sqlite3_value** values);
            using final_call_fn_t = void (*)(void* udfHandle, sqlite3_context* context);
            using memory_alloc = std::pair<void* (*)() /*allocate*/, xdestroy_fn_t /*deallocate*/>;
            using memory_space = std::pair<void* /*udfHandle*/, xdestroy_fn_t /*deallocate*/>;

            std::string name;
            int argumentsCount;
            std::function<void(void* location)> constructAt;
            xdestroy_fn_t destroy;
            sqlite_func_t func;
            final_call_fn_t finalAggregateCall;

            // allocator/deallocator function pair for aggregate UDF
            const memory_alloc udfAllocator;
            // pointer to preallocated memory space for scalar UDF, already constructed by caller if stateless
            const memory_space udfMemorySpace;

            udf_proxy(std::string name,
                      int argumentsCount,
                      std::function<void(void* location)> constructAt,
                      xdestroy_fn_t destroy,
                      sqlite_func_t func,
                      memory_space udfMemorySpace) :
                name{std::move(name)}, argumentsCount{argumentsCount}, constructAt{std::move(constructAt)},
                destroy{destroy}, func{func}, finalAggregateCall{nullptr}, udfAllocator{},
                udfMemorySpace{udfMemorySpace} {}

            udf_proxy(std::string name,
                      int argumentsCount,
                      std::function<void(void* location)> constructAt,
                      xdestroy_fn_t destroy,
                      sqlite_func_t func,
                      final_call_fn_t finalAggregateCall,
                      memory_alloc udfAllocator) :
                name{std::move(name)}, argumentsCount{argumentsCount}, constructAt{std::move(constructAt)},
                destroy{destroy}, func{func}, finalAggregateCall{finalAggregateCall}, udfAllocator{udfAllocator},
                udfMemorySpace{} {}

            ~udf_proxy() {
                // destruct
                if (/*bool aprioriConstructed = */ !constructAt && destroy) {
                    destroy(udfMemorySpace.first);
                }
                // deallocate
                if (udfMemorySpace.second) {
                    udfMemorySpace.second(udfMemorySpace.first);
                }
            }

            udf_proxy(const udf_proxy&) = delete;
            udf_proxy& operator=(const udf_proxy&) = delete;

            // convenience accessors for better legibility;
            // [`friend` is intentional - it ensures that these are core accessors (only found via ADL), yet still be an out-of-class interface]

            friend void* preallocated_udf_handle(udf_proxy* proxy) {
                return proxy->udfMemorySpace.first;
            }

            friend void* allocate_udf(udf_proxy* proxy) {
                return proxy->udfAllocator.first();
            }

            friend void deallocate_udf(udf_proxy* proxy, void* udfHandle) {
                proxy->udfAllocator.second(udfHandle);
            }
        };

        // safety net of doing a triple check at runtime
        inline void assert_args_count(const udf_proxy* proxy, int argsCount) {
            assert((proxy->argumentsCount == -1) || (proxy->argumentsCount == argsCount ||
                                                     /*check fin call*/ argsCount == -1));
            (void)proxy;
            (void)argsCount;
        }

        // safety net of doing a triple check at runtime
        inline void proxy_assert_args_count(sqlite3_context* context, int argsCount) {
            udf_proxy* proxy;
            assert((proxy = static_cast<udf_proxy*>(sqlite3_user_data(context))) != nullptr);
            assert_args_count(proxy, argsCount);
            (void)context;
        }

        // note: may throw `std::bad_alloc` in case memory space for the aggregate function object cannot be allocated
        inline void* ensure_aggregate_udf(sqlite3_context* context, udf_proxy* proxy, int argsCount) {
            // reserve memory for storing a void pointer (which is the `udfHandle`, i.e. address of the aggregate function object)
            void* ctxMemory = sqlite3_aggregate_context(context, sizeof(void*));
            if (!ctxMemory) SQLITE_ORM_CPP_UNLIKELY {
                throw std::bad_alloc();
            }
            void*& udfHandle = *static_cast<void**>(ctxMemory);

            if (udfHandle) SQLITE_ORM_CPP_LIKELY {
                return udfHandle;
            } else {
                assert_args_count(proxy, argsCount);
                udfHandle = allocate_udf(proxy);
                // Note on the use of the `udfHandle` pointer after the object construction:
                // since we only ever cast between void* and UDF* pointer types and
                // only use the memory space for one type during the entire lifetime of a proxy,
                // we can use `udfHandle` interconvertibly without laundering its provenance.
                proxy->constructAt(udfHandle);
                return udfHandle;
            }
        }

        inline void delete_aggregate_udf(udf_proxy* proxy, void* udfHandle) {
            proxy->destroy(udfHandle);
            deallocate_udf(proxy, udfHandle);
        }

        // Return C pointer to preallocated and a priori constructed UDF
        template<class UDF>
        inline UDF*
        proxy_get_scalar_udf(std::true_type /*is_stateless*/, sqlite3_context* context, int argsCount) noexcept {
            proxy_assert_args_count(context, argsCount);
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            return static_cast<UDF*>(preallocated_udf_handle(proxy));
        }

        // Return unique pointer to newly constructed UDF at preallocated memory space
        template<class UDF>
        inline auto proxy_get_scalar_udf(std::false_type /*is_stateless*/, sqlite3_context* context, int argsCount) {
            proxy_assert_args_count(context, argsCount);
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            // Note on the use of the `udfHandle` pointer after the object construction:
            // since we only ever cast between void* and UDF* pointer types and
            // only use the memory space for one type during the entire lifetime of a proxy,
            // we can use `udfHandle` interconvertibly without laundering its provenance.
            proxy->constructAt(preallocated_udf_handle(proxy));
            return std::unique_ptr<UDF, xdestroy_fn_t>{static_cast<UDF*>(preallocated_udf_handle(proxy)),
                                                       proxy->destroy};
        }

        // note: may throw `std::bad_alloc` in case memory space for the aggregate function object cannot be allocated
        template<class UDF>
        inline UDF* proxy_get_aggregate_step_udf(sqlite3_context* context, int argsCount) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            void* udfHandle = ensure_aggregate_udf(context, proxy, argsCount);
            return static_cast<UDF*>(udfHandle);
        }

        inline void aggregate_function_final_callback(sqlite3_context* context) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            void* udfHandle;
            try {
                // note: it is possible that the 'step' function was never called
                udfHandle = ensure_aggregate_udf(context, proxy, -1);
            } catch (const std::bad_alloc&) {
                sqlite3_result_error_nomem(context);
                return;
            }
            proxy->finalAggregateCall(udfHandle, context);
            delete_aggregate_udf(proxy, udfHandle);
        }
    }
}
