/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Path.hh"
#include "utils.hh"
using namespace std;

void Path::merge(const Path& p)
{
    D(cout << "Path::merge " << this->get() << " with " << p.get() << endl;);
    if( p.flags.test(SLASH_BEGIN) )
        (*this) = p;
    else {
        if( ! flags.test(SLASH_END) && ! segmt.empty() && ! p.empty() )
            segmt.pop_back(); // file element at the end
        for(list<string>::const_iterator i = p.segmt.begin(); i != p.segmt.end(); ++i) {
            if( *i == "." ) {
                flags.set(SLASH_END,true);
            } else if( *i == ".." ) {
                if( ! segmt.empty() ) {
                    segmt.pop_back();
                    flags.set(SLASH_END,true);
                }
            } else {
                segmt.push_back(*i);
                flags.set(SLASH_END,false);
            }
        }
        if( p.flags.test(SLASH_END) )
            flags.set(SLASH_END,true);
    //  Fix: When you merge with .. or . the path should end in slash
    //    else
    //        flags.set(SLASH_END,false);
    }
    D(cout << "result: " << this->get() << endl;)
}

void Path::normalize()
{
//    if( flags.test(SLASH_BEGIN) && ! segmt.empty() && *segmt.begin() == ".." )
//        segmt.pop_front();

    list<string>::iterator i = segmt.begin();
    list<string>::iterator j = segmt.begin();
    list<string>::iterator k = segmt.begin();
    ++i;
    while( i != segmt.end() ) {
        if( *i == ".." && j != i && *j != ".." && *j != "." ) {
            segmt.erase(j);
            i=segmt.erase(i);


            if( i == segmt.end() )
                flags.set(SLASH_END,true);

        } else if ( *i == "." ) {
            i=segmt.erase(i);
            if( i == segmt.end() )
                flags.set(SLASH_END,true);
        } else {
            ++j;
            ++i;
        }
        j=k=segmt.begin();
        while(k != i)
            j=k++;

    }
}

size_t Path::size() const
{
    size_t size=0;
    if( empty() )
        return 0;
    else if( segmt.size() > 0 ) {
        if(flags.test(SLASH_BEGIN))
            ++size;

        for(list<string>::const_iterator i = segmt.begin(); i != segmt.end(); ++i)
            size += (*i).size();

        if(flags.test(SLASH_END))
            ++size;

        size += segmt.size()-1; // there will be '/' between elements

        return size;
    } else if (flags.test(SLASH_BEGIN) || flags.test(SLASH_END)){
        return 1;
    } else {
        return 0;
    }
}

string Path::get() const
{
    if( empty() )
        return "";
    string result;
    result.reserve(this->size());
    if( segmt.size() > 0 ) {
        if(flags.test(SLASH_BEGIN))
            result += '/';

        bool iterated=false;
        for(list<string>::const_iterator j = segmt.begin(); j != segmt.end(); ++j) {
            result += (*j);
            result += '/';
            iterated=true;
        }
        if(iterated && ! result.empty() )
            result = result.substr(0,result.size()-1); // remove final '/', since there's no + operator in list iterators

        if(flags.test(SLASH_END))
            result += '/';

        if(result == "//") // just to make sure
            result = "/";
    } else if (flags.test(SLASH_BEGIN) || flags.test(SLASH_END)){
        result = "/";
    } else
        result.clear();

    return result;
}

void Path::assign(const string& s)
{
    clear();
    if( s.empty() )
        return;
    list<string>::size_type depth=0;
    string::const_iterator begin=s.begin();
    string::const_iterator end = s.begin();
    if( *end == '/'  ) {
        flags.set(SLASH_BEGIN,true);
        ++end;
        begin = end;
    }

    while(1) {
        if( end == s.end() ) {
            if( end != begin ) {
                string str(begin,end); // [begin,end)
                segmt.push_back(str);
                ++depth;
                begin = end;
            }
            break;
        } else if( *end == '/'  ) {
            if( end != begin ) {
                string str(begin,end); // [begin,end)
                segmt.push_back(str);
                ++depth;
                begin = end;
            } else {
                ++end;
                begin = end;
            }
        } else {
            ++end;
        }
        // safety check
        if( depth >= segmt.max_size() ) {
            clog << "path depth >= MAXDEPTH" << endl;
            return;
        }
    }
    if( s.at(s.size()-1) == '/' )
        flags.set(SLASH_END,true);
}


bool Path::updir()
{
    if( ! segmt.empty() ) {
        if( ! flags[SLASH_END] ) // a file
            segmt.pop_back();
        segmt.pop_back();
        flags[SLASH_END] = true;
        return true;
    } else {
        return false;
    }
}




