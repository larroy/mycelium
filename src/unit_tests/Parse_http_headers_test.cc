#include <boost/test/unit_test.hpp>

#include <iostream>
#include "utils.hh"

/**
 * @addtogroup unit_tests
 * @{
 */
using namespace std;

BOOST_AUTO_TEST_CASE(HTML_lexer_test)
{
    const string headers_in = R"EOS(HTTP/1.1 200 OK
Server: Apache/2.2.3 (CentOS)
SLASH_LOG_DATA: shtml
Cache-Control: no-cache
Pragma: no-cache
X-XRDS-Location: http://slashdot.org/slashdot.xrds
Content-Type: text/html; charset=utf-8
Content-Length: 98342
Date: Sat, 07 Apr 2012 21:28:26 GMT
X-Varnish: 33994908 33994320
Age: 53
Connection: keep-alive)EOS";

    content_type::content_type_t ctype;
    string charset;
    map<string,string> headermap;

    utils::parse_http_headers(headers_in, ctype, charset, headermap);
    BOOST_CHECK_EQUAL(ctype, content_type::TEXT_HTML);
    BOOST_CHECK_EQUAL(charset, "utf-8");
}

/// @}
