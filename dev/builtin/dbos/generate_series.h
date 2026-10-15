#pragma once

#include "../../functional/gsl.h"
#include "../../schema/virtual_table.h"
#include "../../ast/table_reference.h"

#if SQLITE_VERSION_NUMBER >= 3008012
namespace sqlite_orm::internal {
    struct generate_series_module_tag : eponymous_module {
        // simplify conceptual/meta programming
        using module_type = generate_series_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "generate_series";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  Data structure for the `generate_series` eponymous virtual table.
     */
    struct generate_series {
        int value = 0;

        /** 
         *  Hidden columns of the `generate_series` virtual table.
         */
        struct hidden {
            int start;
            int stop;
            int step;

            using enclosing_type = generate_series;
        };
    };

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    inline constexpr orm_table_reference auto generate_series_table = c<generate_series>();
#else
    inline constexpr auto generate_series_table = c<generate_series>();
#endif
}

namespace sqlite_orm::internal {
    /**
     *  Factory function for a `generate_series` virtual table definition.
     *  
     *  The hidden generate_series columns 'start', 'stop' and 'step' are mapped into each table definition.
     */
    inline auto using_generate_series() {
        return make_virtual_table_definition<generate_series_module_tag>(
            make_column("value", &generate_series::value),
            make_hidden_column("start", &generate_series::hidden::start),
            make_hidden_column("stop", &generate_series::hidden::stop),
            make_hidden_column("step", &generate_series::hidden::step));
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for the `generate_series` default eponymous virtual table.
     */
    inline auto make_generate_series_table() {
        return make_virtual_table<generate_series>(internal::generate_series_module_tag::name(),
                                                   internal::using_generate_series());
    }
}
#endif
