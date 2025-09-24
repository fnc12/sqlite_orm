#pragma once

#include <sqlite3.h>
#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#include <type_traits>  //  std::is_aggregate
#include <utility>  //  std::move
#include <functional>  //  std::function
#endif

#include "serialize_result_type.h"

namespace sqlite_orm {
    namespace internal {
        template<typename T>
        using storage_opt_tag_t = typename T::storage_opt_tag;

        struct on_open_spec {
            using storage_opt_tag = int;

            std::function<void(sqlite3*)> onOpen;
        };
        struct will_run_query_spec {
            using storage_opt_tag = int;

            std::function<void(serialize_arg_type)> willRunQuery;
        };
        struct did_run_query_spec {
            using storage_opt_tag = int;

            std::function<void(serialize_arg_type)> didRunQuery;
        };
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  Database connection control options to be passed to `make_storage()`.
     */
    struct connection_control {
        /// Whether to open the database once and for all.
        /// Required if using a 'storage' instance from multiple threads.
        bool open_forever = false;
        std::string vfs_name{default_vfs_name};
        db_open_mode open_mode = db_open_mode::default_;

        using storage_opt_tag = int;
    };
#if __cpp_lib_is_aggregate >= 201703L
    // design choice: must be an aggregate that can be constructed using designated initializers
    static_assert(std::is_aggregate_v<connection_control>);
#endif

#ifdef SQLITE_ORM_CTAD_SUPPORTED
    /** 
     *  Callback function to be passed to `make_storage()`.
     *  The provided function is called immdediately after the database connection has been established and set up.
     */
    inline internal::on_open_spec on_open(std::function<void(sqlite3*)> onOpen) {
        return {std::move(onOpen)};
    }
#endif
    inline internal::will_run_query_spec
    will_run_query(std::function<void(internal::serialize_arg_type)> willRunQuery) {
        return {std::move(willRunQuery)};
    }

    inline internal::did_run_query_spec did_run_query(std::function<void(internal::serialize_arg_type)> didRunQuery) {
        return {std::move(didRunQuery)};
    }
}
