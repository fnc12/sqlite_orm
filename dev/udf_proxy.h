#pragma once

#include <sqlite3.h>
#include <new>  //  std::launder
#include <string>  //  std::string
#include <functional>  //  std::function
#include <utility>  //  std::move

#include "error_code.h"

namespace sqlite_orm {
    namespace internal {
        /*
         *  Stores type-erased information in relation to a user-defined scalar or aggregate function object:
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

            struct destruct_only_deleter {
                template<class F>
                void operator()(F* f) const noexcept {
                    f->~F();
                }
            };

            std::string name;
            int argumentsCount;
            std::function<void*(void* location)> constructAt;
            xdestroy_fn_t destroy;
            func_call_fn_t func;
            final_call_fn_t finalAggregateCall;
            // flag whether the UDF has been constructed at `udfHandle`;
            // necessary for aggregation operations
            bool constructed;
            // pointer to memory for UDF in derived proxy veneer
            void* const udfHandle;
        };

        /*
         *  A veneer to `udf_proxy` that provides memory space for a user-defined function object.
         *  
         *  Note: it must be a veneer, i.e. w/o any non-trivially destructible member variables.
         */
        template<class UDF>
        struct udf_proxy_veneer : udf_proxy {
            // allocated memory for user-defined function
            alignas(UDF) char storage[sizeof(UDF)];

            udf_proxy_veneer(std::string name,
                             int argumentsCount,
                             std::function<void*(void* location)> constructAt,
                             xdestroy_fn_t destroy,
                             func_call_fn_t run) :
                udf_proxy {
                std::move(name), argumentsCount, std::move(constructAt), destroy, run, nullptr, false,
#if __cpp_lib_launder >= 201606L
                    std::launder((UDF*)this->storage)
#else
                    storage
#endif
            }
            {}

            udf_proxy_veneer(std::string name,
                             int argumentsCount,
                             std::function<void*(void* location)> constructAt,
                             xdestroy_fn_t destroy,
                             func_call_fn_t step,
                             final_call_fn_t finalCall) :
                udf_proxy {
                std::move(name), argumentsCount, std::move(constructAt), destroy, step, finalCall, false,
#if __cpp_lib_launder >= 201606L
                    std::launder((UDF*)this->storage)
#else
                    storage
#endif
            }
            {}
        };

        inline void scalar_function_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            if(proxy->argumentsCount != -1 && proxy->argumentsCount != argsCount) {
                throw std::system_error{orm_error_code::arguments_count_does_not_match};
            }
            const std::unique_ptr<void, xdestroy_fn_t> udfHandle{proxy->constructAt(proxy->udfHandle), proxy->destroy};
            proxy->func(udfHandle.get(), context, argsCount, values);
        }

        inline void aggregate_function_step_callback(sqlite3_context* context, int argsCount, sqlite3_value** values) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            if(!proxy->constructed) {
                if(proxy->argumentsCount != -1 && proxy->argumentsCount != argsCount) {
                    throw std::system_error{orm_error_code::arguments_count_does_not_match};
                }
                proxy->constructAt(proxy->udfHandle);
                proxy->constructed = true;
            }
            proxy->func(proxy->udfHandle, context, argsCount, values);
        }

        inline void aggregate_function_final_callback(sqlite3_context* context) {
            udf_proxy* proxy = static_cast<udf_proxy*>(sqlite3_user_data(context));
            // note: it is possible that the 'step' function was never called
            if(!proxy->constructed) {
                proxy->constructAt(proxy->udfHandle);
                proxy->constructed = true;
            }
            proxy->finalAggregateCall(proxy->udfHandle, context);
            proxy->destroy(proxy->udfHandle);
            proxy->constructed = false;
        }
    }
}
