/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Doc.hh"
#include "utils.hh"

using namespace std;

void Doc::save(mongo::DBClientConnection& c)
{
    if( url.empty() )
        throw std::runtime_error("url is empty");
    url.normalize();
    // FIXME
}

bool Doc::load_url(mongo::DBClientConnection& c, const Url& _url)
{

    url = _url;
    url.normalize();
    // FIXME
#if 0
    istringstream is(value);

    try {
        torrent::Bencode b;
        is >> b;
    //    is.close();
        if( b.has_key("url") )
            url = b["url"].as_string();

        if( b.has_key("http") )
            http_code = b["http"].as_value();

        if( b.has_key("modified") )
            modified = b["modified"].as_value();

        if( b.has_key("crawled") )
            crawled = b["crawled"].as_value();

        if( b.has_key("content_sz") )
            content_sz = b["content_sz"].as_value();

        if( b.has_key("headers") )
            headers = b["headers"].as_string();

        if( b.has_key("etag") )
            etag = b["etag"].as_string();

        if( b.has_key("charset") )
            charset = b["charset"].as_string();

        if( b.has_key("curl_code") )
            curl_code = b["curl_code"].as_value();

        if( b.has_key("curl_error") )
            curl_error = b["curl_error"].as_string();

        if( b.has_key("flags") )
            flags = b["flags"].as_value();

        if( b.has_key("title") )
            title = b["title"].as_string();

        if( b.has_key("rss2") )
            rss2 = b["rss2"].as_string();

        if( b.has_key("rss") )
            rss = b["rss"].as_string();

        if( b.has_key("atom") )
            atom = b["atom"].as_string();

    } catch (std::ios_base::failure& e){
        cerr << __FUNCTION__ << " exception: " << e.what() << endl;
        throw std::runtime_error(e.what());

    }
#endif
    return true;
}

