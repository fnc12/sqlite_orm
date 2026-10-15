#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <map>  //  std::map
#include <functional>  //  std::function, std::reference_wrapper
#include <utility>  //  std::move
#endif

#include "connection_holder.h"

namespace sqlite_orm::internal {
    struct limit_accessor {
        limit_accessor(std::unique_ptr<connection_holder>& connection) : connection{connection} {}

        int length() {
            return this->get(SQLITE_LIMIT_LENGTH);
        }

        void length(int newValue) {
            this->set(SQLITE_LIMIT_LENGTH, newValue);
        }

        int sql_length() {
            return this->get(SQLITE_LIMIT_SQL_LENGTH);
        }

        void sql_length(int newValue) {
            this->set(SQLITE_LIMIT_SQL_LENGTH, newValue);
        }

        int column() {
            return this->get(SQLITE_LIMIT_COLUMN);
        }

        void column(int newValue) {
            this->set(SQLITE_LIMIT_COLUMN, newValue);
        }

        int expr_depth() {
            return this->get(SQLITE_LIMIT_EXPR_DEPTH);
        }

        void expr_depth(int newValue) {
            this->set(SQLITE_LIMIT_EXPR_DEPTH, newValue);
        }

        int compound_select() {
            return this->get(SQLITE_LIMIT_COMPOUND_SELECT);
        }

        void compound_select(int newValue) {
            this->set(SQLITE_LIMIT_COMPOUND_SELECT, newValue);
        }

        int vdbe_op() {
            return this->get(SQLITE_LIMIT_VDBE_OP);
        }

        void vdbe_op(int newValue) {
            this->set(SQLITE_LIMIT_VDBE_OP, newValue);
        }

        int function_arg() {
            return this->get(SQLITE_LIMIT_FUNCTION_ARG);
        }

        void function_arg(int newValue) {
            this->set(SQLITE_LIMIT_FUNCTION_ARG, newValue);
        }

        int attached() {
            return this->get(SQLITE_LIMIT_ATTACHED);
        }

        void attached(int newValue) {
            this->set(SQLITE_LIMIT_ATTACHED, newValue);
        }

        int like_pattern_length() {
            return this->get(SQLITE_LIMIT_LIKE_PATTERN_LENGTH);
        }

        void like_pattern_length(int newValue) {
            this->set(SQLITE_LIMIT_LIKE_PATTERN_LENGTH, newValue);
        }

        int variable_number() {
            return this->get(SQLITE_LIMIT_VARIABLE_NUMBER);
        }

        void variable_number(int newValue) {
            this->set(SQLITE_LIMIT_VARIABLE_NUMBER, newValue);
        }

        int trigger_depth() {
            return this->get(SQLITE_LIMIT_TRIGGER_DEPTH);
        }

        void trigger_depth(int newValue) {
            this->set(SQLITE_LIMIT_TRIGGER_DEPTH, newValue);
        }

#if SQLITE_VERSION_NUMBER >= 3008007
        int worker_threads() {
            return this->get(SQLITE_LIMIT_WORKER_THREADS);
        }

        void worker_threads(int newValue) {
            this->set(SQLITE_LIMIT_WORKER_THREADS, newValue);
        }
#endif

      protected:
        std::reference_wrapper<std::unique_ptr<connection_holder>> connection;

        friend struct storage_base;

        /**
         *  Stores limit set between connections.
         */
        std::map<int, int> limits;

        int get(int id) {
            connection_ref connection = *this->connection.get();
            return sqlite3_limit(connection.get(), id, -1);
        }

        void set(int id, int newValue) {
            this->limits[id] = newValue;
            if (connection_ptr maybeConnection = *this->connection.get()) {
                sqlite3_limit(maybeConnection.get(), id, newValue);
            }
        }
    };
}
