#pragma once

namespace sqlite_orm {

    namespace internal {

        struct serializer_context_base {
            bool replace_bindable_with_question = false;
            bool omit_table_name = true;
            bool use_parentheses = true;
            bool omit_column_type = false;
        };

        template<class DBOs>
        struct serializer_context : serializer_context_base {
            using db_objects_type = DBOs;

            const db_objects_type& db_objects;

            serializer_context(const db_objects_type& dbObjects) : db_objects{dbObjects} {}
        };
    }

}
