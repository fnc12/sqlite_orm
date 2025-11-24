#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3008012
#include <tuple>  // std::tuple_element, std::make_tuple
#include <utility>  // std::forward
#endif
#endif

#include "../functional/gsl.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"
#include "../table_reference.h"

#if SQLITE_VERSION_NUMBER >= 3008012
namespace sqlite_orm::internal {
    struct generate_series_module_tag {
        // simplify conceptual/meta programming
        using module_type = generate_series_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "generate_series";
        }
    };

    template<>
    struct virtual_table_module_traits<generate_series_module_tag> {
        using module_type = generate_series_module_tag;
        using is_eponymous = std::true_type;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    template<class... Cs>
    struct virtual_table_traits<generate_series_module_tag, Cs...>
        : virtual_table_module_traits<generate_series_module_tag> {
        using definition_type = table_definition<Cs...>;
        using elements_type = typename definition_type::elements_type;
    };

    template<class... Cs>
    inline virtual_table_definition<generate_series_module_tag, Cs...> make_generate_series_definition(Cs... columns) {
        return {{std::make_tuple(std::move(columns)...)}};
    }
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
        return make_generate_series_definition(make_column("value", &generate_series::value),
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
