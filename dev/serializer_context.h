#pragma once

namespace sqlite_orm {

    namespace internal {

        struct serializer_context_base {
            bool replace_bindable_with_question = false;
            bool skip_table_name = true;
            bool use_parentheses = true;
        };

        template<class I>
        struct serializer_context : serializer_context_base {
            using schema_objects_type = I;

            const schema_objects_type& impl;

            serializer_context(const schema_objects_type& impl_) : impl(impl_) {}
        };

        template<class S>
        struct serializer_context_builder {
            using storage_type = S;
            using schema_objects_type = typename storage_type::schema_objects_type;

            serializer_context_builder(const storage_type& storage_) : storage(storage_) {}

            serializer_context<schema_objects_type> operator()() const {
                return {obtain_schema_objects(this->storage)};
            }

            const storage_type& storage;
        };

    }

}
