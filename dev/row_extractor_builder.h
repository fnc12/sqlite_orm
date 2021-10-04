#pragma once

#include "row_extractor.h"
#include "mapped_row_extractor.h"

namespace sqlite_orm {

    namespace internal {

        /**
         * This builder is used to construct different row extractors depending on type.
         * It has two specializations: for mapped to storage types (e.g. User, Visit etc) and
         * for non-mapped (e.g. std::string, QString, int etc). For non mapped its operator() returns
         * generic `row_extractor`, for mapped it returns `mapped_row_extractor` instance.
         */
        template<class T, bool IsMapped, class I>
        struct row_extractor_builder;

        template<class T, class I>
        struct row_extractor_builder<T, false, I> {

            row_extractor<T> operator()(const I* /*tableInfo*/) const {
                return {};
            }
        };

        template<class T, class I>
        struct row_extractor_builder<T, true, I> {

            mapped_row_extractor<T, I> operator()(const I* tableInfo) const {
                return {*tableInfo};
            }
        };

        template<class T, bool IsMapped, class I>
        auto make_row_extractor(const I* tableInfo) {
            using builder_t = row_extractor_builder<T, IsMapped, I>;
            return builder_t{}(tableInfo);
        }

    }

}
