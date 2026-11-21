#pragma once

/** @file
 *  Purpose: identify the platform sqlite_orm is used on.
 *
 *  Intention: the platform tier of the configuration gathered by `config.h`; `capabilities.h` builds on it. Its macros
 *  are named `SQLITE_ORM_PLATFORM_<platform>`.
 */

#if defined(_WIN32)
#define SQLITE_ORM_PLATFORM_WIN

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#define SQLITE_ORM_PLATFORM_IOS
#elif TARGET_OS_OSX == 1
#define SQLITE_ORM_PLATFORM_MACOS
#endif
#define SQLITE_ORM_PLATFORM_APPLE
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__linux__)
#if defined(__ANDROID__)
#define SQLITE_ORM_PLATFORM_ANDROID
#endif
#define SQLITE_ORM_PLATFORM_LINUX
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define SQLITE_ORM_PLATFORM_BSD
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__RTP__) || defined(_WRS_KERNEL)
#define SQLITE_ORM_PLATFORM_VXWORKS
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__unix__) || defined(__unix)
#define SQLITE_ORM_PLATFORM_UNIX

#else
#error "Unknown target platform detected"
#endif
