/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup index
 * @{
 * @brief utility for indexing local documents
 * @todo index other document types
 */
#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdlib>
#include <sstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <memory>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/propertyconfigurator.h>

#include "Doc.hh"
#include "Url.hh"
#include "utils.hh"
#include "gzstream.hh"

using namespace std;
namespace bfs = boost::filesystem;

namespace {

log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("mycelium"));

const boost::regex ext_re("(.+)\\.(\\w{3})$",boost::regex_constants::perl);

int PARALLELISM = 2;
int nchilds = 0;

string mongo_server = "localhost";
string mongodb_namespace = "mycelium.crawl";
mongo::DBClientConnection mongodb_conn;

/// filter out ascii control chars from @param[in] in to @param[out] out
void filter_ascii_control(const std::string& in, std::string& out);

/// signal handler for child processes
void chld_reaper(int sig);

/// index PDF files, uses pdftotext
void index_pdf(const bfs::path& path);



void filter_ascii_control(const std::string& in, std::string& out)
{
    out.reserve(in.size());
    for (auto i = in.begin(); i != in.end(); ++i) {
        if( *i <= 0x9 || (*i >= 0xE && *i <= 0x1F) || *i == 0x7F)
            continue;
        out.push_back(*i);
    }
}

void chld_reaper(int sig)
{
    int status=0;
    pid_t pid = wait(&status);
    if ( pid < 0 )
        utils::err_sys("wait error");

    int exi = WEXITSTATUS(status);
    if ( exi )
        cerr << status << " returned exited with nonzero status" << endl;
    --nchilds;
}


void index_pdf(const bfs::path& path)
{
    string p = path.string();
    cout << p << endl;

    boost::shared_ptr<char> canonical_path(canonicalize_file_name(p.c_str()), free);
    string url_str("file://");
    url_str += canonical_path.get();

    Url url(url_str);
    cout << url.get() << endl;
    Doc doc;
    doc.url = url;

    ostringstream contents;

    int res = utils::stdout2stream_system_exec(contents, "pdftotext", "-enc", "UTF-8", canonical_path.get(), "-", (char*)NULL);

    if( WIFEXITED(res) && WEXITSTATUS(res) == EXIT_SUCCESS ) {
        string cnt_str = contents.str();
        doc.content.reserve(cnt_str.size());
        filter_ascii_control(cnt_str, doc.content); 
        doc.flags.set(Doc::FLAG_UTF8_OK);
        doc.http_code = 200;
        doc.save(mongodb_conn, mongodb_namespace);

    } else {
        doc.flags.reset(Doc::FLAG_UTF8_OK);
        // FIXME: a little hack :/
        doc.http_code = 415;
        doc.save(mongodb_conn, mongodb_namespace);

    }
}


void index(const bfs::path& path, bool recur = true)
{
    if (! bfs::exists(path))
        return;
    if (bfs::is_directory(path)) {
        if (recur) {
            for (bfs::directory_iterator i (path), end; i != end; ++i)
                index(i->path());
        }
    } else if(bfs::is_regular(path)) {
        boost::smatch ext_m;
        // todo, use .extension() from bfs::path
        if (boost::regex_match(path.string(), ext_m, ext_re)) {
            string ext = ext_m[2].str();
            boost::algorithm::to_lower(ext);
//#define DEBUG
#ifndef DEBUG
            while(nchilds >= PARALLELISM)
                pause();

            pid_t child = fork();
            if(child) {
                ++nchilds;
            } else {
                try {
                    if (ext == "pdf")
                        index_pdf(path);
                    exit(EXIT_SUCCESS);
                } catch(...) {
                    cerr << "Unhandled exception in pdf indexing: " << path.string() << endl;
                    exit(EXIT_FAILURE);
                }
            }
#else
            if (ext == "pdf")
                index_pdf(path);
#endif
        }
    } else {
        cout << path.string() << " not a file or directory" << endl;
    }
}

void install_sig_handlers()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = chld_reaper;
    act.sa_flags = SA_NOCLDSTOP;
    int res = sigaction(SIGCHLD, &act, NULL);
    if (res < 0)
        utils::err_sys("sigaction error");
}

void init_mongo()
{
    const char* res = 0;
    if ((res = getenv("CRAWLER_DB_HOST")))
        mongo_server.assign(res);

    if ((res = getenv("CRAWLER_DB_NAMESPACE")))
        mongodb_namespace.assign(res);

    try {
        mongodb_conn.connect(mongo_server);
    } catch(mongo::UserException& e) {
        LOG4CXX_ERROR(logger, fs("Error connecting to mongodb server: " << mongo_server));
        exit(EXIT_FAILURE);
    }
}


} // end anon ns


int main(int argc, char *argv[])
try {
    if (argc == 2) {
        bfs::path p = argv[1];
        PARALLELISM = sysconf(_SC_NPROCESSORS_ONLN);
        cout << "Using: " << PARALLELISM << " cpus." << endl;


        install_sig_handlers();
        init_mongo();
        index(p);
    } else {
        cerr << "usage:\n" << argv[0] << " dir" << endl;
        exit(EXIT_FAILURE);
    }
} catch (std::exception& e) {
    cerr << "unhandled exception in main: " << e.what() << endl;
    exit(EXIT_FAILURE);
} catch (...) {
    cerr << "unhandled exception in main" << endl;
    exit(EXIT_FAILURE);
}
/** @} */
