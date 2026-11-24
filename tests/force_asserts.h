#pragma once
// CI builds tests in Release, where NDEBUG turns every assert() into a no-op and the tests would pass
// vacuously. Include this header FIRST in every test so the checks always run.
#ifdef NDEBUG
 #undef NDEBUG
#endif
#include <cassert>
