#pragma once

#include <sqlite3.h>
#include <utility>  //  std::move
#include <iterator>  //  std::input_iterator_tag, std::default_sentinel_t
#include <functional>  //  std::reference_wrapper

#include "functional/cxx_universal.h"  //  ::ptrdiff_t
#include "statement_finalizer.h"
#include "row_extractor.h"
#include "column_result_proxy.h"
#include "util.h"

#if defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)
namespace sqlite_orm::internal {

    template<class ColResult, class DBOs>
    class result_set_iterator;

#ifdef SQLITE_ORM_STL_HAS_DEFAULT_SENTINEL
    using result_set_sentinel_t = std::default_sentinel_t;
#else
    // sentinel
    template<>
    class result_set_iterator<void, void> {};

    using result_set_sentinel_t = result_set_iterator<void, void>;
#endif

    /*  
     *  Input iterator over a result set for a select statement.
     */
    template<class ColResult, class DBOs>
    class result_set_iterator {
      public:
        using db_objects_type = DBOs;

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        using iterator_concept = std::input_iterator_tag;
#else
        using iterator_category = std::input_iterator_tag;
#endif
        using difference_type = ptrdiff_t;
        using value_type = column_result_proxy_t<ColResult>;

      public:
        result_set_iterator(const db_objects_type& dbObjects, statement_finalizer stmt) :
            db_objects{dbObjects}, stmt{std::move(stmt)} {
            this->step();
        }
        result_set_iterator(result_set_iterator&&) = default;
        result_set_iterator& operator=(result_set_iterator&&) = default;
        result_set_iterator(const result_set_iterator&) = delete;
        result_set_iterator& operator=(const result_set_iterator&) = delete;

        /** @pre `*this != std::default_sentinel` */
        value_type operator*() const {
            return this->extract();
        }

        result_set_iterator& operator++() {
            this->step();
            return *this;
        }

        void operator++(int) {
            ++*this;
        }

        friend bool operator==(const result_set_iterator& it, const result_set_sentinel_t&) noexcept {
            return sqlite3_data_count(it.stmt.get()) == 0;
        }

      private:
        void step() {
            perform_step(this->stmt.get(), [](sqlite3_stmt*) {});
        }

        value_type extract() const {
            const auto rowExtractor = make_row_extractor<ColResult>(this->db_objects.get());
            return rowExtractor.extract(this->stmt.get(), 0);
        }

      private:
        std::reference_wrapper<const db_objects_type> db_objects;
        statement_finalizer stmt;
    };
}
#endif
