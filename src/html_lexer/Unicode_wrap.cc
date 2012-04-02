/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Unicode_wrap.hh"
#include <cstring>
#include "utils.hh" // DLOG
using namespace std;

namespace unicode_wrap {

string cp2utf8(const UChar32 c)
{
	string result;
	if ( ! u_isdefined(c) )
		return result;

	vector<char> utf8;
	UnicodeString s=c;
	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("utf8", &status);
	if( ! U_SUCCESS(status)) {
		string err = "Can't open conversion to utf8 reason: ";
		throw unicode_error(err + u_errorName(status));
	}	
	utf8.resize(UCNV_GET_MAX_BYTES_FOR_STRING(s.length(),ucnv_getMaxCharSize(conv)));
	int32_t size = ucnv_fromUChars(conv,&utf8[0],utf8.size(),s.getBuffer(),s.length(),&status);
	if( U_SUCCESS(status) ) {
		result.assign(&utf8[0],size);
	} else {
		string err = "ucnv_fromUChars: ";
		ucnv_close(conv);
		throw unicode_error(err + u_errorName(status));
	}
	ucnv_close(conv);
	return result;
}

// FIXME
void Converter::convert_unicode_to(const string& enc, const UnicodeString &s, vector<char>& res) 
{
	DLOG(cout << "Converter::convert_unicode_to(" << enc << ")" << endl;)
	status = U_ZERO_ERROR;
	if(in_conv) {
		ucnv_close(in_conv);
		in_conv=0;
	}	
	in_conv = my_ucnv_open(enc.c_str());
	if( ! U_SUCCESS(status)) {
		string err = "Can't open conversion to " + enc;
		throw unicode_error(err + u_errorName(status));
	}	
	size_t need = UCNV_GET_MAX_BYTES_FOR_STRING(s.length(),ucnv_getMaxCharSize(in_conv));
	DLOG(cout << "need:" << need << endl; )
	res.resize(need);
	int32_t size = ucnv_fromUChars(in_conv,&res[0],res.size(),s.getBuffer(),s.length(),&status);
	if( ! U_SUCCESS(status)) {
		string err = "ucnv_fromUChars error: ";
		throw unicode_error(err + u_errorName(status));
	}	
	res.resize(size);
	ucnv_close(in_conv);
	in_conv=0;
}

void Converter::convert_to_unicode(const string& in_encoding, const string& in_content, UnicodeString& str) {
	DLOG(cout << "Converter::convert_to_unicode(" << in_encoding << "," << in_content << ")" << endl;)
	DLOG(cout << "in_content_size: " << in_content.size() << endl;)
	status = U_ZERO_ERROR;
	if(in_conv) {
		ucnv_close(in_conv);
		in_conv=0;
	}	
	in_conv = my_ucnv_open(in_encoding.c_str());
	DLOG(cout << "we need: " << UCNV_GET_MAX_BYTES_FOR_STRING(in_content.length(),ucnv_getMaxCharSize(in_conv)) << endl;)
	const char* source = in_content.data();
	const char* sourcelimit =  in_content.data() + in_content.size();
	size_t done=0;
	do {
		status = U_ZERO_ERROR;
		UChar* target = &u_buff[done];
		UChar* targetlimit = &u_buff[u_buff.size()];
		ucnv_toUnicode(in_conv,&target,targetlimit, &source, sourcelimit, NULL, TRUE, &status);
		done += target - &u_buff[done]; 
		if( status == U_BUFFER_OVERFLOW_ERROR) {
			u_buff.resize(((u_buff.size()+1))*2);
			DLOG(cout << "Need to resize to: " << u_buff.size() << endl;)
		} else {
			if( ! U_SUCCESS(status)) 
				throw unicode_error(u_errorName(status));
		}		
	} while(! U_SUCCESS(status));	
	//cout << makeHexDump(string((char*)&u_buff[0],u_buff.size())) << endl;
	//cout << "done: " << done;
	memcpy(str.getBuffer(done),&u_buff[0],sizeof(UChar)*done);
	str.releaseBuffer(done);
	//printUnicodeString(str);
	ucnv_close(in_conv);
	in_conv=0;
}

// FIXME, think if this can leak or not
//
Converter::Converter(const string& in_encoding, const string& out_encoding) :
	in_encoding(in_encoding),
	out_encoding(out_encoding),
	in_conv(my_ucnv_open(in_encoding.c_str())),
	out_conv(my_ucnv_open(out_encoding.c_str())),
	status(U_ZERO_ERROR),
	u_buff(),
	o_buff()
{ }

Converter::Converter() :
	in_encoding(),
	out_encoding(),
	in_conv(0),
	out_conv(0),
	status(U_ZERO_ERROR),
	u_buff(),
	o_buff()

{}

Converter::~Converter()
{
	if(out_conv) {
		ucnv_close(out_conv);
		out_conv=0;
	}	
	if(in_conv)	{
		ucnv_close(in_conv);
		in_conv=0;
	}	
}

void Converter::open(const std::string& in_encoding, const std::string& out_encoding)
{
	this->in_encoding = in_encoding;
	this->out_encoding = out_encoding;
	if(out_conv) {
		ucnv_close(out_conv);
		out_conv=0;
	}	
	if(in_conv)	{
		ucnv_close(in_conv);
		in_conv=0;
	}	
	in_conv = my_ucnv_open(in_encoding.c_str());
	out_conv = my_ucnv_open(out_encoding.c_str());
}

UConverter* Converter::my_ucnv_open(const char* encoding)
{
	status = U_ZERO_ERROR;
	UConverter* cnv = ucnv_open(encoding, &status);
	if( ! U_SUCCESS(status)) {
		string err = "Can't open conversion: '";
		err += encoding + '\'';
		throw unicode_error(err + u_errorName(status));
	}
	return cnv;
}


void Converter::convert(const string& in_content, string& out_content) {
	DLOG(cout << "Converter::convert(" << in_content << "," << out_content << ")" << endl;)
	DLOG(cout << "u_buff.size():" << u_buff.size() << endl; )
	const char* source = in_content.data();
	const char* sourcelimit =  in_content.data() + in_content.size();
	size_t done=0;
	do {
		status = U_ZERO_ERROR;
		UChar* target = &u_buff[done];
		UChar* targetlimit = &u_buff[u_buff.size()];
		ucnv_toUnicode(in_conv,&target,targetlimit, &source, sourcelimit, NULL, TRUE, &status);
		done += target - &u_buff[done]; 
		if( status == U_BUFFER_OVERFLOW_ERROR) {
			u_buff.resize(((u_buff.size()+1))*2);
			DLOG(cout << "Need to resize to: " << u_buff.size() << endl;)
		} else if(! U_SUCCESS(status)) {
			throw unicode_error(u_errorName(status));
	//	} else if( target != targetlimit ) {
	//		cerr << hex << target << " != " << targetlimit << dec << endl;
		}
	} while(! U_SUCCESS(status));	
	//cout << makeHexDump(string((char*)&u_buff[0],u_buff.size())) << endl;

	const UChar* usource = &u_buff[0];
	const UChar* usourcelimit = &u_buff[done];
	char* otarget = 0;
	char* otargetlimit = 0;
	size_t total=0;
//	vector<char> o_buff(ucnv_getMinCharSize(out_conv) * u_buff.size() * sizeof(UChar));
	do {
		status = U_ZERO_ERROR;
		DLOG(cout << "usource: "<< (void*) usource << endl;)
		DLOG(cout << "usourcelimit: "<<(void*) usourcelimit << endl;)
		otarget = &o_buff[total];
		otargetlimit = &o_buff[o_buff.size()];
		/********/
		//cout << "ucnv_fromUnicode(" << (void*) otarget <<","<<(void*)otargetlimit<<","<<(void*)usource <<","<<(void*)usourcelimit<<");" << endl;
		ucnv_fromUnicode(out_conv, &otarget, otargetlimit, &usource, usourcelimit, NULL, TRUE, &status);
		total += otarget - &o_buff[total];
		//cout << "usource: "<< (void*) usource << endl;
		//cout << "otarget: " << (void*) otarget << endl;
		/********/
		//cout.write((char*)&o_buff[0],o_buff.size());
		//if(total > 32)
			//cout << makeHexDump(string((char*)&o_buff[total-32],32)) << endl;
		//cout << makeHexDump(string((char*)otarget,(size_t)(&o_buff[o_buff.size()]-otarget)) << endl;
		if(U_SUCCESS(status))
			break;
		else if(status == U_BUFFER_OVERFLOW_ERROR) {
			o_buff.resize(((o_buff.size()+1))*2);
			DLOG(cout << "Need to resize to: " << o_buff.size() << endl;)
			//cout << "resize: " << o_buff.size() << endl;
		} else if(! U_SUCCESS(status)) {
			throw unicode_error(u_errorName(status));
		}

	} while(status  == U_BUFFER_OVERFLOW_ERROR);	
	out_content.assign(&o_buff[0],total);
}

}; // end namespace

