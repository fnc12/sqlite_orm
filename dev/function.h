#pragma once

#include <functional>
#include <sqlite3.h>

namespace sqlite_orm {

    namespace internal {

        struct scalar_function_t {
            
            template<class T>
            scalar_function_t(T callable) {
                function = [](sqlite3_context *context, int argc, sqlite3_value **argv) {
                    
                };
            }
            
            std::function<void(sqlite3_context *context, int argc, sqlite3_value **argv)> function;
        };

    }

}
