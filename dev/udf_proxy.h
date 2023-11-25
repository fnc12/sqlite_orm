#pragma once

#include <sqlite3.h>
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
            using func_call_fn_t = void (*)(void* udfHandle,
                                            sqlite3_context* context,
                                            int argsCount,
                                            sqlite3_value** values);
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
            func_call_fn_t func;
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
                      func_call_fn_t func,
                      final_call_fn_t finalAggregateCall,
                      memory_space udfMemory) :
                name{std::move(name)},
                argumentsCount{argumentsCount}, constructAt{std::move(constructAt)}, destroy{destroy}, func{func},
                finalAggregateCall{finalAggregateCall}, udfConstructed{false}, udfMemory{udfMemory} {}

            ~udf_proxy() {
                if(!constructAt && destroy) {
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

        inline void check_args_count(const udf_proxy* proxy, int argsCount) {
            if(proxy->argumentsCount != -1) {
                if(proxy->argumentsCount != argsCount &&
                   /*check fin call*/ argsCount != -1)
                    SQLITE_ORM_CPP_UNLIKELY {
                        throw std::system_error{orm_error_code::arguments_count_does_not_match};
                    }
            }
        }

        inline void ensure_udf(udf_proxy* proxy, int argsCount) {
            if(proxy->udfConstructed)
                SQLITE_ORM_CPP_LIKELY {
                    return;
                }
            check_args_count(proxy, argsCount);
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

        inline void scalar_function_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            check_args_count(proxy, argsCount);
            // 1. Thread-safe with regard to the construction/destruction of the function object in the same memory space.
            //    The `udf_proxy` is one instance per database connection,
            //    and SQLite internally locks access to the database object during the generation of a result row with `sqlite3_step()`.
            // 2. Note on the use of the `udfHandle` pointer after the object construction:
            //    since we only ever cast between void* and UDF* pointer types and
            //    only use the memory space for one type during the entire lifetime of a proxy,
            //    we can use `udfHandle` interconvertibly without laundering its provenance.
            proxy->constructAt(udfHandle(proxy));
            const std::unique_ptr<void, xdestroy_fn_t> udfGuard{udfHandle(proxy), proxy->destroy};
            proxy->func(udfHandle(proxy), context, argsCount, values);
        }

        inline void quoted_scalar_function_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            check_args_count(proxy, argsCount);
            proxy->func(udfHandle(proxy), context, argsCount, values);
        }

        SQLITE_ORM_INLINE_VAR constexpr auto stateless_scalar_function_callback = quoted_scalar_function_callback;

        inline void aggregate_function_step_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            ensure_udf(proxy, argsCount);
            proxy->func(udfHandle(proxy), context, argsCount, values);
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
