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
            using impl_type = I;

            const impl_type& impl;

            serializer_context(const impl_type& impl_) : impl(impl_) {}
        };

        template<class S>
        struct serializer_context_builder {
            using storage_type = S;
            using impl_type = typename storage_type::impl_type;

            serializer_context_builder(const storage_type& storage_) : storage(storage_) {}

            serializer_context<impl_type> operator()() const {
                return {obtain_const_impl(this->storage)};
            }

            const storage_type& storage;
        };

    }

}
