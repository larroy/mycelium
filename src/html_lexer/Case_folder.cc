#include "Case_folder.hh"
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uiter.h"
#include "unicode/uchar.h"
#include "unicode/ucasemap.h"

#include "ucnv_filter.hh"
#include "Unicode_wrap.hh"

#include <vector>
#include <stdexcept>
#include <iostream>

#include "utils.hh"


using namespace std;

namespace features {

Case_folder::Case_folder()
{
	status = U_ZERO_ERROR;
	conv = ucnv_open("utf-8",&status);
	if( ! U_SUCCESS(status)) 
		throw runtime_error("ucnv_open failed");
}


Case_folder::~Case_folder()
{
	if( conv )  {
		ucnv_close(conv);
		conv = 0;
	}	
}


std::string Case_folder::fold_case_utf8(const std::string& in)
{
	std::string out;
	const char* src = in.data();
	const char* src_limit = in.data() + in.size();
	size_t done=0;
	std::vector<UChar> u_buff(in.size());
	std::vector<char> o_buff(in.size());

	if(in.empty()) {
		out.clear();
		return out;
	}	

	do {
		status = U_ZERO_ERROR;
		UChar* target = &u_buff[done];
		UChar* targetlimit = &u_buff[u_buff.size()];
		ucnv_toUnicode(conv,&target,targetlimit, &src, src_limit, NULL, TRUE, &status);
		done += target - &u_buff[done]; 
		if( status == U_BUFFER_OVERFLOW_ERROR) {
			u_buff.resize(((u_buff.size()+1))*2);
			D(cout << "Need to resize to: " << u_buff.size() << endl;)
		} else if(! U_SUCCESS(status)) {
			throw unicode_wrap::unicode_error(u_errorName(status));
		}
	} while(! U_SUCCESS(status));	
	u_buff.resize(done);


	status = U_ZERO_ERROR;
	vector<UChar> folded_str(u_buff.size() * 1.2);
	while(true) {
		int32_t len = u_strFoldCase(&folded_str[0], folded_str.size(), &u_buff[0], u_buff.size(), U_FOLD_CASE_DEFAULT, &status);
		if (  (unsigned) len > folded_str.size() && status == U_BUFFER_OVERFLOW_ERROR )
				folded_str.resize(len);	
		else if( U_SUCCESS(status) ) {
			folded_str.resize(len);
			break;
		} else {
			throw std::runtime_error(fs("u_strFoldCase error:" << u_errorName(status)));
		}	
	}

	//const UChar* usource = &u_buff[0];
	//const UChar* usourcelimit = &u_buff[done];

	const UChar* usource = &folded_str[0];
	const UChar* usourcelimit = &folded_str[0] + folded_str.size();
	char* otarget = 0;
	char* otargetlimit = 0;
	size_t total=0;
	do {
		status = U_ZERO_ERROR;
		D(cout << "usource: "<< (void*) usource << endl;)
		D(cout << "usourcelimit: "<<(void*) usourcelimit << endl;)
		otarget = &o_buff[total];
		otargetlimit = &o_buff[o_buff.size()];
		/********/
		//cout << "ucnv_fromUnicode(" << (void*) otarget <<","<<(void*)otargetlimit<<","<<(void*)usource <<","<<(void*)usourcelimit<<");" << endl;
		ucnv_fromUnicode(conv, &otarget, otargetlimit, &usource, usourcelimit, NULL, TRUE, &status);
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
			D(cout << "Need to resize to: " << o_buff.size() << endl;)
			//cout << "resize: " << o_buff.size() << endl;
		} else if(! U_SUCCESS(status)) {
			throw unicode_wrap::unicode_error(u_errorName(status));
		}

	} while(status  == U_BUFFER_OVERFLOW_ERROR);	
	out.assign(&o_buff[0],total);
	return out;
}

}; // end namespace features

#ifdef EXPORT_PYTHON_INTERFACE
#include <boost/python.hpp>
using namespace boost::python;
using namespace features;

BOOST_PYTHON_MODULE_INIT(case_folder)
{
	class_<Case_folder>("Case_folder")
		.def("fold_case_utf8", &Case_folder::fold_case_utf8)
	;	
}
#endif
