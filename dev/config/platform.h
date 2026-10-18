#pragma once

#if defined(_WIN32)
#define SQLITE_ORM_WIN

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#define SQLITE_ORM_IOS
#elif TARGET_OS_OSX == 1
#define SQLITE_ORM_MACOS
#endif
#define SQLITE_ORM_APPLE
#define SQLITE_ORM_UNIX

#elif defined(__linux__)
#if defined(__ANDROID__)
#define SQLITE_ORM_ANDROID
#endif
#define SQLITE_ORM_LINUX
#define SQLITE_ORM_UNIX

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define SQLITE_ORM_BSD
#define SQLITE_ORM_UNIX

#elif defined(__RTP__) || defined(_WRS_KERNEL)
#define SQLITE_ORM_VXWORKS
#define SQLITE_ORM_UNIX

#elif defined(__unix__) || defined(__unix)
#define SQLITE_ORM_UNIX

#else
#error "Unknown target platform detected"
#endif
