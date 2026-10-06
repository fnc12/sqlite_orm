#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <algorithm>  //  std::find_if
#include <cassert>  //  assert
#include <cstddef>  //  std::size_t
#include <cstdint>  //  std::int32_t, std::int64_t, std::uint64_t
#include <deque>  //  std::deque
#include <functional>  //  std::function
#include <memory>  //  std::unique_ptr
#include <mutex>  //  std::mutex, std::lock_guard
#include <utility>  //  std::move
#include <vector>  //  std::vector
#endif

#include "fiber.h"
#include "io_operation.h"
#include "io_uring_engine.h"

namespace sqlite_orm::internal {

    /**
     *  Single-threaded cooperative scheduler: runs fibers and drives io_uring.
     *  Fibers park on I/O by calling read/write/fsync/sleep below; the scheduler
     *  resumes them when the completion arrives.
     *
     *  Cross-thread use is limited to post() and wake(): another thread may hand a
     *  callable to this scheduler, which runs it on the scheduler's own thread.
     */
    class scheduler {
      public:
        struct statistics {
            std::uint64_t operationsSubmitted = 0;
            std::uint64_t operationsCompleted = 0;
            std::uint64_t fiberSwitches = 0;
            std::uint64_t synchronousFallbacks = 0;  //  VFS calls made outside a fiber
            std::uint64_t loopIterations = 0;
            std::uint64_t waits = 0;  //  blocking reaps
            std::uint64_t posts = 0;  //  cross-thread posts received
        };

        explicit scheduler(unsigned queueDepth = 256) : engine(queueDepth) {}

        scheduler(const scheduler&) = delete;
        scheduler& operator=(const scheduler&) = delete;

        /**
         *  The scheduler bound to this thread while a loop iteration executes, i.e.
         *  the scheduler owning the currently running fiber. nullptr on a bare thread.
         */
        static scheduler* current() noexcept {
            return scheduler::current_slot();
        }

        /**
         *  Create a fiber and make it runnable.
         */
        void spawn(std::function<void()> function, std::size_t stackSize = fiber::defaultStackSize) {
            auto newFiber = std::make_unique<fiber>(std::move(function), stackSize);
            this->ready.push_back(newFiber.get());
            this->fibers.push_back(std::move(newFiber));
            ++this->alive;
        }

        /**
         *  Run until every fiber has finished.
         */
        void run() {
            while (this->run_one()) {
            }
        }

        /**
         *  Keep a wake operation armed so that run_one() blocks while idle and post()
         *  from another thread interrupts it.
         */
        void enable_wake() {
            this->arm_wake();
        }

        /**
         *  One loop iteration; blocks for I/O only if no fiber is runnable.
         *  Returns false when there is nothing left to do.
         */
        bool run_one() {
            return this->step(true);
        }

        /**
         *  One non-blocking iteration.
         */
        bool poll() {
            return this->step(false);
        }

        /**
         *  True when nothing is runnable and no I/O (other than the cross-thread wake
         *  operation) is in flight.
         */
        bool is_idle() const noexcept {
            return this->ready.empty() && this->inFlight == (this->wakeArmed ? 1u : 0u);
        }

        std::size_t fibers_alive() const noexcept {
            return this->alive;
        }

        const statistics& stats() const noexcept {
            return this->statistics_;
        }

        int native_handle() const noexcept {
            return this->engine.native_handle();
        }

        //  ---- Called from inside a fiber; they park it until completion ----

        std::int32_t read(int descriptor, void* buffer, unsigned length, std::int64_t offset) {
            io_operation operation;
            this->engine.prepare_read(operation, descriptor, buffer, length, offset);
            return this->await(operation);
        }

        std::int32_t write(int descriptor, const void* buffer, unsigned length, std::int64_t offset) {
            io_operation operation;
            this->engine.prepare_write(operation, descriptor, buffer, length, offset);
            return this->await(operation);
        }

        std::int32_t fsync(int descriptor, bool dataOnly) {
            io_operation operation;
            this->engine.prepare_fsync(operation, descriptor, dataOnly);
            return this->await(operation);
        }

        std::int32_t sleep_microseconds(std::int64_t microseconds) {
            io_operation operation;
            this->engine.prepare_timeout(operation, microseconds);
            return this->await(operation);
        }

        //  ---- Thread-safe ----

        void post(std::function<void()> function) {
            {
                std::lock_guard<std::mutex> lock(this->postMutex);
                this->posted.push_back(std::move(function));
            }
            this->wake();
        }

        void wake() {
            this->engine.wake();
        }

        void note_synchronous_fallback() noexcept {
            ++this->statistics_.synchronousFallbacks;
        }

        /**
         *  Same-thread hook: when it returns true the loop will not block in the
         *  kernel even if no fiber is runnable (the owner has work of its own).
         */
        void set_external_ready(std::function<bool()> predicate) {
            this->externalReady = std::move(predicate);
        }

      private:
        static scheduler*& current_slot() noexcept {
            thread_local scheduler* currentScheduler = nullptr;
            return currentScheduler;
        }

        struct current_guard {
            scheduler* previous;

            explicit current_guard(scheduler* self) : previous(scheduler::current_slot()) {
                scheduler::current_slot() = self;
            }

            ~current_guard() {
                scheduler::current_slot() = this->previous;
            }
        };

        std::int32_t await(io_operation& operation) {
            fiber* self = fiber::current();
            assert(self && "scheduler I/O must be called from inside a fiber");
            operation.waiter = self;
            operation.completed = false;
            ++this->inFlight;
            ++this->statistics_.operationsSubmitted;
            do {
                fiber::yield();
            } while (!operation.completed);
            return operation.result;
        }

        void arm_wake() {
            if (this->wakeArmed) {
                return;
            }
            this->wakeOperation = io_operation{};
            this->wakeOperation.context = this;
            this->wakeOperation.onComplete = [](io_operation*, void* context) {
                auto* self = static_cast<scheduler*>(context);
                self->wakeArmed = false;
                self->drain_posted();
                self->arm_wake();
            };
            this->engine.arm_wake(this->wakeOperation);
            this->wakeArmed = true;
            ++this->inFlight;
        }

        void drain_posted() {
            std::deque<std::function<void()>> batch;
            {
                std::lock_guard<std::mutex> lock(this->postMutex);
                batch.swap(this->posted);
            }
            for (auto& function: batch) {
                ++this->statistics_.posts;
                function();
            }
        }

        bool step(bool mayBlock) {
            current_guard guard(this);
            ++this->statistics_.loopIterations;
            this->drain_posted();

            //  1. Run everything that is runnable right now.
            std::size_t runnable = this->ready.size();
            while (runnable-- > 0) {
                fiber* next = this->ready.front();
                this->ready.pop_front();
                ++this->statistics_.fiberSwitches;
                next->resume();
                if (next->is_done()) {
                    --this->alive;
                    auto it = std::find_if(this->fibers.begin(),
                                           this->fibers.end(),
                                           [next](const std::unique_ptr<fiber>& owned) {
                                               return owned.get() == next;
                                           });
                    if (it != this->fibers.end()) {
                        this->fibers.erase(it);
                    }
                }
            }

            //  2. Push queued I/O to the kernel and collect completions. Block only
            //     when no fiber can make progress without them.
            this->engine.submit();
            if (this->inFlight == 0) {
                return !this->ready.empty();
            }
            const bool block = mayBlock && this->ready.empty() && !(this->externalReady && this->externalReady());
            if (block) {
                ++this->statistics_.waits;
            }
            this->completed.clear();
            this->engine.reap(block, this->completed);
            for (io_operation* operation: this->completed) {
                operation->completed = true;
                --this->inFlight;
                ++this->statistics_.operationsCompleted;
                if (operation->waiter) {
                    this->ready.push_back(operation->waiter);
                } else if (operation->onComplete) {
                    operation->onComplete(operation, operation->context);
                }
            }
            if (this->ready.empty() && this->inFlight == (this->wakeArmed ? 1u : 0u) && this->alive == 0) {
                return false;
            }
            return true;
        }

        io_uring_engine engine;
        std::deque<fiber*> ready;
        std::vector<std::unique_ptr<fiber>> fibers;
        std::vector<io_operation*> completed;
        std::size_t inFlight = 0;  //  operations in the kernel (including the wake operation)
        std::size_t alive = 0;
        statistics statistics_;
        io_operation wakeOperation;
        bool wakeArmed = false;
        std::mutex postMutex;
        std::deque<std::function<void()>> posted;
        std::function<bool()> externalReady;
    };
}
