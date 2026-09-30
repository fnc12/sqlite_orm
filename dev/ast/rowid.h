#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::is_same, std::disjunction
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

/*
 *  The rowid of a table, in each of its spellings ROWID, OID and _ROWID_, and each of them unqualified or qualified
 *  by the table - which is the only thing telling the qualified nodes apart, through their `type`.
 *  Each node carries its own spelling.
 */
namespace sqlite_orm::internal {
    struct rowid_t {
        operator std::string() const {
            return "rowid";
        }
    };

    struct oid_t {
        operator std::string() const {
            return "oid";
        }
    };

    struct _rowid_t {
        operator std::string() const {
            return "_rowid_";
        }
    };

    template<class T>
    struct table_rowid_t : public rowid_t {
        using type = T;
    };

    template<class T>
    struct table_oid_t : public oid_t {
        using type = T;
    };
    template<class T>
    struct table__rowid_t : public _rowid_t {
        using type = T;
    };

    //  note: names each node rather than testing for the unqualified base nodes -
    //  a `std::tuple` of such nodes is derived from them too, as it inherits its empty element types
    template<class T>
    constexpr bool is_any_rowid_v = std::disjunction<std::is_same<T, rowid_t>,
                                                     std::is_same<T, oid_t>,
                                                     std::is_same<T, _rowid_t>,
                                                     polyfill::is_specialization_of<T, table_rowid_t>,
                                                     polyfill::is_specialization_of<T, table_oid_t>,
                                                     polyfill::is_specialization_of<T, table__rowid_t>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline internal::rowid_t rowid() {
        return {};
    }

    inline internal::oid_t oid() {
        return {};
    }

    inline internal::_rowid_t _rowid_() {
        return {};
    }

    template<class T>
    internal::table_rowid_t<T> rowid() {
        return {};
    }

    template<class T>
    internal::table_oid_t<T> oid() {
        return {};
    }

    template<class T>
    internal::table__rowid_t<T> _rowid_() {
        return {};
    }
}
