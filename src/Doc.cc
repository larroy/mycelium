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

void Doc::save(mongo::DBClientConnection& c, const string& ns)
{
    if( url.empty() )
        throw std::runtime_error("url is empty");
    url.normalize();

    mongo::BSONObj doc = BSON(
        "url" << url.get() <<
        "http_code" << http_code <<
        "curl_code" << curl_code <<
        "curl_error" << curl_error <<
        "modified" << (long long) modified <<
        "crawled" << (long long) crawled <<
        "content" << content <<
        "headers" << headers <<
        "etag" << etag <<
        "content_type" << (unsigned int) content_type <<
        "charset" << charset <<
        "flags" << static_cast<unsigned int>(flags.to_ulong()) <<
        "title" << title << 
        "rss2" << rss2 <<
        "rss" << rss <<
        "atom" << atom);

    c.insert(ns, doc);
}

bool Doc::load_url(mongo::DBClientConnection& c, const string& ns, const Url& _url)
{

    url = _url;
    url.normalize();

    std::auto_ptr<mongo::DBClientCursor> cursor = c.query(ns, QUERY("url" << url.get()));
    while (cursor->more()) {
        mongo::BSONObj doc = cursor->next();

        doc["http_code"].Val(http_code);
        doc["curl_code"].Val(curl_code);
        doc["curl_error"].Val(curl_error);
        modified = doc["modified"].numberLong();
        crawled = doc["crawled"].numberLong();
        doc["content"].Val(content);
        doc["headers"].Val(headers);
        doc["etag"].Val(etag);
        doc["content_type"].Val(content_type);
        doc["charset"].Val(charset);
        flags = doc["flags"].Int();
        doc["title"].Val(title);
        doc["rss2"].Val(rss2);
        doc["rss"].Val(rss);
        doc["atom"].Val(atom);
        return true;
    }
    return false;
}

