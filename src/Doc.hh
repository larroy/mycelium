/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#ifndef Doc_hh
#define Doc_hh
#include <string>
#include "Url.hh"
#include <boost/utility.hpp>
#include <memory>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include "client/dbclient.h"

#define LINKSFNAME        "links.gz"
#define CONTENTSFNAME    "contents.gz"
#define LEXEDFNAME        "contents_lexed.gz"
#define WARNINGSFNAME    "warnings"
#define TOKENSFNAME        "tokens.gz"
#define TERM_VEC_DIR    "term_vec_dir"
#define LEXICONFNAME    "lexicon"
#define METAFNAME        "meta"
#define NGRAMS            "ngrams"
#define NGRAMSFILE        "ngrams.txt.gz"
#define NGRAM_SPACE_DIR "ngram_space"
#define NGRAM_SPACE_FILE "ngram_space.gz"
#define NGRAM_SPACE_INFO_FILE "ngram_space_info.txt"
#define NGRAM_VEC        "ngram_vec.gz"
#define    TEMP_SUFFIX        ".tmp"
#define MAXCONTENTSLEX    4194304
#define IIDXFILE        "iidx.bin"

#ifndef DB_DIR
#define DB_DIR "mycelium_db"
#endif


#define DOC_UTF8_OK_FLAG 0x01


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
        content_type(),
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

#endif
