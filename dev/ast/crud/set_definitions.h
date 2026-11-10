#pragma once

/** @file
 *  Purpose: define the members of the runtime SET clause that need the statement serializer and the AST traversal.
 *
 *  Intention: keeps `ast/crud/set.h` free of them; gathered by `interface_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <sstream>  //  std::ostringstream
#endif

#include "../../functional/type_traits.h"
#include "../../serialization/statement_serializer.h"
#include "../../serialization/table_name_collector.h"
#include "../../traversal/ast_iterator.h"
#include "../../vocabulary/node_traits.h"
#include "set.h"

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
        std::ostringstream ss;
        ss << serialize(assign.lhs, lhsContext) << ' ' << assign.serialize() << ' '
           << serialize(assign.rhs, this->context);
        this->entries.push_back({ss.str()});
    }
}
