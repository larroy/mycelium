/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup LexicalAnalisys 
 * @brief parsing of HTML documents
 *
 * @{
 */
#pragma once
#include <string>
#include <map>

class Entity_handler {
public:
	Entity_handler();

	// Return utf8 representation of the given character entity
	std::string char_entity(const std::string&);
	std::string replace_all_entities(const std::string&);
	std::string replace_char_entities(const std::string&);
	std::string replace_dec_numchar_ref(const std::string&);
	std::string replace_hex_numchar_ref(const std::string&);
	std::string dec_numchar_ref(const std::string&);
	std::string hex_numchar_ref(const std::string&);

	typedef std::map<std::string,std::string> charent_t;
	charent_t charent;
};

/** @} */
