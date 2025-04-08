#include <sqlite_orm/sqlite_orm.h>

/******************************************************************************/
/***************************     Windows Tests     ****************************/
/******************************************************************************/
#if defined(_WIN32) && !defined(SQLITE_ORM_WIN)
#error "Windows platform detection failed"
#endif

/******************************************************************************/
/***************************    macOS/iOS Tests    ****************************/
/******************************************************************************/
#ifdef __APPLE__

#ifndef SQLITE_ORM_APPLE
#error "Apple platform detection failed"
#endif

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE == 1 && !defined(SQLITE_ORM_IOS)
#error "iOS platform detection failed"
#elif TARGET_OS_OSX == 1 && !defined(SQLITE_ORM_MACOS)
#error "macOS platform detection failed"
#endif

/******************************************************************************/
/***************************  Linux/BSD/Unix Tests  ***************************/
/******************************************************************************/
#if defined(__linux__) && (!defined(SQLITE_ORM_LINUX) || !defined(SQLITE_ORM_UNIX))
#error "Unix platform detection failed"
#endif

#if defined(__ANDROID__) && !defined(SQLITE_ORM_ANDROID)
#error "Unix platform detection failed"
#endif

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)) &&                 \
    (!defined(SQLITE_ORM_BSD) || !defined(SQLITE_ORM_UNIX))
#error "BSD platform detection failed"
#endif

/******************************************************************************/
/***************************  Linux/BSD/Unix Tests  ***************************/
/******************************************************************************/
#if (defined(__RTP__) || defined(_WRS_KERNEL)) && (!defined(SQLITE_ORM_VXWORKS) || !defined(SQLITE_ORM_UNIX))
#error "VxWorks platform detection failed"
#endif

#if defined(__unix__) && !defined(SQLITE_ORM_UNIX)
#error "Unix platform detection failed"
#endif

#endif
