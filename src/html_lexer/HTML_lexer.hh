/*
 * Copyright 2012 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup LexicalAnalysis 
 * @brief parsing of HTML documents
 *
 * @{
 */


#ifndef HTML_lexer_hh
#define HTML_lexer_hh 1
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <map>
#include <list>
#include <bitset>
#include <stdexcept>

#include <tr1/unordered_map>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#undef yyFlexLexer
#include <FlexLexer.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <unicode/utypes.h>   /* Basic ICU data types */
#include <unicode/ucnv.h>     /* C   Converter API    */
#include <unicode/uchar.h>    /* char names           */


#include "Unicode_wrap.hh"
#include "Case_folder.hh"
#include "Entity_handler.hh"
#include "utils.hh"
#include "Url.hh"

namespace content_type {
    typedef enum content_type_t {
        UNSET=0,

        // No Content-type header
        NONE, 

        // Content-type header that we aren't interested
        UNRECOGNIZED,

        // text/html
        TEXT_HTML,

        // application/xhtml+xml
        XHTML,

        // application/rss+xml
        RSS_XML,

        // application/atom+xml
        ATOM_XML,

        // text/xml
        TEXT_XML,

        // text/plain
        TEXT_PLAIN,

        // application/pdf
        APPLICATION_PDF,

        FILE_DIRECTORY,

        // Empty file, used for local files 
        EMPTY,

        CONTENT_TYPE_MAX
    } content_type_t;
};    


/**
 * @brief Parse HTTP headers
 * @param[in] headers
 * @param[out] content_type 
 * @param[out] charset_http_head charset of the document, if any
 */
void parse_headers(const std::string& headers, content_type::content_type_t& content_type, std::string& charset_http_head);

struct link {
    std::string url;
    std::string    txt;
//    std::list<std::string> words;
    link() : url(), txt() {}
    void clear() { url.clear(); txt.clear(); }
    operator bool() const { return ! url.empty(); }
};


/// @cond
std::ostream& operator<<(std::ostream& os, const struct link& l);
/// @endcond

/// Meta information resulting from lexical analysis
class Analysis {
public:
    Analysis() :
        title(),
        rss2(),
        rss(),
        atom(),
        charset(),
        index(true),
        follow(true)
    {}
    std::string title;
    std::string rss2;
    std::string rss;
    std::string atom;
    std::string charset;
    bool index;
    bool follow;

};


/**
 * @class HTML_lexer HTML_lexer.hh
 *
 * @brief a lexical analyzer for SGML/HTML based on w3c flex sgml lexer
 * 
 * Sgml entities are decomposed in tokens, for example the tag &lt;a href="http..." lang=es&gt;
 * will produce:
 *
 *    SGML_START a (with &lt; and space, etc removed)
 *
 *    SGML_ATTRNAME href  (with = removed)
 *
 *    SGML_LITERAL "http..."
 *
 *    SGM_TAGC
 *
 * Tokens get pushed into a vector, then some instructions call HTML_lexer::process()
 *
 * Tokens have type of token (an enum), and contents
 *
 * To process several streams with the same lexer object you can call the base class switch_streams member function. Or set_istream and set_doc
 *
 * lexer.switch_streams(static_cast<istream*>(&is),NULL);
 * lexer.yylex();
 * When yylex() is called, the doc that was passed is filled
 */
class HTML_lexer:public yyFlexLexer 
{
private:
    HTML_lexer(const HTML_lexer&);
    void operator=(const HTML_lexer&);

public:
    /**
     * @brief Constructor
     * @param[in] in istream with the HTML data to parse
     * @param[out] txtout if it's not NULL, textual representation would be written to this stream
     * @param[in] base_url if it's not NULL it should be an absolute url to resolve
     * the relative links of the document to absolute urls
     * @param[out] lnkout if it's not NULL links will be written to this stream, one link per line
     * @param[out] warnings if it's not NULL any warnings will be written to this stream
     * @param[in] analysis if it's not NULL will be set with meta information about document, such as related feeds, title, etc 
     * @param get_text_if_body_tag_only might be scripts or any bizarre stuff before the body tag, that we don't want. Also if it doesn't contain body it might be garbage
     */
    HTML_lexer(std::istream* in, std::ostream* txtout, const std::string* base_url=0, std::ostream* lnkout=0, std::ostream* warnings = 0, Analysis* analysis=0, bool get_text_if_body_tag_only=true);

    /// Change input stream, etc.
    HTML_lexer& reset(std::istream* i, std::ostream* txtout, const std::string* base_url=0, std::ostream* lnkout=0, std::ostream* warnings = 0,  Analysis* analysis=0, bool get_text_if_body_tag_only=true);

    /// Process it @return 0 on success -1 on failure
    int yylex();

protected:
    /// SGML types of token
    enum SGML_tok_t {
      SGML_EOF,
      SGML_DATA, SGML_START, SGML_END, SGML_TAGC, //4
      SGML_ATTRNAME, SGML_NAME, SGML_NUMBER, SGML_NMTOKEN, SGML_LITERAL, //9
      SGML_GEREF, SGML_NUMCHARREF, SGML_REFC, // not used, currently we use a regex perhaps, depending on speed
      SGML_PI,
      SGML_MARKUP_DECL, SGML_COMMENT,
      SGML_LIMITATION, SGML_ERROR, SGML_WARNING,
      SGML_MAXTOK
    };

    static std::string sgml_tok_xlate(enum SGML_tok_t tok);
    void warn(const char* warning, const char* str, int len);
    void warn(const char* warning);
    void warn(const std::string& warning);


    /// A token, tags, are subdivided in tokens
    struct Token {
        SGML_tok_t type;
        std::string    content;
        Token(): type(),content() {}
        Token(SGML_tok_t t, std::string c): type(t), content(c) {}
    };

    friend std::ostream& operator <<(std::ostream& os, const struct Token& t);


    typedef std::vector<struct Token> tokens_t;
    typedef boost::function<void()> callback_t;
    typedef std::tr1::unordered_map<std::string, callback_t> tag_op_t; 

    void link_add(const std::string& link);
    void link_text_add(const std::string& text);
    void link_text_word_break();
    void submit_link();

    void text_add(const std::string& text);
    void text_word_break();

    void finalize();

    void addtoken(SGML_tok_t toktype, const char* str, int len, bool case_insensitive=false);
    void process();

    void tag_map(tokens_t& tok, std::map<std::string, std::string>& m);


    tag_op_t stag_op;
    tag_op_t ctag_op;

    tokens_t tokens;
    enum flag_t { 
        /**
         * if set text will be indexed
         */
        FLAG_GET_TEXT,
        FLAG_GET_TITLE,
        /**
         * Inside a link
         */
        FLAG_INLINK,
        FLAG_LINK_NOFOLLOW,
        FLAGSIZE
    };

    /**
     * toktype to char* array
     * for debug 
     */
    static const char* toknames[];


private:
    bool normalize;
    bool did_word_break;
    std::ostream* txtout;
    Url    base_url;
    std::ostream* lnkout;
    std::ostream* warnings;
    Analysis* analysis;        
    std::bitset<FLAGSIZE> flags;
    struct link curlink;
    
    void op_a();
    void op_a_c();
    void op_link();
    void op_link_c();
    void op_meta();
    void meta_name_robots(const std::string& content);
    void op_body();
    void op_body_c();
    void op_frame();
    void op_frame_c();
    void op_script();
    void op_script_c();
    void op_style();
    void op_style_c();
    void op_title();
    void op_title_c();

    /**
     * a new word is appended on the words vector
     * calls text_word_break or link_text_word_break
     */
    void word_break();
    Entity_handler entity_handler;
    bool get_text_if_body_tag_only;

};

/// @cond
std::ostream& operator<<(std::ostream& os, const struct HTML_lexer::Token& t);
/// @endcond



/**
 * @brief Processed HTML document result
 */
class ProcHTML {
public:
    ProcHTML() :
        base_url(),
        text(),
        links(),
        warnings(),
        analysis()
    {}
    /// The supplied base_url
    std::string base_url;
    /// Raw text for this document
    std::string text;
    /// Links for this HTML, one link per line
    std::string links;
    /// Warnings during parsing
    std::string warnings;
    Analysis analysis;
};

/**
 * @brief Parse HTML document
 * @param html_in input HTML
 * @param base_url the base url needed for relative links in the HTML
 * @return ProcHTML result
 */
ProcHTML html_lex(const std::string& html_in, const std::string& base_url);


/// Regex to find charset, in meta or http headers
extern const boost::regex charset_re;




inline HTML_lexer& HTML_lexer::reset(std::istream* i, std::ostream* txtout, const std::string* base_url, std::ostream* lnkout, std::ostream* warnings, Analysis* analysis, bool get_text_if_body_tag_only)
{
    switch_streams(i,NULL);
    this->txtout = txtout;
    if( base_url )
        this->base_url = *base_url;
    this->lnkout = lnkout;
    this->warnings = warnings;
    this->analysis = analysis;
    this->get_text_if_body_tag_only = get_text_if_body_tag_only;

    return *this;    
}




#endif
/** @} */

