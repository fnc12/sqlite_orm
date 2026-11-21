#pragma once

/** @file
 *  Purpose: fetch the rows of a query one at a time from a coroutine.
 *
 *  Intention: realizes `async_cursor`, what `async_storage::iterate()` returns, over the views of
 *  `result_set/mapped_view.h` and `result_set/result_set_view.h` that the synchronous `storage_t::iterate()` returns;
 *  uses `task.h`, and the storage's `run()` for every step.
 */

#ifdef SQLITE_ORM_WITH_ASYNC
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::shared_ptr, std::make_shared
#include <optional>  //  std::optional, std::nullopt
#include <type_traits>  //  std::invoke_result_t, std::remove_cvref_t
#include <utility>  //  std::declval, std::exchange, std::move
#endif
#endif

#include "../functional/coroutines/task.h"

#ifdef SQLITE_ORM_WITH_ASYNC
namespace sqlite_orm::internal {

    /**
     *  Where a cursor stands in its result set. Shared between the cursor and the step in progress, and handed
     *  to the storage when the cursor is destroyed before the rows are exhausted.
     */
    template<class Storage, class MakeView>
    struct cursor_position {
        using view_type = std::invoke_result_t<MakeView&, Storage&>;
        using iterator_type = decltype(std::declval<view_type&>().begin());
        using sentinel_type = decltype(std::declval<view_type&>().end());
        using value_type = std::remove_cvref_t<decltype(*std::declval<iterator_type&>())>;

        explicit cursor_position(MakeView makeView_) : makeView(std::move(makeView_)) {}

        /**
         *  Step to the next row and return it, or nothing when the rows are exhausted. Runs on a fiber.
         */
        std::optional<value_type> advance(Storage& storage) {
            if (this->finished) {
                return std::nullopt;
            }
            try {
                if (!this->iterator) {
                    view_type view = (*this->makeView)(storage);
                    this->makeView.reset();
                    this->iterator.emplace(view.begin());
                    this->end.emplace(view.end());
                } else {
                    ++*this->iterator;
                }
                if (*this->iterator == *this->end) {
                    this->finish();
                    return std::nullopt;
                }
                auto&& row = **this->iterator;
                return std::optional<value_type>{std::move(row)};
            } catch (...) {
                this->finish();
                throw;
            }
        }

        /**
         *  Finalize the statement.
         */
        void finish() {
            this->iterator.reset();
            this->end.reset();
            this->makeView.reset();
            this->finished = true;
        }

        bool holds_statement() const noexcept {
            return this->iterator.has_value();
        }

      private:
        std::optional<MakeView> makeView;
        std::optional<iterator_type> iterator;
        std::optional<sentinel_type> end;
        bool finished = false;
    };

    /**
     *  The rows of a query, fetched one at a time; returned by `async_storage::iterate()`.
     *
     *      auto users = storage.iterate<User>(where(c(&User::age) > 30));
     *      while (std::optional<User> user = co_await users.next()) {
     *          ...
     *      }
     *
     *  The first `next()` prepares the statement; it is finalized when the rows are exhausted or the cursor is
     *  destroyed. Between two rows the connection is free: other operations on the storage run in between,
     *  further cursors included, as they may inside a loop over the synchronous `storage.iterate()`.
     *
     *  A cursor must not outlive its storage.
     */
    template<class AsyncStorage, class MakeView>
    class async_cursor {
        using position_type = internal::cursor_position<typename AsyncStorage::storage_type, MakeView>;

      public:
        using value_type = typename position_type::value_type;

        async_cursor(AsyncStorage& storage_, MakeView makeView) :
            storage(&storage_), position(std::make_shared<position_type>(std::move(makeView))) {}

        async_cursor(async_cursor&& other) noexcept :
            storage(other.storage), position(std::exchange(other.position, nullptr)) {}

        async_cursor& operator=(async_cursor&& other) noexcept {
            if (this != &other) {
                this->close();
                this->storage = other.storage;
                this->position = std::exchange(other.position, nullptr);
            }
            return *this;
        }

        async_cursor(const async_cursor&) = delete;
        async_cursor& operator=(const async_cursor&) = delete;

        ~async_cursor() {
            this->close();
        }

        /**
         *  The next row, or `std::nullopt` after the last one.
         */
        async_task<std::optional<value_type>> next() {
            return this->storage->run([position = this->position](typename AsyncStorage::storage_type& storage) {
                return position->advance(storage);
            });
        }

        /**
         *  Stop fetching: the statement is finalized as soon as no operation is executing on the connection.
         */
        void close() noexcept {
            if (this->position && this->position->holds_statement()) {
                this->storage->retire(std::move(this->position));
            }
            this->position = nullptr;
        }

      private:
        AsyncStorage* storage = nullptr;
        std::shared_ptr<position_type> position;
    };
}
#endif
