/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup crawler
 * @brief utilities for robots.txt files
 *
 * Detailed information about robots.txt format: norobots-rfc.txt
 *
 * @{
 */

#ifndef    robots_hh
#define    robots_hh
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <map>
#include <boost/regex.hpp>
#include "Path.hh"

#undef yyFlexLexer
#include <FlexLexer.h>

namespace robots {
    typedef enum robots_state_t {
        EMPTY,
        PRESENT,
        NOT_AVAILABLE,
        EPARSE,
    } robots_state_t;

    /**
     * @brief Tests if path is allowed by robots.txt
     *
     * Parses a robots.txt file and sets up internal machinery for easy testing if an url is allowed.
     * To evaluate if access to a URL is allowed.  A robot must attempt to match
     * the paths in Allow and Disallow lines against the URL, in the order they
     * occur in the record. The first match found is used.  If no match is found,
     * the default assumption is that the URL is allowed.
     *
     * @todo Crawl-delay: seems to be specified by some sites
     */

    class Robots : public yyFlexLexer {
    public:
        Robots();

        /// Construct directly from an istream containing robots.txt information
        Robots(std::istream* in);

        /**
         * @brief parse supplied robots.txt content
         * @return 0 if there was success parsing the file or -1 if the syntax is invalid, nonfatal errors would be stored on errors but the parser will keep going and might return 0
         */
        virtual int yylex();

        /**
         * @return true if a path is allowed for the supplied user_agent
         * @param user_agent User agent to match rule against. "*" will match the default or * user agent
         *
         * A path matches if there's a rule that is a prefix of this path
         *
         * <PRE>
         *   This table illustrates some examples:
         *
         *     Record Path        URL path         Matches
         *     /tmp               /tmp               yes
         *     /tmp               /tmp.html          yes
         *     /tmp               /tmp/a.html        yes
         *     /tmp/              /tmp               no
         *     /tmp/              /tmp/              yes
         *     /tmp/              /tmp/a.html        yes
         *
         *     /a%3cd.html        /a%3cd.html        yes
         *     /a%3Cd.html        /a%3cd.html        yes
         *     /a%3cd.html        /a%3Cd.html        yes
         *     /a%3Cd.html        /a%3Cd.html        yes
         *
         *     /a%2fb.html        /a%2fb.html        yes
         *     /a%2fb.html        /a/b.html          no
         *     /a/b.html          /a%2fb.html        no
         *     /a/b.html          /a/b.html          yes
         *
         *     /%7ejoe/index.html /~joe/index.html   yes
         *     /~joe/index.html   /%7Ejoe/index.html yes
         * </PRE>
         */
        bool path_allowed(const std::string& user_agent, const std::string& path) const;

        bool valid;
        void clear() {
            current.clear();
            uas_rules_all.clear();
            state = START;
        }
        /// parsing errors
        std::string    errors;

    protected:
        // currently unused
        void reset(std::istream* in);

        /// Types of Rules
        typedef enum rule_type_t {
            DISALLOW,
            ALLOW,
            CRAWL_DELAY
        } rule_type_t;

        /// Rule for a user agent, @sa rule_type_t
        struct Rule {
            Rule() : type(DISALLOW), str() {}
            Rule(rule_type_t type, const std::string& s) : type(type), str(s) {}
            rule_type_t type;

            /// Text of the rule, for DISALLOW / ALLOW is a path
            std::string str;
        };

    private:
        typedef enum state_t {
            START,
            READING_UAS,
            RULES,
            //UNMATCH,
            STATE_EOF,
        } state_t;

        // state changers
        void start() { state=START; }
        void reading_uas();
        void rules();
        //void unmatch() {state=UNMATCH;}
        void eof();

        /**
         * Agregation of several "User-agent:" lines and it's corresponding rules
         */
        struct Uas_rules {
            Uas_rules() :
                ua(),
                rules()
            {}
            void clear() { ua.clear(); rules.clear(); }
            bool empty() { return ua.empty(); }
            std::vector<std::string> ua;
            std::vector<Rule> rules;
        };


        state_t state;
        Uas_rules current;
        std::vector<Uas_rules> uas_rules_all;


        /// Sometimes we get html, this checks for html tags to avoid further parsing and wasting time
        static boost::regex sgml_tag;

        /// Dumps parsed info, mainly for debug
        friend std::ostream& operator<<(std::ostream& os, const Robots& r);
    };

    /**
     * @brief Extends Robots so we have state
     */
    struct Robots_entry : public Robots {
        Robots_entry(std::istream* in) :
            Robots(in),
            state(EMPTY)
        {}

        Robots_entry() :
            Robots(),
            state(EMPTY)
        {}

        Robots_entry(robots_state_t state) :
            Robots(),
            state(state)
        {}

        bool tried_but_failed() const
        {
            return (state == NOT_AVAILABLE || state == EPARSE);
        }

        robots_state_t state;
    };
};
#endif
/** @} */
