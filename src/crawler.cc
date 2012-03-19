/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup crawler
 * @{
 * @brief utility for retrieving documents via http
 * @details Uses libevent and asynchronous IO
 * Running: start the program and pipe "http://" urls into crawler.fifo
 * The urls are classified with Url_classifier
 */
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>
#include <curl/curl.h>
#include <event.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <signal.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <boost/tokenizer.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/propertyconfigurator.h>

#include "Doc.hh"
#include "Url_classifier.hh"
#include "Robots.hh"
#include "utils.hh"     // err_sys
//#include "Doc_store.hh"
#include "timer.hh"

/// Number of concurrent retrievals to run
#define NUMHANDLES 20
/// Bytes/s
#define LOW_SPEED_LIMIT 1024
/// Seconds for the conn to be below LOW_SPEED_LIMIT
#define LOW_SPEED_TIME 60
#define CONNECTTIMEOUT 60
#define MAXREDIRS 5

/// default size for buffers
#define BSIZE    8192

#define my_curl_easy_setopt(HANDLE, OPTION, PARAM) do{\
        if ( curl_easy_setopt((HANDLE), (OPTION), (PARAM)) != CURLE_OK  )\
            throw std::runtime_error("curl_easy_setopt failed");\
    } while(0);

using namespace std;


namespace {

struct GlobalInfo;
struct SockInfo;

bool quit_program = false;
int sigint_cnt = 0;

void sigint_handler(int);
log4cxx::LoggerPtr crawlog(log4cxx::Logger::getLogger("crawlog"));

/// CURLMOPT_SOCKETFUNCTION
int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);

/// CURLMOPT_TIMERFUNCTION
/// Update the event timer after curl_multi library calls
int multi_timer_cb(CURLM *multi, long timeout_ms, GlobalInfo *g);

/// ev callback for events in the fifo for URLs
int init_fifo (GlobalInfo *g);

/// CURLMOPT_TIMERFUNCTION callback
void timer_cb(int fd, short kind, void *userp);

/// ev_timer callback to periodically reschedule to dequeue work and print some stats
void scheduler_cb(int fd, short kind, void *userp);

void mcode_or_die(const char* where, CURLMcode code);

/// curl callback for headers
size_t header_write_cb (void* ptr, size_t size, size_t nmemb, void* data);

/// curl callback for content
size_t content_write_cb (void* ptr, size_t size, size_t nmemb, void* data);

/// progress callback
int progress_cb (void* p, double dltotal, double dlnow, double ult, double uln);

/// callback for interactive / console control
void on_read_interactive_cb(int fd, short ev, void* arg);

/// This gets called whenever data is received from the fifo
void fifo_cb(int fd, short event, void* arg);

/// Initialize a new SockInfo structure (curl multi interface)
void addsock(curl_socket_t, CURL*, int action, GlobalInfo*);

void remsock(SockInfo*);

/// Assign information to a SockInfo structure and program the event in ev (curl multi interface)
void setsock(SockInfo*, curl_socket_t, CURL*, int action, GlobalInfo*);

/// Called by libevent when we get action on a multi socket 
void multi_cb (int fd, short kind, void *userp);




/// Information associated with a specific curl easy handle
/// We have one per connection
/// It works as a state machine
class EasyHandle {
private:
    EasyHandle(const EasyHandle&);
    void operator=(const EasyHandle&);

public:
    static std::string state_to_string(int state);

    EasyHandle(GlobalInfo* g, size_t id):
        id(id),
        easy(0),
        dl_bytes(0),
        dl_kBs(0),
        prev_dl_time(utils::timer::current()),
        last_resched_time(utils::timer::current()),
        prev_dl_cnt(0),
        doc(),
        content_os(),
        headers_os(),
        robots_entry(),
        global(g),
        curl_error(),
        state(EasyHandle::IDLE),
        headers()
    {
        easy = curl_easy_init();
        if( ! easy )
            utils::err_sys("curl_easy_init failed");
        if( ! g )
            utils::err_sys("GlobalInfo is NULL");
    }

    ~EasyHandle()
    {
        curl_easy_cleanup(easy);
        if (headers) {
            curl_slist_free_all(headers);
            headers = 0;
        }
    }


    void reschedule();
    void done(CURLcode result);

    size_t   id;
    CURL     *easy;
    uint64_t dl_bytes;
    double   dl_kBs;
    utils::timer prev_dl_time;
    utils::timer last_resched_time;
    double   prev_dl_cnt;

    boost::scoped_ptr<Doc> doc;

    std::ostringstream content_os;
    std::ostringstream headers_os;

    boost::scoped_ptr<robots::Robots_entry> robots_entry;

    GlobalInfo* global;
    char curl_error[CURL_ERROR_SIZE];

    // local fds where retrieved data is stored

    typedef enum state_t {
        IDLE,
        ROBOTS,
        HEAD,
        CONTENT,
        FINISHED
    } state_t;
    state_t state;

    curl_slist *headers;

private:
    void get_content(const Url& url);
};


/// Global information, common to all connections
class GlobalInfo {
private:
    GlobalInfo(const GlobalInfo&);
    void operator=(const GlobalInfo&);

public:
    GlobalInfo() :
        mongodb_conn(),
        fifo_buff(),
        interactive_buff(),
        //multi(curl_multi_init())
        dl_bytes(0),
        dl_bytes_prev(0),
        dl_prev_sample(utils::timer::current()),
        prev_running(0),
        still_running(0),
        classifier(NUMHANDLES),
        EasyHandles(),
        user_agent("pn-0.2-BETA")
    {
        string mongo_server = "localhost";
        try {
            const char* res = 0;
            if ((res = getenv("DB_HOST")))
                mongo_server.assign(res);
            mongodb_conn.connect(mongo_server);
        } catch(mongo::UserException& e) {
            LOG4CXX_ERROR(crawlog, fs("Error connecting to mongodb server: " << mongo_server));
            exit(EXIT_FAILURE);
        }

        memset(&fifo_event, 0, sizeof(struct event));
        memset(&interactive_event, 0, sizeof(struct event));
        memset(&timer_event, 0, sizeof(struct event));
        multi = curl_multi_init();
        if(multi==NULL)
            throw std::runtime_error("Couldn't initialize multi interface");

        /* setup the generic multi interface options we want */
        curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
        curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, this);
        curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
        curl_multi_setopt(multi, CURLMOPT_TIMERDATA, this);

        EasyHandles.reserve(NUMHANDLES);
        for(size_t i=0; i<NUMHANDLES; ++i)
            EasyHandles.push_back(new EasyHandle(this,i));

        init_fifo(this);
        evtimer_set(&timer_event, timer_cb, this);
        evtimer_set(&scheduler_event, scheduler_cb, this);

        long timeout_ms = 5000;
        struct timeval timeout;
        timeout.tv_sec = timeout_ms/1000;
        timeout.tv_usec = (timeout_ms%1000)*1000;
        evtimer_add(&scheduler_event, &timeout);


        event_set (&interactive_event, STDIN_FILENO, EV_READ | EV_PERSIST, on_read_interactive_cb, this);
        event_add (&interactive_event, NULL);

    }

    ~GlobalInfo()
    {
        for(size_t i=0; i<NUMHANDLES; ++i)
            delete EasyHandles[i];

        curl_multi_cleanup(multi);
    }
    void process_fifo_buff(bool flush=false);
    void check_run_count();
    /// call reschedule on IDLE easy handles
    void reschedule();


    mongo::DBClientConnection mongodb_conn;
    std::string    fifo_buff;
    struct event fifo_event;

    struct event interactive_event;
    std::string interactive_buff;
    void interactive_process(bool flush=false);
    void interactive_cmd(const std::string& cmd);
    void status();

    struct event timer_event;
    struct event scheduler_event;

    CURLM *multi;
    uint64_t    dl_bytes;
    uint64_t    dl_bytes_prev;
    utils::timer dl_prev_sample;
    int prev_running;
    int still_running;


    Url_classifier classifier;

    // easy handles
    std::vector<EasyHandle*> EasyHandles;

    std::string user_agent;
    //int rate_limit;

};


/**
 * @brief Information associated with a specific socket
 *
 * Used to add the curl_socket_t filedescriptors to libevent
 * @sa sock_cb
 */
struct SockInfo {
    SockInfo() :
        sockfd(),
        easy(),
        action(),
        timeout(),
        evset(),
        global()
    {
        memset(&ev, 0, sizeof(event));
    }
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct event ev;
    int evset;
    GlobalInfo *global;
};

void sigint_handler(int)
{
    quit_program = true;
    ++sigint_cnt;
    if (sigint_cnt>3)
        exit(EXIT_FAILURE);
}


int sock_cb(CURL * e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    GlobalInfo *g = static_cast<GlobalInfo*>(cbp);
    SockInfo *sockInfo = static_cast<SockInfo*>(sockp);
    if (what == CURL_POLL_REMOVE) {
        remsock(sockInfo);
    } else {
        if (! sockInfo) {
            addsock(s, e, what, g);
        } else {
            setsock(sockInfo, s, e, what, g);
        }
    }
    return 0;
}


int multi_timer_cb(CURLM *multi, long timeout_ms, GlobalInfo *g)
{
  struct timeval timeout;
  (void) multi;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  evtimer_add(&g->timer_event, &timeout);
  return 0;
}


/// Create a named pipe and tell libevent to monitor it
int init_fifo (GlobalInfo* g)
{
    struct stat st;
    static const char* fifo = "crawler.fifo";
    int sockfd;
    if (lstat(fifo, &st) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            errno = EEXIST;
            perror("lstat");
            exit(1);
        }
    }
    unlink (fifo);
    if (mkfifo(fifo, 0600) == -1) {
        perror("mkfifo");
        exit (1);
    }
    sockfd = open (fifo, O_RDWR | O_NONBLOCK, 0);
    if (sockfd == -1) {
        perror("open");
        exit(1);
    }
    event_set(&g->fifo_event, sockfd, EV_READ | EV_PERSIST, fifo_cb, g);
    event_add(&g->fifo_event, NULL);
    return (0);
}


/// Called by libevent when our timeout expires
void timer_cb (int fd, short kind, void *userp)
{
    GlobalInfo *g = (GlobalInfo *) userp;
    CURLMcode rc;
    (void) fd;
    (void) kind;
    //cout << "timer cb!" << endl;
    do {
        rc = curl_multi_socket_action (g->multi,
            CURL_SOCKET_TIMEOUT, 0, &g->still_running);
    } while (rc == CURLM_CALL_MULTI_PERFORM);
    mcode_or_die("timer_cb: curl_multi_socket_action", rc);

    LOG4CXX_DEBUG(crawlog,fs("timer_cb: still_running: " << g->still_running));
    g->check_run_count ();
}


void scheduler_cb(int fd, short kind, void *userp)
{
    GlobalInfo *g = (GlobalInfo *)userp;

    g->reschedule();
    utils::timer delta = utils::timer::current() - g->dl_prev_sample;
    double kBs = (static_cast<double>(g->dl_bytes -  g->dl_bytes_prev) / delta.usec()) * 1000;
    g->dl_bytes_prev = g->dl_bytes;
    g->dl_prev_sample = utils::timer::current();

    cout << "down: " << utils::fmt_bytes(g->dl_bytes) << " " << utils::fmt_kbytes_s(kBs) << endl;
    if (quit_program)
        //throw runtime_error("quit_program");
        event_loopbreak();

    long timeout_ms = 5000;
    struct timeval timeout;
    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms%1000)*1000;
    evtimer_add(&g->scheduler_event, &timeout);
}


/// Die if we get a bad CURLMcode somewhere
void mcode_or_die(const char *where, CURLMcode code)
{
    if (CURLM_OK != code) {
        const char *s;
        switch (code) {
            case CURLM_CALL_MULTI_PERFORM:
                s = "CURLM_CALL_MULTI_PERFORM";
                break;

            case CURLM_OK:
                s = "CURLM_OK";
                break;

            case CURLM_BAD_HANDLE:
                s = "CURLM_BAD_HANDLE";
                break;

            case CURLM_BAD_EASY_HANDLE:
                s = "CURLM_BAD_EasyHandle";
                break;

            case CURLM_OUT_OF_MEMORY:
                s = "CURLM_OUT_OF_MEMORY";
                break;

            case CURLM_INTERNAL_ERROR:
                s = "CURLM_INTERNAL_ERROR";
                break;

            case CURLM_UNKNOWN_OPTION:
                s = "CURLM_UNKNOWN_OPTION";
                break;

            case CURLM_LAST:
                s = "CURLM_LAST";
                break;

            default:
                s = "CURLM_unknown";
                break;

            case CURLM_BAD_SOCKET:
                s = "CURLM_BAD_SOCKET";
                LOG4CXX_WARN(crawlog,fs("curl_multi: " << where << " returns " << s));
                /* ignore this error */
                return;
        }
        LOG4CXX_ERROR(crawlog,fs("curl_multi: " << where << " returns " << s));
        exit (code);
    }
}

/// CURLOPT_HEADERFUNCTION
size_t header_write_cb (void* buff, size_t size, size_t nmemb, void* data)
{
    EasyHandle* handle = static_cast<EasyHandle*>(data);
    if(! handle)
        throw runtime_error("null data on write_cb");

    size_t realsize = size * nmemb;
    handle->dl_bytes += realsize;
    handle->global->dl_bytes += realsize;
    handle->headers_os.write(static_cast<char*>(buff), realsize);
    return realsize;
}

/* CURLOPT_WRITEFUNCTION */
size_t content_write_cb (void* buff, size_t size, size_t nmemb, void* data)
{
    EasyHandle* handle = static_cast<EasyHandle*>(data);
    if( ! handle )
        throw runtime_error("null WRITEDATA on write_cb");

    size_t realsize = size * nmemb;
    handle->dl_bytes += realsize;
    handle->global->dl_bytes += realsize;
    handle->content_os.write(static_cast<char*>(buff), realsize);
    return realsize;
}


/// CURLOPT_PROGRESSFUNCTION
int progress_cb (void *p, double dltotal, double dlnow, double ult, double uln)
{
    EasyHandle* handle = static_cast<EasyHandle*>(p);
    double dl_diff = dlnow - handle->prev_dl_cnt;
    handle->prev_dl_cnt = dlnow;
    if( dl_diff < 0 )
        dl_diff = dlnow;
    utils::timer timediff = utils::timer::current() - handle->prev_dl_time;
    handle->dl_kBs = (dl_diff / static_cast<double>(timediff.usec())) * 1000;

    //cout << "id: " << handle->id << " " << handle->dl_kBs << endl;
    return 0;
}


/// Callback for processing interactive commands
void on_read_interactive_cb(int fd, short ev, void *arg)
{
    //In* in = (In*) arg;
    GlobalInfo *g = (GlobalInfo *)arg;
    if( ! g )
        utils::err_sys("NULL GlobalInfo pointer on_read_interactive_cb: " __FILE__ ":__LINE__");
    char b[16];
    ssize_t cnt;
    cnt = read(fd, b, 16);
    cout << "on_read_interactive_cb: read: " << cnt << endl;
    if( cnt == 0 ) {
        LOG4CXX_ERROR(crawlog,fs("on_read_interactive_cb: EOF fd: " << fd));
        event_del(&g->interactive_event);
        g->interactive_process(true);
    } else if( cnt < 0) {
        LOG4CXX_ERROR(crawlog,fs("on_read_interactive_cb: read error: " << strerror(errno) << " fd: " << fd));
        event_del(&g->interactive_event);
    } else {
        g->interactive_buff.append(b,cnt);
        g->interactive_process(false);
    }
}


void fifo_cb(int fd, short event, void *arg)
{
    GlobalInfo *g = (GlobalInfo *)arg;
    char b[BSIZE];
    ssize_t cnt;
    cnt = read(fd, b, BSIZE);
    //cerr << "fifo read: " << cnt << endl;
    if( cnt == 0 ) {
        //new_conn
        g->process_fifo_buff(true);
    } else if (cnt < 0) {
        utils::err_sys("fifo read error");
    } else {
        g->fifo_buff.append(b, cnt);
        g->process_fifo_buff();
    }
}

void addsock(curl_socket_t s, CURL* easy, int action, GlobalInfo* g)
{
    SockInfo* p = new SockInfo();
    p->global = g;
    setsock(p, s, easy, action, g);
    curl_multi_assign (g->multi, s, p);
}


/// Clean up the SockInfo structure
void remsock(SockInfo* s)
{
    if (s && s->evset)
        event_del(&s->ev);
    delete(s);
}


void setsock (SockInfo* sockInfo, curl_socket_t s, CURL* e, int action, GlobalInfo * g)
{
    int kind = (action & CURL_POLL_IN ? EV_READ : 0)
        | (action & CURL_POLL_OUT ? EV_WRITE : 0)
        | EV_PERSIST;

    sockInfo->sockfd = s;
    sockInfo->action = action;
    sockInfo->easy = e;
    if (sockInfo->evset)
        event_del (&sockInfo->ev);
    event_set(&sockInfo->ev, sockInfo->sockfd, kind, multi_cb, g);
    sockInfo->evset = 1;
    event_add(&sockInfo->ev, NULL);
}


void multi_cb(int fd, short kind, void *userp)
{
    GlobalInfo *g = (GlobalInfo *) userp;
    CURLMcode rc;

    int action =
        (kind & EV_READ ? CURL_CSELECT_IN : 0) |
        (kind & EV_WRITE ? CURL_CSELECT_OUT : 0);

    do {
        rc = curl_multi_socket_action (g->multi, fd, action, &g->still_running);
    } while (rc == CURLM_CALL_MULTI_PERFORM);

    mcode_or_die("multi_cb: curl_multi_socket_action", rc);

    if (g->still_running <= 0) {
        if (evtimer_pending (&g->timer_event, NULL)) {
            evtimer_del (&g->timer_event);
        }
    }
    g->check_run_count ();
}




/// puts handle back to work, tries to dequeue next URL and set up a retrieval
void EasyHandle::reschedule()
{
    CURLMcode rc;
    if( state != IDLE )
        return;

    if( global->classifier.empty_top() && global->classifier.empty(id) )
        return;

    //curl_easy_cleanup(easy);
    //easy = curl_easy_init();
    curl_easy_reset(easy);

    Url url = global->classifier.peek(id);
    //LOG4CXX_DEBUG(crawlog,fs("peek("<<id<<"): " << url.get()));
    //url.normalize_host();
    url.normalize();
    if( ! robots_entry
        || url.host() != robots_entry->host
        || robots_entry->state == robots::EMPTY ) {

        LOG4CXX_INFO(crawlog,fs("handle id: " << id << " retrieving robots: " << url.host()));

        /*******/
        state = ROBOTS;
        /*******/

        content_os.str("");
        headers_os.str("");

        doc.reset(new Doc());
        doc->url.scheme("http");
        doc->url.host(url.host());
        doc->url.path("robots.txt");

        string url_string = doc->url.get();

        my_curl_easy_setopt(easy, CURLOPT_URL, url_string.c_str());

        my_curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, NULL);
        my_curl_easy_setopt(easy, CURLOPT_HEADERDATA, NULL);

        //my_curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, url_string.c_str());
        my_curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, header_write_cb);
        my_curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);
        my_curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
        //my_curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
        memset(curl_error,0,CURL_ERROR_SIZE);
        my_curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_error);
        my_curl_easy_setopt(easy, CURLOPT_PRIVATE, this);
        my_curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
        my_curl_easy_setopt(easy, CURLOPT_PROGRESSFUNCTION, progress_cb);
        my_curl_easy_setopt(easy, CURLOPT_PROGRESSDATA, this);
        my_curl_easy_setopt(easy, CURLOPT_HTTPHEADER, 0);
        // timeouts
        my_curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
        my_curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
        my_curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);


        rc = curl_multi_add_handle(global->multi, easy);
        mcode_or_die("reschedule: curl_multi_add_handle", rc);

    } else if( robots_entry->state == robots::NOT_AVAILABLE
        || robots_entry->state == robots::EPARSE ) {

        global->classifier.pop(id);

        /*******/
        state = CONTENT;
        /*******/

        doc.reset(new Doc());
        content_os.str("");
        headers_os.str("");

        get_content(url);

    } else if( robots_entry->state == robots::PRESENT
            && url.host() == robots_entry->host ) {

        while( ! global->classifier.empty(id) ) {
            url = global->classifier.peek(id);
            //LOG4CXX_DEBUG(crawlog,fs("peek("<<id<<"): " << url.get()));
            global->classifier.pop(id);

            if( robots_entry->path_allowed(global->user_agent, url.path()) ) {
                /*******/
                state = CONTENT;
                /*******/

                doc.reset(new Doc());
                content_os.str("");
                headers_os.str("");

                get_content(url);
                break;

            } else {
                // disallowed by robots.txt
                LOG4CXX_DEBUG(crawlog,fs("handle id: " << id << ", url: " << url.get() << " not allowed (robots.txt)"));
            }
        }
    } else {
        throw runtime_error("unknown state in reschedule");
    }

    // reset counters
    prev_dl_time = utils::timer::current();
    last_resched_time = utils::timer::current();
    prev_dl_cnt = 0;
    dl_kBs = 0;
}


void EasyHandle::done(CURLcode result)
{
    char *eff_url = NULL;
    doc->curl_code = static_cast<int>(result);
    doc->curl_error.assign(curl_error);
    curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &doc->http_code);
    curl_easy_getinfo(easy, CURLINFO_FILETIME, &doc->modified);
    if (headers) {
        curl_slist_free_all(headers);
        headers = 0;
    }


    timeval t;
    gettimeofday(&t,0);
    doc->crawled = t.tv_sec;

    if( result == CURLE_OK ) {
        LOG4CXX_INFO (crawlog,fs("handle id: " << id << " DONE: "
            << eff_url << " HTTP " << doc->http_code));
    } else {
        LOG4CXX_WARN(crawlog,fs("handle id: " << id << " DONE: "
            << eff_url << " HTTP " << doc->http_code
            << " curl_code: " << result << " (" << curl_error << ")"));
    }
    switch(state) {
        case IDLE:
            throw runtime_error("done called on IDLE handle");
            break;

        case ROBOTS:
            // try to parse robots contents
            if( doc->http_code == 200 && result == CURLE_OK && ! doc->content.empty() ) {
                try {
                    istringstream robots_is(content_os.str());
                    robots_entry.reset(new robots::Robots_entry(doc->url.host(), &robots_is));
                    int res = robots_entry->yylex();
                    if( res < 0 ) {
                        robots_entry->clear();
                        ////////////
                        robots_entry->state = robots::EPARSE;
                        ////////////
                    } else {
                        ////////////
                        robots_entry->state = robots::PRESENT;
                        ////////////
                    }
                } catch(...) {
                    ////////////
                    robots_entry->state = robots::EPARSE;
                    ////////////
                    robots_entry->clear();
                }
            } else {
                robots_entry.reset(new robots::Robots_entry(doc->url.host(), robots::NOT_AVAILABLE));
            }
            doc->content.clear();
            break;

        case CONTENT:
            // Content retrieval finished, we save the document 
            doc->url = eff_url;
            doc->url.normalize();
            doc->headers = headers_os.str();
            doc->content = content_os.str();
            doc->save(global->mongodb_conn);
            break;

        case HEAD:
        case FINISHED:
        default:
            LOG4CXX_DEBUG(crawlog,"Unsupported state in EasyHandle::done");
            break;
    }
    state = EasyHandle::IDLE;
    doc->url.clear();
    reschedule();
}


void EasyHandle::get_content(const Url& url)
{
    LOG4CXX_INFO(crawlog,fs("handle id: " << id << " retrieving content: " << url.host()));
    bool preexisting = doc->load_url(global->mongodb_conn, url);

    string url_string = doc->url.get();

    my_curl_easy_setopt(easy, CURLOPT_URL, url_string.c_str());

    // header
    my_curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, header_write_cb);
    my_curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);

    // content
    my_curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, content_write_cb);
    my_curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);

    //my_curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
    my_curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
    memset(curl_error,0,CURL_ERROR_SIZE);
    my_curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_error);
    my_curl_easy_setopt(easy, CURLOPT_PRIVATE, this);
    my_curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
    my_curl_easy_setopt(easy, CURLOPT_PROGRESSFUNCTION, progress_cb);
    my_curl_easy_setopt(easy, CURLOPT_PROGRESSDATA, this);
    my_curl_easy_setopt(easy, CURLOPT_FILETIME, 1L);
    my_curl_easy_setopt(easy, CURLOPT_HTTPHEADER, 0);
    // timeouts
    my_curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, CONNECTTIMEOUT);
    my_curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
    my_curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);

    my_curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1);
    my_curl_easy_setopt(easy, CURLOPT_MAXREDIRS, MAXREDIRS);
    my_curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);

    // Refresh only
    if ( preexisting ) {
        if (! doc->etag.empty()) {
            if (headers) {
                curl_slist_free_all(headers);
                headers = 0;
            }
            string header("If-None-Match: ");
            header += doc->etag;
            headers = curl_slist_append(headers, header.c_str());
            assert(headers);
            my_curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

        } else if (doc->modified > 0) {
            my_curl_easy_setopt(easy, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
            my_curl_easy_setopt(easy, CURLOPT_TIMEVALUE, doc->modified);
        }
    }

    CURLMcode rc = curl_multi_add_handle(global->multi, easy);
    mcode_or_die("get_content: curl_multi_add_handle", rc);
}


void GlobalInfo::reschedule()
{
    for(std::vector<EasyHandle*>::iterator i = EasyHandles.begin(); i != EasyHandles.end(); ++i) {
        EasyHandle* h = *i;
        if( h->state == EasyHandle::IDLE ) {
            h->reschedule();
        }
    }
}



/* Check for completed transfers, and remove their easy handles */
void GlobalInfo::check_run_count ()
{
    if (prev_running > still_running) {
        CURLMsg *msg;
        int msgs_left;
        EasyHandle *handle = NULL;
        CURL *easy;
        CURLcode res;

        LOG4CXX_DEBUG(crawlog,fs("REMAINING: " << still_running));
        /*
           I am still uncertain whether it is safe to remove an easy handle
           from inside the curl_multi_info_read loop, so here I will search
           for completed transfers in the inner "while" loop, and then remove
           them in the outer "do-while" loop...
         */
        do {
            easy = NULL;
            while ((msg = curl_multi_info_read (multi, &msgs_left))) {
                if (msg->msg == CURLMSG_DONE) {
                    easy = msg->easy_handle;
                    res = msg->data.result;
                    break;
                }
            }
            if (easy) {
                curl_easy_getinfo (easy, CURLINFO_PRIVATE, &handle);
                if( ! handle )
                    throw runtime_error("NULL handle retrieved from CURLINFO_PRIVATE");

                curl_multi_remove_handle (multi, easy);
                handle->done(res);

            }
        } while (easy);
    }
    prev_running = still_running;
}








void GlobalInfo::process_fifo_buff(bool flush)
{
    if( fifo_buff.empty() )
        return;

    size_t tortoise=0;
    size_t hare=0;
    while( (hare=fifo_buff.find_first_of("\n\r", tortoise)) != string::npos ) {
        if( hare > tortoise+1 ) { // avoid runs of separators
            string line = fifo_buff.substr(tortoise, hare - tortoise);
            //LOG4CXX_DEBUG(crawlog,fs("fifo read line: " << line));
            try {
                Url url(line);
                //cout << "url: " << url << endl;
                //LOG4CXX_INFO(crawlog,fs("read url: " << url.get()));
                // TODO only accept http scheme
                if( url.absolute() && url.scheme() == "http" )
                    classifier.push(url);
                else
                    LOG4CXX_WARN(crawlog,fs("non-http scheme, ignoring " << url.to_string() << endl));

            } catch(UrlParseError& e) {
                LOG4CXX_ERROR(crawlog,fs("url parse error: " << line << " : " << e.what() ));
            }
        }
        tortoise = ++hare;
    }
    if(flush && tortoise < fifo_buff.size()) {
        // process from tortoise
        string line = fifo_buff.substr(tortoise);
        if( line.empty() )
            return;

        LOG4CXX_DEBUG(crawlog,fs("fifo flush line: " << line));
        try {
            Url url(line);
            //cout << "url: " << url << endl;
            LOG4CXX_DEBUG(crawlog,fs("read url: " << url.get()));
            classifier.push(url);
        } catch(UrlParseError& e) {
            LOG4CXX_ERROR(crawlog,fs("url parse error: " << line << " : " << e.what() ));
        }
        fifo_buff.clear();
    } else if(tortoise < fifo_buff.size()) {
        fifo_buff =    fifo_buff.substr(tortoise);
    } else {
        fifo_buff.clear();
    }

//    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
//    boost::char_separator<char> nl_cr("\n\r");
//    tokenizer tokens(g->fifo_buff, nl_cr);
//    for (tokenizer::iterator i=tokens.begin(); i != tokens.end(); ++i) {
//        LOG4CXX_DEBUG(crawlog,fs("fifo read line: " << *i));
//        try {
//            Url url(*i);
//            //cout << "url: " << url << endl;
//            LOG4CXX_INFO(crawlog,fs("read url: " << url.get()));
//            g->classifier.push(url);
//        } catch(UrlParseError& e) {
//            cerr << "UrlParseError: " << e.what() << endl;
//        }
//    }
//    g->fifo_buff.clear();
}


void GlobalInfo::interactive_process(bool flush)
{
    if( interactive_buff.empty() )
        return;

    size_t tortoise=0;
    size_t hare=0;
    while( (hare=interactive_buff.find_first_of("\n\r", tortoise)) != string::npos ) {
        if( hare > tortoise+1 ) { // avoid runs of separators
            string line = interactive_buff.substr(tortoise, hare - tortoise);
            LOG4CXX_DEBUG(crawlog,fs("interactive read line: " << line));
            interactive_cmd(line);
        }
        tortoise = ++hare;
    }
    if(flush && tortoise < interactive_buff.size()) {
        // process from tortoise
        string line = interactive_buff.substr(tortoise);
        if( ! line.empty() ) {
            LOG4CXX_DEBUG(crawlog,fs("interactive flush line: " << line));
            interactive_cmd(line);
        }
    }
    interactive_buff = interactive_buff.substr(tortoise);
}


void GlobalInfo::status()
{
    const char *statestr[] = { "IDLE", "ROBOTS", "HEAD", "CONTENT", "FINISHED" };
    for(std::vector<EasyHandle*>::iterator i = EasyHandles.begin(); i != EasyHandles.end(); ++i) {
        utils::timer timediff = utils::timer::current() - (*i)->last_resched_time;
        if ((*i)->doc)
            cout << "handle " << (*i)->id << ": " << statestr[(*i)->state] << ": " << format_timediff(timediff) << ": " << (*i)->dl_kBs << " k: " << (*i)->doc->url.to_string() << endl;
        else
            cout << "handle " << (*i)->id << ": " << statestr[(*i)->state] << ": " << "NULL" << endl;

    }
}


void GlobalInfo::interactive_cmd(const std::string& cmd)
{
    if( cmd == "qlen" ) {
        cout << "Top q len: " << classifier.q_len_top() << endl;
        for(size_t i=0; i<NUMHANDLES; ++i)
            cout << "q("<<i<<") len: " << classifier.q_len(i) << endl;
        cout << endl;
    } else if (cmd == "dumpq") {
        cout << classifier << endl;
    } else if (cmd == "quit") {
        throw std::runtime_error("quit (interactive)");

    } else if (cmd == "reschedule") {
        reschedule();
    } else if (cmd == "status") {
        status();
    } else if (cmd == "help" || cmd == "h") {
        cout << "commands: qlen dumpq reschedule status help quit" << endl;
    }
}

} // end anon ns

int main(int argc, char **argv)
{
    try {
        log4cxx::BasicConfigurator::configure();
        log4cxx::PropertyConfigurator::configure("log.cfg");

        LOG4CXX_INFO(crawlog,fs("Initializing system..."));
        LOG4CXX_DEBUG(crawlog,fs("debug info enabled"));

        if (signal(SIGINT, sigint_handler) == SIG_ERR)
            utils::err_sys("signal");

        event_init();
        GlobalInfo g;
        event_dispatch();
    } catch(std::exception& e) {
        cerr << "main: caught exception: " << e.what() << endl;
        throw;
    } catch(...) {
        cerr << "main: caught exception ..." << endl;
        throw;
    }
    return EXIT_SUCCESS;
}

/** @} */

