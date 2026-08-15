#pragma once

#include "functional/cxx_string_view.h"

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <algorithm>  // std::ranges::find
#endif

#include "error_code.h"
#include "conditions.h"
#include "function.h"
#include "storage_base.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    /*
     *  AST iteration callable that matches function call node expressions.
     *  - Throws a `orm_error_code::function_not_found` exception
     *    if an application-defined scalar or aggregate function was not registered.
     *  - Throws a `sqlite_errc(SQLITE_ERROR_MISSING_COLLSEQ)` if a named collation function was not registered.
     */
    struct udf_existence_checker {
        const std::list<udf_proxy>& _scalarFunctions;
        const std::list<udf_proxy>& _aggregateFunctions;
        const std::map<std::string, storage_base::collating_function>& _collatingFunctions;

        // examine `function_call` node expressions
        template<class UDF, class... CallArgs>
        void operator()(std::true_type, const function_call<UDF, CallArgs...>& udfCall) const {
            auto&& name = udfCall.name();
            SQLITE_ORM_CPP_UNLIKELY {
                if (!_contains(_scalarFunctions, name) && !_contains(_aggregateFunctions, name))
                    throw std::system_error{orm_error_code::function_not_found, std::string(name)};
            }
        }

        // examine `named_collate` node expressions
        void operator()(std::true_type, const named_collate_base& collateCall) const {
            if (_collatingFunctions.find(collateCall.name) == _collatingFunctions.end()) SQLITE_ORM_CPP_UNLIKELY {
#if SQLITE_VERSION_NUMBER >= 3008008
                throw std::system_error{sqlite_errc(SQLITE_ERROR_MISSING_COLLSEQ), std::string(collateCall.name)};
#else
                throw std::system_error{sqlite_errc(SQLITE_ERROR), std::string(collateCall.name)};
#endif
            }
        }

        // swallow leaf expressions
        template<class T>
        SQLITE_ORM_STATIC_CALLOP void operator()(const T&) SQLITE_ORM_OR_CONST_CALLOP {}

        static bool _contains(const std::list<udf_proxy>& functions, const std::string_view& name) {
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
            auto it = std::ranges::find(functions, name, &udf_proxy::name);
#else
            auto it = std::find_if(functions.begin(), functions.end(), [&name](const udf_proxy& udfProxy) {
                return udfProxy.name == name;
            });
#endif
            return it != functions.end();
        }
    };
#endif
}
