#pragma once

#if __has_include(<boost/pfr.hpp>)
#define SQLITE_ORM_HAS_BOOST_PFR
#endif

#if defined(SQLITE_ORM_HAS_BOOST_PFR) && (!defined(BOOST_PFR_ENABLED) || (BOOST_PFR_ENABLED == 1))
#include <boost/pfr/config.hpp>
#endif
