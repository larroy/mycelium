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
#include <zlib.h>
#include "Url.hh"
#include <boost/utility.hpp>
#include <memory>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include "bighash.hh"

#define LINKSFNAME		"links.gz"
#define CONTENTSFNAME	"contents.gz"
#define LEXEDFNAME		"contents_lexed.gz"
#define WARNINGSFNAME	"warnings"
#define TOKENSFNAME		"tokens.gz"
#define TERM_VEC_DIR	"term_vec_dir"
#define LEXICONFNAME	"lexicon"
#define METAFNAME		"meta"
#define NGRAMS			"ngrams"
#define NGRAMSFILE		"ngrams.txt.gz"
#define NGRAM_SPACE_DIR "ngram_space"
#define NGRAM_SPACE_FILE "ngram_space.gz"
#define NGRAM_SPACE_INFO_FILE "ngram_space_info.txt"
#define NGRAM_VEC		"ngram_vec.gz"
#define	TEMP_SUFFIX		".tmp"
#define MAXCONTENTSLEX	4194304
#define IIDXFILE		"iidx.bin"

#ifndef DB_DIR
#define DB_DIR "mycelium_db"
#endif


#define DOC_UTF8_OK_FLAG 0x01


const char* get_dbdir();
/**
 * @class Doc
 * @author piotr
 * @brief Base class for documents
 * A crawled url becomes a document
 *
 */

struct Doc : boost::noncopyable {
	Doc() :
		url(),
		http_code(0),
		curl_code(-1),
		curl_error(),
		modified(-1),
		crawled(-1),
		content_sz(0),
		content_fd(-1),
		content_gz_f(NULL),
		content(),
		headers(),
		etag(),
		content_type(),
		charset(),
		flags(0),
		title(),
		rss2(),
		rss(),
		atom(),
        hash_bucket()
	{}
	//Doc(const boost::filesystem::path&);

	~Doc();

	enum {
		FLAG_EMPTY = 0,
		FLAG_UTF8_OK = 1,
		FLAG_HAS_SYNDICATION,
		FLAG_IS_SYNDICATED,
		FLAG_INDEX,
		FLAG_FOLLOW,
		FLAG_SIZE
	};

	void save();
	bool load_url(const Url& url);
	void load_leaf(const std::string& leaf);
	std::string leaf();
	void open_content();
    bool is_open_content()
    {
        if (content_fd >= 0 || content_gz_f )
            return true;
        return false;
    }
    void unlink_content();
	void close_content();

    void lock();
    void unlock() { hash_bucket.reset(); }
    bool locked() { return hash_bucket ? true : false; }

	Url			url;
	long		http_code;
	int			curl_code;
	std::string	curl_error;
	// modification time in seconds since the epoch
	long		modified;
	// in seconds since the epoch
	long		crawled;
	size_t		content_sz;
	int			content_fd;
	gzFile		content_gz_f;
	std::string content;
	std::string	headers;
	std::string	etag;
	int			content_type;
	std::string	charset;
	std::bitset<FLAG_SIZE> flags;
	std::string	title;
	std::string	rss2;
	std::string	rss;
	std::string	atom;


private:
    boost::shared_ptr<big_hash::big_hash_bucket> hash_bucket;


//	void set_curl_info_http_headers(CURL* easy,CURLcode code);
//	bool apply_index_criteria();
//	void set_content_type();
//	bool set_content_length();
//
//	/**
//	 * Docs that we want to crawl
//	 */
//	static void headers_map(const std::string& headers, std::map<std::string,std::string>& header);

};

struct Doc_factory {
	static std::auto_ptr<Doc> create_from_meta_file(const char* metaf);
	//static Doc* create_from_url(const char* metaf);
};


class Less_doc{
public:
	bool operator()(const Doc& left, const Doc& right);
};



#endif
