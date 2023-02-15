#pragma once

#include "cxx_universal.h"

#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) &&                 \
    defined(SQLITE_ORM_INLINE_VARIABLES_SUPPORTED)
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif
