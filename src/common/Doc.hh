/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */
#pragma once

#include <string>
#include "Url.hh"
#include <boost/utility.hpp>
#include <memory>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include "client/dbclient.h"
#include "content_type.hh"


/**
 * @class Doc
 * @author piotr
 * @brief Base class for documents
 * A crawled url becomes a document
 */
struct Doc : boost::noncopyable {
    Doc() :
        url(),
        http_code(0),
        curl_code(-1),
        curl_error(),
        modified(-1),
        crawled(-1),
        content(),
        headers(),
        etag(),
        content_type(content_type::UNSET),
        charset(),
        flags(0),
        title(),
        rss2(),
        rss(),
        atom()
    {}
    //Doc(const boost::filesystem::path&);

    ~Doc()
    {}

    enum {
        FLAG_EMPTY = 0,
        FLAG_UTF8_OK = 1,
        FLAG_HAS_SYNDICATION,
        FLAG_IS_SYNDICATED,
        FLAG_INDEX,
        FLAG_FOLLOW,
        FLAG_SIZE
    };

    void save(mongo::DBClientConnection&, const std::string& ns);
    bool load_url(mongo::DBClientConnection&, const std::string& ns, const Url&);

    Url            url;
    int            http_code;
    int            curl_code;
    std::string    curl_error;
    /// modification time in seconds since the epoch
    long           modified;
    /// in seconds since the epoch
    long           crawled;
    std::string    content;
    std::string    headers;
    std::string    etag;
    int            content_type;
    std::string    charset;
    std::bitset<FLAG_SIZE> flags;
    std::string    title;
    std::string    rss2;
    std::string    rss;
    std::string    atom;
};
