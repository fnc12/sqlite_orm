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

        template<class DBOs>
        struct serializer_context_with_no_types_and_constraints : serializer_context<DBOs> {
            using super = serializer_context<DBOs>;

            serializer_context_with_no_types_and_constraints(const super& parentContext) : super(parentContext) {}
        };

        template<class DBOs>
        serializer_context_with_no_types_and_constraints<DBOs>
        make_serializer_context_with_no_types_and_constraints(const serializer_context<DBOs>& parentContext) {
            return {parentContext};
        }

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

        template<class T>
        struct no_need_types_and_constraints : std::false_type {};

        template<class DBOs>
        struct no_need_types_and_constraints<serializer_context_with_no_types_and_constraints<DBOs>> : std::true_type {
        };
    }

}
