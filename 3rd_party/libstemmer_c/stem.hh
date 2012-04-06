#ifndef STEM_HH
#define STEM_HH

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

class Stemmer : boost::noncopyable  {
public:
    Stemmer(const char* language);
    ~Stemmer();
    std::string stem(const std::string&);

private:
    struct Pimpl;
    boost::scoped_ptr<Pimpl> pimpl;
};

#endif
