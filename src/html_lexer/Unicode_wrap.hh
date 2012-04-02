/*
 * Copyright 2012 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */
/**
 * @addtogroup Unicode
 * @{
 */
#pragma once

#include <unicode/utypes.h>   /* Basic ICU data types */
#include <unicode/ucnv.h>     /* C   Converter API    */
#include <unicode/ustring.h>  /* some more string fcns*/
#include <unicode/uchar.h>    /* char names           */
#include <unicode/uloc.h>
#include <unicode/unistr.h>
#include <unicode/utext.h>
#include <unicode/ubrk.h>
#include <unicode/brkiter.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

namespace unicode_wrap {

/// Exception throw when there's a conversion error, or the encoding is not found
class unicode_error : public std::runtime_error {
    public:
        unicode_error(const std::string& reason) : std::runtime_error(reason) { }
        unicode_error() : std::runtime_error("unspecified") { }
};

/**
 * Code point to UTF-8
 */
std::string cp2utf8(const UChar32);


/** 
 * @brief Wrapper class for text transcoding
 */
class Converter {
	public:
		Converter(const std::string& in_encoding, const std::string& out_encoding);
		Converter();
		~Converter();

		void open(const std::string& in_encoding, const std::string& out_encoding);
		void convert(const std::string& in_content, std::string& out_content);
		void convert_to_unicode(const std::string& in_encoding, const std::string& in_content, UnicodeString& str); 

		void convert_unicode_to(const std::string& enc, const UnicodeString &s, std::vector<char>& res);

	private:
		UConverter* my_ucnv_open(const char* encoding);
		std::string in_encoding;
		std::string out_encoding;
		UConverter* in_conv;
		UConverter* out_conv;
		UErrorCode status;
		std::vector<UChar> u_buff;
		std::vector<char> o_buff;
		Converter(const Converter&);	
		Converter& operator=(const Converter&);	
};

}; // end namespace
/** @} */
