#ifndef utils_hh
#define utils_hh

#include <memory>
#include <sstream>

#define fs(a) \
   (static_cast<const std::ostringstream&>(((*::utils::getoss().get()) << a)).str ())

namespace utils {
inline std::auto_ptr<std::ostringstream> getoss()
{
   return std::auto_ptr<std::ostringstream> (new std::ostringstream);
}
}


#endif
