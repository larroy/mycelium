/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Doc.hh"
#include "utils.hh"
#include <cassert>

using namespace std;

void Doc::save(mongo::DBClientConnection& c, const string& ns)
{
    if( url.empty() )
        throw std::runtime_error("url is empty");
    url.normalize();

    c.ensureIndex(ns, BSON("url" << 1));

    bson::bob b;
    //b.genOID();
    //

    b.append("url", url.get());

    if (! eff_url.empty())
        b.append("eff_url", eff_url.get());

    if (http_code != 0)
        b.append("http_code", http_code);

    if (curl_code != -1)
        b.append("curl_code", curl_code);

    if (! curl_error.empty())
        b.append("curl_error", curl_error);

    if (modified != -1)
        b.append("modified", (long long) modified);

    if (crawled != -1)
        b.append("crawled", (long long) crawled);

    if (! content.empty())
        //b.append("content", content);
        b.appendBinData("content", static_cast<int>(content.size()), mongo::BinDataGeneral, content.c_str());

    if (! headers.empty())
        b.append("headers", headers);

    if (! etag.empty())
        b.append("etag", etag);

    if (content_type != content_type::UNSET)
        b.append("content_type", (unsigned int) content_type);

    if (! charset.empty())
        b.append("charset", charset);

    if (flags.any())
        b.append("flags", static_cast<unsigned int>(flags.to_ulong()));

    if (! title.empty())
        b.append("title", title);

    if (! rss2.empty())
        b.append("rss2", rss2);

    if (! rss.empty())
        b.append("rss", rss);

    if (! atom.empty())
        b.append("atom", atom);

    b.append("indexed", indexed);

    // upsert
    c.update(ns, BSON("url" << url.get()), BSON("$set" << b.obj()), true);
    //c.insert(ns, b.obj());
}

bool Doc::load_url(mongo::DBClientConnection& c, const string& ns, const Url& _url)
{

    url = _url;
    url.normalize();

    std::auto_ptr<mongo::DBClientCursor> cursor = c.query(ns, QUERY("url" << url.get()));
    bool gotone = false;
    while (cursor->more()) {
        mongo::BSONObj doc = cursor->next();

        if (doc.hasField("eff_url")) {
            string eff_url_tmp;
            doc["eff_url"].Val(eff_url_tmp);
            eff_url = eff_url_tmp;
        }

        if (doc.hasField("http_code"))
            doc["http_code"].Val(http_code);

        if (doc.hasField("curl_code"))
            doc["curl_code"].Val(curl_code);

        if (doc.hasField("curl_error"))
            doc["curl_error"].Val(curl_error);

        if (doc.hasField("modified"))
            modified = doc["modified"].numberLong();

        if (doc.hasField("crawled"))
            crawled = doc["crawled"].numberLong();

        // We store binary as the data is in various encodings != UTF-8
        if (doc.hasField("content")) {
            int len = 0;
            const char* buff = doc["content"].binData(len);
            content.assign(buff, static_cast<size_t>(len));
        }

        if (doc.hasField("headers"))
            doc["headers"].Val(headers);

        if (doc.hasField("etag"))
            doc["etag"].Val(etag);

        if (doc.hasField("content_type"))
            doc["content_type"].Val(content_type);

        if (doc.hasField("charset"))
            doc["charset"].Val(charset);

        if (doc.hasField("flags"))
            flags = doc["flags"].Int();

        if (doc.hasField("title"))
            doc["title"].Val(title);

        if (doc.hasField("rss2"))
            doc["rss2"].Val(rss2);

        if (doc.hasField("rss"))
            doc["rss"].Val(rss);

        if (doc.hasField("atom"))
            doc["atom"].Val(atom);

        if (doc.hasField("indexed"))
            doc["indexed"].Val(indexed);

        if (gotone) {
            clog << "Got a duplicated document by url :(" << endl;
            assert(0);
        }
        gotone = true;
    }
    return false;
}

