#include <boost/test/unit_test.hpp>

#include <iostream>
#include "Url.hh"

/**
 * @addtogroup unit_tests
 * @{
 */
using namespace std;

namespace {

void test_parsing(
    Url *u,
    const string& url,
    const string& scheme,
    const string& userinfo,
    const string& host,
    const string& port ,
    const string& path,
    const string& query,
    const string& fragment,
    bool has_authority,
    bool has_query,
    bool has_fragment
    )
{

    u->assign(url);

    if( u->scheme() != scheme ) {
        cerr << "scheme error: " << u->scheme() << " != " << scheme << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->userinfo() != userinfo ) {
        cerr << "userinfo error: " << u->userinfo() << " != " << userinfo << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->host() != host ) {
        cerr << "host error: " << u->host() << " != " << host << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->port() != port ) {
        cerr << "port error: " << u->port() << " != " << port << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->path() != path ) {
        cerr << "path error: " << u->path() << " != " << path << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->query() != query ) {
        cerr << "query error: " << u->query() << " != " << query << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->fragment() != fragment ) {
        cerr << "fragment error: " << u->fragment() << " != " << fragment << endl << " url: " << url << endl;
        throw std::runtime_error("fail");
    }
    if( u->has_authority() != has_authority ) {
        cerr << "has_authority error: " << u->has_authority() << " != " << has_authority << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->has_query() != has_query ) {
        cerr << "has_query error: " << u->has_query() << " != " << has_query << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    if( u->has_fragment() != has_fragment ) {
        cerr << "has_fragment error: " << u->has_fragment() << " != " << has_fragment << " url: " << url << endl;
        throw std::runtime_error("fail");
    }

    Url url_(url);
    if( *u != url_ ) {
        cerr << "get error: " << u->get() << " != " << url_.get() << endl;
        throw std::runtime_error("fail");
    }

    cout << "url: '" << url << "' : get(): '" << u->get() << "' PASSED." << endl;

}


void test_eq(const string& u1, const string& u2)
{
    Url url1(u1);
    Url url2(u2);
    if( url1 == url2 ) {
        cout << "url1: '" << url1.get() << "' url2: '" << url2.get() << "' EQUAL. PASSED." << endl;
    } else {
        cout << "url1: '" << url1.get() << "' url2: '" << url2.get() << "' NOT EQUAL. FAILED." << endl;
        throw std::runtime_error("fail");
    }

}

void test_size(const string& url_in)
{
    Url url(url_in);
    if( url_in.size() != url.size() ) {
    cout << "SIZES NOT EQUAL. FAILED." << " url_in:" << url_in << " url:" << url.get()  << endl;
    cout << "url.size(): " << url.size() << " url_in.size():" << url_in.size() << endl;
        throw std::runtime_error("fail");
    } else {
        cout << "OK" << " url_in:" << url_in << " url:" << url.get()  << endl;
        cout << "\turl.size(): " << url.size() << " url_in.size():" << url_in.size() << endl;
    }
}

void test_not_eq(const string& u1, const string& u2)
{
    Url url1(u1);
    Url url2(u2);
    if( url1 != url2 ) {
        cout << "url1: '" << url1.get() << "' url2: '" << url2.get() << "' NOT EQUAL. PASSED." << endl;
    } else {
        cout << "url1: '" << url1.get() << "' url2: '" << url2.get() << "' EQUAL. FAILED." << endl;
        throw std::runtime_error("fail");
    }
}


} // end anon ns


BOOST_AUTO_TEST_CASE(Url_test_parsing)
{
    Url u;
    test_parsing(&u,"",        "",        "",        "",        "",        "",        "",        "", false, false, false);
    test_parsing(&u,"mojito/para/todos","","","","","mojito/para/todos","","",false,false,false);
    test_parsing(&u,"mojito?q=a+b+c&r=c#r","","","","","mojito","?q=a+b+c&r=c","#r",false,true,true);
    test_parsing(&u,"culo?q=a&r=c#r","","","","","culo","?q=a&r=c","#r",false,true,true);
    test_parsing(&u,"file:///","file","","","","/","","",true,false,false);
    test_parsing(&u,"file:///a/b/c.html",    "file",        "",        "",        "",        "/a/b/c.html",        "",        "",true,false,false);
    test_parsing(&u,"ftp://foo.com/bar/b.html?q=r#nn",        "ftp",        "",        "foo.com",        "",        "/bar/b.html",        "?q=r",        "#nn", true, true, true);
    test_parsing(&u,"ftp://domo@foo.com:69/bar/b.html?q=r#nn",        "ftp",        "domo",        "foo.com",        "69",        "/bar/b.html",        "?q=r",        "#nn", true, true, true);
    test_parsing(&u,"ftp://%32o@f%33oo.com:69/bar/b.html?q=r#nn",        "ftp",        "%32o",        "f%33oo.com",        "69",        "/bar/b.html",        "?q=r",        "#nn", true, true, true);
    test_parsing(&u,"ftp://%32o@f%33oo.com:69/%2Fbar/b.html?q=r+b#nn",        "ftp",        "%32o",        "f%33oo.com",        "69",        "/%2Fbar/b.html",        "?q=r+b",        "#nn", true, true, true);
    test_parsing(&u,"http://[fe80::202:3fff:feb7:e652]/rabo/mo?q=a#f",        "http",        "",        "fe80::202:3fff:feb7:e652",        "",        "/rabo/mo",        "?q=a",        "#f", true, true, true);

}


BOOST_AUTO_TEST_CASE(Url_test_compare)
{
    test_eq("","");
    test_eq("/a/../b/","/b/");
    test_eq("/%61/../%62/","/b/");
    test_eq("/a/../b","/b");
    test_eq("/a/../b","/%62");
    test_eq("../b","../b");
    test_eq("../b/","../b/");
    test_eq("b/../c","c");
    test_eq("b/a/i/../c","b/a/c");
    test_eq("b/a/././../i/../c","b/c");
    test_eq(".",".");
    // test_eq(".","./"); // this one should pass I guess FIXME
    test_eq("http://domo.com/a/../b","http://domo.com/b");
    test_eq("http://domo.com/a/../b/","http://domo.com/b/");
    test_eq("http://note@domo.com/a/../b/","http://note@domo.com/b/");

    test_not_eq("","?");
    test_not_eq("","#");
    test_not_eq("http://host.com/#","http://host.com/");
    test_not_eq("http://host.com/?","http://host.com/");
    test_not_eq("http://note@domo.com/a/../b/","http://note@domo.com/?q#f");
    test_not_eq("http://note@domo.com/","http://note@domo.com/?#");

    test_size("http://host.com/#");
    test_size("http://host.com/?");
    test_size("http://host.com/path#crap");
    test_size("http://host.com/omg?query");
    test_size("http://note@domo.com/a/../b/");
    test_size("http://note@domo.com/hello_cat");
}
/// @}
