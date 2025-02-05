#pragma once

#include "vfs_object.h"
#include "open_mode.h"

namespace sqlite_orm {

    /**
     * Struct used to pass options into your storage object that will be maintained over its lifetime.
     */
    struct storage_options {
        vfs_object vfs_option = vfs_object::default_vfs;
        open_mode open_option = open_mode::default_mode;
    };

}
