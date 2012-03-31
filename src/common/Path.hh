/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */


/**
 * @addtogroup Url
 * @brief Handling of RFC 3986 urls
 *
 * @{
 */


/**
 * @class Path Path.hh
 * @author piotr
 * @brief a class for paths
 *
 */

#ifndef Path_hh
#define Path_hh 1
#include <list>
#include <ostream>
#include <iostream>
#include <string>
#include <bitset>

#define MAXDEPTH    256

class Path {
    public:
        Path(const std::string& s):segmt(),flags() {
            assign(s);
        }
        Path():segmt(),flags() {
        }

        void merge(const Path& p);
        std::string get() const;
        void assign(const std::string& s);

        Path& operator=(const std::string& s) {
            assign(s);
            return *this;
        }
        Path& operator+=(const Path& p) {
            this->merge(p);
            return *this;
        }

        /**
         * Take care of /../ /./ sequences
         */
        void normalize();
        size_t size() const;

        /**
         * sets this path as absolute
         */
        void absolute(bool a) {
            flags[SLASH_BEGIN] = a;
        }

        void set_dir() {
            flags[SLASH_END] = true;
        }

        /**
         * returns true if the path is absolute, false otherwise
         */
        bool absolute() const {
            return flags.test(SLASH_BEGIN);
        }

        /**
         * goes one directory up if possible, else noop
         */
        bool updir();

        size_t depth()  {
            return segmt.size();
        }

        /**
         * Make this path empty
         */
        void clear() {
            segmt.clear();
            flags.reset(SLASH_END);
            flags.reset(SLASH_BEGIN);
        }

        bool empty() const {
            if( ! flags.test(SLASH_END) && segmt.empty() && ! flags.test(SLASH_BEGIN) )
                return true;
            else
                return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const Path& p) {
            if( ! p.empty() )
                os << p.get();

            return os;
        }

    friend class Url_lexer;
    friend class Url;

    protected:
        std::list<std::string> segmt;
        static size_t const FLAGSSIZE = 2;
        enum flags {
            SLASH_END,
            SLASH_BEGIN
        };
        std::bitset<FLAGSSIZE> flags;

    private:
};

#endif
/** @} */

