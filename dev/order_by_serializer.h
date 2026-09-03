#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <array>
#include <string>  //  std::string
#include <sstream>  //  std::stringstream
#include <utility>  //  std::exchange
#endif

#include "functional/gsl.h"
#include "type_traits.h"
#include "vocabulary/node_fwd.h"  // order_by_base
#include "vocabulary/node_traits.h"

namespace sqlite_orm::internal {
    template<class T, class SFINAE = void>
    struct order_by_serializer;

    template<class T, class Ctx>
    std::string serialize_order_by(const T& t, const Ctx& context) {
        order_by_serializer<T> serializer;
        return serializer(t, context);
    }

    inline void seralize_collate(std::ostream& ss, const order_by_base& orderBy) {
        if (!orderBy._collate_argument.empty()) {
            ss << " COLLATE " << orderBy._collate_argument;
        }
        switch (orderBy._order) {
            case 1:
                ss << " ASC";
                break;
            case -1:
                ss << " DESC";
                break;
        }
#if SQLITE_VERSION_NUMBER >= 3030000
        switch (orderBy._nulls) {
            case 1:
                ss << " NULLS FIRST";
                break;
            case -1:
                ss << " NULLS LAST";
                break;
        }
#endif
    }

    template<class T>
    struct order_by_serializer<T, match_if<is_order_by, T>> {
        using statement_type = T;

        template<class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::string operator()(const statement_type& orderBy,
                                                        const Ctx& context) SQLITE_ORM_OR_CONST_CALLOP {
            std::stringstream ss;
            auto newContext = context;
            newContext.omit_table_name = false;

            ss << serialize(orderBy._expression, newContext);
            seralize_collate(ss, orderBy);
            return ss.str();
        }
    };

    template<class T>
    struct order_by_serializer<T, match_if<is_dynamic_order_by, T>> {
        using statement_type = T;

        template<class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::string operator()(const statement_type& orderBy,
                                                        const Ctx&) SQLITE_ORM_OR_CONST_CALLOP {
            std::stringstream ss;
            ss << static_cast<std::string>(orderBy) << " ";
            static constexpr std::array<orm_gsl::czstring, 2> sep = {", ", ""};
#ifdef SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
            for (bool first = true; const dynamic_order_by_entry_t& entry: orderBy) {
                ss << sep[std::exchange(first, false)] << entry.name;
                seralize_collate(ss, entry);
            }
#else
            bool first = true;
            for (const dynamic_order_by_entry_t& entry: orderBy) {
                ss << sep[std::exchange(first, false)] << entry.name;
                seralize_collate(ss, entry);
            }
#endif
            return ss.str();
        }
    };
}
