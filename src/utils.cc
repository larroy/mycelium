#include "utils.hh"
#include <ctime>
#include <sstream>
//#include <boost/algorithm/string.hpp>
#include <cstdio>

// access
#include <unistd.h>

// mkdir
#include <sys/stat.h>

#include <iostream>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/socket.h>
#include <netdb.h>

#define LISTENQ 1024

//#include "gzstream.hh"


//#include "Common.hh"

using namespace std;
/*void marshall_urlb(const vector<Url>& urls, vector<char>& buff) {
    size_t ser_size=0;
    char* dest = &buff[0];
    for(vector<Url*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
        ser_size += sizeof(int16_t) + (*i).size();
    }
    buff.resize(ser_size);
    for(vector<Url*>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
        if( (*i).size() > SLZURL_MAXSIZE )
            throw(runtime_error("Url size > INT16_MAX"));
        utils::put_int16(dest,(int16_t) (*i).size());
        dest += sizeof(int16_t);
        string url = (*i).get();
        memcpy(dest, url.c_str(), url.size());
        dest += url.size();
    }
}

vector<uint8_t>& operator<<(vector<uint8_t>&buffer, int32_t value) {
    uint32_t tmp;
    //int32_t value = va_arg(*args, int32_t);
    if (value < 0) {
        value = 0 - value;
        tmp = 0 - static_cast<uint32_t>(value);
    } else
        tmp = static_cast<uint32_t>(value);
    vector<uint8_t>::size_type oldsize = buffer.size();
    buffer.resize(oldsize + 4);
    cout << "resize to: " << (oldsize + 4) << endl;
    buffer[oldsize]        = (tmp >> 24) & 0x000000ff;
    buffer[oldsize+1]    = (tmp >> 16) & 0x000000ff;
    buffer[oldsize+2]    = (tmp >>  8) & 0x000000ff;
    buffer[oldsize+3]    = (tmp >>  0) & 0x000000ff;
    return buffer;
}
*/

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */



static char text_chars[256] = {
    /*                  BEL BS HT LF    FF CR    */
    F, F, F, F,        F, F, F, T,        T, T, T, F,        T, T, F, F,  /* 0x0X */
    /*                                    ESC                 */
    F, F, F, F,        F, F, F, F,        F, F, F, T,        F, F, F, F,  /* 0x1X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, T,  /* 0x2X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, T,  /* 0x3X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, T,  /* 0x4X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, T,  /* 0x5X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, T,  /* 0x6X */
    T, T, T, T,        T, T, T, T,        T, T, T, T,        T, T, T, F,  /* 0x7X */
    /*                  NEL                                 */
    X, X, X, X,        X, T, X, X,        X, X, X, X,        X, X, X, X,  /* 0x8X */
    X, X, X, X,        X, X, X, X,        X, X, X, X,        X, X, X, X,  /* 0x9X */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I,  /* 0xaX */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I,  /* 0xbX */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I,  /* 0xcX */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I,  /* 0xdX */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I,  /* 0xeX */
    I, I, I, I,        I, I, I, I,        I, I, I, I,        I, I, I, I   /* 0xfX */
};
//#undef F
//#undef T
//#undef I
//#undef X



namespace utils {
    int system_exec(const char* cmd, ...)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        struct sigaction old_act;
        memset(&act, 0, sizeof(old_act));

        act.sa_handler = SIG_DFL;
        if( sigaction(SIGCHLD, &act, &old_act) < 0)
            utils::err_sys("sigaction error");

        va_list ap;
        vector<char*> arglist;
        arglist.push_back(const_cast<char*>(cmd));
        char* cur;
        va_start(ap,cmd);
        while( (cur=va_arg(ap,char*)) != NULL)
            arglist.push_back(strdup(cur));
        va_end(ap);
        arglist.push_back(NULL);
        int status=0;
        pid_t chld;
        if( (chld = fork()) > 0 ) {
            for(vector<char*>::iterator i = arglist.begin()+1; i != arglist.end(); ++i)
                free(*i);
            if( waitpid(chld, &status, 0) < 0 ) {
                perror("wait failed");
                exit(EXIT_FAILURE);
            }
        } else if(chld < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {
            execvp(cmd, &arglist[0]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        // restore old signal handler
        if( sigaction(SIGCHLD, &old_act, NULL) < 0)
            utils::err_sys("sigaction error");
        return status;
    }


#define W 1
#define R 0

    int stdout2stream_system_exec(std::ostream& os, const char* cmd, ...)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        struct sigaction old_act;
        memset(&act, 0, sizeof(old_act));

        act.sa_handler = SIG_DFL;
        if( sigaction(SIGCHLD, &act, &old_act) < 0)
            utils::err_sys("sigaction error");

        va_list ap;
        vector<char*> arglist;
        arglist.push_back(const_cast<char*>(cmd));
        char* cur;
        va_start(ap,cmd);
        while( (cur=va_arg(ap,char*)) != NULL)
            arglist.push_back(strdup(cur));
        va_end(ap);
        arglist.push_back(NULL);

        int fd[2];
        if(pipe(fd) < 0)
            utils::err_sys("pipe failed");

        int status=0;
        pid_t chld;
        if( (chld = fork()) > 0 ) {
            for(vector<char*>::iterator i = arglist.begin()+1; i != arglist.end(); ++i)
                free(*i);
            close(fd[W]);
            char buf[1024];
            ssize_t nread;
            while( (nread=read(fd[R],buf,1024)) > 0 && os.good() ) {
                os.write(buf,nread);

            }
            close(fd[R]);
            if( waitpid(chld, &status, 0) < 0 ) {
                perror("wait failed");
                exit(EXIT_FAILURE);
            }
        } else if(chld < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {
            close(fd[R]);
            // no need according to manpage
            // close(STDOUT_FILENO);
            if( dup2(fd[W], STDOUT_FILENO) != STDOUT_FILENO) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd[W]);
            execvp(cmd, &arglist[0]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        // restore old signal handler
        if( sigaction(SIGCHLD, &old_act, NULL) < 0)
            utils::err_sys("sigaction error");

        return status;
    }
#undef R
#undef W

    int err_sys(std::string s) {
        std::string err;
        char *err_str = std::strerror(errno);
        if( err_str )
            err = s + " error: " + std::strerror(errno)  + "\n";
        else
            err = s + " error: unknown (strerror returned NULL)\n";
        throw std::runtime_error(err);
    }

    void create_directories(const char *path)
    {
        int res=0;
        int len = strlen(path);
        //char *buf = malloc(len + 1);
        std::string buf;
        const char *slash = path;
        if( access(path, X_OK) == 0 )
            return;

        while (true) {
            slash = strchr(slash+1, '/');
            if( slash != NULL )
                len = slash - path;
            else
                len = strlen(path);
            //memcpy(buf, path, len);
            buf.assign(path,len);
            //buf[len] = 0;
            res = mkdir(buf.c_str(), 00775);
            if( res != 0 && errno != EEXIST)
                err_sys(fs("mkdir \"" << buf << "\""));
            if(!slash)
                break;
            //cout << "mkdir " << buf << endl;

            //if( res )
            //    throw err_sys(fs("mkdir " << buf << " :"));
        }
    }


    unsigned hexval(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return ~0;
    }

    int get_sha1_from_hex(const char *hex, unsigned char *sha1)
    {
        int i;
        for (i = 0; i < 20; i++) {
            unsigned int val = (hexval(hex[0]) << 4) | hexval(hex[1]);
            if (val & ~0xff)
                return -1;
            *sha1++ = val;
            hex += 2;
        }
        return 0;
    }

    char * sha1_to_hex(const unsigned char *sha1)
    {
        static char buffer[50];
        static const char hex[] = "0123456789abcdef";
        char *buf = buffer;
        int i;

        for (i = 0; i < 20; i++) {
            unsigned int val = *sha1++;
            *buf++ = hex[val >> 4];
            *buf++ = hex[val & 0xf];
        }
        *buf++ = '\0';
        return buffer;
    }



    string makeHexDump(const string& str) {
        char tmp[5];
        string ret;
        ret.reserve((int)(str.size()*2.2));
        for(string::size_type n=0;n<str.size();++n) {
            sprintf(tmp,"%02x ", (unsigned char)str[n]);
            ret+=tmp;
        }
        return ret;
    }

#if 0
    std::string scheme_int_2_string(int scheme)
    {
        if( scheme  == SCHEME_HTTP )
            return "http";
        else if( scheme == SCHEME_FILE)
            return "file";
        else if( scheme == SCHEME_FTP)
            return "ftp";
        else if( scheme == SCHEME_UNSUPPORTED )
            return "unsupported";
        else
            throw logic_error("unrecognized scheme");
    }
#endif

#if 0
    int scheme_string_2_int(const string& scheme_in)
    {
        string scheme = scheme_in;
        boost::to_lower(scheme);
        if( scheme  == "http" )
            return SCHEME_HTTP;
        else if( scheme == "file")
            return SCHEME_FILE;
        else if( scheme == "ftp")
            return SCHEME_FTP;
        else
            return SCHEME_UNSUPPORTED;
    }
#endif


    std::string Timestamp::get()
    {
        ostringstream os;
        time_t tnow;
        time(&tnow);
        struct tm* now  = localtime(&tnow);
        os << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec;
        return os.str();

    }

    bool looks_ascii(const uint8_t *buf, size_t nbytes)
    {
        size_t i;
        for (i = 0; i < nbytes; i++) {
            int t = text_chars[static_cast<int>(buf[i])];
            if (t != T)
                return false;
        }
        return true;
    }

    bool looks_latin1(const uint8_t *buf, size_t nbytes)
    {
        size_t i;
        for (i = 0; i < nbytes; i++) {
            int t = text_chars[static_cast<int>(buf[i])];
            if (t != T && t != I)
                return false;
        }
        return true;
    }

    bool looks_extended(const uint8_t *buf, size_t nbytes)
    {
        size_t i;
        for (i = 0; i < nbytes; i++) {
            int t = text_chars[static_cast<int>(buf[i])];

            if (t != T && t != I && t != X)
                return false;
        }
        return true;
    }

    bool looks_utf8(const uint8_t *buf, size_t nbytes)
    {
        size_t i;
        int n;
        bool gotone = false;

        for (i = 0; i < nbytes; i++) {
            if ((buf[i] & 0x80) == 0) {       /* 0xxxxxxx is plain ASCII */
                /*
                 * Even if the whole file is valid UTF-8 sequences,
                 * still reject it if it uses weird control characters.
                 */

                if (text_chars[static_cast<int>(buf[i])] != T)
                    return false;

            } else if ((buf[i] & 0x40) == 0) { /* 10xxxxxx never 1st byte */
                return false;
            } else {               /* 11xxxxxx begins UTF-8 */
                int following;

                if ((buf[i] & 0x20) == 0) {        /* 110xxxxx */
                    following = 1;
                } else if ((buf[i] & 0x10) == 0) {    /* 1110xxxx */
                    following = 2;
                } else if ((buf[i] & 0x08) == 0) {    /* 11110xxx */
                    following = 3;
                } else if ((buf[i] & 0x04) == 0) {    /* 111110xx */
                    following = 4;
                } else if ((buf[i] & 0x02) == 0) {    /* 1111110x */
                    following = 5;
                } else
                    return false;

                for (n = 0; n < following; n++) {
                    i++;
                    if (i >= nbytes)
                        goto done;

                    if ((buf[i] & 0x80) == 0 || (buf[i] & 0x40))
                        return false;

                }

                gotone = true;
            }
        }
    done:
        return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
    }

    const char* unicode_BOM(const uint8_t *buf, size_t nbytes)
    {
        if(nbytes < 2)
            return NULL;
        else if(nbytes >= 2) {
            if( *(uint16_t*)buf == 0xFEFF )
                //return content_type::FILE_TEXT_UTF16BE;
                return "UTF-16BE";
            else if( *(uint16_t*)buf  == 0xFFFE )
                //return content_type::FILE_TEXT_UTF16LE;
                return "UTF-16LE";
            if(nbytes >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
                //return content_type::FILE_TEXT_UTF8;
                return "UTF-8";
            if(nbytes >= 4 && *(uint32_t*)buf == 0x0000FEFF)
                //return content_type::FILE_TEXT_UTF32BE;
                return "UTF-32BE";
            if(nbytes >= 4 && *(uint32_t*)buf == 0xFEFF0000)
                //return content_type::FILE_TEXT_UTF32LE;
                return "UTF-32LE";
        }
        return NULL;
    }

    bool pdf_ver(const uint8_t *buf, size_t nbytes, int* maj, int* min)
    {
        if(nbytes < 8)
            return false;
        if( strncmp((const char*)(buf),"%PDF-",5) == 0 ) {
            if(maj)
                *maj = buf[5];
            if(min)
                *min = buf[7];
            return true;
        }
        return false;
    }


    size_t allocated;



    // TODO unicode
//    int looks_unicode(const char *buf, size_t nbytes)
//    {
//        int bigend;
//        size_t i;
//
//        if (nbytes < 2)
//            return 0;
//
//        if (buf[0] == 0xff && buf[1] == 0xfe)
//            bigend = 0;
//        else if (buf[0] == 0xfe && buf[1] == 0xff)
//            bigend = 1;
//        else
//            return 0;
//
//        *ulen = 0;
//
//        for (i = 2; i + 1 < nbytes; i += 2) {
//            /* XXX fix to properly handle chars > 65536 */
//
//            if (bigend)
//            else
//                ubuf[(*ulen)++] = buf[i] + 256 * buf[i + 1];
//
//            if (ubuf[*ulen - 1] == 0xfffe)
//                return 0;
//            if (ubuf[*ulen - 1] < 128 &&
//                text_chars[(size_t)ubuf[*ulen - 1]] != T)
//                return 0;
//        }
//
//        return 1 + bigend;
//    }



#if 0
    int rmdir_recursive(const char *base)
    {
        DIR *d = opendir(basedir);
        struct dirent *dp;

        if (!d)
        {
            perror(basedir);
            return -1;
        }
        while (dp = readdir(d))
        {
            size_t len = strlen(basedir) + strlen(dp->d_name) + 3;
            struct stat sb;
            char *path;

            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                continue;

            path = malloc(len);

            if (!path)
            {
                perror("malloc");
                free(path);
                continue;
            }

            memset(path, 0, len);
            snprintf(path, len - 1, "%s/%s", basedir, dp->d_name);

            if (lstat(path, &sb))
            {
                perror(path);
                free(path);
                continue;
            }

            if (sb.st_mode & S_IFDIR)
                rmdir_recursive(path);
            else
                unlink(path);

            free(path);
        }

        closedir(d);

        return rmdir(basedir);
    }
#endif
std::string fmt_bytes(uint64_t bytes)
{
    int suf_i=0;
    const char* SUFFIXES[] = {"iB","KiB","MiB","GiB","TiB","PiB"};
    double res=bytes;
    while( res > 1000 && suf_i <= 4) {
        res /= 1000;
        ++suf_i;
    }
    assert(suf_i <= 5);
    ostringstream os;
    os.precision(2);
    os.setf(ios::fixed,ios::floatfield);
    os << res << " " << SUFFIXES[suf_i];
    return os.str();

}

std::string fmt_kbytes_s(double kBs) {
    int suf_i=0;
    const char* SUFFIXES[] = {"KB/s","MB/s","GB/s","TB/s","PB/s"};
    double res=kBs;
    while( res > 1000 && suf_i <= 3) {
        res /= 1000;
        ++suf_i;
    }
    assert(suf_i <= 4);
    ostringstream os;
    os.precision(2);
    os.setf(ios::fixed,ios::floatfield);
    os << res << " " << SUFFIXES[suf_i];
    return os.str();

}


void Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(fd, level, optname, optval, optlen) < 0)
        err_sys("setsockopt error");
}

void Listen(int fd, int backlog)
{
    char* ptr;
    /*4can override 2nd argument with environment variable */
    if ( (ptr = getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if (listen(fd, backlog) < 0)
        err_sys("listen error");
}

int Tcp_connect(const char *host, const char *serv)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;
    std::string error;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
        error = "tcp_connect error for";
        if( host != 0 )
            error+= host;
        if( serv != 0 ) {
            error+= ":";
            error+= serv;
        }
        error+= gai_strerror(n);
        err_sys(error);
    }

    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;    /* ignore this one */

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;        /* success */

        close(sockfd);    /* ignore this one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL) {    /* errno set from final connect() */

        error = "tcp_connect error for ";
        if( host != 0 )
            error+= host;
        if( serv != 0 ) {
            error+= ":";
            error+= serv;
        }
        err_sys(error);
    }

    freeaddrinfo(ressave);

    return(sockfd);
}

int Tcp_connect_retry(const char *host, const char *serv, int tries)
{
    int                sockfd, n;
    struct addrinfo    hints, *res, *ressave;
    std::string error;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    while(true) {
        if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
            error = "tcp_connect error for";
            if( host != 0 )
                error+= host;
            if( serv != 0 ) {
                error+= ":";
                error+= serv;
            }
            error+= gai_strerror(n);
            err_sys(error);
        }

        ressave = res;
        do {
            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd < 0)
                continue;    /* ignore this one */

            if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                goto success;

            close(sockfd);    /* ignore this one */
        } while ( (res = res->ai_next) != NULL);

        if (res == NULL) {    /* errno set from final connect() */
            error = "tcp_connect error for ";
            if( host != 0 )
                error+= host;
            if( serv != 0 ) {
                error+= ":";
                error+= serv;
            }
            //err_sys(error);
            std::clog <<  error << std::endl;
            std::clog << "Waiting 1 seconds for retry..." << std::endl;
        }
        if( tries > 0)
            --tries;
        else if(tries == 0)
            err_sys(error);
        // for tries < 0 we loop forever
        sleep(1);
    }
success:
    freeaddrinfo(ressave);

    return(sockfd);
}

int Tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;
    std::string error;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {

        error = "tcp_listen error for";
        if( host != 0 )
            error+= host;
        if( serv != 0 )
            error+= serv;
        error+= gai_strerror(n);
        err_sys(error);
    }

    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue;        /* error, try next one */

        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break;            /* success */

        close(listenfd);    /* bind error, close and try next one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL) {    /* errno from final socket() or bind() */
        error = "tcp_listen error for";
        if( host != 0 )
            error+= host;
        if( serv != 0 )
            error+= serv;
        err_sys(error);
    }

    Listen(listenfd, LISTENQ);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;    /* return size of protocol address */

    freeaddrinfo(ressave);

    return(listenfd);
}
/* end tcp_listen */

int Accept(int fd, struct ::sockaddr *sa, socklen_t *salenptr)
{
    int n;

again:
    if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef  EPROTO
        if (errno == EPROTO || errno == ECONNABORTED)
#else
        if (errno == ECONNABORTED)
#endif
            goto again;
        if(errno == EAGAIN)
            return(n);
        else
            err_sys("accept");
    }
    return(n);
}


} // end namespace utils

std::ostream& operator<<(std::ostream& os, const utils::Timestamp& t)
{
    os << utils::Timestamp::get();
    return os;
}

std::auto_ptr<std::ostringstream> utils::getoss()
{
   return std::auto_ptr<std::ostringstream> (new std::ostringstream);
}

