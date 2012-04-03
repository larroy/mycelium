%{
#include "HTML_lexer.hh"
%}

%option noyywrap
%option c++
%option yyclass="HTML_lexer"
%option case-insensitive
%option batch

	/* Figure 1 -- Character Classes: Abstract Syntax */

Digit		[0-9]
LCLetter	[a-z]
Special		['()_,\-\./:=?]
UCLetter	[A-Z]

	/* Figure 2 -- Character Classes: Concrete Syntax */

LCNMCHAR	[\._:-]
	// CombiningChar
	/* LCNMSTRT	[] */
UCNMCHAR	[\.:-]
	/* UCNMSTRT	[] */
	/* @# hmmm. sgml spec says \015 */
RE		\n
	/* @# hmmm. sgml spec says \012 */
RS		\r
SEPCHAR		\011
SPACE		\040

	/* Figure 3 -- Reference Delimiter Set: General */

COM	"--"
CRO	"&#"
DSC	"]"
DSO	"["
ERO	"&"
ETAGO	"</"
LIT	\"
LITA	"'"
MDC	">"
MDO	"<!"
MSC	"]]"
NET     "/"
PERO    "%"
PIC	">"
PIO	"<?"
REFC	";"
STAGO	"<"
TAGC	">"
CDATAO	"<![CDATA["
CDATAC	"]]>"

	/* 9.2.1 SGML Character */

	/*name_start_character	{LCLetter}|{UCLetter}|{LCNMSTRT}|{UCNMSTRT}*/
name_start_character	{LCLetter}|{UCLetter}|[_:]
name_character		{name_start_character}|{Digit}|{LCNMCHAR}|{UCNMCHAR}

	/* 9.3 Name */

name		{name_start_character}{name_character}*
number		{Digit}+
number_token	{Digit}{name_character}*
name_token	{name_character}+

	/* 6.2.1 Space */
s		{SPACE}|{RE}|{RS}|{SEPCHAR}
ps		({SPACE}|{RE}|{RS}|{SEPCHAR})+

	/* trailing white space */
ws		({SPACE}|{RE}|{RS}|{SEPCHAR})*

	/* 9.4.5 Reference End */
reference_end	{REFC}|{RE}

	/*
	 * 10.1.2 Parameter Literal
	 * 7.9.3  Attribute Value Literal
	 * (we leave recognition of character references and entity references,
	 *  and whitespace compression to further processing)
	 *
	 * @# should split this into minimum literal, parameter literal,
	 * @# and attribute value literal.
	 */
	/* "something" 'something' */
literal		({LIT}[^\"]*{LIT})|({LITA}[^\']*{LITA})

	/* 9.6.1 Recognition modes */

	/*
	 * Recognition modes are represented here by start conditions.
	 * The default start condition, INITIAL, represents the
	 * CON recognition mode. This condition is used to detect markup
	 * while parsing normal data charcters (mixed content).
	 *
	 * The CDATA start condition represents the CON recognition
	 * mode with the restriction that only end-tags are recognized,
	 * as in elements with CDATA declared content.
	 * (@# no way to activate it yet: need hook to parser.)
	 *
	 * The TAG recognition mode is split into two start conditions:
	 * ATTR, for recognizing attribute value list sub-tokens in
	 * start-tags, and TAG for recognizing the TAGC (">") delimiter
	 * in end-tags.
	 *
	 * The MD start condition is used in markup declarations. The COM
	 * start condition is used for comment declarations.
	 *
	 * The DS condition is an approximation of the declaration subset
	 * recognition mode in SGML. As we only use this condition after signalling
	 * an error, it is merely a recovery device.
	 *
	 * The CXT, LIT, PI, and REF recognition modes are not separated out
	 * as start conditions, but handled within the rules of other start
	 * conditions. The GRP mode is not represented here.
	 */

	 /* EXCERPT ACTIONS: START */

	/* %x CON == INITIAL */
%x CDATA

%x TAG
%x ATTR
%x ATTRVAL
%x SCRIPT

	/* Markup Declaration */
%x MD
%x COM
	/* Declaration Subset */
%x DS

 /* EXCERPT ACTIONS: STOP */

%%

	/* </title> -- end tag */
<INITIAL>{ETAGO}{name}{ws} {
		++yytext; // remove <
		++yytext; // remove	/ 
		--yyleng;
		--yyleng;
		while( yyleng && ( yytext[yyleng-1] == 0x20 // space
			|| yytext[yyleng-1] == 0x11 // sepchar
			|| yytext[yyleng-1] == 0x0A // CR
			|| yytext[yyleng-1] == 0x0D)) { // LF
			--yyleng;
		}	
		addtoken(SGML_END, yytext, yyleng,true);
		BEGIN(TAG);
	}

<SCRIPT>{ETAGO}{ws}script{ws}{TAGC} {
	++yytext; // remove <
	++yytext; // remove	/ 
	--yyleng;
	--yyleng;
    assert( yyleng >= 0 );
    // theoretically this is illegal
    while( *yytext == ' ' ) {
	    ++yytext;
        --yyleng;
    }

	while( yyleng && (yytext[yyleng-1] == 0x20 // space
		|| yytext[yyleng-1] == 0x11 // sepchar
		|| yytext[yyleng-1] == 0x0A // CR
		|| yytext[yyleng-1] == 0x0D // LF
		|| yytext[yyleng-1] == '>')) { 
		--yyleng;
	}	
	addtoken(SGML_END, yytext, yyleng,true);
	//BEGIN(TAG);

	BEGIN(INITIAL);
}

  /* @# HACK for XMP, LISTING?
  Date: Fri, 19 Jan 1996 23:13:43 -0800
  Message-Id: <v01530502ad25cc1a251b@[206.86.76.80]>
  To: www-html@w3.org
  From: chris@walkaboutsoft.com (Chris Lovett)
  Subject: Re: Daniel Connolly's SGML Lex Specification
  */

  /* </> -- empty end tag */
{ETAGO}{TAGC}			{
		warn("empty end tag not supported", yytext, yyleng);
	}

  /* <!DOCTYPE -- markup declaration */
{MDO}{name}{ws} {
	addtoken(SGML_MARKUP_DECL, yytext, yyleng, true);
	//process();
	BEGIN(MD);
}

  /* <!> -- empty comment */
{MDO}{MDC} { 
	process();
}

  /* <!--  -- comment declaration */
{MDO}{COM}	{
	//addtoken(SGML_MARKUP_DECL, yytext, yyleng);
	//process();
	BEGIN(COM);
}

  /* <![ -- marked section */
{MDO}{DSO}{ws}			{
		warn("marked sections not supported", yytext, yyleng);
		BEGIN(DS); /* @# skip past some stuff */
	}

  /* ]]> -- marked section end */
{MSC}{MDC}			{
		warn("unmatched marked sections end", yytext, yyleng);
	}

  /* <? ...> -- processing instruction */
{PIO}[^>]*{PIC}			{
		addtoken(SGML_PI, yytext, yyleng);
		process();
	}
  /* <name -- start tag */
{STAGO}{name}{ws}		{
		++yytext; // remove <
		--yyleng;
        assert( yyleng >= 0 );
		while( yyleng && ( yytext[yyleng-1] == 0x20 // space
			|| yytext[yyleng-1] == 0x11 // sepchar
			|| yytext[yyleng-1] == 0x0A // CR
			|| yytext[yyleng-1] == 0x0D)) { // LF
			--yyleng;
		}	
		if( strncasecmp(yytext, "script", yyleng) == 0 ) {
			addtoken(SGML_START, yytext, yyleng,true);
	        process();
			BEGIN(SCRIPT);
		} else {	
			addtoken(SGML_START, yytext, yyleng,true);
			BEGIN(ATTR);
		}	
	}


  /* <> -- empty start tag */
{STAGO}{TAGC}			{
		warn("empty start tag not supported", yytext, yyleng);
	}

{CDATAO} {
		warn("CDATA open: ",yytext,yyleng);
		BEGIN(CDATA);
	}

  /* abcd -- data characters */
<CDATA>.*/{CDATAC} {
		warn("CDATA close: ",yytext,yyleng);
		//addtoken(SGML_DATA, yytext, yyleng);
		process();
		BEGIN(INITIAL);
	}

<CDATA>{CDATAC}	{
		warn("CDATA small close: ",yytext,yyleng);
		BEGIN(INITIAL);
}


  /* abcd -- data characters */
  /* ([^<&]|(<[^<&a-zA-Z!->?])|(&[^<&#a-zA-Z]))+|.	{ */
[^<]*|.	{ 
		addtoken(SGML_DATA, yytext, yyleng);
		process();
	}

<SCRIPT>[^<]*|.	{ 
		//std::cout << "script: " << yytext << std::endl;
	}


	/* 7.4 Start Tag */
	/* Actually, the generic identifier specification is consumed
	 * along with the STAGO delimiter ("<"). So we're only looking
	 * for tokens that appear in an attribute specification list,
	 * plus TAGC (">"). NET ("/") and STAGO ("<") signal limitations.
	 */

	/* 7.5 End Tag */
	/* Just looking for TAGC. NET, STAGO as above */

	/* <html xmlns="http://www.w3.org/1999/xhtml" xml:lang="es" lang="es"> */
	/* <a ^href = "xxx"> -- attribute name */
<ATTR>{name}{s}*={ws} {
		if(normalize){
			/* strip trailing space and = */
			while(yyleng && (yytext[yyleng-1] == '=' || isspace(yytext[yyleng-1]))) {
				--yyleng;
			}
		}

		addtoken(SGML_ATTRNAME, yytext, yyleng,true);
		BEGIN(ATTRVAL);
	}

	/* <img src="xxx" ^ismap> -- name */
<ATTR>{name}{ws} {
		addtoken(SGML_NAME, yytext, yyleng, true);
	}


	/* <a name = ^xyz> -- name token */
<ATTRVAL>{name_token}{ws}	{
		addtoken(SGML_NMTOKEN, yytext, yyleng);
		BEGIN(ATTR);
	}

	/* <a href = ^"a b c"> -- literal */
<ATTRVAL>{literal}{ws}		{
	/*	if(yyleng > 2 && yytext[yyleng-2] == '=' && memchr(yytext, '>', yyleng)){
			ERROR(SGML_WARNING, "missing attribute end-quote?", yytext, yyleng);
		} */
		while( yyleng && (yytext[yyleng-1] == 0x20 // space
			|| yytext[yyleng-1] == 0x11 // sepchar
			|| yytext[yyleng-1] == 0x0A // CR
			|| yytext[yyleng-1] == 0x0D)) { // LF
			--yyleng;
		}	
		if( yyleng && ( yytext[yyleng-1] == '"' || yytext[yyleng-1] == '\'' )) {
			--yyleng;
		}	
		if( yyleng && ( yytext[0] == '"' || yytext[0] == '\'' ))  {
			--yyleng;
			++yytext;
		}	

		addtoken(SGML_LITERAL, yytext, yyleng);
		BEGIN(ATTR);
	}

	/* <a href = ^http://foo/> -- unquoted literal HACK */
<ATTRVAL>[^ '"\t\n>]+{ws}	{
		while( yyleng && (yytext[yyleng-1] == 0x20 // space
			|| yytext[yyleng-1] == 0x11 // sepchar
			|| yytext[yyleng-1] == 0x0A // CR
			|| yytext[yyleng-1] == 0x0D)) { // LF
			--yyleng;
		}	
		addtoken(SGML_LITERAL, yytext, yyleng);
		//warn("unquoted attribute value", yytext, yyleng);
		BEGIN(ATTR);
	}

	/* <a name= ^> -- illegal tag close */
<ATTRVAL>{TAGC}			{
		process();
		BEGIN(INITIAL);
	}

  /* <a name=foo ^>,</foo^> -- tag close */
<ATTR,TAG>{TAGC}		{
		addtoken(SGML_TAGC, yytext, yyleng);
		process();
		BEGIN(INITIAL);
	}

  /* <em^/ -- NET tag */
<ATTR,ATTRVAL>{NET}{TAGC} {
		//ERROR(SGML_LIMITATION, "NET tags not supported", yytext, yyleng); 
		addtoken(SGML_TAGC, yytext, yyleng);
		process();
		BEGIN(INITIAL);
	}

	/* <foo^<bar> -- unclosed start tag */
<ATTR,ATTRVAL,TAG>{STAGO}	{
		//ERROR(SGML_LIMITATION, "Unclosed tags not supported", yytext, yyleng);
		/* report pending tag */
		process();
		BEGIN(INITIAL);
	}

<ATTR,ATTRVAL,TAG>.	{
		//	ERROR(SGML_ERROR, "bad character in tag", yytext, yyleng);
		warn("bad character in tag", yytext, yyleng);
	}

    /* 10 Markup Declarations: General */

	/* <!^--...-->   -- comment */
    /*<MD,COM>{COM}([^-]|-[^-])*{COM}{ws}	{*/
    /*
<MD,COM>{COM}.*{COM}{ws}	{
		addtoken(SGML_COMMENT, yytext, yyleng);
	}
*/	

	/* <!doctype ^%foo;> -- parameter entity reference */
<MD>{PERO}{name}{reference_end}?{ws}		{
		//ERROR(SGML_LIMITATION, "parameter entity reference not supported", yytext, yyleng);
		warn("parameter entity reference", yytext, yyleng);
	}

 /* The limited set of markup delcarations we're interested in
  * use only numbers, names, and literals.
  */
<MD>{number}{ws} {
		addtoken(SGML_NUMBER, yytext, yyleng);
	}

<MD>{name}{ws}	{
		addtoken(SGML_NAME, yytext, yyleng, true);
	}

<MD>{literal}{ws} {
		addtoken(SGML_LITERAL, yytext, yyleng);
	}

<MD>{MDC}	{
		addtoken(SGML_TAGC, yytext, yyleng);
		process();
		BEGIN(INITIAL);
	}

<COM>{COM}{ws}*{TAGC} {	
		//std::cout << "comment close\n" << std::endl;
		process();
		BEGIN(INITIAL);
	}

	/* other constructs are errors. */
	/* <!doctype foo ^[  -- declaration subset */
<MD>{DSO}	{
		BEGIN(DS);
	}

<COM>. {
		//std::cout << "comment: " << yytext << std::endl;
	}

<MD>.	{
		warn("illegal character in markup declaration", yytext, yyleng);
	}


	/* 10.4 Marked Section Declaration */
	/* 11.1 Document Type Declaration Subset */

	/* Our parsing of declaration subsets is just an error recovery technique:
	 * we attempt to skip them, but we may be fooled by "]"s
	 * inside comments, etc.
	 */

	/* ]]> -- marked section end */
<DS>{MSC}{MDC}	{
		BEGIN(INITIAL);
	}
  /* ] -- declaration subset close */
<DS>{DSC}	{ BEGIN(COM); }

<DS>[^\]]+	{
		warn("declaration subset: skipping", yytext, yyleng);
	}

<CDATA>. {
		warn("cdata unmatched: ", yytext, yyleng);
	}

<*>[\n\r]+ {
		//warn("unmatched: ", yytext, yyleng);
	}

<*>. {
		warn("unmatched: ", yytext, yyleng);
	}

 /* EXCERPT ACTIONS: STOP */
<*><<EOF>> {
        BEGIN(INITIAL);
		finalize();
        yyterminate(); // returns 0
    }



%%
