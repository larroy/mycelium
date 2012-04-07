#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <stdexcept>
#include <functional>

#include "Url.hh"
#include "utils.hh"
#include "HTML_lexer.hh"


using namespace std;

const char* HTML_lexer::toknames[] = {
			NULL,
			"Data", "Start Tag", "End Tag", "Tag Close",
			"Attr Name", "Name", "Number", "Name Token", "Literal",
			"Entity", "CharRef", "RefC",
			"Processing Instruction",
			"Markup Decl", "Comment",
			"!!Limitation!!", "!!Error!!", "!!Warning!!"
};


#define LWS "(?:\\r\\n)?[ \\t]+"
#define token "[^[:cntrl:]()<>@,;\\\\:\"/\\[\\]?={}\\t]"
#define separator "()<>@,;\\\\:\"/\\[\\]?={}\\t"
//const boost::regex content_type_re("([^/;]+/[^/;]+)",boost::regex_constants::perl);

static const boost::regex header_re("([^"separator"]+):(.+)",boost::regex_constants::perl);
const boost::regex charset_re("charset=("token"+)",boost::regex_constants::perl);
static const boost::regex meta_refresh_re("^\\d+; url=(.+)$",boost::regex_constants::perl | boost::regex_constants::icase);
#undef LWS
#undef token
#undef separator


HTML_lexer::HTML_lexer(std::istream* i, std::ostream* txtout, const std::string* base_url, std::ostream* lnkout, std::ostream* warnings, Analysis* analysis, bool get_text_if_body_tag_only) : 
	yyFlexLexer(i,0),
	stag_op(),
	ctag_op(),
	tokens(),
	normalize(true),
	did_word_break(false),
	txtout(txtout),
	base_url(),
	lnkout(lnkout),
	warnings(warnings),
	analysis(analysis),
	flags(),
	curlink(),
	entity_handler(),
	get_text_if_body_tag_only(get_text_if_body_tag_only)
{
	if( base_url ) {
		this->base_url = *base_url;
		if( ! this->base_url.absolute() )
			throw runtime_error(fs("base_url: " << *base_url << " is not absolute" << endl));
	}	

	// tag ops
	// boost::function<void()> fnb = boost::bind(&Test::f,this);
	if (! get_text_if_body_tag_only)
		flags.set(FLAG_GET_TEXT);


	stag_op.insert(make_pair("body",boost::bind(&HTML_lexer::op_body,this)));
	stag_op.insert(make_pair("a",boost::bind(&HTML_lexer::op_a,this)));
	stag_op.insert(make_pair("frame",boost::bind(&HTML_lexer::op_frame,this)));
	stag_op.insert(make_pair("iframe",boost::bind(&HTML_lexer::op_frame,this)));
	stag_op.insert(make_pair("script", boost::bind(&HTML_lexer::op_script,this)));
	stag_op.insert(make_pair("style", boost::bind(&HTML_lexer::op_style,this)));

	// close tag ops
	ctag_op.insert(make_pair("body",boost::bind(&HTML_lexer::op_body_c,this)));
	ctag_op.insert(make_pair("a",boost::bind(&HTML_lexer::op_a_c,this)));
	ctag_op.insert(make_pair("frame",boost::bind(&HTML_lexer::op_frame_c,this)));
	ctag_op.insert(make_pair("iframe",boost::bind(&HTML_lexer::op_frame_c,this)));
	ctag_op.insert(make_pair("script", boost::bind(&HTML_lexer::op_script_c,this)));
	ctag_op.insert(make_pair("style", boost::bind(&HTML_lexer::op_style_c,this)));


	stag_op.insert(make_pair("link", boost::bind(&HTML_lexer::op_link,this)));
	ctag_op.insert(make_pair("link", boost::bind(&HTML_lexer::op_link_c,this)));

	// word breaks
	stag_op.insert(make_pair("applet", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("applet", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("base", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("base", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("blockquote", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("blockquote", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("br", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("br", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("button", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("button", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("caption", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("caption", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("dd", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("dd", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("div", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("div", boost::bind(&HTML_lexer::word_break,this)));
//	stag_op.insert(make_pair("span", boost::bind(&HTML_lexer::word_break,this)));
//	ctag_op.insert(make_pair("span", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("dfn", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("dfn", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("dl", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("dl", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("dt", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("dt", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("fieldset", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("fieldset", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("form", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("form", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("h1", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("h1", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("head", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("head", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("hr", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("hr", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("img", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("img", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("input", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("input", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("li", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("li", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("map", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("map", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("menu", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("menu", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("meta", boost::bind(&HTML_lexer::op_meta,this)));
	ctag_op.insert(make_pair("meta", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("noframes", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("noframes", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("object", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("object", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("ol", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("ol", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("optgroup", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("optgroup", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("option", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("option", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("p", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("p", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("param", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("param", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("pre", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("pre", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("q", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("q", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("samp", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("samp", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("select", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("select", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("table", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("table", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("tbody", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("tbody", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("td", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("td", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("textarea", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("textarea", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("tfoot", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("tfoot", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("th", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("th", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("thead", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("thead", boost::bind(&HTML_lexer::word_break,this)));
//	stag_op.insert(make_pair("title", boost::bind(&HTML_lexer::word_break,this)));
//	ctag_op.insert(make_pair("title", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("title", boost::bind(&HTML_lexer::op_title,this)));
	ctag_op.insert(make_pair("title", boost::bind(&HTML_lexer::op_title_c,this)));

	stag_op.insert(make_pair("tr", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("tr", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("tt", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("tt", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("ul", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("ul", boost::bind(&HTML_lexer::word_break,this)));
	stag_op.insert(make_pair("xmp", boost::bind(&HTML_lexer::word_break,this)));
	ctag_op.insert(make_pair("xmp", boost::bind(&HTML_lexer::word_break,this)));
}

void HTML_lexer::addtoken(SGML_tok_t toktype, const char* str, int len, bool case_insensitive) 
{
	Token t;
	string tmp(str,len);
	string s = entity_handler.replace_all_entities(tmp);
	if( case_insensitive ) {
		boost::to_lower(s);
		t.type = toktype;
		t.content = s;
	} else {
		t.type = toktype;
		t.content = s;
	}
	tokens.push_back(t);
}

std::ostream& operator<<(std::ostream& os, const struct HTML_lexer::Token& t)
{
	os << "type: " << HTML_lexer::sgml_tok_xlate(t.type) << endl
		<< "content: " << t.content;
	return os;	
}


void HTML_lexer::warn(const std::string& warning) {
	if( warnings && warnings->good() ) {
		(*warnings) << warning;
	}	
}

void HTML_lexer::warn(const char* warning) {
	if( warnings && warnings->good() ) {
		(*warnings) << warning;
	}	
}

void HTML_lexer::warn(const char* warning, const char* str, int len) {
	if( warnings && warnings->good() ) {
		(*warnings) << warning << ": yytext: \"";
		(*warnings).write(str, len);
		(*warnings) << "\"" << endl;
	}

		
//	ostringstream os(curdoc->warnings,ios::app);
//	os.write(str,len);

//	cerr << "===========" << endl;
//	cerr << "warning: " << warning << " '" << str << "'" << endl;
//	cerr << "tokens: " << endl;
//	for(tokens_t::const_iterator i = tokens.begin(); i != tokens.end(); ++i) { 
//		cerr <<  (*i) << endl;
//	}
//	cerr << "===========" << endl;
}


void HTML_lexer::text_add(const string& text) {
	if( unlikely(flags.test(FLAG_GET_TITLE)) && analysis ) 
		analysis->title += text;	

    if( txtout && txtout->good() )
        (*txtout) << text;

	did_word_break = false;	
}

void HTML_lexer::text_word_break() {
	//curdoc->words.resize(curdoc->words.size()+1);
	//curdoc->words.insert(curdoc->words.end(),string());
	if( txtout && txtout->good() && ! did_word_break ) {
		(*txtout) << endl;
		did_word_break = true;
	}	

}

void HTML_lexer::link_add(const string& link) {
	if( base_url ) {
		try {
			Url url(link);
			url.normalize();
			if( url.absolute() ) {
				//cout << "link_add absolute: " << url.to_string() << endl;
				curlink.url = url.to_string();
			} else {
				if ( ! base_url ) {
					curlink.clear();
					return;
				}	
				Url full = base_url;
				full.merge_ref(url);
				//cout << "link_add merged: " << full.to_string() << endl;
				curlink.url = full.to_string();
			}
		} catch(BadUrl& e) {
			curlink.clear();
		} catch(runtime_error& e) {
			curlink.clear();
		} 
	} else {
		curlink.url = link;
	}
	//curlink.words.clear();
	// we will append to back later
	//curlink.words.insert(curlink.words.end(),string());
	//curlink.words.resize(1);
}

void HTML_lexer::link_text_add(const string& text) {
	// add it to normal text
	text_add(text);	
	curlink.txt.append(text);
//	if(curlink.words.empty())
//		curlink.words.insert(curlink.words.end(),string());
//	curlink.words.back().append(text);
}

void HTML_lexer::link_text_word_break() {
	//++curdoc->tf[curlink.words.back()];
//	text_word_break();

//	curlink.words.resize(curlink.words.size()+1);
//	curlink.words.insert(curlink.words.end(),string());
	curlink.txt.append(" ");
}

void HTML_lexer::submit_link() {
	//curdoc->links.push_back(curlink);
	// FIXME
	if( curlink && lnkout && lnkout->good() )
		(*lnkout) << curlink;
	curlink.clear();
}

/*
string HTML_lexer::ref_xlate(const string& s) {
	cout << "REF:" << s << endl;
	return s;
}

string HTML_lexer::numref_xlate(const string& s) {
	cout << "NUMREF:" << s << endl;
	return s;
}

*/

//void literal_from_atribute
//

void HTML_lexer::meta_name_robots(const string& content)
{
	/*
	 * all 
	 * noindex
	 * none (noindex, nofollow)
	 * index
	 * follow 
	 * nofollow
	 */
	if ( ! analysis ) 
		return;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(", ");
	tokenizer tokens(content,sep);
	for (tokenizer::iterator i=tokens.begin(); i != tokens.end(); ++i) {
		string tok = *i;
		boost::to_lower(tok);
		if ( tok == "index" ) 
			analysis->index = true;

		else if ( tok == "follow" )	
			analysis->follow = true;

		else if ( tok == "all" ) {
			analysis->follow = true;
			analysis->index = true;
			return;

		} else if ( tok == "noindex" ) 
			analysis->index = false;

		else if ( tok == "nofollow" )	
			analysis->follow = false;

		else if ( tok == "none" ) {	 
			analysis->follow = false;
			analysis->index = false;
			return;
		}	
	}
}

void HTML_lexer::op_meta()
{
	using namespace boost;

	if( ! analysis ) 
		return;

#if 0
	for(tokens_t::const_iterator i = tokens.begin(); i != tokens.end(); ++i) {
		cout << toknames[i->type] << ": \"" << i->content << "\"" << endl;
	}
#endif	


	typedef map<string,string> tag_t;
	tag_t tag;
	tag_map(tokens, tag);
	// robots
	{
		tag_t::iterator name_i;
		tag_t::iterator content_i;
		if( (name_i = tag.find("name")) != tag.end() && iequals(name_i->second, "robots")
			&& (content_i = tag.find("content")) != tag.end() ) {

			meta_name_robots(content_i->second);
		}
	}	
	// http-equiv
	{
		tag_t::iterator http_equiv_i;
		tag_t::iterator content_i;
		if( ((http_equiv_i = tag.find("http-equiv")) != tag.end() ) ) {
			if( iequals(http_equiv_i->second, "content-type") 
				&& (content_i = tag.find("content")) != tag.end()) {

				smatch charset_m;
				if ( regex_search(content_i->second, charset_m, charset_re) )
					analysis->charset = boost::trim_copy(charset_m[1].str());

			} else if( iequals(http_equiv_i->second, "refresh") 
				&& (content_i = tag.find("content")) != tag.end()) {
				smatch m;
				if ( regex_search(content_i->second, m, meta_refresh_re) ) {
					link_add(m[1].str());
					submit_link();
				}							
			}
		}
	}
}

void HTML_lexer::op_title()
{
	flags.set(FLAG_GET_TEXT);
	flags.set(FLAG_GET_TITLE);
}
void HTML_lexer::op_title_c()
{
	flags.reset(FLAG_GET_TITLE);
	flags.reset(FLAG_GET_TEXT);
}


#if 0
struct Token_compare : public std::unary_function<Token, bool> {
	Token_compare(

};
#endif

void HTML_lexer::op_link() 
{
	// <link rel="alternate" type="application/rss+xml" title="RSS 2.0" href="http:// .... " />
	if( ! analysis )
		return;

	using namespace boost;
	typedef map<string,string> tag_t;
	tag_t tag;
	tag_map(tokens, tag);
	tag_t::iterator href_i;
	if( (href_i = tag.find("href")) != tag.end() ) {
		// see if it looks like rss or atom
		tag_t::iterator rel_i;
		tag_t::iterator type_i;
		//tag_t::iterator title_i;
		if( ((rel_i = tag.find("rel")) != tag.end() && iequals(rel_i->second,"alternate"))
			&& ((type_i = tag.find("type")) != tag.end())
		) {
			string href = href_i->second;
			boost::trim(href);
			string href_cpy = href;
			if( base_url ) {
				try {
					Url url(href);
					url.normalize();
					if( url.absolute() ) {
						href = url.to_string();
					} else {
						Url full = base_url;
						full.merge_ref(url);
						href = full.to_string();
					}
				} catch(BadUrl& e) {
					href = href_cpy;
					warn(fs("BadUrl exception (link rel): " << e.what() << " href: " << href << " base_url: " << base_url.get()));
				} catch(runtime_error& e) {
					href = href_cpy;
					warn(fs("runtime_error exception (link rel): " << e.what() << " href: " << href << " base_url: " << base_url.get()));
				} 
			}	
			if (iequals(type_i->second,"application/rss+xml")) {
				analysis->rss2 = href;
			} else if( iequals(type_i->second,"application/atom+xml")) {
				analysis->atom = href;
			} else if(iequals(type_i->second,"text/xml")) {
				analysis->rss = href;
			}
		}
	}
}

void HTML_lexer::op_link_c() 
{
}

void HTML_lexer::op_a() 
{
	//cout <<  "op_a" << endl;
	if( ! flags.test(FLAG_GET_TEXT))
		return;

	for(tokens_t::const_iterator i = tokens.begin(); i != tokens.end(); ++i) { // look for href
		if( i->type == SGML_ATTRNAME && i->content.compare(0,4,"href",0,4) == 0 && (i+1) != tokens.end() && (i+1)->type == SGML_LITERAL ){
			//cout << "link add: |" << (i+1)->content <<"|"<< endl;
			link_add((i+1)->content);
			flags[FLAG_INLINK] = true;
		} else if(i->type == SGML_ATTRNAME && i->content.compare(0,4,"href",0,4) == 0 && (i+1) == tokens.end()) {
			cerr << "empty href=" << " yyin: " << yyin->tellg() << endl;
			cerr << "token stack: " << endl;
			for(tokens_t::const_iterator j = tokens.begin(); j != tokens.end(); ++j) { 
				cerr <<  (*j) << endl;
			}
			cerr << endl;
			//throw runtime_error("OMG");
			//
		} else if( i->type == SGML_ATTRNAME && i->content.compare(0,3,"rel") == 0 && (i+1) != tokens.end() && (i+1)->type == SGML_LITERAL ){
			string content = (i+1)->content;
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			boost::char_separator<char> sep("\" ");
			tokenizer tokens(content,sep);
			for (tokenizer::iterator j=tokens.begin(); j != tokens.end(); ++j) {
				//cout << "rel: " << *j << " " << curlink.url << endl;
				string tok = *j;
				boost::to_lower(tok);
				if( tok == "nofollow" ) {
					flags[FLAG_LINK_NOFOLLOW] = true;	
				}
				// rss
			}	
		}
	}
}

void HTML_lexer::op_a_c() 
{
	//cout << "op_a_c()" << endl;
	if(flags.test(FLAG_LINK_NOFOLLOW)) {
		flags.reset(FLAG_INLINK);
		flags.reset(FLAG_LINK_NOFOLLOW);
		return;
	}	

	if(flags.test(FLAG_INLINK)) {
		submit_link();
		flags.reset(FLAG_INLINK);
	}
}


void HTML_lexer::op_frame() 
{
	DLOG(cout <<  "op_frame" << endl;)
	if( ! flags.test(FLAG_GET_TEXT))
		return;
	for(tokens_t::const_iterator i = tokens.begin(); i != tokens.end(); ++i) { // look for href
		if( i->type == SGML_ATTRNAME && i->content.compare(0,3,"src",0,3) == 0 &&  (i+1) != tokens.end() && (i+1)->type == SGML_LITERAL ){
			//cout << "add link in frame: " << (i+1)->content << endl;
			//cout << "link add: " << (i+1)->content << endl;
			link_add((i+1)->content);
			flags[FLAG_INLINK] = true;
		} else if(i->type == SGML_ATTRNAME && i->content.compare(0,3,"src",0,3) == 0 && (i+1) == tokens.end()) {

			//throw runtime_error("jeebus");
			cerr << "empty frame src=" << " yyin: " << yyin->tellg() << endl;
			cerr << "token stack: " << endl;
			for(tokens_t::const_iterator j = tokens.begin(); j != tokens.end(); ++j) { 
				cerr <<  (*j) << endl;
			}
			cerr << endl;

		}
	}
	word_break();
}
void HTML_lexer::op_frame_c() 
{
	DLOG(cout << "op_frame_c()" << endl;)
	if(flags.test(FLAG_INLINK)) {
		submit_link();
		flags.reset(FLAG_INLINK);
	}
	word_break();
}



void HTML_lexer::op_body()
{
	flags.set(FLAG_GET_TEXT);
	word_break();
}

void HTML_lexer::op_body_c()
{
	flags.reset(FLAG_GET_TEXT);
}

void HTML_lexer::op_script()
{
	flags.reset(FLAG_GET_TEXT);
}

void HTML_lexer::op_script_c()
{
	if( ! flags.test(FLAG_GET_TEXT) ) 
		flags.set(FLAG_GET_TEXT);
}

void HTML_lexer::op_style()
{
	flags.reset(FLAG_GET_TEXT);
}

void HTML_lexer::op_style_c()
{
	if( ! flags.test(FLAG_GET_TEXT) ) 
		flags.set(FLAG_GET_TEXT);
}



void HTML_lexer::word_break()
{
	if( flags[FLAG_GET_TEXT] ) {
		if( flags.test(FLAG_INLINK) ) { // link text
			link_text_word_break();
		} else {
			text_word_break();
		}
	}	
}


std::string HTML_lexer::sgml_tok_xlate(enum HTML_lexer::SGML_tok_t tok)
{
	switch(tok) {
		case SGML_EOF:
			return "SGML_EOF";

		case SGML_DATA:
			return "SGML_DATA";

		case SGML_START:
			return "SGML_START";

		case SGML_END:
			return "SGML_END";

		case SGML_TAGC:
			return "SGML_TAGC";

		case SGML_ATTRNAME:
			return "SGML_ATTRNAME";

		case SGML_NAME:
			return "SGML_NAME";

		case SGML_NUMBER:
			return "SGML_NUMBER";

		case SGML_NMTOKEN:
			return "SGML_NMTOKEN";

		case SGML_LITERAL:
			return "SGML_LITERAL";

		case SGML_GEREF:
			return "SGML_GEREF";

		case SGML_NUMCHARREF:
			return "SGML_NUMCHARREF";

		case SGML_REFC:
			return "SGML_REFC";

		case SGML_PI:
			return "SGML_PI";

		case SGML_MARKUP_DECL:
			return "SGML_MARKUP_DECL";

		case SGML_COMMENT:
			return "SGML_COMMENT";

		case SGML_LIMITATION:
			return "SGML_LIMITATION";

		case SGML_ERROR:
			return "SGML_ERROR";

		case SGML_WARNING:
			return "SGML_WARNING";

		case SGML_MAXTOK:
			return "SGML_MAXTOK";
		
		default:
			return "SGML ???";

	}
}

void HTML_lexer::tag_map(tokens_t& tok, std::map<std::string, std::string>& m)
{
	m.clear();
	for(tokens_t::const_iterator i = tok.begin(); i != tok.end(); ++i) {
		if( i->type == SGML_ATTRNAME && (i+1) != tok.end() && (i+1)->type == SGML_LITERAL ) {
			m[i->content] = (i+1)->content;
		}
	}
}

void HTML_lexer::process() {

	if (tokens.empty())
		return;

	D(cout << "************" << endl;
	cout << "Process: " << endl;
	for(tokens_t::const_iterator i = tokens.begin(); i != tokens.end(); ++i) {
		cout << toknames[i->type] << ": \"" << i->content << "\"" << endl;
	}
	cout << endl;);


	Token cur = tokens.front();
	if(cur.type == SGML_START ) {
		tag_op_t::iterator i;
		if( ( i = stag_op.find(cur.content)) != stag_op.end() ) 
			i->second(); // exec function pointer associated with this tag start
	}
	if(cur.type == SGML_END) {
		tag_op_t::iterator i;
		if( ( i = ctag_op.find(cur.content)) != ctag_op.end() ) 
			i->second();

	}
	if(cur.type == SGML_DATA) {
		if( flags.test(FLAG_GET_TEXT) ) {
			if( flags.test(FLAG_INLINK) ) { // link text
				link_text_add(cur.content);
			} else { // out of link text
				text_add(cur.content);	
			}	
		}
	}
/*
	if(cur.type == SGML_GEREF) {
		cout << "HELLO" << endl;
		if( flags[FLAG_GET_TEXT] ) {
			if( flags[FLAG_INLINK] ) { // link text
				link_text_add(ref_xlate(cur.content));
			} else { // out of link text
				text_add(ref_xlate(cur.content));	
			}	
		}
	}
	if(cur.type == SGML_NUMCHARREF) {
		if( flags[FLAG_GET_TEXT] ) {
			if( flags[FLAG_INLINK] ) { // link text
				link_text_add(numref_xlate(cur.content));
			} else { // out of link text
				text_add(numref_xlate(cur.content));	
			}	
		}
	}
*/

	tokens.clear();
}

/* void HTML_lexer::word_breaker() {
	for(list<string>::const_iterator word = curdoc->words.begin(); word != curdoc->words.end(); ++word) { 
		cout << "|word:|" << (*word) << "|" << endl;
	}
} */


void HTML_lexer::finalize() {
}

void parse_headers(const string& headers, content_type::content_type_t& content_type, string& charset_http_head)
{
#ifdef USE_BOOST_TOKENIZER
	using namespace boost;
	smatch header_m;
	typedef tokenizer<char_separator<char> > tokenizer;
	char_separator<char> header_sep("\n\r");
	tokenizer tokens(headers,header_sep);
	for (tokenizer::iterator i=tokens.begin(); i != tokens.end(); ++i) {
		if( *i == "\n" )
			continue;
		if( regex_match(*i, header_m, header_re) ) {
			if( header_m[1].matched && header_m[2].matched) {
				string name = header_m[1].str();
				string value = header_m[2].str();
				static const regex content_type_re("^Content-Type$", regex::perl|regex::icase);
				//if( name == "Content-Type" ) {
				if( regex_match(name, content_type_re) ) {
					//cout << "Content-Type:" << value << endl;
					if( value.find("text/html") != string::npos ) {
						content_type = content_type::TEXT_HTML;
					} else if ( value.find("application/xhtml+xml") ) {	
						content_type = content_type::APP_XHTML;
					} else if ( value.find("text/plain") != string::npos ) {
						content_type = content_type::TEXT_PLAIN;
						//cout << "TYPE: text/plain" <<endl;
					} else if ( value.find("application/pdf") != string::npos ) {
						content_type = content_type::APPLICATION_PDF;
					} else {
						content_type = content_type::UNRECOGNIZED;
					//	DLOG(cout << "TYPE: unrecognized: " << value << endl;)
					}
					smatch charset_m;
					if(regex_search(value,charset_m,charset_re)) {
						charset_http_head = charset_m[1].str();
					}
					return;
				}
			} else {
		//		cerr << "header captures: " << *i << " don't match" << endl;
			}
		} else {
			//cerr << "header: |" << *i << "| doesn't match" << endl;
		}
	}


#else	
	using namespace boost;
	smatch header_m;

	size_t tortoise=0;
	size_t hare=0;
	while( (hare=headers.find_first_of("\n\r", tortoise)) != string::npos ) {
		if( hare > tortoise+1 ) {
			string header = headers.substr(tortoise, hare - tortoise);
			if(regex_match(header, header_m, header_re) ) {
				if( header_m[1].matched && header_m[2].matched) {
					string name = header_m[1].str();
					string value = header_m[2].str();
					static const regex content_type_re("^Content-Type$", regex::perl|regex::icase);
					if( regex_match(name, content_type_re) ) {
						if( value.find("text/html") != string::npos ) {
							content_type = content_type::TEXT_HTML;
							//cout << "TYPE: text/html" << endl;
						} else if ( value.find("text/plain") != string::npos ) {
							content_type = content_type::TEXT_PLAIN;
							//cout << "TYPE: text/plain" <<endl;
						} else if ( value.find("application/pdf") != string::npos ) {
							content_type = content_type::APPLICATION_PDF;
						} else {
							content_type = content_type::UNRECOGNIZED;
						//	DLOG(cout << "TYPE: unrecognized: " << value << endl;)
						}
						smatch charset_m;
						if(regex_search(value,charset_m,charset_re)) {
							charset_http_head = charset_m[1].str();
						}
						return;
					}
				} else {
			//		cerr << "header captures: " << *i << " don't match" << endl;
				}
			} else {
				//cerr << "header: |" << *i << "| doesn't match" << endl;
			}
		}
		tortoise = ++hare;
	}	
#endif	
}

std::ostream& operator<<(std::ostream& os, const struct link& l)
{
//	string::size_type pos = string::npos;
//	while( (pos=l.url.find_first_of("\x01\x02")) != string::npos )
//		l.url.erase(pos,1);

	//os << l.url.size() << ":" <<  l.url << l.txt.size() << ":"  << l.txt  << endl;
#if 0	
	using namespace torrent;
	Bencode b(Bencode::TYPE_LIST);
	Bencode u(l.url);

	string txt = l.txt;
	boost::trim(txt);
	Bencode t(txt);

	b.as_list().push_back(u);
	b.as_list().push_back(t);
	os << b;
#endif	
	os << '\x01';
	for(string::const_iterator i = l.url.begin(); i != l.url.end(); ++i)
		if( *i > 0x08 )
			os << *i;
	os << '\x02';		
	for(string::const_iterator i = l.txt.begin(); i != l.txt.end(); ++i)
		if( *i > 0x08 )
			os << *i;
	os << '\x03';		
	return os;
}


ProcHTML html_lex(const std::string& html_in, const std::string& base_url)
{
	ProcHTML result;
	istringstream in(html_in);
	ostringstream txt;
	ostringstream lnk;
	ostringstream warn;
	const string* base = 0;
	if (!base_url.empty())
		base = &base_url;

	HTML_lexer html_lexer(&in, &txt, base, &lnk, &warn, &result.analysis, false);
	html_lexer.yylex();

	result.base_url = base_url;
	result.text = txt.str();
	result.links = lnk.str();
	result.warnings = warn.str();

	return result;
}
