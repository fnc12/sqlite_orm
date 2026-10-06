#pragma once

/** @file Out-of-class definitions of `dynamic_set_t` members, which need the statement serializer and the AST
 *  traversal machinery that `ast/crud/set.h` stays free of.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <sstream>  //  std::stringstream
#endif

#include "../functional/type_traits.h"
#include "../vocabulary/node_traits.h"
#include "../ast_iterator.h"
#include "../serialization/table_name_collector.h"
#include "../serialization/statement_serializer.h"
#include "../ast/crud/set.h"

namespace sqlite_orm::internal {
    template<class C>
    template<class T, satisfies<is_assign, T>>
    void dynamic_set_t<C>::push_back(T assign) {
        // note: we are only interested in the table name on the left-hand side of the assignment operator expression
        table_name_collector<typename context_t::db_objects_type> collector{this->context.db_objects};
        iterate_ast(assign.lhs, collector);
        this->table_names.merge(collector.table_names);

        auto lhsContext = this->context;
        lhsContext.omit_table_name = true;
        std::stringstream ss;
        ss << serialize(assign.lhs, lhsContext) << ' ' << assign.serialize() << ' '
           << serialize(assign.rhs, this->context);
        this->entries.push_back({ss.str()});
    }
}
