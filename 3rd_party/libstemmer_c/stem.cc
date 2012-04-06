/*
 * Copyright 2012 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */
#include "stem.hh"

#include <libstemmer.h>
#include <stdexcept>
#include "utils.hh"


struct Stemmer::Pimpl {
    
    Pimpl(const char* language);
    ~Pimpl();
    std::string stem(const std::string&);

    sb_stemmer* sb_stemmer_instance;
};

Stemmer::Pimpl::Pimpl(const char* language) :
    sb_stemmer_instance()
{
    sb_stemmer_instance = sb_stemmer_new(language, "UTF_8");
    if ( ! sb_stemmer_instance) 
        throw std::runtime_error(fs("sb_stemmer_new for: " << language << " not found"));

}


Stemmer::Pimpl::~Pimpl()
{
    sb_stemmer_delete(sb_stemmer_instance);
}

std::string Stemmer::Pimpl::stem(const std::string& word)
{
    std::string result;
    const sb_symbol* stemmed = sb_stemmer_stem(sb_stemmer_instance, reinterpret_cast<const sb_symbol *>(word.c_str()), static_cast<int>(word.size())); 
    if ( ! stemmed)
        throw std::runtime_error(fs("sb_stemmer_stem returns NULL"));

    int len = sb_stemmer_length(sb_stemmer_instance);
    if ( len < 0)
        throw std::runtime_error("sb_stemmer_stem negative length");

    result.assign(reinterpret_cast<const char*>(stemmed), static_cast<size_t>(len)); 
    return result;
}


Stemmer::Stemmer(const char* language) :
    pimpl(new Stemmer::Pimpl(language))
{}

Stemmer::~Stemmer()
{}

std::string Stemmer::stem(const std::string& word)
{
    return pimpl->stem(word);
}


#ifdef TEST_MAIN
using namespace std;
int main(int argc, const char* argv[])
{
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " [english|spanish|...]" << endl;
        exit(1);
    }

    Stemmer stemmer(argv[1]);

    string instr;
    while(getline(cin, instr)) {
        cout << stemmer.stem(instr) << endl;
    }
}
#endif


#ifdef EXPORT_PYTHON_INTERFACE
#include <boost/python.hpp>
using namespace boost::python;
BOOST_PYTHON_MODULE_INIT(Stemmer)
{
	class_<Stemmer, boost::noncopyable>("Stemmer", "Interface for the snowball libstemmer C lib", init<const char*>())
		.def("stem", &Stemmer::stem)
	;	
}

#endif
