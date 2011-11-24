%{
/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Robots.hh"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "Url.hh"
#include "utils.hh" // fs D(x)
using namespace std;
using namespace robots;
%}

%option case-insensitive
%option noyywrap
%option c++
%option yyclass="Robots"
/* %option never-interactive */
%option batch

not_special		[^()<>@,;:\\"/\[\]?={} \t#\n\r]
SPACE [\t ]
NEWLINE	\r?\n
COMMENT	#
COMMENTL {SPACE}*{COMMENT}.*
%%
<*>{COMMENTL}({NEWLINE})* {
	D(std::cout << "comment: |" << yytext << "|" << std::endl;)
}


<*>^User-agent:{SPACE}*[^#\n\r]+({COMMENTL})?{NEWLINE} {
	string ua_name(yytext);
	ua_name = ua_name.substr(11);
	string::size_type pos;
	if( (pos=ua_name.find("#")) != string::npos )
		ua_name = ua_name.substr(0,pos);
	boost::trim(ua_name);
	/*******/
	reading_uas();
	/*******/
	current.ua.push_back(ua_name);
	D(std::cout << "ua|" << yytext << "|" <<std::endl;)
	D(std::cout << "ua+|" << ua_name << "|" <<std::endl;)

}

<*>^Allow:{SPACE}*[^#\n\r]*({COMMENTL})?({NEWLINE})? {
	D(std::cout << "allow|" << yytext << "|" << std::endl;)
	//state = RULES;
	string rule(yytext);
	rule = rule.substr(6);

	string::size_type pos;
	if( (pos=rule.find("#")) != string::npos )
		rule = rule.substr(0,pos);

	boost::trim(rule);
	/*******/
	rules();
	/*******/
	rule = Url::unescape_not(Url::escape(rule,Url_util::URL_CHAR_PATH),Url_util::URL_CHAR_PATH);
	D(std::cout << "allow+|" << rule << "|" << std::endl;)
	Rule r(ALLOW,rule);
	current.rules.push_back(r);
}

<*>^Disallow:{SPACE}*[^#\n\r]*({COMMENTL})?({NEWLINE})? {
	D(std::cout << "disallow|" << yytext << "|" << std::endl;)
	//state = RULES;
	string rule(yytext);
	rule = rule.substr(9);

	string::size_type pos;
	if( (pos=rule.find("#")) != string::npos )
		rule = rule.substr(0,pos);

	boost::trim(rule);
	/*******/
	rules();
	/*******/
	rule = Url::unescape_not(Url::escape(rule,Url_util::URL_CHAR_PATH),Url_util::URL_CHAR_PATH);
	D(std::cout << "disallow|" << rule << "|" << std::endl;)
	Rule r(DISALLOW,rule);
	current.rules.push_back(r);
}

<*>^Crawl-delay:{SPACE}*[^#\n\r]*({COMMENTL})?({NEWLINE})? {
	string delay(yytext);
	delay = delay.substr(12);

	string::size_type pos;
	if( (pos=delay.find("#")) != string::npos )
		delay = delay.substr(0,pos);

	boost::trim(delay);

	/*******/
	rules();
	/*******/
	D(cout << "delay: " << delay << endl;)
	Rule r(CRAWL_DELAY,delay);
	current.rules.push_back(r);
}

<*>{SPACE}*{NEWLINE} {
	D(cout << "space: |" << yytext << "|" << endl;)
}

<*>.+ {
	D(cout << "unmatched: |" << yytext << "|" << endl;)
	errors.append(fs("unmatched: |" << yytext <<"|" << endl));
	if( state == START && regex_search(yytext,sgml_tag) ) {
		//cout << "looks like html" << endl;
		//yyterminate();
		return -1;
	}

//	if (state == START && *yytext == '<') {
//		cout << "looks like html" << endl;
//	//	yyterminate();
//	}
	//state = UNMATCH;
	//unmatch();
	//std::cout << "unmatched: " << yytext << std::endl;
}
<<EOF>> {
	eof();
	start();
	BEGIN(INITIAL);
	yyterminate(); // returns 0
}

%%
