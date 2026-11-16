#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <ostream>
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

    enum class sync_schema_result {

        /**
         *  created new table, table with the same tablename did not exist
         */
        new_table_created,

        /**
         *  table schema is the same as storage, nothing to be done
         */
        already_in_sync,

        /**
         *  removed excess columns in table (than storage) without dropping a table
         */
        old_columns_removed,

        /**
         *  lacking columns in table (than storage) added without dropping a table
         */
        new_columns_added,

        /**
         *  both old_columns_removed and new_columns_added
         */
        new_columns_added_and_old_columns_removed,

        /**
         *  old table is dropped and new is recreated. Reasons:
         *  1. an existing column differs from the storage definition by one of the compared
         *     properties: primary key membership and order, `NOT NULL`, the presence of a
         *     default value, the generated flag
         *  2. delete excess columns in the table than storage if preserve = false and
         *     SQLite is older than 3.35.0 (no `DROP COLUMN` support)
         *  3. a new column cannot be added with `ALTER TABLE ... ADD COLUMN`: it is a STORED
         *     generated column, has a `PRIMARY KEY` or `UNIQUE` constraint or a non-constant
         *     default value
         *  Data is preserved through a backup table when preserve = true.
         *  Note that changes of the column type, of the default value itself, of a generated
         *  column expression and of `UNIQUE`/`CHECK`/`COLLATE`/`FOREIGN KEY` constraints are
         *  not detected.
         */
        dropped_and_recreated,

        /**
         *  old table is dropped and new is recreated with data loss.
         *  Data cannot be preserved because a new NOT NULL column without
         *  a default value is being added, making backup impossible.
         */
        dropped_and_recreated_with_data_loss,
    };

    inline std::ostream& operator<<(std::ostream& os, sync_schema_result value) {
        switch (value) {
            case sync_schema_result::new_table_created:
                return os << "new table created";
            case sync_schema_result::already_in_sync:
                return os << "table and storage is already in sync.";
            case sync_schema_result::old_columns_removed:
                return os << "old excess columns removed";
            case sync_schema_result::new_columns_added:
                return os << "new columns added";
            case sync_schema_result::new_columns_added_and_old_columns_removed:
                return os << "old excess columns removed and new columns added";
            case sync_schema_result::dropped_and_recreated:
                return os << "old table dropped and recreated";
            case sync_schema_result::dropped_and_recreated_with_data_loss:
                return os << "old table dropped and recreated with data loss";
        }
        return os;
    }
}
