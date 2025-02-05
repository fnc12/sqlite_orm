#pragma once

#include "vfs_mode.h"
#include "open_mode.h"

namespace sqlite_orm {

    /**
     * Struct used to pass options into your storage object that will be maintained over its lifetime.
     */
    struct storage_options {
        vfs_mode vfs_mode = vfs_mode::default_vfs;
        open_mode open_mode = open_mode::default_mode;
    };

}
