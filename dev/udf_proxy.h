#pragma once

#include <sqlite3.h>
#include <cassert>  //  assert
#include <memory>  //  std::allocator, std::allocator_traits, std::unique_ptr
#include <string>  //  std::string
#include <functional>  //  std::function
#include <utility>  //  std::move, std::pair

#include "error_code.h"

namespace sqlite_orm {
    namespace internal {
        /*
         *  Returns allocated memory space for the specified application-defined function
         *  paired with a deallocation function.
         */
        template<class UDF>
        std::pair<void*, xdestroy_fn_t> allocate_udf_storage() {
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
         *  Stores type-erased information in relation to a application-defined scalar or aggregate function object:
         *  - name and argument count
         *  - function pointers for construction/destruction
         *  - function dispatch
         *  - allocated memory and location
         *  
         *  As such, it also serves as a context for aggregation operations instead of using `sqlite3_aggregate_context()`.
         */
        struct udf_proxy {
            using sqlite_func_t = void (*)(sqlite3_context* context, int argsCount, sqlite3_value** values);
            using final_call_fn_t = void (*)(void* udfHandle, sqlite3_context* context);
            using memory_space = std::pair<void* /*udfHandle*/, xdestroy_fn_t /*deallocate*/>;

            struct destruct_only_deleter {
                template<class UDF>
                void operator()(UDF* f) const noexcept {
                    std::allocator<UDF> allocator;
                    using traits = std::allocator_traits<decltype(allocator)>;
                    traits::destroy(allocator, f);
                }
            };

            std::string name;
            int argumentsCount;
            std::function<void(void* location)> constructAt;
            xdestroy_fn_t destroy;
            sqlite_func_t func;
            final_call_fn_t finalAggregateCall;

            // flag whether the UDF has been constructed at `udfHandle`;
            // necessary for aggregation operations
            bool udfConstructed;
            // pointer to memory space for UDF
            const memory_space udfMemory;

            udf_proxy(std::string name,
                      int argumentsCount,
                      std::function<void(void* location)> constructAt,
                      xdestroy_fn_t destroy,
                      sqlite_func_t func,
                      final_call_fn_t finalAggregateCall,
                      memory_space udfMemory) :
                name{std::move(name)},
                argumentsCount{argumentsCount}, constructAt{std::move(constructAt)}, destroy{destroy}, func{func},
                finalAggregateCall{finalAggregateCall}, udfConstructed{false}, udfMemory{udfMemory} {}

            ~udf_proxy() {
                if(/*bool constructedOnce = */ !constructAt && destroy) {
                    destroy(udfMemory.first);
                }
                if(udfMemory.second) {
                    udfMemory.second(udfMemory.first);
                }
            }

            friend void* udfHandle(udf_proxy* proxy) {
                return proxy->udfMemory.first;
            }

            udf_proxy(const udf_proxy&) = delete;
            udf_proxy& operator=(const udf_proxy&) = delete;
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

        inline void ensure_udf(udf_proxy* proxy, int argsCount) {
            if(proxy->udfConstructed)
                SQLITE_ORM_CPP_LIKELY {
                    return;
                }
            assert_args_count(proxy, argsCount);
            // Note on the use of the `udfHandle` pointer after the object construction:
            // since we only ever cast between void* and UDF* pointer types and
            // only use the memory space for one type during the entire lifetime of a proxy,
            // we can use `udfHandle` interconvertibly without laundering its provenance.
            proxy->constructAt(udfHandle(proxy));
            proxy->udfConstructed = true;
        }

        inline void destruct_udf(udf_proxy* proxy) {
            proxy->udfConstructed = false;
            proxy->destroy(udfHandle(proxy));
        }

        inline auto new_scalar_udf_handle(sqlite3_context* context, int argsCount) {
            proxy_assert_args_count(context, argsCount);
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            // Note on the use of the `udfHandle` pointer after the object construction:
            // since we only ever cast between void* and UDF* pointer types and
            // only use the memory space for one type during the entire lifetime of a proxy,
            // we can use `udfHandle` interconvertibly without laundering its provenance.
            proxy->constructAt(udfHandle(proxy));
            return std::unique_ptr<void, xdestroy_fn_t>{udfHandle(proxy), proxy->destroy};
        }

        template<class UDF>
        inline UDF* proxy_get_scalar_udf(std::true_type /*is_stateless*/, sqlite3_context* context, int argsCount) {
            proxy_assert_args_count(context, argsCount);
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            return static_cast<UDF*>(udfHandle(proxy));
        }

        template<class UDF>
        inline auto proxy_get_scalar_udf(std::false_type /*is_stateless*/, sqlite3_context* context, int argsCount) {
            std::unique_ptr<void, xdestroy_fn_t> p = new_scalar_udf_handle(context, argsCount);
            return std::unique_ptr<UDF, xdestroy_fn_t>{static_cast<UDF*>(p.release()), p.get_deleter()};
        }

        template<class UDF>
        inline UDF* proxy_get_aggregate_step_udf(sqlite3_context* context, int argsCount) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            ensure_udf(proxy, argsCount);
            return static_cast<UDF*>(udfHandle(proxy));
        }

        inline void aggregate_function_final_callback(sqlite3_context* context) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            // note: it is possible that the 'step' function was never called
            ensure_udf(proxy, -1);
            proxy->finalAggregateCall(udfHandle(proxy), context);
            destruct_udf(proxy);
        }
    }
}
