/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */
#include "Url.hh"
#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/string.hpp>

//#include "Url_lexer.hh"

using namespace std;
using namespace boost;
using namespace Url_util;

static const char* scheme_defport[] = {
    "80",
    "21",
    "0"
};

static const boost::regex url_re(url_regex[RE_URL],regex_constants::perl);


#define base_ctors\
    _path(),\
    m_suspicious(),\
    m_scheme(),\
    m_has_authority(),\
    m_host_ip_literal(),\
    m_userinfo(),\
    m_host(),\
    m_port(),\
    m_query(),\
    m_fragment()

Url::Url(const std::string& s) :
    base_ctors
{
    assign(s);
}

Url::Url()  :
    base_ctors
{
}
#undef base_ctors

void Url::assign(const string& s)
{
    this->clear();
    enum {
        URL = 0,
        SCHEME_CLN = 1,
        SCHEME    = 2,
        DSLASH_AUTH = 3,
        AUTHORITY = 4,
        PATH    = 5,
        QUERY    = 6,
        FRAGMENT = 7
    };

    try {
        //regex re(url_regex[RE_URL],regex_constants::perl);
        smatch match;
        if( regex_match(s, match, url_re) ) {
            /* bad cases:
             * ////rrara
             * 3: //
             * 5: //rara
             * then is this url an absolute path like /rara  ?
             * so rule of thumb is to sanityze double slashes on paths.
             */
            if( match[SCHEME].matched )
                scheme(match[SCHEME].str());

            if( match[DSLASH_AUTH].matched ) {
                // Only valid for locally scoped urls like file:/// that define a default host, since http:/// makes no sense, authority must exists for globally scoped urls
                // in this case 3: // 4 matches, but its empty
                // if url == "/////" then 3: // 4 matches but empty 5: /// we treat this like an absolute path, but only if there's no scheme.
                if( match[DSLASH_AUTH].str() == "//") {
                    // if( scheme().empty() )
                    // this is a relative uri with an absolute path we are ok.
                    if ( ! scheme().empty() && scheme() != "file" )
                        throw UrlParseError("empty authority part, with // is not allowed for schemes other than 'file'");
                }
            }

            if( match[AUTHORITY].matched )
                authority(match[AUTHORITY].str());
            if( match[PATH].matched )
                //DLOG(cout << "PATH match: |" << match[PATH].str() << "|" << endl);
                path(match[PATH].str());

            if( match[QUERY].matched )
                query(match[QUERY].str());

            if( match[FRAGMENT].matched )
                fragment(match[FRAGMENT].str());

            if( ! syntax_ok() )
                throw UrlParseError(" ! syntax_ok() for this url, sanity checks failed");

            if( ! valid_host())
                throw UrlParseError(" ! valid_host() for this url, sanity checks failed");

        } else {
            throw UrlParseError("Url doesn't match primary url regex");
        }
    } catch(regex_error re) {
        string s("Url::assign() boost::regex_error: ");
        s+=re.what();
        throw UrlParseError(s);
    }
}

bool Url::syntax_ok() const  {
    string s;
    if( has_authority() )
        if( ! ( _path.empty() || _path.absolute() ) )
            return false;

    if( scheme().find_first_of(":/?#") != string::npos )
        return false;

    if( host().find_first_of("/?#") != string::npos )
        return false;

    if( ! valid_host())
        return false;

    if( path().find_first_of("?#") != string::npos )
        return false;

    if( query().find_first_of("#") != string::npos )
        return false;

    return true;
}

bool Url::valid_host() const
{
    if( host().empty() )
        return true;
    regex re;

    string h = unescape_safe(host());
    re.assign(url_regex[RE_HOST]);
    if( regex_match(h,re) )
        return true;

    re.assign(url_regex[RE_IPVFUT]);
    if( regex_match(h,re) )
        return true;

    re.assign(url_regex[RE_IPV6]);
    if( regex_match(h,re) )
        return true;

    re.assign(url_regex[RE_IPV4]);
    if( regex_match(h,re) )
        return true;

    return false;
}

bool Url::valid_host(const std::string& h)
{
    if( h.empty() )
        return true;
    regex re;
    string host = unescape_safe(h);

    re.assign(url_regex[RE_HOST]);
    if( regex_match(host,re) )
        return true;

    re.assign(url_regex[RE_IPVFUT]);
    if( regex_match(host,re) )
        return true;

    re.assign(url_regex[RE_IPV6]);
    if( regex_match(host,re) )
        return true;

    re.assign(url_regex[RE_IPV4]);
    if( regex_match(host,re) )
        return true;

    return false;
}

Url& Url::merge_ref(const Url& u)
{

    if( ! u.syntax_ok() )
        throw BadUrl("supplied url ! syntax_ok");



    if( this->absolute() && ! u.absolute() ) {
        if( u.has_scheme() ) {
            this->clear();
            // SCHEME
            this->scheme(u.scheme());
            // AUTHORITY
            if( u.has_authority() )
                this->authority(u.authority());
            // PATH
            this->path(u.path());
            // QUERY
            this->query(u.query());
//            if( u.has_query() )
//                this->query(u.query());
//            else
//                this->clear_query();
            // FRAGMENT
        } else {
            if( u.has_authority() ) {
                this->authority(u.authority());
                this->path(u.path());
                // QUERY
                if( u.has_query() )
                    this->query(u.query());
                //else
                //    this->clear_query();
            } else {
                if( u._path.empty() ) {
                    if( u.has_query() )
                        this->query(u.query());
                } else {
                    this->_path.merge(u._path); // magic lies here
                    if( u.has_query() )
                        this->query(u.query());
                    else
                        this->clear_query();
                }

                // FRAGMENT
                if( u.has_fragment() )
                    this->fragment(u.fragment());
                else
                    this->clear_fragment();
            }
        }
        if( u.has_fragment() )
            this->fragment(u.fragment());
        else
            this->clear_fragment();

    } else if ( ! this->absolute() && u.absolute() ) {
        throw runtime_error("Can only merge an absolute url with a reference");
    } else if ( this->absolute() && u.absolute() ) {
        throw runtime_error("Can't merge two absolute urls");
    } else if ( ! this->absolute() && ! u.absolute() ) {
        throw runtime_error("Can't merge two relative references");
    } else {
        throw logic_error("Url::merge_ref");
    }

    return *this;
}

Url& Url::operator+=(const Url& u)
{
    return merge_ref(u);
}

Url& Url::operator=(const string& s)
{
    assign(s);
    return *this;
}

/*
 * Note: URIs that differ in the replacement of a reserved character with its
 * corresponding percent-encoded octet are not equivalent
 */
bool Url::operator==(const Url& u) {
    //cout << "operator==" << endl;
    Url rhs = u;
    Url lhs = *this;
    rhs.normalize();
    lhs.normalize();
    string rhss = rhs.get();
    string lhss = lhs.get();
    //cout << rhss << " " << lhss << endl;
    if( rhss == lhss )
        return true;
    else
        return false;
}

bool Url::operator!=(const Url& u) {
    //cout << "operator!=" << endl;
    if( *this == u )
        return false;
    else
        return true;
}

ostream& operator<<(ostream& os, const Url& u) {

    if( u.has_authority() )
        os << "has_authority" << endl;
    if( ! u.m_scheme.empty() )
        os << "scheme: " << u.m_scheme << endl;
    if( ! u.m_userinfo.empty() )
        os << "userinfo: " << u.m_userinfo << endl;
    if( ! u.m_host.empty() )
        os << "host: " << u.m_host << endl;
    if( ! u.m_port.empty() )
        os << "port: " << u.m_port << endl;
    if( ! u._path.empty() )
        os << "path: " << u._path << endl;
    if( u.has_query() ) {
        os << "has_query" << endl;
        os << "query: " << u.m_query << endl;
    }
    if( u.has_fragment() )  {
        os << "has_authority" << endl;
        os << "fragment: " << u.m_fragment << endl;
    }
    os << endl;
    return os;
}

void Url::normalize_scheme()  {
    to_lower(m_scheme);
}


void Url::normalize_host()  {
    for(string::iterator i = m_host.begin(); i != m_host.end(); ++i) {
        //(*i) = tolower(*i);
        // tolower depends on locale, according to rfc 4343 we should only lowercase
        // from ascii 0x41-0x5A to 0x61-0x7A
        if( *i >= 0x41 && *i <= 0x5A )
            *i = *i + 0x20;

    }
}

string Url::normalize_escapes(const string& s)  {
    string res = unescape_safe(s);
    for(string::iterator i = res.begin(); i != res.end(); ++i) {
        if(*i == '%' && (i+1) != res.end() && (i+2) != res.end()
            && isxdigit(*(i+1)) && isxdigit(*(i+2)) ) {
            *(i+1) = toupper(*(i+1)); // mandated by the rfc
            *(i+2) = toupper(*(i+2));
            i += 2;
        }
    }
    return res;
}

void Url::normalize_escapes()
{
    string s = get();
    s = normalize_escapes(s);
    try {
        assign(s);
    } catch(UrlParseError e) {
        string w("normalize_escapes: ");
        w+=e.what();
        throw runtime_error(w);
    }
}

void Url::normalize()
{
    normalize_scheme();
    normalize_host();
    normalize_escapes();
    normalize_path();

}

/***** ACCESSORS *****/
void Url::scheme(const string& s)
{
    try {
        regex re(url_regex[RE_SCHEME]);
        if( regex_match(s,re) ) {
            m_scheme = s;
            to_lower(m_scheme);
            //if( m_scheme == "file" )
            //    authority("/");
            m_has_authority = true;
        } else
            throw UrlParseError("scheme: " + s + " doesn't match scheme validation regex");
    } catch(regex_error re) {
        throw UrlParseError("Url::scheme("+s+"): boost::regex_error: " + re.what());
    } catch(UrlParseError e) {
        throw;
    } catch(...) {
        throw UrlParseError("Url::scheme("+s+"): throwed an unknown exception");
    }
}

/**
 * authority     = [ userinfo "@" ] host [ ":" port ]
 */
void Url::authority(const string& s)
{
    try {
        // [userinfo@]host[:port]
        string::size_type user_b = 0, user_e=0, host_b=0, host_e=0, port_b=0;
        if( (user_e = s.find('@')) != string::npos ) {
            assert(user_e - user_b > 0);
            userinfo(s.substr(0,user_e - user_b));
            if( ! ((host_b = (user_e+1)) < s.size()) ) throw UrlParseError("authority doesn't have host part: " + s);
        }
        // IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
        if(    s[host_b] == '[' ) {
            m_host_ip_literal = true;
            if( ! ((++host_b) < s.size()) ) throw UrlParseError("authority incomplete host part, nothing follows \'[\': " + s);
            if( (host_e = s.find(']',host_b)) != string::npos) {
                assert(host_e - host_b > 0 );
                host(s.substr(host_b,host_e - host_b));
            } else
                throw UrlParseError("authority incomplete host part, couldn't find closing \']\':" + s.substr(host_b) + " : " + s);
        } else {
            if( (host_e = s.find(':',host_b)) == string::npos) { // no ":port" follows
                host(s.substr(host_b));
                /* if( ! m_scheme.empty() )
                    set_def_port(); */
            } else {
                if( host_e == host_b ) {
                    // starts with :
                    // invalid
                    throw UrlParseError("authority starts with : without @ part");
                } else {
                    assert(host_e - host_b > 0 ); // :rabo problem
                    host(s.substr(host_b,host_e - host_b));
                    if(  (port_b = (host_e+1)) < s.size() ) // no port part
                        port(s.substr(port_b));
                    else
                        throw UrlParseError("no port number after :");
                }
            }
        }
    } catch(regex_error re) {
        throw UrlParseError("Url::authority("+s+"): boost::regex_error: " + re.what());
    } catch(UrlParseError e) {
        throw;
    } catch(...) {
        throw UrlParseError("Url::authority("+s+"): throwed an unknown exception");
    }
}

string Url::authority() const
{
    string result;
    if( m_host.empty() )
        return result;

    result.reserve(m_userinfo.size()+m_host.size()+m_port.size()+3);
    if ( ! m_userinfo.empty() ) {
        result += m_userinfo;
        result += "@";
    }
    if( m_host_ip_literal ) {
        result += '[';
        result += m_host;
        result += ']';
    } else {
        result += m_host;
    }
    if ( ! m_port.empty()  ) {
        result += ":";
        result += m_port;
    }
    return result;
}


bool Url::empty() const
{
    if( ! m_scheme.empty() || m_has_authority || ! _path.empty() || has_query() || has_fragment() )
        return false;
    else
        return true;
}

bool Url::absolute() const
{
    if(  ! m_scheme.empty()  )
        return true;
    else
        return false;
}

void Url::userinfo(const string& s)
{
    m_userinfo.assign(escape(s,URL_CHAR_AUTH));
}

void Url::host(const string& s)
{
    // the rfc allows reg-name (host) to be escaped for non ASCII registered names
    // FIXME rfc 3490
    try {
        //string norm = escape(unescape_safe(s),URL_CHAR_AUTH); // Unescape safe on host names or not??
        string norm = escape(s,URL_CHAR_AUTH);
        if(valid_host(norm)) {
            m_host.assign(norm);
            m_has_authority = true;
        } else
            throw UrlParseError("Url::host("+s+"): Invalid host");
    } catch(UrlParseError) {
        throw;
    } catch(regex_error re) {
        throw UrlParseError("Url::host("+s+"): boost::regex_error: " + re.what());
    } catch(...) {
        throw UrlParseError("Url::host("+s+"): throwed an unknown exception");
    }
}

void Url::port(const string& s)
{
    // the rfc doesn't allow port to be escaped
    try {
        if( s.empty() ) {
            m_port.clear();
        } else {
            regex re(url_regex[RE_PORT]);
            if( ! regex_match(s,re) )
                throw UrlParseError("Url::port("+s+"): Invalid port: regex didn't match");
            int port;
            istringstream is(s);
            is >> port;
            if( is.fail() )
                throw UrlParseError("Url::port("+s+"): not an integer");
            if( ! (port < (1<<16) && port > 0) )
                throw UrlParseError("Url::port("+s+"): out of range (0,2^16)");
            m_port.assign(s);
        }
    } catch(regex_error re) {
        throw UrlParseError("Url::port("+s+"): boost::regex_error: " + re.what());
    } catch(UrlParseError e) {
        throw;
    } catch(...) {
        throw UrlParseError("Url::port("+s+"): throwed an unknown exception");
    }
}

int Url::port_int() const
{
    if( m_port.empty() )
        throw BadUrl("port is empty");
    else {
        int port;
        istringstream is(m_port);
        is >> port;
        if( is.fail() )
            throw BadUrl("port is not an integer");
        if( ! (port < (1<<16) && port >= 0) )
            throw BadUrl("port is out of range [0,2^16)");
        return port;
    }
}

std::string Url::port() const
{
/*    if( m_port.empty() )
        if( m_scheme == "http")
            return scheme_defport[SCHEME_HTTP];
        else if ( m_scheme == "ftp")
            return scheme_defport[SCHEME_FTP];
        else if ( m_scheme == "file")
            return "";
        else
            return "";
    else */
        return m_port;
}

void Url::path(const string& s)
{
    _path.assign(escape(s,URL_CHAR_PATH));
    if( has_authority() )
        _path.absolute(true);
}

void Url::query(const string& s)
{
    m_query.assign(escape(s,URL_CHAR_QUERY));
    //_has_query = true;
}

void Url::fragment(const string& s)
{
    m_fragment.assign(escape(s,URL_CHAR_FRAGMENT));
    //_has_fragment = true;
}
/***** END ACCESSORS *****/

// This one is a bad idea, don't use it
void Url::set_def_port()
{
    using namespace utils;
    if( ! m_scheme.empty() ) {
        if( scheme() == "http")
            port(scheme_defport[SCHEME_HTTP]);
        else if ( scheme() == "ftp")
            port(scheme_defport[SCHEME_FTP]);
        else if ( scheme() == "file")
            m_port.assign(scheme_defport[SCHEME_FILE]);
        else
            throw UrlParseError("set_def_port: unknown default port for scheme: " + m_scheme);

    } else
        throw UrlParseError("set_def_port: scheme is empty");

}


string Url::get() const {
    string res;
    res.reserve(size());

    if( ! m_scheme.empty() )  {
        res += scheme();
        res += ":";
    }
    if( has_authority() ) {
        res += "//";
        res += authority();
    }
    res += path();
    res += query();
    res += fragment();
//    if( has_query() ) {
//        res += "?";
//        res += query();
//    }
//    if( has_fragment() ) {
//        res += "#";
//        res += fragment();
//    }
    return res;
}

size_t Url::size() const
{
    size_t res = 0;
    if( ! m_scheme.empty() )  {
        res += m_scheme.size();
        ++res;//  ":";
    }
    if( has_authority() ) {
        res+=2;// += "//";
        res += authority().size();
    }
    res += path().size();
    res += query().size();
    res += fragment().size();
//    if( has_query() ) {
//        ++res;// += "?";
//        res += query().size();
//    }
//    if( has_fragment() ) {
//        ++res;// += "#";
//        res += fragment().size();
//    }
    return res;
}


std::string Url::escape_reserved_unsafe(const std::string& s)
{
    return escape(s,(URL_CHAR_RESERVED|URL_CHAR_UNSAFE));
}

std::string Url::escape(const string& s, const unsigned char mask)
{
    if(s.empty())
        return s;
    string result;
    string::size_type reserve_ahead = s.size();
    for(string::const_iterator j = s.begin(); j != s.end(); ++j)  {
        if( url_char_test(*j, mask) ) {
            if(*j == '%' && (j+1) != s.end() && (j+2) != s.end()
                && isxdigit(*(j+1)) && isxdigit(*(j+2)) ) {
            } else {
                reserve_ahead+=2; // escaping one, augments it by two more
            }
        }
    }
    result.reserve(reserve_ahead);
    for(string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if( url_char_test(*i, mask) ) {
            //cout << "test: " << *i << hex <<(int) *i << endl;
            if(*i == '%' && (i+1) != s.end() && (i+2) != s.end()
                && isxdigit(*(i+1)) && isxdigit(*(i+2)) ) {
                // this is a valid escaped sequence, don't escape the %
                result += *i;
                result += *(i+1);
                result += *(i+2);
                i += 2;
            } else {
                char c1,c2;
                //cout << " (*i) >> 4:" << hex << ((*i) >> 4) <<endl;
                c1 = utils::digit_to_xnum((unsigned char)(*i) >> 4);
                c2 = utils::digit_to_xnum( (*i) & 0xf);
                result += "%";
                result += c1;
                result += c2;
            }
        } else {
            result += (*i);
        }
    }
    result.reserve(); // shrink to fit
    return result;
}

string Url::unescape(const string& s)
{
    // avoid copying if there's nothing to unescape
    if( s.empty() || s.find('%') == string::npos )
        return s;
    string result;
    result.reserve(s.size());
    for(string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if( *i == '%' && (i+1) != s.end() && (i+2) != s.end()
        && isxdigit(*(i+1)) && isxdigit(*(i+2)) //
        && ((*(i+1)) != 0 || (*(i+2)) != 0)) { // can't unescape null

            char c = utils::x2digits_to_num(*(i+1),*(i+2));
            result += c;
            i += 2; // increment the iterator
        } else
            result += (*i);
    }
    result.reserve(); // shrink to fit
    return result;
}

string Url::unescape(const string& s, const unsigned char mask) {
    // avoid copying if there's nothing to unescape
    if( s.empty() || (s.find('%') == string::npos ))
        return s;
    string result;
    result.reserve(s.size());
    for(string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if( *i == '%' && (i+1) != s.end() && (i+2) != s.end()
        && isxdigit(*(i+1)) && isxdigit(*(i+2)) //
        && ((*(i+1)) != 0 || (*(i+2)) != 0)) { // can't unescape null

            char c = utils::x2digits_to_num(*(i+1),*(i+2));
            if( url_char_test(c, mask)  ) {
                result += c;
                i += 2;
            } else
                result += (*i);
        } else
            result += (*i);
    }
    result.reserve(); // shrink to fit
    return result;
}

string Url::unescape_not(const string& s, const unsigned char mask) {
    // avoid copying if there's nothing to unescape
    if( s.empty() || (s.find('%') == string::npos ))
        return s;
    string result;
    result.reserve(s.size());
    for(string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if( *i == '%' && (i+1) != s.end() && (i+2) != s.end()
        && isxdigit(*(i+1)) && isxdigit(*(i+2)) //
        && ((*(i+1)) != 0 || (*(i+2)) != 0)) { // can't unescape null

            char c = utils::x2digits_to_num(*(i+1),*(i+2));
            if( (url_char_table[(unsigned char)(c)] & mask) == 0  ) {
                result += c;
                i += 2;
            } else
                result += (*i);
        } else
            result += (*i);
    }
    result.reserve(); // shrink to fit
    return result;
}

string Url::unescape_safe(const string& s) {
    // avoid copying if there's nothing to unescape
    if( s.empty() || (s.find('%') == string::npos ))
        return s;
    string result;
    result.reserve(s.size());
    for(string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if( *i == '%' && (i+1) != s.end() && (i+2) != s.end()
        && isxdigit(*(i+1)) && isxdigit(*(i+2)) //
        && ((*(i+1)) != 0 || (*(i+2)) != 0)) { // can't unescape null

            char c = utils::x2digits_to_num(*(i+1),*(i+2));
            if( (url_char_table[(unsigned char)(c)] & (URL_CHAR_RESERVED|URL_CHAR_UNSAFE)) == 0 ) { // not reserved or unsafe
                result += c;
                i += 2;
            } else
                result += (*i);
        } else
            result += (*i);
    }
    result.reserve(); // shrink to fit
    return result;
}
