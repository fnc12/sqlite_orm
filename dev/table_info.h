#pragma once

#include <string>  //  std::string

#include "functional/cxx_universal.h"

namespace sqlite_orm {

    struct table_info {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;

#if !defined(SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED) || !defined(SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED)
        table_info(decltype(cid) cid_,
                   decltype(name) name_,
                   decltype(type) type_,
                   decltype(notnull) notnull_,
                   decltype(dflt_value) dflt_value_,
                   decltype(pk) pk_) :
            cid(cid_),
            name(move(name_)), type(move(type_)), notnull(notnull_), dflt_value(move(dflt_value_)), pk(pk_) {}
#endif
    };

    struct table_xinfo {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;
        int hidden = 0;  // different than 0 => generated_always_as() - TODO verify

#if !defined(SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED) || !defined(SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED)
        table_xinfo(decltype(cid) cid_,
                    decltype(name) name_,
                    decltype(type) type_,
                    decltype(notnull) notnull_,
                    decltype(dflt_value) dflt_value_,
                    decltype(pk) pk_,
                    decltype(hidden) hidden_) :
            cid(cid_),
            name(move(name_)), type(move(type_)), notnull(notnull_), dflt_value(move(dflt_value_)),
            pk(pk_), hidden{hidden_} {}
#endif
    };
}
