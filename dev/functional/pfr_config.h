#pragma once

#if SQLITE_ORM_HAS_INCLUDE(<boost/pfr.hpp>)
#define SQLITE_ORM_HAS_BOOST_PFR
#endif

#ifdef SQLITE_ORM_HAS_BOOST_PFR
#include <boost/pfr/config.hpp>
#endif
