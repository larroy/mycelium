#include "Entity_handler.hh"

#include <boost/regex.hpp>
#include <stdexcept>
#include <iterator>
#include <algorithm>

#include "Unicode_wrap.hh"
#include "utils.hh"

using namespace std;
using boost::regex;
using boost::smatch;
using unicode_wrap::cp2utf8;
/**
 * ENTITIES
 */
Entity_handler::Entity_handler() :
	charent()
{
	// Standard
	charent.insert(make_pair("quot", cp2utf8(0x0022)));
	charent.insert(make_pair("amp", cp2utf8(0x0026)));
	charent.insert(make_pair("apos", cp2utf8(0x0027)));
	charent.insert(make_pair("lt", cp2utf8(0x003c)));
	charent.insert(make_pair("gt", cp2utf8(0x003e)));
	charent.insert(make_pair("nbsp", cp2utf8(0x00a0)));
	charent.insert(make_pair("iexcl", cp2utf8(0x00a1)));
	charent.insert(make_pair("cent", cp2utf8(0x00a2)));
	charent.insert(make_pair("pound", cp2utf8(0x00a3)));
	charent.insert(make_pair("curren", cp2utf8(0x00a4)));
	charent.insert(make_pair("yen", cp2utf8(0x00a5)));
	charent.insert(make_pair("brvbar", cp2utf8(0x00a6)));
	charent.insert(make_pair("sect", cp2utf8(0x00a7)));
	charent.insert(make_pair("uml", cp2utf8(0x00a8)));
	charent.insert(make_pair("copy", cp2utf8(0x00a9)));
	charent.insert(make_pair("ordf", cp2utf8(0x00aa)));
	charent.insert(make_pair("laquo", cp2utf8(0x00ab)));
	charent.insert(make_pair("not", cp2utf8(0x00ac)));
	charent.insert(make_pair("shy", cp2utf8(0x00ad)));
	charent.insert(make_pair("reg", cp2utf8(0x00ae)));
	charent.insert(make_pair("macr", cp2utf8(0x00af)));
	charent.insert(make_pair("deg", cp2utf8(0x00b0)));
	charent.insert(make_pair("plusmn", cp2utf8(0x00b1)));
	charent.insert(make_pair("sup2", cp2utf8(0x00b2)));
	charent.insert(make_pair("sup3", cp2utf8(0x00b3)));
	charent.insert(make_pair("acute", cp2utf8(0x00b4)));
	charent.insert(make_pair("micro", cp2utf8(0x00b5)));
	charent.insert(make_pair("para", cp2utf8(0x00b6)));
	charent.insert(make_pair("middot", cp2utf8(0x00b7)));
	charent.insert(make_pair("cedil", cp2utf8(0x00b8)));
	charent.insert(make_pair("sup1", cp2utf8(0x00b9)));
	charent.insert(make_pair("ordm", cp2utf8(0x00ba)));
	charent.insert(make_pair("raquo", cp2utf8(0x00bb)));
	charent.insert(make_pair("frac14", cp2utf8(0x00bc)));
	charent.insert(make_pair("frac12", cp2utf8(0x00bd)));
	charent.insert(make_pair("frac34", cp2utf8(0x00be)));
	charent.insert(make_pair("iquest", cp2utf8(0x00bf)));
	charent.insert(make_pair("Agrave", cp2utf8(0x00c0)));
	charent.insert(make_pair("Aacute", cp2utf8(0x00c1)));
	charent.insert(make_pair("Acirc", cp2utf8(0x00c2)));
	charent.insert(make_pair("Atilde", cp2utf8(0x00c3)));
	charent.insert(make_pair("Auml", cp2utf8(0x00c4)));
	charent.insert(make_pair("Aring", cp2utf8(0x00c5)));
	charent.insert(make_pair("AElig", cp2utf8(0x00c6)));
	charent.insert(make_pair("Ccedil", cp2utf8(0x00c7)));
	charent.insert(make_pair("Egrave", cp2utf8(0x00c8)));
	charent.insert(make_pair("Eacute", cp2utf8(0x00c9)));
	charent.insert(make_pair("Ecirc", cp2utf8(0x00ca)));
	charent.insert(make_pair("Euml", cp2utf8(0x00cb)));
	charent.insert(make_pair("Igrave", cp2utf8(0x00cc)));
	charent.insert(make_pair("Iacute", cp2utf8(0x00cd)));
	charent.insert(make_pair("Icirc", cp2utf8(0x00ce)));
	charent.insert(make_pair("Iuml", cp2utf8(0x00cf)));
	charent.insert(make_pair("ETH", cp2utf8(0x00d0)));
	charent.insert(make_pair("Ntilde", cp2utf8(0x00d1)));
	charent.insert(make_pair("Ograve", cp2utf8(0x00d2)));
	charent.insert(make_pair("Oacute", cp2utf8(0x00d3)));
	charent.insert(make_pair("Ocirc", cp2utf8(0x00d4)));
	charent.insert(make_pair("Otilde", cp2utf8(0x00d5)));
	charent.insert(make_pair("Ouml", cp2utf8(0x00d6)));
	charent.insert(make_pair("times", cp2utf8(0x00d7)));
	charent.insert(make_pair("Oslash", cp2utf8(0x00d8)));
	charent.insert(make_pair("Ugrave", cp2utf8(0x00d9)));
	charent.insert(make_pair("Uacute", cp2utf8(0x00da)));
	charent.insert(make_pair("Ucirc", cp2utf8(0x00db)));
	charent.insert(make_pair("Uuml", cp2utf8(0x00dc)));
	charent.insert(make_pair("Yacute", cp2utf8(0x00dd)));
	charent.insert(make_pair("THORN", cp2utf8(0x00de)));
	charent.insert(make_pair("szlig", cp2utf8(0x00df)));
	charent.insert(make_pair("agrave", cp2utf8(0x00e0)));
	charent.insert(make_pair("aacute", cp2utf8(0x00e1)));
	charent.insert(make_pair("acirc", cp2utf8(0x00e2)));
	charent.insert(make_pair("atilde", cp2utf8(0x00e3)));
	charent.insert(make_pair("auml", cp2utf8(0x00e4)));
	charent.insert(make_pair("aring", cp2utf8(0x00e5)));
	charent.insert(make_pair("aelig", cp2utf8(0x00e6)));
	charent.insert(make_pair("ccedil", cp2utf8(0x00e7)));
	charent.insert(make_pair("egrave", cp2utf8(0x00e8)));
	charent.insert(make_pair("eacute", cp2utf8(0x00e9)));
	charent.insert(make_pair("ecirc", cp2utf8(0x00ea)));
	charent.insert(make_pair("euml", cp2utf8(0x00eb)));
	charent.insert(make_pair("igrave", cp2utf8(0x00ec)));
	charent.insert(make_pair("iacute", cp2utf8(0x00ed)));
	charent.insert(make_pair("icirc", cp2utf8(0x00ee)));
	charent.insert(make_pair("iuml", cp2utf8(0x00ef)));
	charent.insert(make_pair("eth", cp2utf8(0x00f0)));
	charent.insert(make_pair("ntilde", cp2utf8(0x00f1)));
	charent.insert(make_pair("ograve", cp2utf8(0x00f2)));
	charent.insert(make_pair("oacute", cp2utf8(0x00f3)));
	charent.insert(make_pair("ocirc", cp2utf8(0x00f4)));
	charent.insert(make_pair("otilde", cp2utf8(0x00f5)));
	charent.insert(make_pair("ouml", cp2utf8(0x00f6)));
	charent.insert(make_pair("divide", cp2utf8(0x00f7)));
	charent.insert(make_pair("oslash", cp2utf8(0x00f8)));
	charent.insert(make_pair("ugrave", cp2utf8(0x00f9)));
	charent.insert(make_pair("uacute", cp2utf8(0x00fa)));
	charent.insert(make_pair("ucirc", cp2utf8(0x00fb)));
	charent.insert(make_pair("uuml", cp2utf8(0x00fc)));
	charent.insert(make_pair("yacute", cp2utf8(0x00fd)));
	charent.insert(make_pair("thorn", cp2utf8(0x00fe)));
	charent.insert(make_pair("yuml", cp2utf8(0x00ff)));
	charent.insert(make_pair("OElig", cp2utf8(0x0152)));
	charent.insert(make_pair("oelig", cp2utf8(0x0153)));
	charent.insert(make_pair("Scaron", cp2utf8(0x0160)));
	charent.insert(make_pair("scaron", cp2utf8(0x0161)));
	charent.insert(make_pair("Yuml", cp2utf8(0x0178)));
	charent.insert(make_pair("fnof", cp2utf8(0x0192)));
	charent.insert(make_pair("circ", cp2utf8(0x02c6)));
	charent.insert(make_pair("tilde", cp2utf8(0x02dc)));
	charent.insert(make_pair("Alpha", cp2utf8(0x0391)));
	charent.insert(make_pair("Beta", cp2utf8(0x0392)));
	charent.insert(make_pair("Gamma", cp2utf8(0x0393)));
	charent.insert(make_pair("Delta", cp2utf8(0x0394)));
	charent.insert(make_pair("Epsilon", cp2utf8(0x0395)));
	charent.insert(make_pair("Zeta", cp2utf8(0x0396)));
	charent.insert(make_pair("Eta", cp2utf8(0x0397)));
	charent.insert(make_pair("Theta", cp2utf8(0x0398)));
	charent.insert(make_pair("Iota", cp2utf8(0x0399)));
	charent.insert(make_pair("Kappa", cp2utf8(0x039a)));
	charent.insert(make_pair("Lambda", cp2utf8(0x039b)));
	charent.insert(make_pair("Mu", cp2utf8(0x039c)));
	charent.insert(make_pair("Nu", cp2utf8(0x039d)));
	charent.insert(make_pair("Xi", cp2utf8(0x039e)));
	charent.insert(make_pair("Omicron", cp2utf8(0x039f)));
	charent.insert(make_pair("Pi", cp2utf8(0x03a0)));
	charent.insert(make_pair("Rho", cp2utf8(0x03a1)));
	charent.insert(make_pair("Sigma", cp2utf8(0x03a3)));
	charent.insert(make_pair("Tau", cp2utf8(0x03a4)));
	charent.insert(make_pair("Upsilon", cp2utf8(0x03a5)));
	charent.insert(make_pair("Phi", cp2utf8(0x03a6)));
	charent.insert(make_pair("Chi", cp2utf8(0x03a7)));
	charent.insert(make_pair("Psi", cp2utf8(0x03a8)));
	charent.insert(make_pair("Omega", cp2utf8(0x03a9)));
	charent.insert(make_pair("alpha", cp2utf8(0x03b1)));
	charent.insert(make_pair("beta", cp2utf8(0x03b2)));
	charent.insert(make_pair("gamma", cp2utf8(0x03b3)));
	charent.insert(make_pair("delta", cp2utf8(0x03b4)));
	charent.insert(make_pair("epsilon", cp2utf8(0x03b5)));
	charent.insert(make_pair("zeta", cp2utf8(0x03b6)));
	charent.insert(make_pair("eta", cp2utf8(0x03b7)));
	charent.insert(make_pair("theta", cp2utf8(0x03b8)));
	charent.insert(make_pair("iota", cp2utf8(0x03b9)));
	charent.insert(make_pair("kappa", cp2utf8(0x03ba)));
	charent.insert(make_pair("lambda", cp2utf8(0x03bb)));
	charent.insert(make_pair("mu", cp2utf8(0x03bc)));
	charent.insert(make_pair("nu", cp2utf8(0x03bd)));
	charent.insert(make_pair("xi", cp2utf8(0x03be)));
	charent.insert(make_pair("omicron", cp2utf8(0x03bf)));
	charent.insert(make_pair("pi", cp2utf8(0x03c0)));
	charent.insert(make_pair("rho", cp2utf8(0x03c1)));
	charent.insert(make_pair("sigmaf", cp2utf8(0x03c2)));
	charent.insert(make_pair("sigma", cp2utf8(0x03c3)));
	charent.insert(make_pair("tau", cp2utf8(0x03c4)));
	charent.insert(make_pair("upsilon", cp2utf8(0x03c5)));
	charent.insert(make_pair("phi", cp2utf8(0x03c6)));
	charent.insert(make_pair("chi", cp2utf8(0x03c7)));
	charent.insert(make_pair("psi", cp2utf8(0x03c8)));
	charent.insert(make_pair("omega", cp2utf8(0x03c9)));
	charent.insert(make_pair("thetasym", cp2utf8(0x03d1)));
	charent.insert(make_pair("upsih", cp2utf8(0x03d2)));
	charent.insert(make_pair("piv", cp2utf8(0x03d6)));
	charent.insert(make_pair("ensp", cp2utf8(0x2002)));
	charent.insert(make_pair("emsp", cp2utf8(0x2003)));
	charent.insert(make_pair("thinsp", cp2utf8(0x2009)));
	charent.insert(make_pair("zwnj", cp2utf8(0x200c)));
	charent.insert(make_pair("zwj", cp2utf8(0x200d)));
	charent.insert(make_pair("lrm", cp2utf8(0x200e)));
	charent.insert(make_pair("rlm", cp2utf8(0x200f)));
	charent.insert(make_pair("ndash", cp2utf8(0x2013)));
	charent.insert(make_pair("mdash", cp2utf8(0x2014)));
	charent.insert(make_pair("lsquo", cp2utf8(0x2018)));
	charent.insert(make_pair("rsquo", cp2utf8(0x2019)));
	charent.insert(make_pair("sbquo", cp2utf8(0x201a)));
	charent.insert(make_pair("ldquo", cp2utf8(0x201c)));
	charent.insert(make_pair("rdquo", cp2utf8(0x201d)));
	charent.insert(make_pair("bdquo", cp2utf8(0x201e)));
	charent.insert(make_pair("dagger", cp2utf8(0x2020)));
	charent.insert(make_pair("Dagger", cp2utf8(0x2021)));
	charent.insert(make_pair("bull", cp2utf8(0x2022)));
	charent.insert(make_pair("hellip", cp2utf8(0x2026)));
	charent.insert(make_pair("permil", cp2utf8(0x2030)));
	charent.insert(make_pair("prime", cp2utf8(0x2032)));
	charent.insert(make_pair("Prime", cp2utf8(0x2033)));
	charent.insert(make_pair("lsaquo", cp2utf8(0x2039)));
	charent.insert(make_pair("rsaquo", cp2utf8(0x203a)));
	charent.insert(make_pair("oline", cp2utf8(0x203e)));
	charent.insert(make_pair("frasl", cp2utf8(0x2044)));
	charent.insert(make_pair("euro", cp2utf8(0x20ac)));
	charent.insert(make_pair("image", cp2utf8(0x2111)));
	charent.insert(make_pair("weierp", cp2utf8(0x2118)));
	charent.insert(make_pair("real", cp2utf8(0x211c)));
	charent.insert(make_pair("trade", cp2utf8(0x2122)));
	charent.insert(make_pair("alefsym", cp2utf8(0x2135)));
	charent.insert(make_pair("larr", cp2utf8(0x2190)));
	charent.insert(make_pair("uarr", cp2utf8(0x2191)));
	charent.insert(make_pair("rarr", cp2utf8(0x2192)));
	charent.insert(make_pair("darr", cp2utf8(0x2193)));
	charent.insert(make_pair("harr", cp2utf8(0x2194)));
	charent.insert(make_pair("crarr", cp2utf8(0x21b5)));
	charent.insert(make_pair("lArr", cp2utf8(0x21d0)));
	charent.insert(make_pair("uArr", cp2utf8(0x21d1)));
	charent.insert(make_pair("rArr", cp2utf8(0x21d2)));
	charent.insert(make_pair("dArr", cp2utf8(0x21d3)));
	charent.insert(make_pair("hArr", cp2utf8(0x21d4)));
	charent.insert(make_pair("forall", cp2utf8(0x2200)));
	charent.insert(make_pair("part", cp2utf8(0x2202)));
	charent.insert(make_pair("exist", cp2utf8(0x2203)));
	charent.insert(make_pair("empty", cp2utf8(0x2205)));
	charent.insert(make_pair("nabla", cp2utf8(0x2207)));
	charent.insert(make_pair("isin", cp2utf8(0x2208)));
	charent.insert(make_pair("notin", cp2utf8(0x2209)));
	charent.insert(make_pair("ni", cp2utf8(0x220b)));
	charent.insert(make_pair("prod", cp2utf8(0x220f)));
	charent.insert(make_pair("sum", cp2utf8(0x2211)));
	charent.insert(make_pair("minus", cp2utf8(0x2212)));
	charent.insert(make_pair("lowast", cp2utf8(0x2217)));
	charent.insert(make_pair("radic", cp2utf8(0x221a)));
	charent.insert(make_pair("prop", cp2utf8(0x221d)));
	charent.insert(make_pair("infin", cp2utf8(0x221e)));
	charent.insert(make_pair("ang", cp2utf8(0x2220)));
	charent.insert(make_pair("and", cp2utf8(0x2227)));
	charent.insert(make_pair("or", cp2utf8(0x2228)));
	charent.insert(make_pair("cap", cp2utf8(0x2229)));
	charent.insert(make_pair("cup", cp2utf8(0x222a)));
	charent.insert(make_pair("int", cp2utf8(0x222b)));
	charent.insert(make_pair("there4", cp2utf8(0x2234)));
	charent.insert(make_pair("sim", cp2utf8(0x223c)));
	charent.insert(make_pair("cong", cp2utf8(0x2245)));
	charent.insert(make_pair("asymp", cp2utf8(0x2248)));
	charent.insert(make_pair("ne", cp2utf8(0x2260)));
	charent.insert(make_pair("equiv", cp2utf8(0x2261)));
	charent.insert(make_pair("le", cp2utf8(0x2264)));
	charent.insert(make_pair("ge", cp2utf8(0x2265)));
	charent.insert(make_pair("sub", cp2utf8(0x2282)));
	charent.insert(make_pair("sup", cp2utf8(0x2283)));
	charent.insert(make_pair("nsub", cp2utf8(0x2284)));
	charent.insert(make_pair("sube", cp2utf8(0x2286)));
	charent.insert(make_pair("supe", cp2utf8(0x2287)));
	charent.insert(make_pair("oplus", cp2utf8(0x2295)));
	charent.insert(make_pair("otimes", cp2utf8(0x2297)));
	charent.insert(make_pair("perp", cp2utf8(0x22a5)));
	charent.insert(make_pair("sdot", cp2utf8(0x22c5)));
	charent.insert(make_pair("lceil", cp2utf8(0x2308)));
	charent.insert(make_pair("rceil", cp2utf8(0x2309)));
	charent.insert(make_pair("lfloor", cp2utf8(0x230a)));
	charent.insert(make_pair("rfloor", cp2utf8(0x230b)));
	charent.insert(make_pair("lang", cp2utf8(0x2329)));
	charent.insert(make_pair("rang", cp2utf8(0x232a)));
	charent.insert(make_pair("loz", cp2utf8(0x25ca)));
	charent.insert(make_pair("spades", cp2utf8(0x2660)));
	charent.insert(make_pair("clubs", cp2utf8(0x2663)));
	charent.insert(make_pair("hearts", cp2utf8(0x2665)));
	charent.insert(make_pair("diams", cp2utf8(0x2666)));

	// Non standard
	charent.insert(make_pair("QUOT", cp2utf8(0x0022)));
	charent.insert(make_pair("AMP", cp2utf8(0x0026)));
	charent.insert(make_pair("COPY", cp2utf8(0x00a9)));
	charent.insert(make_pair("GT", cp2utf8(0x003e)));
	charent.insert(make_pair("LT", cp2utf8(0x003c)));
	charent.insert(make_pair("REG", cp2utf8(0x00ae)));


}

string Entity_handler::char_entity(const string& str)
{
	string result;
	charent_t::iterator i;	
	if( (i = charent.find(str)) != charent.end() ) 
		result = i->second;
	return result;	
}

string Entity_handler::dec_numchar_ref(const string& str)
{
	string result;
	try {
		int num=0;
		int pos=0;
		for(string::const_reverse_iterator i = str.rbegin(); i != str.rend(); ++i,++pos) {
			if( ! isdigit(*i) )
				//throw runtime_error("not a decimal digit in dec_numchar_ref: " + str);
				return result;

			int val = *i - '0';
			for(int pow=0; pow < pos; ++pow)
				val *= 10;
			num += val;
			if( num > UCHAR_MAX_VALUE)
				return result;
				//throw runtime_error("Dec numchar ref out of range" + str);
		}
		if( num <= UCHAR_MAX_VALUE  && num >= UCHAR_MIN_VALUE ) {
			//cout << "dec_numchar: " << num << endl;
			result = cp2utf8(static_cast<UChar32>(num));
		}
	} catch(logic_error& e) {
	} catch(unicode_wrap::unicode_error& e) {
	} catch(runtime_error& e) {
	}

	return result;
}

string Entity_handler::hex_numchar_ref(const string& str)
{
	string result;
	try {
		int num=0;
		int pos=0;
		for(string::const_reverse_iterator i = str.rbegin(); i != str.rend(); ++i,++pos) {
			num += (utils::xdigit_to_num(*i) << (4*pos));
			if( num > UCHAR_MAX_VALUE)
				//throw runtime_error("Hex_numchar_ref out of range" + str);
				return result;
		}
		if( num <= UCHAR_MAX_VALUE  && num >= UCHAR_MIN_VALUE ) {
			//cout << "hex_numchar: " << num << endl;
			result = cp2utf8(static_cast<UChar32>(num));
		}
	} catch(logic_error& e) {
	} catch(unicode_wrap::unicode_error& e) {
	} catch(runtime_error& e) {
	}

	return result;
}

string Entity_handler::replace_all_entities(const string& str) 
{
	 string result = replace_char_entities(str);
	 result = replace_dec_numchar_ref(result);
	 result = replace_hex_numchar_ref(result);
	 return result;
}

string Entity_handler::replace_char_entities(const string& str) {
	string result;
	regex re("&([[:alpha:]_:][\\w._:-]*);",boost::regex_constants::perl);
	smatch m;
	string::const_iterator from = str.begin();
	while( regex_search(from, str.end(), m, re) ) {
		string entity = string(m[1].first,m[1].second);
		//DLOG(cout << "found char_entity_ref: " << entity << endl;)
		copy(from,m[0].first,back_inserter(result));
		from = m[0].second;
		result.append(char_entity(entity));
		//DLOG(cout << "replaced by: " << char_entity(entity) << endl;)
	}
	if( from != str.end() )  // copy the rest
		copy(from,str.end(),back_inserter(result));
	return result;	
}

string Entity_handler::replace_dec_numchar_ref(const string& str) {
	string result;
	regex re("&#(\\d+);",boost::regex_constants::perl);
	smatch m;
	string::const_iterator from = str.begin();
	while( regex_search(from, str.end(), m, re) ) {
		string entity = string(m[1].first,m[1].second);
		//DLOG(cout << "found dec_numchar_ref: " << entity << endl;)
		copy(from,m[0].first,back_inserter(result));
		from = m[0].second;
		result.append(dec_numchar_ref(entity));
		//DLOG(cout << "replaced by: " << dec_numchar_ref(entity) << endl;)
	}
	if( from != str.end() ) // copy the rest
		copy(from,str.end(),back_inserter(result));
	return result;	

}

string Entity_handler::replace_hex_numchar_ref(const string& str) {
	string result;
	regex re("&#x([[:xdigit:]]+);",boost::regex_constants::perl);
	smatch m;
	string::const_iterator from = str.begin();
	while( regex_search(from, str.end(), m, re) ) {
		string entity = string(m[1].first,m[1].second);
		//DLOG(cout << "found hex_numchar_ref: " << entity << endl;)
		copy(from,m[0].first,back_inserter(result));
		from = m[0].second;
		result.append(hex_numchar_ref(entity));
		//DLOG(cout << "replaced by: " << hex_numchar_ref(entity) << endl;)
	}
	if( from != str.end() ) // copy the rest
		copy(from,str.end(),back_inserter(result));
	return result;	
}

