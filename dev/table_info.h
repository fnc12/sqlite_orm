#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

    struct table_info {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        table_info(decltype(cid) cid_,
                   decltype(name) name_,
                   decltype(type) type_,
                   decltype(notnull) notnull_,
                   decltype(dflt_value) dflt_value_,
                   decltype(pk) pk_) :
            cid(cid_), name(std::move(name_)), type(std::move(type_)), notnull(notnull_),
            dflt_value(std::move(dflt_value_)), pk(pk_) {}
#endif
    };

    struct table_xinfo {
        int cid = 0;
        std::string name;
        std::string type;
        bool notnull = false;
        std::string dflt_value;
        int pk = 0;
        //  as reported by `PRAGMA table_xinfo`: 0 = normal column, 1 = hidden column in a virtual table,
        //  2 = VIRTUAL generated column, 3 = STORED generated column;
        //  `get_table_info()` stores 1 for a generated column, so only zero vs non-zero may be compared
        int hidden = 0;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        table_xinfo(decltype(cid) cid_,
                    decltype(name) name_,
                    decltype(type) type_,
                    decltype(notnull) notnull_,
                    decltype(dflt_value) dflt_value_,
                    decltype(pk) pk_,
                    decltype(hidden) hidden_) :
            cid(cid_), name(std::move(name_)), type(std::move(type_)), notnull(notnull_),
            dflt_value(std::move(dflt_value_)), pk(pk_), hidden{hidden_} {}
#endif
    };
}
