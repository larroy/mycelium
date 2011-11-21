/*
 * Copyright 2007 Pedro Larroy Tovar 
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Doc.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.hh"
#include "gzstream.hh"
#include "bencode.hh"


#include <boost/filesystem.hpp>


using namespace big_hash;

const char* get_dbdir()
{
	const char* res=0;
	if ( (res = getenv("DB_DIR")) )
		return res;
	else	
		return DB_DIR;
}

void Doc::open_content()
{
	using namespace std;
	namespace bfs = boost::filesystem;
	using bfs::path;

	if( url.empty() )
		throw std::runtime_error("url is empty");

    /*********************/
    if ( ! locked() )
        lock();
    /*********************/

	path contents(hash_bucket->bucket_dir());
	contents /= CONTENTSFNAME;

	content_fd = open(contents.string().c_str(), O_RDWR | O_CREAT | O_TRUNC, 00644);
	if( content_fd < 0 )
		utils::err_sys("open_content failure");
	content_gz_f = gzdopen(content_fd, "w+b");	
	if( content_gz_f == NULL ) {
		int err;
		utils::err_sys(fs("gzdopen failed: "<< gzerror(content_gz_f,&err) ));
	}	
	content_sz = 0;
}

void Doc::unlink_content()
{
	namespace bfs = boost::filesystem;
	using bfs::path;
    if (! is_open_content())
        return;

    /*********************/
    if ( ! locked() )
        lock();
    /*********************/
	path contents(hash_bucket->bucket_dir());
	contents /= CONTENTSFNAME;
    
    close_content();
    if( unlink(contents.string().c_str()) < 0)
        utils::err_sys("unlink");
}

void Doc::close_content()
{
	if(content_gz_f) {
		if ( gzclose(content_gz_f) != Z_OK )
		    utils::err_sys("gzclose");
		content_gz_f = NULL;
	}	
    if(content_fd != -1) {
        // gzclose will have closed this
        // close(content_fd);
        content_fd = -1;
    }
}	

#if 0
void Doc::write_headers()
{
	using namespace std;
	string buckdir;
	doc_store::buckdir(url.get().c_str(), buckdir);
	buckdir.append("headers");
	ogzstream os(buckdir.c_str(), ios_base::out | ios_base::trunc);
	if( ! os.good() )
		utils::err_sys("bad ogzstream");

	os << headers;	
	os.close();
}
#endif

void Doc::save()
{
	using namespace std;
	using namespace torrent;
	ostringstream os;

	if( url.empty() )
		throw std::runtime_error("url is empty");

	url.normalize();
    /*********************/
    if ( ! locked() )
        lock();
    /*********************/

	Bencode map(Bencode::TYPE_MAP);
	map.insert_key("url", url.get());
	map.insert_key("http", http_code);
	if( modified != -1 )
		map.insert_key("modified", modified);

	if( crawled != -1)
		map.insert_key("crawled", crawled);

	if( content_sz != 0 )
		map.insert_key("content_sz", content_sz);

	if( flags.to_ulong() )
		map.insert_key("flags", flags.to_ulong());

	if( ! headers.empty() )
		map.insert_key("headers", headers);

	if( ! etag.empty() )
		map.insert_key("etag", etag);

	if( ! charset.empty() )
		map.insert_key("charset", charset);

	if( ! title.empty() )	
		map.insert_key("title", title);

	if( ! rss2.empty() )	
		map.insert_key("rss2", rss2);

	if( ! rss.empty() )	
		map.insert_key("rss", rss);

	if( ! atom.empty() )	
		map.insert_key("atom", atom);

	if( curl_code != 0 ) {
		map.insert_key("curl_code", curl_code);
		map.insert_key("curl_error", curl_error);
	}
	os << map;
	hash_bucket->set(os.str());
}

std::string Doc::leaf()
{
	if ( ! url.empty() ) {
        /*********************/
        if ( ! locked() )
            lock();
        /*********************/
		return hash_bucket->bucket_dir();
	}
	return std::string();
}

bool Doc::load_url(const Url& _url)
{
	using namespace std;
	using namespace torrent;

    url = _url;
    url.normalize();
    /*********************/
    if ( ! locked() )
        lock();
    /*********************/

	string value = hash_bucket->get();
	if (value.empty()) {
		http_code	=  0;
		curl_code	= -1;
		curl_error.clear();
		modified	= -1;
		crawled		= -1;
		content_sz	=  0;
		content_fd	= -1;
		content_gz_f = NULL;
		content.clear();
		headers.clear();
		etag.clear();
		content_type = 0;
		charset.clear();
		flags.reset();
		title.clear();
		rss2.clear();
		rss.clear();
		atom.clear();
		return false;
	}	
		
	istringstream is(value);

	try {
		torrent::Bencode b;
		is >> b;
	//	is.close();
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
    return true;
}

void Doc::load_leaf(const std::string& leaf)
{
	using namespace std;
	using namespace torrent;


    /*********************/
    //if ( ! locked() )
    //    lock();
    /*********************/

    hash_bucket.reset(new big_hash_bucket(leaf.c_str(), true));
    string content = hash_bucket->get();
    if ( content.empty() )
        throw runtime_error("empty contents");

	istringstream is(content);

	try {
		torrent::Bencode b;
		is >> b;
	//	is.close();
		if( b.has_key("url") )
			url = b["url"].as_string();

		if( b.has_key("http") )	
			http_code = b["http"].as_value();

		if( b.has_key("modified") )	
			modified = b["modified"].as_value();

		if( b.has_key("content_sz") )	
			content_sz = b["content_sz"].as_value();

		if( b.has_key("headers") )	
			headers = b["headers"].as_string();

		if( b.has_key("charset") )	
			charset = b["charset"].as_string();

		if( b.has_key("curl_code") )	
			curl_code = b["curl_code"].as_value();

		if( b.has_key("curl_error") )	
			curl_error = b["curl_error"].as_string();

		if( b.has_key("flags") )	
			flags = b["flags"].as_value();
	} catch (std::ios_base::failure& e){
		cerr << __FUNCTION__ << " exception: " << e.what() << endl;
		throw std::runtime_error(e.what());
	}
}

void Doc::lock()
{
    if ( locked() )
        unlock();
    url.normalize(); 
    hash_bucket.reset(new big_hash_bucket(get_dbdir(), url.get()));
}

Doc::~Doc()
{
	close_content();
}
