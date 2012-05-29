
#include <string>
#include <boost/version.hpp>

#include "mongo/util/version.h"

namespace mongo {
    const char * gitVersion() { return "3609145e1bc425f8578754d9e5a8a6e96f2be884"; }
    std::string sysInfo() { return "Linux gominola 2.6.39 #18 SMP PREEMPT Mon Oct 10 19:24:22 CEST 2011 x86_64 BOOST_LIB_VERSION=" BOOST_LIB_VERSION ; }
}  // namespace mongo
