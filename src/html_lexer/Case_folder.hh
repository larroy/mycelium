/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup FeatureExtraction 
 * @brief tools to convert text to features
 * @{
 */

#ifndef case_folder_hh 
#define case_folder_hh 1
#include <string>
#include <unicode/utypes.h>   /* Basic ICU data types */
#include <unicode/ucnv.h>     /* C   Converter API    */

namespace features {

class Case_folder {
public:
	Case_folder();
	~Case_folder();
	std::string fold_case_utf8(const std::string& in);
private:	
	UConverter* conv;
	UErrorCode status;
};

};
#endif
/** @} */
