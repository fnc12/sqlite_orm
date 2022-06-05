#pragma once

namespace sqlite_orm {

    namespace internal {

        struct serializer_context_base {
            bool replace_bindable_with_question = false;
            bool skip_table_name = true;
            bool use_parentheses = true;
        };

        template<class DBOs>
        struct serializer_context : serializer_context_base {
            using db_objects_type = DBOs;

            const db_objects_type& db_objects;

            serializer_context(const db_objects_type& dbObjects) : db_objects{dbObjects} {}
        };

        template<class S>
        struct serializer_context_builder {
            using storage_type = S;
            using db_objects_type = typename storage_type::db_objects_type;

            serializer_context_builder(const storage_type& storage_) : storage{storage_} {}

            serializer_context<db_objects_type> operator()() const {
                return {obtain_db_objects(this->storage)};
            }

            const storage_type& storage;
        };

    }

}
