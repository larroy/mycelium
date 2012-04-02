/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "HTML_lexer.hh"
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gzstream.hh"
#include "timer.hh"
#include "Doc.hh"
#include "bencode.hh"

#include "ucnv_filter.hh"
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>



using namespace std;
using namespace boost;


void help(int argc, char* argv[])
{
	cerr << argv[0] << " [-i input] [-o output] [-l links] [-w warnings] [-p] [-h]"<< endl;
    cerr << "\
if no options are given it reads HTML from stdin at output text to stdout\n\
\t-i\t\tinput file (HTML)\n\
\t-o\t\toutput file (text)\n\
\t-w\t\toutput warnings file\n\
\t-h\t\tthis help\n\
" << endl;

}



void try_lex(const boost::filesystem::path& bucket)
{
	using namespace boost::filesystem;
	struct stat st;
	path meta = bucket / METAFNAME;
	path lexed = bucket / LEXEDFNAME;
	int res;
	bool again=false;
	int tries=0;
	if( (res=stat(lexed.string().c_str(), &st)) == 0 && S_ISREG(st.st_mode) &&  st.st_size ) {
		// skip, already lexed
		cerr << "skip: " << bucket.string() <<  " \"" << lexed.string() << "\" exists " << endl;
		return;
	} else {
	
		bool d_pending_write = false;
		do {
			// TODO
			// update meta, noindex nofollow, charset, last modified etc.
			path contents = bucket / CONTENTSFNAME;
			//path lexed = bucket / LEXEDFNAME;
			path links = bucket / LINKSFNAME;
			path warnings = bucket / WARNINGSFNAME;
			res=stat(contents.string().c_str(), &st);
			if( res != 0 ) {
				cerr << "can't stat: " << contents.string()	<< endl;
			} else if( res == 0 && S_ISREG(st.st_mode) &&  st.st_size > 0 ) {
				try {
					using namespace torrent;
					using namespace boost::iostreams;
					string url;
					Doc d;
					d.load_leaf(bucket.string());
					url = d.url;
					// get charset if any
					string charset;
					string charset_http_head;	
					string charset_metaf;
					content_type::content_type_t c_type;
					//cout << "http: " << d.http_code << endl;
					if( d.http_code == 200  ) {
						filtering_streambuf<input> filt_streamb;
						//igzstream in(contents.string().c_str(), ios_base::in | ios_base::binary);
						ifstream in(contents.string().c_str(), ios_base::in | ios_base::binary);
						ogzstream os(lexed.string().c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
						ogzstream lnk(links.string().c_str() , ios_base::out | ios_base::binary | ios_base::trunc);
						ofstream warn(warnings.string().c_str() , ios_base::out | ios_base::binary | ios_base::trunc);
						if( ! in || ! os || ! lnk || ! warn ) {
							cerr << "failure opening in || os || lnk || warn: bucket: " << bucket.string() << endl;
							return;
						}

						Analysis analysis;
						if( ! d.headers.empty()  ) {
							parse_headers(d.headers, c_type, charset_http_head);
							if( c_type != content_type::TEXT_HTML && c_type != content_type::APP_XHTML ) {
								cerr << "content type ! html && ! xhtml: bucket: " << bucket.string() << endl;
								return;
							}	
						} else 
							return;

						if( ! d.charset.empty()  )
							charset_metaf = d.charset;

						// write charset key in meta file if found on http headers
						if( charset_metaf.empty() && ! charset_http_head.empty() ) {
							d.charset = charset_http_head;
							d_pending_write = true;
						}	

						if( ! charset_metaf.empty() ) {
							charset = charset_metaf;
						} else if( ! charset_http_head.empty() ) {
							charset = charset_http_head;
						} else {	
							// FIXME
							//cout << "no charset found" << endl;
						}	
						d.flags.reset(Doc::FLAG_UTF8_OK);
						d_pending_write = true;

						DLOG(cout << url << endl;)
						if( ! charset.empty() ) {
							// if ! utf8 setup a filter to convert it to utf8
							boost::regex utf8("^utf-?8$", boost::regex::perl | boost::regex::icase);
							boost::smatch match;
							if( boost::regex_match(charset, match, utf8) ) {
								// no ucnv_filter
								//cout << "already in utf-8, no conversion needed" << endl;
								d.flags.set(Doc::FLAG_UTF8_OK);
							} else {
								// FIXME
								try {
									// setup a conversion from charset to utf-8
									filt_streamb.push(utils::ucnv_filter(charset.c_str(), "utf-8"));
									d.flags.set(Doc::FLAG_UTF8_OK);
									DLOG(cout << " conversion from: " << charset);
									//cout << "convert " << charset << "->" << "utf-8" << endl;
								} catch(utils::ucnv_conversion_not_found_error& e) {	
									cerr << "conversion not found: " << charset << " bucket: " << bucket.string() << endl;
									d.flags.reset(Doc::FLAG_UTF8_OK);
								}
							}
						}

						//cout << "--------------------" << endl << "size: " << st.st_size << endl << "url: " << url << endl;


						filt_streamb.push(gzip_decompressor());
						filt_streamb.push(in);
						istream is(&filt_streamb);

						HTML_lexer lexer(&is, &os, &url, &lnk, &warn, &analysis);
						utils::timer t = utils::timer::current();	
						lexer.yylex();
						utils::timer diff = utils::timer::current() - t;

						DLOG(cout << " took: " << diff.usec()/1000 << " ms" << endl;)

						++tries;

						if( ! again && charset.empty() && ! analysis.charset.empty() ) {
							//cout << "charset found in meta tags, lex again" << endl;
							d.charset = analysis.charset;
							d_pending_write = true;
							again = true;
						}
						//if( d_pending_write )
						d.title = analysis.title;
						d.rss2 = analysis.rss2;
						d.rss = analysis.rss;
						d.atom = analysis.atom;
						if( ! d.atom.empty() || ! d.rss.empty() || ! d.rss2.empty() )
							d.flags.set(Doc::FLAG_HAS_SYNDICATION);
						d.flags[Doc::FLAG_INDEX] = analysis.index;
						d.flags[Doc::FLAG_FOLLOW] = analysis.follow;
						d.save();
						if( (res=stat(warnings.string().c_str(), &st)) == 0 && S_ISREG(st.st_mode) &&  st.st_size == 0 )
							if( (res=unlink(warnings.string().c_str())) != 0 )
								utils::err_sys(fs("unlink: " << warnings.string().c_str()));


					} else {
						//cout << "--------------------" << endl << "size: " << st.st_size << endl << "url: " << url << endl;
						//cout << "http code != 200" << endl;
					}
				} catch (std::exception& e){
					cerr << "Exception : " << e.what() << endl;
					//exit(EXIT_FAILURE);
				}
			} else if( res == 0 && S_ISREG(st.st_mode) &&  st.st_size > MAXCONTENTSLEX ) {	
				cerr << contents.string() << ": size > MAXCONTENTSLEX, not lexing" << endl;
			} else {	
				cerr << "not lexing, empty: " << contents.string() << endl;
			}	
		} while(again && tries <= 1);
	}	
}

#if 1
int main(int argc, char* argv[]) {
	std::ios::sync_with_stdio(false);
	int opt;
	string linksf;
	string warningsf;
	string inf;
	string outf;
	string metaf;

	string url;

	bool	do_poin=false;
	while((opt = getopt(argc, argv, "u:i:o:l:w:m:ph")) != -1) {
		switch(opt) {
			case 'u':
				url.assign(optarg);
				break;

			case 'i':
				inf.assign(optarg);
				break;

			case 'o':
				outf.assign(optarg);
				break;

			case 'l':
				linksf.assign(optarg);
				break;
			
			case 'w':
				warningsf.assign(optarg);	
				break;

			case 'm':	
				metaf.assign(optarg);
				break;

			case 'p':
				do_poin = true;
				break;
				
			
			case 'h':
				help(argc,argv);
				exit(EXIT_FAILURE);
			
			default:
				help(argc,argv);
				throw runtime_error("unknown option");
		}
	}
	//namespace bsf = boost::filesystem;
	struct stat st;
	int res;
	//std::map<std::string, std::ofstream> 
	ifstream in_ifs;
	bool	in_ifs_opened=false;
	ofstream out_ofs;
	bool	out_ofs_opened=false;
	ofstream links_ofs;		
	bool	links_ofs_opened=false;
	ofstream warnings_ofs;
	bool	warnings_ofs_opened=false;

	if( ! inf.empty() ) {
		in_ifs.open(inf.c_str(), ios::in);
		if( in_ifs ) {
			in_ifs_opened = true;
		} else	
			throw runtime_error(fs("can't open " << inf));
	}	

	if( ! outf.empty() ) {
		out_ofs.open(outf.c_str(), ios::out | ios::trunc);
		if( out_ofs ) {
			out_ofs_opened = true;
		} else	
			throw runtime_error(fs("can't open " << outf));
	}	
	if( ! warningsf.empty() ) {
		warnings_ofs.open(warningsf.c_str(), ios::out | ios::trunc);
		if( warnings_ofs ) {
			warnings_ofs_opened = true;
		} else	
			throw runtime_error(fs("can't open " << warningsf));
	}	
	if( ! linksf.empty() ) {
		links_ofs.open(linksf.c_str(), ios::out | ios::trunc);
		if( links_ofs ) {
			links_ofs_opened = true;
		} else	
			throw runtime_error(fs("can't open " << linksf));
	}	

	if(in_ifs_opened) cout << "in ifs opened" << endl; 
	if(out_ofs_opened) cout << "out ofs opened" << endl;
	if(links_ofs_opened) cout << "links_ofs_opened" << endl;
	if(warnings_ofs_opened) cout << "warnings ofs opened" <<endl;

	if( do_poin ) {
		namespace bfs = boost::filesystem;
		using namespace bfs;
		for(directory_iterator root_i(get_dbdir()), root_end; root_i != root_end; ++root_i) {
			//cout << "---> " << root_i->leaf() << endl;
			if( ! exists(root_i->path()) || ! is_directory( root_i->path() )  )
				continue;
			for(directory_iterator leaf_i( root_i->path()), leaf_end; leaf_i != leaf_end; ++leaf_i) {
				cout << leaf_i->path().string() << endl;
				try_lex(leaf_i->path());
			}
		}
	} else if( ! url.empty() ) {	
#if 0	
		string buckdir;
		if( doc_store::buckdir(url.c_str(), buckdir) != 0 )
			exit(EXIT_FAILURE);
			
		boost::filesystem::path buckdir_path(buckdir); 
		try_lex(buckdir_path);
#endif		

	} else {
		HTML_lexer lexer(	
			in_ifs_opened ? &in_ifs : &cin,
			out_ofs_opened ? &out_ofs : &cout,
			NULL,
			links_ofs_opened ? &links_ofs : NULL,
			warnings_ofs_opened ? &warnings_ofs : NULL
			);


		lexer.yylex();
		links_ofs.close();

		// remove warnings if empty
		if( warnings_ofs_opened && (res=stat(warningsf.c_str(), &st)) == 0 && S_ISREG(st.st_mode) &&  st.st_size == 0) 
			if( (res=unlink(warningsf.c_str())) != 0 )
				utils::err_sys(fs("unlink: " << warningsf.c_str()));
		
		exit(EXIT_SUCCESS);
	}

//	lexer.switch_streams(static_cast<istream*>(&is),NULL);
}
#endif


#if 0
int main(int argc, char* argv[]) {
	ProcHTML p;
	p = html_lex("<html><body>make <a href=\"http://lml.com\">fight</a> on the hill</body></html>","");

}
#endif
