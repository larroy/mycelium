/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Robots.hh"
#include "Url.hh"
using namespace std;

namespace robots {

boost::regex Robots::sgml_tag("<[^>]+>",boost::regex_constants::perl);

Robots::Robots() :
    yyFlexLexer(0,0),
    valid(false),
    errors(),
    state(START),
    current(),
    uas_rules_all()
    //crawl_delay(0)
{
}

Robots::Robots(std::istream* in) :
    yyFlexLexer(in,0),
    valid(false),
    state(START),
    current(),
    uas_rules_all()
    //crawl_delay(0)
{
}

void Robots::reset(std::istream* in)
{
    switch_streams(in,0);
    clear();
//    BEGIN(0);
//    BEGIN(INITIAL);
}


bool Robots::path_allowed(const std::string& user_agent, const std::string& p) const
{
    string path = Url::unescape_not(Url::escape(p,Url_util::URL_CHAR_PATH),Url_util::URL_CHAR_PATH);
    for(vector<Uas_rules>::const_iterator i = uas_rules_all.begin(); i != uas_rules_all.end(); ++i) {
        for(vector<string>::const_iterator u = i->ua.begin(); u != i->ua.end(); ++u) {
            if( u->compare(0,u->size(),user_agent) == 0 || *u == "*" ) {
                //cout << "ua: " << *u << endl;
                for(vector<Rule>::const_iterator r = i->rules.begin(); r != i->rules.end(); ++r) {
                    //cout << static_cast<int>(r->type) << " " << r->str << endl;
                    if(r->str.compare(0,r->str.size(),path) == 0) {
                        if(r->type == ALLOW)
                            return true;
                        else
                            return false;
                    }
                }
                return true;
            }
        }
    }
    return true;
}

void Robots::rules()
{
    state = RULES;
}

void Robots::reading_uas()
{
    if (state == RULES) {
        uas_rules_all.push_back(current);
        current.clear();
        valid = true;
    }
    state = READING_UAS;
}

void Robots::eof()
{
    if ( ! current.empty() ) {
        uas_rules_all.push_back(current);
        current.clear();
        valid = true;
    }
    state = STATE_EOF;
}

ostream& operator<<(ostream& os, const Robots& robots)
{
    for (vector<Robots::Uas_rules>::const_iterator i = robots.uas_rules_all.begin(); i != robots.uas_rules_all.end(); ++i) {
        for (vector<string>::const_iterator u = i->ua.begin(); u != i->ua.end(); ++u) {
            os << "ua: " << *u << endl;
        }
        for (vector<Robots::Rule>::const_iterator r = i->rules.begin(); r != i->rules.end(); ++r) {
            os << static_cast<int>(r->type) << " " << r->str << endl;
        }
    }
    return os;
}

}; // namespace robots


#ifdef MAIN_TEST
int main(int argc, char* argv[])
{
    using namespace robots;
    Robots robots(&cin);
    robots.yylex();
    cout << robots << endl;
    cout << "*****************" << endl;
    if(argc<3)
        exit(EXIT_SUCCESS);
    string ua = argv[1];
    for(int i = 2; i < argc; ++i) {
        string path = argv[i];
        if( robots.path_allowed(ua,path) )
            cout << "allow path: " << path << endl;
        else
            cout << "disallow path: " << path << endl;
    }

}
#endif

