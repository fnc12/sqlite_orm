// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

#define _BUILD_SQLITE_ORM_MODULE

export module sqlite_orm;

#pragma warning(push)
#pragma warning(disable : 5244)  // '#include <meow>' in the purview of module 'sqlite_orm' appears erroneous.

#include <sqlite_orm/sqlite_orm.h>

#pragma warning(pop)
