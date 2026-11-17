#pragma once

namespace sqlite_orm::internal {
    struct serializer_context_base {
        bool replace_bindable_with_question = false;
        bool omit_table_name = true;
        bool use_parentheses = true;
        bool omit_column_type = false;
    };

    /**
     *  Purpose: carry the context of a serialization.
     *
     *  Intention: created by the storage and the views of `result_set/`; the statement serializer passes it down to its
     *  helpers, and the dynamic nodes of `ast/` keep it to serialize their entries as they are added.
     */
    template<class DBOs>
    struct serializer_context : serializer_context_base {
        using db_objects_type = DBOs;

        const db_objects_type& db_objects;

        serializer_context(const db_objects_type& dbObjects) : db_objects{dbObjects} {}
    };
}
