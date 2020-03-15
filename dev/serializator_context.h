#pragma once

namespace sqlite_orm {

    namespace internal {

        struct serializator_context_base {
            bool replace_bindable_with_question = false;
            bool skip_table_name = true;

            template<class O, class F>
            std::string column_name(F O::*) const {
                return {};
            }
        };

        template<class I>
        struct serializator_context : serializator_context_base {
            using impl_type = I;

            const impl_type &impl;

            serializator_context(const impl_type &impl_) : impl(impl_) {}

            template<class O, class F>
            std::string column_name(F O::*m) const {
                return this->impl.column_name(m);
            }
        };

    }

}
