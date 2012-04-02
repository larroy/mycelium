/* Python interface */
#ifdef EXPORT_PYTHON_INTERFACE
#include "Url.hh"
#include <boost/python.hpp>
using namespace boost::python;

//BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(xf_overloads, scheme, 0, 1)
BOOST_PYTHON_MODULE_INIT(common)
{

    std::string (Url::*get_scheme)() const = &Url::scheme;
    void (Url::*set_scheme)(const std::string&) = &Url::scheme;

    std::string (Url::*get_authority)() const = &Url::authority;
    void (Url::*set_authority)(const std::string&) = &Url::authority;

    std::string (Url::*get_host)() const = &Url::host;
    void (Url::*set_host)(const std::string&) = &Url::host;

    std::string (Url::*get_port)() const = &Url::port;
    void (Url::*set_port)(const std::string&) = &Url::port;

    std::string (Url::*get_path)() const = &Url::path;
    void (Url::*set_path)(const std::string&) = &Url::path;

    std::string (Url::*get_query)() const = &Url::query;
    void (Url::*set_query)(const std::string&) = &Url::query;

    std::string (Url::*get_fragment)() const = &Url::fragment;
    void (Url::*set_fragment)(const std::string&) = &Url::fragment;

    std::string (*unescape_all)(const std::string&) = &Url::unescape;




    class_<Url>("Url", "Constructs from a string", init<const std::string&>())
        .def(init<>())
        .def("__str__", &Url::operator std::string)
        .def("__repr__", &Url::operator std::string)
        .def(self == self)
        .def(self != self)
        .def(not self)
        //.def("merge_ref", &Url::merge_ref)
        .def("assign", &Url::assign,
            "assign new url from string")

        .def("normalize", &Url::normalize,
            "lowercases case insensitive parts and normalizes escape sequences, unescaping safe sequences, uppercasing escape indices and escaping unsafe characters. Two normalized urls should have the same string representation if they point to the same resource")

        .def("clear", &Url::clear)
        .def("empty", &Url::empty,
            "true if the url is empty")
        .def("get", &Url::get,
            "get the string representation")

        //.def("get_scheme", &Url::scheme,xf_overloads() )
        .def("get_scheme", get_scheme)
        .def("set_scheme", set_scheme)
        .def("has_scheme", &Url::has_scheme)

        .def("get_authority", get_authority)
        .def("set_authority", set_authority)
        .def("has_authority", &Url::has_authority)
        .def("clear_authority", &Url::clear_authority)


        .def("get_host", get_host)
        .def("set_host", set_host)

        .def("get_port", get_port)
        .def("set_port", set_port)

        .def("get_path", get_path)
        .def("set_path", set_path)

        .def("get_query", get_query)
        .def("set_query", set_query)

        .def("has_query", &Url::has_query)
        .def("clear_query", &Url::clear_query)

        .def("get_fragment", get_fragment)
        .def("set_fragment", set_fragment)

        .def("has_fragment", &Url::has_fragment)
        .def("clear_fragment", &Url::clear_fragment)

        .def("absolute", &Url::absolute)

    ;

    def("escape_reserved_unsafe", &Url::escape_reserved_unsafe);
    def("unescape_all", unescape_all);

#if 0
	class_<Entity_handler>("Entity_handler")
		.def("replace_all_entities", &Entity_handler::replace_all_entities)
	;	
#endif

}
#endif
