/*
mydownlib
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

    Copyright 2006-2021 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

// note that these functions contain some experimental code that
// I consider a "work-around" like SSL and global cookies support

//#define NOLFS
#ifndef NOLFS
    #define _LARGE_FILES        // if it's not supported the tool will work
    #define __USE_LARGEFILE64   // without support for large files
    #define __USE_FILE_OFFSET64
    #define _LARGEFILE_SOURCE
    #define _LARGEFILE64_SOURCE
    #define _FILE_OFFSET_BITS   64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <time.h>
#include <ctype.h>
#include <zlib.h>
#include "mydownlib.h"

#ifdef WIN32
    #include <winsock2.h>   // it works on Win98 too
    #include <ws2tcpip.h>
    #include <wspiapi.h>
    #include <direct.h>
    #define PATHSLASH   '\\'
    #define make_dir(x) mkdir(x)
    #define close       closesocket
    #define in_addr_t   u32
    #define MYDOWN_TEMPOZ1
    #define MYDOWN_TEMPOZ2  GetTickCount()
    #define ONESEC      1000
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <sys/times.h>
    //#include <sys/timeb.h>
    #include <dirent.h>
    #define PATHSLASH   '/'
    #define make_dir(x) mkdir(x, 0755)
    #define stricmp     strcasecmp
    #define strnicmp    strncasecmp
    #define MYDOWN_TEMPOZ1     gettimeofday(&timex, NULL) //ftime(&timex)
    #define MYDOWN_TEMPOZ2     ((timex.tv_sec * 1000) + (timex.tv_usec / 1000)) //((timex.time * 1000) + timex.millitm)
    #define ONESEC      1
#endif

#if defined(_LARGE_FILES)
    #if defined(__APPLE__)
        #define fseek   fseeko
        #define ftell   ftello
    #elif defined(__FreeBSD__)
    #elif !defined(NOLFS)       // use -DNOLFS if this tool can't be compiled on your OS!
        #define off_t   off64_t
        #define fopen   fopen64
        #define fseek   fseeko64
        #define ftell   ftello64
        #ifndef fstat
            #ifdef WIN32
                #define fstat   _fstati64
                #define stat    _stati64
            #else
                #define fstat   fstat64
                #define stat    stat64
            #endif
        #endif
    #endif
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;



#define MYDOWN_VISDELAY     500
#define MYDOWN_BUFFSZ       8192
#define MYDOWN_MAXTIMEOUT   60      // one minute
#define MYDOWN_MAXARGS      16      // modify it if you need more args in mydown_scanhead
#define MYDOWN_MAXDNS       1024
#define MYDOWN_VERPRINTF    if(verbose >= 0) fprintf(stderr,
#define MYDOWN_VERPRINTF2   if(verbose > 0)  fprintf(stderr,
#define MYDOWN_TEMPOZ(x)    MYDOWN_TEMPOZ1; \
                            x = MYDOWN_TEMPOZ2

#ifdef DISABLE_SSL  // used in QuickBMS and probably useful elsewhere
    #undef MYDOWN_SSL
#endif
#ifdef MYDOWN_SSL
    #include <openssl/ssl.h>    // link with libssl.a libcrypto.a -lgdi32
#else                           // on linux: gcc -o code code.c -lssl -lcrypto -lpthread
    #define SSL     char
    #define SSL_CTX char
    #define SSL_read(A,B,C)     0
    #define SSL_write(A,B,C)    0
    #define SSL_shutdown(X)     mydown_donothing()
    #define SSL_free(X)         mydown_donothing()
    #define SSL_CTX_free(X)     mydown_donothing()
#endif
#define SSL_COMPAT(X)   SSL_CTX_set_cipher_list(X, "ALL"); \
                        SSL_CTX_set_options(X, SSL_OP_ALL);



#define MYDOWN_AI_ADDR_MAX  64
typedef struct {            // lame DNS caching implementation, currently mandatory for getaddrinfo support
    u8      *host;          // it's possible to use a static host of 253+1 bytes (FQDN)
    struct addrinfo peer;
    u8      ai_addr[MYDOWN_AI_ADDR_MAX];    // necessary for freeing AddrInfo, maximum size for AF_INET* should be sizeof(sockaddr_in6) or 28
    //int         sd; // currently NOT used, maybe in future
} mydown_dns_db_t;

int             mydown_dns_db_max               = 0,
                mydown_dns_db_add               = 0,
                mydown_force_rename             = 0,
                mydown_force_overwrite          = 0;
mydown_dns_db_t mydown_dns_db[MYDOWN_MAXDNS]    = {{NULL}};
u8              *mydown_global_cookie           = NULL; // used only with -DMYDOWN_mydown_global_cookie



void mydown_donothing(void) {}
int mydown_get_host(u8 *url, u8 **hostx, u16 *portx, u8 **urix, u8 **userx, u8 **passx, int verbose);
u8 *mydown_uri2hex(u8 *uri);
u8 *mydown_hex2uri(u8 *uri);
void mydown_scanhead(u8 *data, int datalen, ...);
int *mydown_global_keep_alive(int save_info, struct addrinfo *peer, int *sd, SSL **ssl_sd, SSL_CTX **ctx_sd);
void mydown_free_sock(int global_keep_alive, struct addrinfo *peer, int *sd, SSL **ssl_sd, SSL_CTX **ctx_sd);
void mydown_free(u8 **buff);
int mydown_chunked_skip(u8 *buff, int chunkedsize);
int mydown_unzip(z_stream *z, u8 *in, int inlen, u8 **outx, int *outxlen);
u8 *mydown_tmpnam(void);
int mydown_sscanf_hex(u8 *data, int datalen);
int mydown_timeout(SSL *ssl_sd, int sd, int secs);
int mydown_block_recv(SSL *ssl_sd, int sd, u8 *data, int len, int timeout);
void mydown_showstatus(u64 fsize, u64 ret, u64 oldret, int timediff);
u8 *mydown_base64_encode(u8 *data, int *length);
struct addrinfo *mydown_resolv(struct addrinfo *ret, char *host, int port, void *ai_addr);
in_addr_t mydown_resolv_legacy(char *host);
char *mydown_stristr(const char *String, const char *Pattern);
int mydown_create_dir(u8 *name);
u8 *mydown_rename_auto(int cnt, u8 *name);
int mydown_check_overwrite(u8 *fname, int check_if_present_only, int resume, int *asked);



int mydown_send(SSL *ssl_sd, int sd, u8 *data, int datasz) {
    //if((verbose > 0) && (verbose & 8)) {
        //if(datasz > 0) show_dump(2, data, datasz, stdout);
        //if(datasz > 0) fwrite(data, 1, datasz, stdout);
    //}
    if(ssl_sd) return(SSL_write(ssl_sd, data, datasz));
    return(send(sd, data, datasz, 0));
}



int mydown_recv(SSL *ssl_sd, int sd, u8 *data, int datasz) {
    int     ret;
    if(ssl_sd) ret = SSL_read(ssl_sd, data, datasz);
    else       ret = recv(sd, data, datasz, 0);
    //if((verbose > 0) && (verbose & 8)) {
        //if(ret > 0) show_dump(2, data, ret, stdout);
        //if(ret > 0) fwrite(data, 1, ret, stdout);
    //}
    return ret;
}



#define mydown_opt_pop \
    if(opt) { \
        if(!from)       from        = opt->from; \
        if(!tot)        tot         = opt->tot; \
        if(!showhead)   showhead    = opt->showhead; \
        if(!resume)     resume      = opt->resume; \
        if(!onlyifdiff) onlyifdiff  = opt->onlyifdiff; \
        if(!user)       user        = opt->user; \
        if(!pass)       pass        = opt->pass; \
        if(!referer)    referer     = opt->referer; \
        if(!useragent)  useragent   = opt->useragent; \
        if(!cookie)     cookie      = opt->cookie; \
        if(!more_http)  more_http   = opt->more_http; \
        if(!verbose)    verbose     = opt->verbose; \
        if(!filedata)   filedata    = opt->filedata; \
        if(!keep_alive) keep_alive  = opt->keep_alive; \
        if(!timeout)    timeout     = opt->timeout; \
        if(!ret_code)   ret_code    = opt->ret_code; \
        if(!onflyunzip) onflyunzip  = opt->onflyunzip; \
        if(!content)    content     = opt->content; \
        if(!contentsize)contentsize = opt->contentsize; \
        if(!get)        get         = opt->get; \
        if(!proxy)      proxy       = opt->proxy; \
        if(!proxy_port) proxy_port  = opt->proxy_port; \
        if(!recv_bytes) recv_bytes  = opt->recv_bytes; \
        if(!fd)         fd          = opt->fd; \
    }

#define mydown_opt_push \
    if(opt) { \
        if(mydown_global_cookie) mydown_global_cookie = cookie; \
        opt->from        = from; \
        opt->tot         = tot; \
        opt->showhead    = showhead; \
        opt->resume      = resume; \
        opt->onlyifdiff  = onlyifdiff; \
        opt->user        = user; \
        opt->pass        = pass; \
        opt->referer     = referer; \
        opt->useragent   = useragent; \
        opt->cookie      = cookie; \
        opt->more_http   = more_http; \
        opt->verbose     = verbose; \
        opt->filedata    = filedata; \
        opt->keep_alive  = keep_alive; \
        opt->timeout     = timeout; \
        opt->ret_code    = ret_code; \
        opt->onflyunzip  = onflyunzip; \
        opt->content     = content; \
        opt->contentsize = contentsize; \
        opt->get         = get; \
        opt->proxy       = proxy; \
        opt->proxy_port  = proxy_port; \
        opt->recv_bytes  = recv_bytes; \
        opt->fd          = fd; \
    }
    
#define MYDOWN_NEW_CONNECTION  { \
        if(sd && !keep_alive) mydown_free_sock(global_keep_alive, &peer, &sd, &ssl_sd, &ctx_sd); \
        mydown_free_sock(global_keep_alive, &peer, keep_alive, &ssl_sd, &ctx_sd); \
        ret = mydown_http2file(keep_alive, timeout, host, port, user, pass, referer, useragent, cookie, more_http, verbose, getstr, fd, filename, showhead, (dossl << 1) | (mydownlib_flags & ~MYDOWN_OPTION_DOSSL), resume, from, tot, filesize, filedata, ret_code, onflyunzip, content, contentsize, get, proxy, proxy_port, opt); \
    }

#define MYDOWN_RETRY_CONNECTION { \
        if(retry >= 3) MYDOWN_GOTOQUIT \
        retry++; \
        if(sd && !keep_alive) mydown_free_sock(global_keep_alive, &peer, &sd, &ssl_sd, &ctx_sd); \
        mydown_free_sock(global_keep_alive, &peer, keep_alive, &ssl_sd, &ctx_sd); \
        goto mydown_retry; \
    }



u64 mydown(u8 *myurl, u8 *filename, mydown_options *opt) {
    mydown_options      tmp_opt;
    FILE    *fd         = NULL;
    u64     from        = 0,
            tot         = 0,
            filesize    = MYDOWN_ERROR,
            contentsize = 0,
            *recv_bytes = NULL;
    int     showhead    = 0,
            resume      = 0,
            onlyifdiff  = 0,
            verbose     = 0,
            *keep_alive = NULL,
            *ret_code   = NULL,
            timeout     = 0,
            onflyunzip  = 0,
            dossl       = 0;
    u16     port        = 0,
            proxy_port  = 0;
    u8      *url        = NULL,
            *uri        = NULL,
            *host       = NULL,
            *user       = NULL,
            *pass       = NULL,
            *referer    = NULL,
            *useragent  = NULL,
            *cookie     = NULL,
            *more_http  = NULL,
            **filedata  = NULL,
            *content    = NULL,
            *get        = NULL,
            *proxy      = NULL;

#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2), &wsadata);
#endif

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    mydown_opt_pop
    if(mydown_global_cookie) cookie = mydown_global_cookie;
    if(user) {
        MYDOWN_VERPRINTF2
            "  user   %s\n"
            "  pass   %s\n",
            user,
            pass);
    }

    if(!myurl) return MYDOWN_ERROR;
    url = strdup(myurl);
    if(!url) return MYDOWN_ERROR;
    dossl = mydown_get_host(url, &host, &port, &uri, user ? NULL : &user, pass ? NULL : &pass, verbose);

    MYDOWN_VERPRINTF2"  start download\n");

    if(opt) memcpy(&tmp_opt, opt, sizeof(tmp_opt));
    
    filesize = mydown_http2file(
        keep_alive,         // int *sock
        timeout,            // int timeout
        host,               // u8 *host
        port,               // u16 port
        user,               // u8 *user
        pass,               // u8 *pass
        referer,            // u8 *referer
        useragent,          // u8 *useragent
        cookie,             // u8 *cookie
        more_http,          // u8 *more_http
        verbose,            // int verbose
        uri,                // u8 *getstr
        fd,                 // FILE *fd
        filename,           // u8 *filename
        showhead,           // int showhead
        (dossl << 1) | (onlyifdiff & ~MYDOWN_OPTION_DOSSL), // int onlyifdiff
        resume,             // int resume
        from,               // u64 from
        tot,                // u64 tot
        NULL,               // u64 *filesize
        filedata,           // u8 **filedata
        ret_code,           // int *ret_code
        onflyunzip,         // int onflyunzip
        content,            // u8 *content
        contentsize,        // u64 contentsize
        get,                // u8 *get
        proxy,
        proxy_port,
        opt ? &tmp_opt : NULL
    );

    if(fd && (fd != stdout)) fclose(fd);
    mydown_free(&url);
    mydown_free(&uri);
    return(filesize);
}



int mydown_get_host(u8 *url, u8 **hostx, u16 *portx, u8 **urix, u8 **userx, u8 **passx, int verbose) {
    int     dossl = 0;
    u16     port  = 80;
    u8      *host = NULL,
            *uri  = NULL,
            *user = NULL,
            *pass = NULL,
            *p;

    if(url) {
        host = url;

        p = strstr(host, "://");    // handle http://
        if(!p) p = strstr(host, ":\\\\");
        if(p) {
            if(!strnicmp(host, "https", 5) || !strnicmp(host, "wss", 3) || !strnicmp(host, "ssl", 3) || !strnicmp(host, "tls", 3)) {
                dossl = 1;
                port  = 443;
            }
            for(p += 3; *p; p++) {  // in case of http:////
                if((*p != '/') && (*p != '\\')) break;
            }
            host = p;
        }

        for(p = host; *p; p++) {    // search the uri
            if((*p == '/') || (*p == '\\')) {
                uri = p;
                break;
            }
        }
        if(uri) {
            *uri++ = 0;
            uri = mydown_uri2hex(uri);
        }
        if(!uri) uri = strdup("");  // in case of no uri or mydown_uri2hex fails

        p = strrchr(host, '@');
        if(p) {
            *p = 0;

            user = host;

            pass = strchr(host, ':');
            if(pass) {
                *pass++ = 0;
            } else {
                pass = "";
            }

            host = p + 1;
        }

        if(host[0] == '[') {
            host++;
            p = strchr(host, ']');
            if(p) *p++ = 0;
            else  p = host;
        } else {
            p = host;
        }

        // http://::1/file http://[::1]:80/file
        int is_port = 0;
        u8  *s;
        for(s = p; *s; s++) {
            if(*s == ':') is_port++;
        }
        if(is_port == 1) {
            p = strchr(p, ':');
            if(p) {
                *p = 0;
                port = atoi(p + 1);
            }
        }

        MYDOWN_VERPRINTF"  %s\n", url);
        MYDOWN_VERPRINTF2
            "  host   %s : %u\n"
            "  uri    %s\n",
            host, port,
            uri);

        if(user) {
            MYDOWN_VERPRINTF2
                "  user   %s\n"
                "  pass   %s\n",
                user,
                pass);
        }
    }

    if(hostx) *hostx = host;
    if(portx) *portx = port;
    if(urix)  *urix  = uri;
    if(userx) *userx = user;
    if(passx) *passx = pass;
    return(dossl);
}



u8 *mydown_http_delimit(u8 *data, u8 *limit, int *mod) {
    u8      *p;

    if(mod) *mod = -1;
    if(!data || !data[0]) return(NULL);

    for(p = data;; p++) {
        if(p >= limit) return(NULL);
        if(!*p) return(NULL);
        if((*p == '\r') || (*p == '\n')) break;
    }
    *p = 0;
    if(mod) *mod = p - data;
    for(++p;; p++) {
        if(p >= limit) return(NULL);
        if(!*p) return(NULL);
        if((*p != '\r') && (*p != '\n')) break;
    }
    return(p);
}



u8 *mydown_uri2hex(u8 *uri) {
    static const u8 hex[] = "0123456789abcdef";
    u8      *ret,
            *p,
            c;

    if(!uri) return(NULL);
    ret = calloc((strlen(uri) * 3) + 1, 1);
    if(!ret) return(NULL);

    for(p = ret; *uri; uri++) {
        c = *uri;
        if(isprint(c) && !strchr(" \"<>#" /*"%\\"*/ "{}|^~[]`", c)) {   // I have left the backslash and the percentage out
            *p++ = c;
        } else {
            *p++ = '%';
            *p++ = hex[c >> 4];
            *p++ = hex[c & 15];
        }
    }
    *p = 0;

    return(ret);
}



u8 *mydown_hex2uri(u8 *uri) {
    int     t,
            n;
    u8      *ret,
            *p;

    if(!uri) return(NULL);
    ret = strdup(uri);
    if(!ret) return(NULL);

    for(p = ret; *uri; uri++, p++) {
        t = *uri;
        if((*uri == '%') && (*(uri + 1) == 'u')) {
            if(sscanf(uri + 1, "u%04x", &t) == 1) uri += 5;
        } else if(*uri == '%') {
            if(sscanf(uri + 1, "%02x",  &t) == 1) uri += 2;
        } else if(*uri == '&') {
            if(*(uri + 1) == '#') {
                if((sscanf(uri + 1, "#%d;%n", &t, &n) == 1) && (n > 2)) {
                    uri += n;
                } else {
                    t = *uri;
                }
            } else if(!strnicmp(uri, "&quot;",   6)) {
                t = '\"';
                uri += 5;
            } else if(!strnicmp(uri, "&amp;",    5)) {
                t = '&';
                uri += 4;
            } else if(!strnicmp(uri, "&frasl;",  7)) {
                t = '/';
                uri += 6;
            } else if(!strnicmp(uri, "&lt;",     4)) {
                t = '<';
                uri += 3;
            } else if(!strnicmp(uri, "&gt;",     4)) {
                t = '>';
                uri += 3;
            } else if(!strnicmp(uri, "&nbsp;",   6)) {
                t = 160;
                uri += 5;
            } else if(!strnicmp(uri, "&middot;", 8)) {
                t = 183;
                uri += 7;
            }
        }
        *p = t;
    }
    *p = 0;

    return(ret);
}



// can be used only one time because modifies the input!
void mydown_scanhead(u8 *data, int datalen, ...) {
    va_list ap;
    int     i,
            mod,
            vals,
            cookie_len  = 0;
    u8      *par[MYDOWN_MAXARGS],
            **val[MYDOWN_MAXARGS],
            *l,
            *p,
            *t,
            *datal,
            *limit;

    if(!data) return;
    if(datalen <= 0) return;

    va_start(ap, datalen);
    for(i = 0; i < MYDOWN_MAXARGS; i++) {
        par[i] = va_arg(ap, u8 *);
        if(!par[i]) break;
        val[i] = va_arg(ap, u8 **);
        if(!val[i]) break;
        *val[i] = NULL;
    }
    vals = i;
    va_end(ap);

    for(limit = data + datalen; data; data = l) {   // in case data becomes NULL
        l = mydown_http_delimit(data, limit, &mod);
        datal = strchr(data, ':');
        if(datal) {
            *datal = 0; // restore later
            for(i = 0; i < vals; i++) {
                if(stricmp(data, par[i])) continue;
                for(p = datal + 1; *p && ((*p == ' ') || (*p == '\t')); p++);
                if(!stricmp(data, "Set-Cookie")) {
                    t = strchr(p, ';');
                    if(t) *t = 0;   // restore later
                    *val[i] = realloc(*val[i], cookie_len + 2 + strlen(p) + 1);
                    if(*val[i]) cookie_len += sprintf(*val[i] + cookie_len, "%s%s", cookie_len ? "; " : "", p);
                    if(t) *t = ';';
                } else {
                    *val[i] = p;
                }
                break;
            }
            *datal = ':';
        }
        //if(mod >= 0) data[mod] = '\r';  // never enable this, the data must remain modified
        if(!l) break;
    }
}



// fname is provided by the user or from the network, the cleaning here is intended only
// about the bad chars not supported by the file-system, it's not a security function
void mydown_filename_clean(u8 *fname) {
    u8      *p;

    if(!fname) return;
    p = strchr(fname, ':'); // the first ':' is allowed
    if(p) fname = p + 1;
    for(p = fname; *p && (*p != '\r') && (*p != '\n'); p++) {
        if(strchr("?%*:|\"<>", *p)) {   // invalid filename chars not supported by the most used file systems
            *p = '_';
        }
    }
    if(*p) *p = 0;
    for(p--; (p >= fname) && ((*p == ' ') || (*p == '.')); p--) *p = 0;   // remove final spaces and dots
}



u64 mydown_http2file(int *keep_alive, int timeout, u8 *host, u16 port, u8 *user, u8 *pass, u8 *referer, u8 *useragent, u8 *cookie, u8 *more_http, int verbose, u8 *getstr, FILE *fd, u8 *filename, int showhead, int onlyifdiff, int resume, u64 from, u64 tot, u64 *filesize, u8 **filedata, int *ret_code, int onflyunzip, u8 *content, u64 contentsize, u8 *get, u8 *proxy, u16 proxy_port, mydown_options *opt) {
#ifndef WIN32
    //struct  timeb   timex;
    struct  timeval timex;
#endif
    static struct linger ling = {1,1};
    z_stream    z;
    FILE    *oldfd          = fd;
    struct  addrinfo    peer;
    u8      ai_addr[MYDOWN_AI_ADDR_MAX];    // necessary
    struct  stat    xstat;
    time_t  oldtime         = 0,
            newtime         = 0;
    u64     ret             = 0,
            httpret         = 0,
            vishttpret      = 0,
            fsize           = 0,
            filedatasz      = 0,
            *recv_bytes     = NULL;
    int     sd              = 0,
            t               = 0,
            cnt             = 0,
            err             = 0,
            len             = 0,
            mydownlib_flags = 0,
            code            = 0,
            b64len          = 0,
            httpcompress    = 0,
            httpgzip        = 0,
            httpdeflate     = 0,
            httpz           = 0,
            chunked         = 0,
            chunkedsize     = 0,
            chunkedlen      = 0,
            wbits           = 0,
            zbufflen        = 0,
            httpskipbytes   = 0,
            ask_overwrite   = 0,
            query_len       = 0,
            global_keep_alive = 0,
            retry           = 0,
            *keep_alive_bck = NULL;
    u8      tmp[32]         = "",
            *buff           = NULL,
            *query          = NULL,
            *data           = NULL,
            *header_limit   = NULL,
            *s              = NULL,
            *userpass       = NULL,
            *b64            = NULL,
            *conttype       = NULL,
            *contlen        = NULL,
            *contdisp       = NULL,
            *icyname        = NULL,
            *transenc       = NULL,
            *contenc        = NULL,
            *connection     = NULL,
            *location       = NULL,
            *filedatatmp    = NULL,
            *zbuff          = NULL,
            *ztmp           = NULL,
            *chunkedbuff    = NULL,
            *chunkedtmp     = NULL,
            *filenamemalloc = NULL,
            *filename_tmp1  = NULL,
            *filename_tmp2  = NULL,
            *remote_filename= NULL,
            httpgzip_flags  = 0;

    int     dossl           = 0;
    SSL     *ssl_sd         = NULL;
    SSL_CTX *ctx_sd         = NULL;
#ifdef MYDOWN_SSL
    static int  ssl_loaded  = 0;
    SSL_METHOD  *ssl_method = NULL;
#endif

#define MYDOWN_GOTOQUIT { ret = MYDOWN_ERROR; goto quit; }

    mydown_opt_pop

    // lame work-around to avoid to use additional fields
    // basically onlyifdiff is now a bit flag
    mydownlib_flags = onlyifdiff;
    t = mydownlib_flags;
    onlyifdiff = t & 1;         t = (unsigned)t >> 1;
    dossl = t & ((1 << 4) - 1); t = (unsigned)t >> 4;
    ask_overwrite = t & 1;      t = (unsigned)t >> 1;
    global_keep_alive = t & 1;  t = (unsigned)t >> 1;

mydown_retry:
    if(dossl && keep_alive) {
        // long story short, ssl_sd and ctx_sd are stored nowhere due to the old mydown_http2file prototype.
        // the only solution would be to rewrite parts of the source code and the tools that use mydown_http2file.
        // indeed mydown_http2file was the first implementation of the library made for not using the context/options,
        // few arguments in version 0.1 were ok but now it's a problem and there is no plan to break compatibility or
        // further upgrading the library (except for some bugfixes).
        global_keep_alive = 1;
    }
    if(global_keep_alive) {
        keep_alive_bck = keep_alive;
        keep_alive = NULL;
    }

    memset(&peer, 0, sizeof(struct addrinfo));

    if(keep_alive && *keep_alive) {
        sd = *keep_alive;
    } else {
        if(proxy) {
            if(!mydown_resolv(&peer, proxy, proxy_port, ai_addr)) MYDOWN_GOTOQUIT
        } else {
            if(!mydown_resolv(&peer, host, port, ai_addr)) MYDOWN_GOTOQUIT
        }

        if(global_keep_alive) { // take the existent one
            keep_alive = mydown_global_keep_alive(0, &peer, &sd, &ssl_sd, &ctx_sd);
            if(keep_alive_bck) *keep_alive_bck = sd;
        }

        if(verbose > 0) {   // not needed, MYDOWN_VERPRINTF2 does everything
            u8  tmp_addr[(peer.ai_addrlen*2)+1];
            #ifdef WIN32
            if(peer.ai_family == AF_INET) {
                MYDOWN_VERPRINTF2"  connect to %s:%u...", inet_ntoa(((struct sockaddr_in *)peer.ai_addr)->sin_addr), ntohs(((struct sockaddr_in *)peer.ai_addr)->sin_port));
            } else if(peer.ai_family == AF_INET6) {
                tmp_addr[0] = 0;    // this is only a placeholder
                for(t = 0; t < sizeof(((struct sockaddr_in6 *)peer.ai_addr)->sin6_addr); t++) {
                    sprintf(tmp_addr + strlen(tmp_addr), "%02x", ((u8*)&((struct sockaddr_in6 *)peer.ai_addr)->sin6_addr)[t]);
                }
                MYDOWN_VERPRINTF2"  connect to %s:%hu...", tmp_addr, ntohs(((struct sockaddr_in6 *)peer.ai_addr)->sin6_port));
            } else {
                tmp_addr[0] = 0;    // this is only a placeholder
                for(t = 0; t < peer.ai_addrlen; t++) {
                    sprintf(tmp_addr + strlen(tmp_addr), "%02x", ((u8*)peer.ai_addr)[t]);
                }
                MYDOWN_VERPRINTF2"  connect to %s...", tmp_addr);
            }
            #else
            MYDOWN_VERPRINTF2"  connect to %s...", inet_ntop(peer.ai_family, peer.ai_addr, tmp_addr, peer.ai_addrlen));
            #endif
        }

        if(keep_alive && *keep_alive) {
            sd = *keep_alive;
        } else {        // needed to handle global_keep_alive
            sd = socket(peer.ai_family, peer.ai_socktype, peer.ai_protocol);
            if(sd < 0) MYDOWN_GOTOQUIT
            setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
            if(connect(sd, peer.ai_addr, peer.ai_addrlen) < 0) {
                fprintf(stderr, "\nError: connection refused\n");
                MYDOWN_GOTOQUIT
            }

            if(global_keep_alive) {
                keep_alive = mydown_global_keep_alive(1, &peer, &sd, &ssl_sd, &ctx_sd);
                if(keep_alive_bck) *keep_alive_bck = sd;
            }
            if(keep_alive) *keep_alive = sd;
        }

        MYDOWN_VERPRINTF2" socket %d... done\n", (int)sd); // continues from "if(verbose > 0) {"
        
        if(proxy && dossl) {    // requires CONNECT
            query = realloc(query, 64 + strlen(host) + 1 + 6 + 64 + 1);
            len = sprintf(
                query,
                "CONNECT %s:%u HTTP/1.0\r\n"
                "\r\n",
                host, port);
            if(mydown_send(NULL, sd, query, len) != len) MYDOWN_GOTOQUIT
            mydown_free(&query);

            len = 0;
            while((t = mydown_block_recv(NULL, sd, tmp, 1, timeout)) > 0) {
                if((tmp[0] == '\r') || (tmp[0] == '\n')) len++;
                else                                     len = 0;
                if(len >= 4) break;
            }
        }

#ifdef MYDOWN_SSL
        if(dossl) {
            if(!ssl_loaded) {
                SSL_library_init();
                //SSL_load_error_strings();
                ssl_loaded = 1;
            }
            const SSL_METHOD *ssl_method_list[] = {
                // SSLv23_method does the job automatically and it's retro-compatible
                SSLv23_method(),    // in 1.1 it's a define for TLS_method
                /*
                #if OPENSSL_VERSION_NUMBER >= 0x10100000L
                TLS_method(),
                #endif
                # ifndef OPENSSL_NO_TLS1_2_METHOD
                TLSv1_2_method(),
                #endif
                # ifndef OPENSSL_NO_TLS1_1_METHOD
                TLSv1_1_method(),
                #endif
                # ifndef OPENSSL_NO_TLS1_METHOD
                TLSv1_method(),
                #endif
                # ifndef OPENSSL_NO_SSL3_METHOD
                SSLv3_method(),
                #endif
                SSLv23_method(),    // in 1.1 it's a define for TLS_method
                //SSLv2_method(),
                DTLS_method(),
                # ifndef OPENSSL_NO_DTLS1_2_METHOD
                DTLSv1_2_method(),
                #endif
                # ifndef OPENSSL_NO_DTLS1_METHOD
                DTLSv1_method(),
                #endif
                */
                NULL
            };
            ssl_method = NULL;
            for(t = 0; ssl_method_list[t]; t++) {
                if((dossl - 1) == t) {  // dossl is 1 but first element is 0
                    ssl_method = (SSL_METHOD *)ssl_method_list[t];
                    break;
                }
            }
            if(!ssl_method) MYDOWN_GOTOQUIT
            if(!ctx_sd || !ssl_sd) {
                t = 0;
                if(!ctx_sd) ctx_sd = SSL_CTX_new(ssl_method);
                if(!ctx_sd) {
                    t = -1;
                } else {
                    SSL_COMPAT(ctx_sd)
                    if(!ssl_sd) ssl_sd = SSL_new(ctx_sd);
                    #ifdef SSL_set_tlsext_host_name
                    SSL_set_tlsext_host_name(ssl_sd, host);
                    #endif
                    SSL_set_fd(ssl_sd, sd);
                    if(SSL_connect(ssl_sd) <= 0) {
                        //mydown_free_sock(global_keep_alive, &peer, NULL, &ssl_sd, NULL);  // return < 0
                        t = -1;    // only 1 is successful
                    }
                }
                if(t < 0) {
                    dossl++;
                    MYDOWN_VERPRINTF2"  try next ssl method %u\n", dossl);
                    MYDOWN_RETRY_CONNECTION
                    goto quit;
                }
            }
        }
#endif
    }

    if(global_keep_alive) { // necessary
        keep_alive = mydown_global_keep_alive(1, &peer, &sd, &ssl_sd, &ctx_sd);
        if(keep_alive_bck) *keep_alive_bck = sd;
    }

    if(user && pass) {
        userpass = realloc(userpass, strlen(user) + 1 + strlen(pass) + 1);
        if(!userpass) MYDOWN_GOTOQUIT
        b64len = sprintf(userpass, "%s:%s", user, pass);
        b64 = mydown_base64_encode(userpass, &b64len);
    }

    if(!get) get = "GET";
    if(content && !contentsize) contentsize = strlen(content);

    query_len =
        400                                       + // my format strings and HTTP parameters
        strlen(host)                              + // http proxy
        6                                         +
        strlen(get)                               +
        strlen(getstr)                            +
        strlen(host)                              +
        6                                         +
        ((from || tot)   ? 20 + 20           : 0) +
        (b64             ? strlen(b64)       : 0) +
        (referer         ? strlen(referer)   : 0) +
        (useragent       ? strlen(useragent) : 0) +
        (cookie          ? strlen(cookie)    : 0) +
        (more_http       ? strlen(more_http) : 0);

    query = realloc(query, query_len + 1);
    if(!query) MYDOWN_GOTOQUIT

    if(port == 80) tmp[0] = 0;
    else           sprintf(tmp, ":%u", port);

#define MYDOWN_HTTP_APPEND  len += sprintf(query + len,

    len = 0;
    MYDOWN_HTTP_APPEND "%s ", get);
    if(proxy && !dossl) MYDOWN_HTTP_APPEND "http://%s%s", host, tmp);
    MYDOWN_HTTP_APPEND "/%s HTTP/1.1\r\n", getstr);
    //MYDOWN_HTTP_APPEND "Host: %s%s\r\n", host, tmp);  // some websites don't like it
    MYDOWN_HTTP_APPEND "Host: %s\r\n", host);
    MYDOWN_HTTP_APPEND "Connection: %s\r\n", keep_alive ? "keep-alive" : "close");
    MYDOWN_HTTP_APPEND "Pragma: no-cache\r\n");        // useful?
    MYDOWN_HTTP_APPEND "Cache-Control: no-cache\r\n"); // useful?

    if(from || tot) {
        MYDOWN_HTTP_APPEND "Range: bytes=");
        if(from != -1LL) MYDOWN_HTTP_APPEND "%"PRIu64, from);
        MYDOWN_HTTP_APPEND "-");
        if(tot > 0)      MYDOWN_HTTP_APPEND "%"PRIu64, (tot - 1) + ((from == -1LL) ? 0 : from));
        MYDOWN_HTTP_APPEND "\r\n");
    } else {
        // unfortunately doesn't seem possible to use chunks with Range, example:
        // mydown -m 8 http://ftp.hp.com/pub/softpaq/sp50501-51000/sp50693.exe
        if(onlyifdiff || (onflyunzip < 0) || (showhead == 2)) {
            // disable Accept-Encoding
        } else {
            MYDOWN_HTTP_APPEND "Accept-Encoding: deflate,gzip,x-gzip,compress,x-compress\r\n");    // x-compress needs unlzw???
        }
    }
    if(b64) {
        MYDOWN_HTTP_APPEND "Authorization: Basic %s\r\n", b64);
    }
    if(referer) {
        MYDOWN_HTTP_APPEND "Referer: %s\r\n", referer);
    }
    if(useragent) {
        MYDOWN_HTTP_APPEND "User-Agent: %s\r\n", useragent);
    }
    if(cookie) {
        MYDOWN_HTTP_APPEND "Cookie: %s\r\n", cookie);
    }
    if(more_http) {
        MYDOWN_HTTP_APPEND "%s", more_http);
        if(query[len - 1] == '\r') {
            MYDOWN_HTTP_APPEND "\n");
        } else if(query[len - 1] != '\n') {
            MYDOWN_HTTP_APPEND "\r\n");
        }
    }
    if(content) {
        MYDOWN_HTTP_APPEND "Content-length: %"PRIu64"\r\n", contentsize);
    }
    MYDOWN_HTTP_APPEND "\r\n");

#undef MYDOWN_HTTP_APPEND

    if(len > query_len) {
        fprintf(stderr, "\nError: mydownlib len (%d) > query_len (%d)\n", len, query_len);
        MYDOWN_GOTOQUIT
    }

    if((verbose > 0) && (verbose & 4)) fputs(query, stdout);
    if(mydown_send(ssl_sd, sd, query, len) != len) MYDOWN_RETRY_CONNECTION
    mydown_free(&query);
    if(content) {
        if(mydown_send(ssl_sd, sd, content, contentsize) != contentsize) MYDOWN_GOTOQUIT
    }

    // buff has a fixed size, that's all the memory we are going to use
    buff = realloc(buff, MYDOWN_BUFFSZ + 1);
    if(!buff) MYDOWN_GOTOQUIT
    buff[0] = 0;
    data = buff;
    len  = MYDOWN_BUFFSZ;
    while((t = mydown_block_recv(ssl_sd, sd, data, len, timeout)) > 0) {
        data += t;
        len  -= t;
        *data = 0;

        header_limit = strstr(buff, "\r\n\r\n");
        if(header_limit) {
            *header_limit = 0;
            header_limit += 4;
            break;
        }
        header_limit = strstr(buff, "\n\n");
        if(header_limit) {
            *header_limit = 0;
            header_limit += 2;
            break;
        }
    }
    
    // probably the server closed the connection
    if(t < 0) MYDOWN_RETRY_CONNECTION
    if(data == buff) MYDOWN_RETRY_CONNECTION

    if(!header_limit) {    // no header received
        header_limit = buff;
    } else {
        if(showhead == 1) {
            if(filedata) {
                *filedata = realloc(*filedata, strlen(buff) + 1);
                if(!*filedata) MYDOWN_GOTOQUIT
                strcpy(*filedata, buff);
            }
            MYDOWN_VERPRINTF"\n%s", buff);
            goto quit;
        }
        if((verbose > 0) && (verbose & 2)) fprintf(stderr, "\n%s", buff);

        mydown_scanhead(buff, header_limit - buff,
            "content-length",       &contlen,
            "content-type",         &conttype,
            "content-disposition",  &contdisp,
            "icy-name",             &icyname,
            "transfer-encoding",    &transenc,
            "content-encoding",     &contenc,
            "Set-Cookie",           &cookie,
            "Connection",           &connection,
            "location",             &location,
            NULL,                   NULL);

#ifdef MYDOWN_mydown_global_cookie
        if(cookie) mydown_global_cookie = cookie;
#endif

        s = strchr(buff, ' ');
        if(s) {
            code = atoi(s + 1);

            if((code / 100) == 3) {
                if(!location) {
                    fprintf(stderr, "\nError: remote file is temporary unavailable (%d)\n", code);
                    MYDOWN_GOTOQUIT
                }
                //s = mydown_hex2uri(location); // not needed
                //if(s) {
                    //free(location);
                    //location = s;
                //}
                // the problem is that the not all the webservers follow a standard
                // so happens that you find something Location: example.com/path
                // or Location: /path or Location: path and so on...
                // that's why I need to guess it
                MYDOWN_VERPRINTF2"\n- redirect: %s\n", location);
                t = 0;
                if(!location[0]) goto quit;
                else if(location[0] == '/') t = 0;
                else if(strstr(location, "://")) t = 1;
                else if(strstr(location, ":\\\\")) t = 1;
                if(!t) {
                    if(!location[1]) goto quit; // "/" returns to the index
                    getstr = location;
                    getstr = mydown_uri2hex(getstr);
                    if(!getstr) getstr = strdup("");
                    fprintf(stderr, "\n- redirect to URI on the same host: %s\n", location); // better to show it
                } else {
                    dossl = mydown_get_host(location, &host, &port, &getstr, &user, &pass, verbose);
#ifdef MYDOWN_SSL
                    if(!dossl) {    // mandatory or will not work, for example multiple redirects like: http->https->http
                        mydown_free_sock(global_keep_alive, &peer, NULL, &ssl_sd, &ctx_sd); //keep_alive);
                    }
#endif
                }
                if(!host || !host[0] || !port) {
                    fprintf(stderr, "\nError: the Location field is invalid (error code %d)\n", code);
                    MYDOWN_GOTOQUIT
                }
                MYDOWN_NEW_CONNECTION
                goto quit;
            }

            if(code == 204) goto quit;  // No Content
            if(code == 101) goto quit;  // Switching protocol is not supported. Used on WebSockets

                // if((code != 200) && (code != 206)) {
            if((code / 100) != 2) {
                fprintf(stderr, "\nError: remote file is temporary unavailable (%d)\n", code);
                //MYDOWN_GOTOQUIT   // giving an error or trying to download the data anyway? commented from 0.3.5 because it may contain info about the error
                if(!ret_code) MYDOWN_GOTOQUIT   // return the error if no HTTP status has been requested since it can't be handled by the caller
            }
        }

        if(connection) {
            if(keep_alive && !stricmp(connection, "close")) *keep_alive = 0;
        }

        if(contlen) {
            s = strchr(contlen, '/');
            if(s) contlen = s + 1;
            //if(_atoi64(contlen) > (u64)0x7fffffff) {
            //    fprintf(stderr, "\nError: large files are not yet supported by mydownlib\n");
            //    MYDOWN_GOTOQUIT
            //}
            if(sscanf(contlen, "%"PRIu64, &fsize) != 1) fsize = 0;   //break;
        }

        if(conttype && onflyunzip) {    // needed or better
            if(mydown_stristr(conttype, "compress"))   httpcompress  = 1;
            if(mydown_stristr(conttype, "gzip"))       httpgzip      = 1;
            if(!onflyunzip) if(mydown_stristr(conttype, "x-gzip")) httpgzip = 0;   // work-around
            if(mydown_stristr(conttype, "deflate"))    httpdeflate   = 1;
        }
        if(contenc) {
            if(mydown_stristr(contenc,  "compress"))   httpcompress  = 1;
            if(mydown_stristr(contenc,  "gzip"))       httpgzip      = 1;
            if(!onflyunzip) if(mydown_stristr(contenc,  "x-gzip")) httpgzip = 0;   // work-around
            if(mydown_stristr(contenc,  "deflate"))    httpdeflate   = 1;
        }

        if(!contdisp && icyname) contdisp = icyname;

        if(transenc && mydown_stristr(transenc, "chunked")) chunked = 1;

        if(filesize) *filesize = fsize;
    }

        if(filename && ((resume == 2) || (resume == 3))) {
            fd = (void *)filename;
            if(!fd) {
                fprintf(stderr, "\nError: no FILE used with resume %d\n", resume);
                MYDOWN_GOTOQUIT
            }
            filename = "."; // this instruction is useless, I have added it only to skip the checks below
        }

            if(contdisp) {
                s = (u8 *)mydown_stristr(contdisp, "filename=");
                if(!s) s = (u8 *)mydown_stristr(contdisp, "filename*=");
                if(!s) s = (u8 *)mydown_stristr(contdisp, "file=");
                if(!s) s = (u8 *)mydown_stristr(contdisp, "file*=");
                if(s) {
                    s = strchr(s, '=') + 1;
                } else {
                    s = contdisp;
                }
                while(*s && ((*s == '\"') || (*s == ' ') || (*s == '\t'))) s++;
                remote_filename = mydown_hex2uri(s);
                if(remote_filename) filenamemalloc = remote_filename; // needed for freeing it later!
                if(remote_filename && remote_filename[0]) {
                    for(s = remote_filename; *s && !strchr("\\/;:\"&?", *s); s++);  // \r \n are don't exist, it's a line
                    for(s--; (s >= remote_filename) && ((*s == ' ') || (*s == '\t')); s--);
                    s[1] = 0;
                }
            } else {
                remote_filename = mydown_hex2uri(getstr);
                if(remote_filename) filenamemalloc = remote_filename; // needed for freeing it later!
                if(remote_filename && remote_filename[0]) {
                    for(s = remote_filename + strlen(remote_filename) - 1; (s >= remote_filename) && !strchr("/\\:&?=", *s); s--);
                    remote_filename = s + 1;
                }
            }

            if(remote_filename) {                          // last useless check to avoid directory traversal
                s = strrchr(remote_filename, ':');     if(s) remote_filename = s + 1;
                s = strrchr(remote_filename, '\\');    if(s) remote_filename = s + 1;
                s = strrchr(remote_filename, '/');     if(s) remote_filename = s + 1;
            }

        if(!filename || !filename[0]) filename = remote_filename;

        if(!filename || !filename[0]) {
            filename = mydown_tmpnam();
            if(filename) filenamemalloc = filename; // needed for freeing it later!
            //MYDOWN_VERPRINTF"\nError: no filename retrieved, you must specify an output filename\n\n");
            //MYDOWN_GOTOQUIT
        }

        // automatic gzip decompression on the fly
        if(onflyunzip) {
            s = remote_filename ? strrchr(remote_filename, '.') : NULL;
            if(!(s && !stricmp(s, ".gz"))) s = filename ? strrchr(filename, '.') : NULL;
            if(s && !stricmp(s, ".gz")) {
                httpgzip = 1;
                *s = 0;
            }
        }

        if(showhead == 2) {
            if(filedata) {
                *filedata = realloc(*filedata, strlen(filename) + 1);
                if(!*filedata) MYDOWN_GOTOQUIT
                strcpy(*filedata, filename);
            }
            ret = fsize;
            goto quit;
        }

        if(!filedata && !fd) {
            if(!strcmp(filename, "-")) {
                fd = stdout;
                MYDOWN_VERPRINTF"  file   %s\n", "stdout");
            } else {
                for(;;) {
                    // lame but needed to avoid problems with mydown_filename_clean
                    filename_tmp1 = realloc(filename_tmp1, strlen(filename) + (1 + 8) + 1 + 1);   // reserve space for auto renaming "_%08x."
                    memmove(filename_tmp1, filename, strlen(filename) + 1);
                    filename_tmp2 = filename;
                    filename = filename_tmp1;

                    mydown_filename_clean(filename);
                    MYDOWN_VERPRINTF"  file   %s\n", filename);
                    err = stat(filename, &xstat);
                    if(onlyifdiff && !err && (xstat.st_size == fsize)) {
                        MYDOWN_VERPRINTF"  the remote file has the same size of the local one, skip\n");
                        MYDOWN_GOTOQUIT
                    }

                    if(ask_overwrite) {
                        t = 0;
                        for(cnt = 0;;) {
                            if(mydown_check_overwrite(filename, 0, resume, &t) >= 0) break;
                            if(mydown_force_rename) {
                                mydown_rename_auto(++cnt, filename);
                            } else {
                                MYDOWN_GOTOQUIT
                            }
                        }
                        if(t || cnt) { // if there was an used input retry the connection
                            // do not use MYDOWN_RETRY_CONNECTION, risk of memory leak (filename_tmp1)
                            // necessary or it will ask to overwrite forever
                            mydownlib_flags &= ~MYDOWN_OPTION_ASK_OVERWRITE;            // useless
                            onlyifdiff      &= ~MYDOWN_OPTION_ASK_OVERWRITE;            // doesn't work when opt is used
                            if(opt) opt->onlyifdiff &= ~MYDOWN_OPTION_ASK_OVERWRITE;    // necessary
                            MYDOWN_NEW_CONNECTION
                            goto quit;
                        }
                    }
                    
                    mydown_create_dir(filename);
                    if((err < 0) || !resume) {  // file doesn't exist or must not resume
                        fd = fopen(filename, "wb");
                        if(!fd) goto mydown_fd_error;
                    } else {
                        fd = fopen(filename, "ab");
                        if(!fd) goto mydown_fd_error;
                        from = xstat.st_size;
                        MYDOWN_VERPRINTF2"  resume %"PRIu64"\n", from);
                        mydown_free(&filename_tmp1);
                        filename = filename_tmp2;
                        MYDOWN_RETRY_CONNECTION
                    }
                    
                    mydown_fd_error:
                    mydown_free(&filename_tmp1);
                    filename = filename_tmp2;
                    if(fd) break;   // ok
                    if(!ask_overwrite) MYDOWN_GOTOQUIT // don't interrupt the download if the user wants no prompting
                    fprintf(stderr, 
                        "\n"
                        "  Impossible to create the file:\n"
                        "  %s\n"
                        "  Please type another name or delete the existent file/folder and press ENTER:\n"
                        "  ", filename);
                    t = strlen(filename) + 256;
                    s = malloc(t + 1);
                    if(!s) MYDOWN_GOTOQUIT
                    if(!fgets(s, t, stdin)) MYDOWN_GOTOQUIT
                    for(t = 0; s[t] && (s[t] != '\n') && (s[t] != '\r'); t++);
                    s[t] = 0;
                    if(s[0]) {
                        filename_tmp1 = s;
                        filename = filename_tmp1;
                    } else {
                        mydown_free(&s);
                    }
                }
            }
        }

    len = data - header_limit;
    memmove(buff, header_limit, len);

    httpz = 1;
    if(onflyunzip < 0) httpz = 0;   // disabled forcely!
    if(httpcompress) {
        MYDOWN_VERPRINTF2"  compression: compress\n");
        wbits =  15;
    } else if(httpgzip) {
        MYDOWN_VERPRINTF2"  compression: gzip\n");
        wbits = -15;
    } else if(httpdeflate) {
        MYDOWN_VERPRINTF2"  compression: deflate\n");
        wbits = -15;
    } else {
        httpz = 0;
    }
    if(httpz) {
        memset(&z, 0, sizeof(z_stream));
        if(inflateInit2(&z, wbits)) MYDOWN_GOTOQUIT

        zbufflen = MYDOWN_BUFFSZ * 4;   // the buffer is automatically resized
        zbuff    = realloc(zbuff, zbufflen + 1);
        if(!zbuff) MYDOWN_GOTOQUIT
    }

    if(verbose > 0) {
        fprintf(stderr, "\n");
        if(fsize) fprintf(stderr, "    ");
        fprintf(stderr, " | downloaded   | kilobytes/second\n");
        if(fsize) fprintf(stderr, "----");
        fprintf(stderr, "-/--------------/-----------------\n");
    }

    if(verbose >= 0) {
        MYDOWN_TEMPOZ(oldtime);
        oldtime -= MYDOWN_VISDELAY;
    }

    if(filedata) {  // first allocation
        filedatasz  = fsize;
        if(filedatasz == -1LL) MYDOWN_GOTOQUIT
        filedatatmp = *filedata;
        filedatatmp = realloc(filedatatmp, filedatasz + 1); // was calloc
        if(!filedatatmp) MYDOWN_GOTOQUIT
        //filedatatmp[filedatasz] = 0;
        memset(filedatatmp, 0, filedatasz + 1);
    }

    if(chunked) chunkedsize = len;

    do {
        // here it's the only good location for "Content-Length: 0" without requiring additional testing
        if(contlen && !fsize) goto quit;
redo:
        httpret += len;

        if(chunked) {
            for(;;) {
                chunkedsize = mydown_chunked_skip(buff, chunkedsize);

                err = mydown_sscanf_hex(buff, chunkedsize);
                if(err > 0) break;
                if(!err) {
                    chunkedsize = mydown_chunked_skip(buff, chunkedsize);
                    break;
                }

                t = mydown_block_recv(ssl_sd, sd, buff + chunkedsize, MYDOWN_BUFFSZ - chunkedsize, timeout);
                if(t <= 0) MYDOWN_GOTOQUIT
                chunkedsize += t;
                if(chunkedsize > MYDOWN_BUFFSZ) MYDOWN_GOTOQUIT
            }

            chunkedlen = err;
            if(!chunkedlen) break;

            //if(chunkedbuff) free(chunkedbuff);
            //chunkedbuff = calloc(chunkedlen, 1);
            chunkedbuff = realloc(chunkedbuff, chunkedlen + 1);
            if(!chunkedbuff) MYDOWN_GOTOQUIT
            memset(chunkedbuff, 0, chunkedlen);

            s = (u8 *)strchr(buff, '\n');
            if(!s) MYDOWN_GOTOQUIT
            err = (s + 1) - buff;
            chunkedsize -= err;
            memmove(buff, buff + err, chunkedsize);

            if(chunkedlen < chunkedsize) {      // we have more data than how much we need
                memcpy(chunkedbuff, buff, chunkedlen);
                chunkedsize -= chunkedlen;
                memmove(buff, buff + chunkedlen, chunkedsize);
            } else {                            // we have only part of the needed data
                memcpy(chunkedbuff, buff, chunkedsize);
                for(len = chunkedsize; len < chunkedlen; len += t) {
                    t = mydown_block_recv(ssl_sd, sd, chunkedbuff + len, chunkedlen - len, timeout);
                    if(t <= 0) MYDOWN_GOTOQUIT
                }
                chunkedsize = 0;
            }

            chunkedtmp  = buff;
            buff        = chunkedbuff;
            len         = chunkedlen;
        }

            /* DECOMPRESSION */

        if(httpz) {
            if(httpgzip && !ret) {
                // Google sends the gzip header in 9 sequences of 1-byte long chunks...
                s = buff;
                if(httpgzip > 0) {
                    for(;;) {
                        for(t = len - (s - buff); t > 0; t--) {
                            if(httpskipbytes <= 0) break;
                            httpskipbytes--;
                            s++;
                        }
                        if(t <= 0) break;   // remaining bytes

                        if(httpgzip == 1) {
                            httpskipbytes = 0;
                            if(*s++ != 0x1f) { httpgzip = -1LL; s = buff; break; }  // raw deflate
                            httpgzip++;
                        } else if(httpgzip == 2) {
                            httpskipbytes = 0;
                            if(*s++ != 0x8b) { httpgzip = -1LL; s = buff; break; }  // raw deflate
                            httpgzip++;
                        } else if(httpgzip == 3) {
                            httpskipbytes = 0;
                            s++;                        // CM is usually 8, no need to check it
                            httpgzip++;
                        } else if(httpgzip == 4) {
                            httpgzip_flags = *s++;      // flags
                            httpskipbytes = 4 + 1 + 1;  // mtime + xfl + os
                            httpgzip++;
                        } else if(httpgzip == 5) {      // xfl flag
                            httpskipbytes = 0;
                            if(httpgzip_flags & 4) {
                                if(t >= 2) {            // not 100% correct but it's never used
                                    httpskipbytes = s[0] | (s[1] << 8);
                                    s += 2;
                                }
                            }
                            httpgzip++;
                        } else if(httpgzip == 6) {
                            httpskipbytes = 0;
                            if(httpgzip_flags & 8) {    // name
                                while((s - buff) < len) {
                                    if(!*s++) {
                                        httpgzip++;
                                        break;
                                    }
                                }
                            } else {
                                httpgzip++;
                            }
                        } else if(httpgzip == 7) {
                            httpskipbytes = 0;
                            if(httpgzip_flags & 16) {   // comment
                                while((s - buff) < len) {
                                    if(!*s++) {
                                        httpgzip++;
                                        break;
                                    }
                                }
                            } else {
                                httpgzip++;
                            }
                        } else if(httpgzip == 8) {
                            httpskipbytes = 0;
                            if(httpgzip_flags & 2) {    // crc
                                httpskipbytes = 2;
                            }
                            httpgzip++;
                        } else {
                            httpgzip = -1LL;            // raw compressed data
                            break;
                        }
                    }
                }
                t = s - buff;
                if(httpgzip > 0) {  // skip decompression
                    len = 0;
                    t   = 0;
                }
            } else {
                t = 0;  // just in case there are other compression algorithms that require an header
            }
            len -= t;
            if(len <= 0) {
                len = 0;    // useless, mydown_unzip works even if len is <= 0
            } else {
                len = mydown_unzip(&z, buff + t, len, &zbuff, &zbufflen);
                if(len < 0) MYDOWN_GOTOQUIT
            }
            ztmp = buff;
            buff = zbuff;
        }

            /* UPDATE THE AMOUNT OF UNCOMPRESSED BYTES DOWNLOADED */
            // ret is the total size of the data we have downloaded (uncompressed)
            // httpret is the total size of the data we have downloaded from the server
            // len is the size of the current block of data we have downloaded (uncompressed)

        ret += len;

            /* WRITE THE DATA INTO FILE OR MEMORY */

        if(tot && (ret > tot)) {
            len = tot - (ret - len);
            ret = tot;
        }

        if(filedata) {
            if(filedatasz < ret) {
                filedatasz  = ret;
                if(filedatasz == -1LL) MYDOWN_GOTOQUIT
                filedatatmp = realloc(filedatatmp, filedatasz + 1);
                if(!filedatatmp) MYDOWN_GOTOQUIT
                filedatatmp[filedatasz] = 0;
            }
            memcpy(filedatatmp + ret - len, buff, len);
            filedatatmp[ret] = 0;
        } else if(fd) {
            if(fwrite(buff, 1, len, fd) != len) {
                fprintf(stderr, "\nError: I/O error. Probably your disk is full or the file is write protected\n");
                MYDOWN_GOTOQUIT
            }
            //fflush(fd);   // disabled for some reason that I don't remember
        }
        
        // do not initialize them because this is a recursive function
        // recv_bytes is the official parameter
        // ret_code is a work-around used for backward compatibility, don't use it
        if(recv_bytes) *recv_bytes += len;

            /* VISUALIZATION */

        if(verbose >= 0) {
            MYDOWN_TEMPOZ(newtime);
            if((newtime - oldtime) >= MYDOWN_VISDELAY) {
                mydown_showstatus(fsize, httpret, vishttpret, (int)(newtime - oldtime));
                oldtime = newtime;
                vishttpret = httpret;
            }
        }

            /* FREE, EXCHANGE OR OTHER STUFF */

        if(httpz) {
            zbuff = buff;
            buff  = ztmp;
        }
        if(chunked) {
            chunkedbuff = buff;
            buff        = chunkedtmp;
            httpret    += len; // lame work-around, sorry
            len         = 0;
            goto redo;
        }

            /* FSIZE CHECK */

        if(tot && (ret == tot)) break;
        if(fsize) {
            if(httpret >= fsize) break;
        }

            /* READ NEW DATA FROM THE STREAM */

    } while((len = mydown_block_recv(ssl_sd, sd, buff, MYDOWN_BUFFSZ, timeout)) > 0);

    if(verbose >= 0) {
        MYDOWN_TEMPOZ(newtime);
        mydown_showstatus(fsize, httpret, vishttpret, (int)(newtime - oldtime));
    }

    if(fsize && (len < 0)) MYDOWN_GOTOQUIT

    if(filedata) {
        *filedata = filedatatmp;    // reassign
    }

quit:
    if(httpz) {
        if(zbuff) inflateEnd(&z);
        if(zbuff != buff) mydown_free(&zbuff);
    }
    if(chunkedbuff != buff) mydown_free(&chunkedbuff);
    mydown_free(&userpass);
    mydown_free(&b64);
    mydown_free(&buff);
    mydown_free(&filenamemalloc);
    mydown_free(&query);
    if(ret_code) *ret_code = code;
    if(sd && !keep_alive) mydown_free_sock(global_keep_alive, &peer, &sd, &ssl_sd, &ctx_sd);
    MYDOWN_VERPRINTF"\n");
    if(ret == MYDOWN_ERROR) mydown_free_sock(global_keep_alive, &peer, keep_alive, &ssl_sd, &ctx_sd);
    if(filename && ((resume == 2) || (resume == 3))) {
        // do nothing
    } else {
        if(oldfd) {
            // do nothing
        } else if(fd && (fd != stdout)) {
            fclose(fd);
        }
    }
    mydown_opt_push
    return(ret);

#undef MYDOWN_GOTOQUIT
}



struct addrinfo *mydown_ai_addr_copy(struct addrinfo *dst, struct addrinfo *src, void *ai_addr) {
    if(dst && src) {
        memcpy(dst, src, sizeof(struct addrinfo));
        if(ai_addr) {
            if(dst->ai_addrlen > MYDOWN_AI_ADDR_MAX) dst->ai_addrlen = MYDOWN_AI_ADDR_MAX;  // ???
        } else {
            ai_addr = malloc(dst->ai_addrlen);  // yes, it's a memory leak
        }
        memcpy(ai_addr, dst->ai_addr, dst->ai_addrlen);
        dst->ai_addr = (void *)ai_addr;
    }
    return dst;
}



int *mydown_global_keep_alive(int save_info, struct addrinfo *peer, int *sd, SSL **ssl_sd, SSL_CTX **ctx_sd) {
    // the allocation must be fixed (no realloc) so I use a linked list
    typedef struct {
        struct addrinfo  peer;
        u8      ai_addr[MYDOWN_AI_ADDR_MAX];
        int     sd;
        void    *next;
        SSL     *ssl_sd;
        SSL_CTX *ctx_sd;
    } peer_db_t;
    static peer_db_t    *peer_db = NULL;
    peer_db_t   *p      = NULL,
                *last   = NULL;

    if(!peer) {
        //return NULL;  // let's try to find the entry by using the sockets
        for(p = peer_db; p; p = (peer_db_t *)p->next) {
            last = p;
            if(sd     && *sd     && (p->sd     == *sd))     break;
            if(ssl_sd && *ssl_sd && (p->ssl_sd == *ssl_sd)) break;
            if(ctx_sd && *ctx_sd && (p->ctx_sd == *ctx_sd)) break;
        }
        //if(!p) p = last; // using the last one?
    } else {
        for(p = peer_db; p; p = (peer_db_t *)p->next) {
            last = p;
            //NO, this is wrong! if(!memcmp(&p->peer, peer, sizeof(struct addrinfo))) break;
            if(p->peer.ai_family   != peer->ai_family)  continue;
            if(p->peer.ai_socktype != peer->ai_socktype) continue;  // useless
            if(p->peer.ai_protocol != peer->ai_protocol) continue;  // useless
            if(p->peer.ai_addrlen  != peer->ai_addrlen) continue;
            if(!memcmp(p->peer.ai_addr, peer->ai_addr, peer->ai_addrlen)) break;
        }
    }
    if(!p) {
        if(!save_info) return NULL;    // search failed
        if(save_info) { // nothing useful to add
            if(!sd && !ssl_sd && !ctx_sd ) return NULL;
            if((sd && !*sd) && (ssl_sd && !*ssl_sd) && (ctx_sd && !*ctx_sd)) return NULL;
        }
        p = calloc(sizeof(peer_db_t), 1);
        if(!p) return NULL;
        if(last) last->next = p;
        else     peer_db    = p;
        if(peer) mydown_ai_addr_copy(&p->peer, peer, p->ai_addr);
    }
    if(save_info) {
        if(sd)     p->sd     = *sd;
        if(ssl_sd) p->ssl_sd = *ssl_sd;
        if(ctx_sd) p->ctx_sd = *ctx_sd;
    } else {
        if(sd)     *sd       = p->sd;
        if(ssl_sd) *ssl_sd   = p->ssl_sd;
        if(ctx_sd) *ctx_sd   = p->ctx_sd;
    }
    return &(p->sd);
}



void mydown_free_sock(int global_keep_alive, struct addrinfo *peer, int *sd, SSL **ssl_sd, SSL_CTX **ctx_sd) {
    // note that ssl_sd and ctx_sd exist only in http2file so if the socket is saved/reused
    // they get lost... yeah it's lame but it's only a work-around
    if(ssl_sd && *ssl_sd) {
        SSL_shutdown(*ssl_sd);
        SSL_free(*ssl_sd);
        *ssl_sd = NULL;
    }

    if(ctx_sd && *ctx_sd) {
        SSL_CTX_free(*ctx_sd);
        *ctx_sd = NULL;
    }

    if(sd && *sd) {
        //MYDOWN_VERPRINTF2"  close connection on socket %d\n", (int)*sd);
        close(*sd);
        *sd = 0;
    }
    if(global_keep_alive) mydown_global_keep_alive(1, peer, sd, ssl_sd, ctx_sd);
}



void mydown_free(u8 **buff) {
    if(!buff || !*buff) return;
    free(*buff);
    *buff = NULL;
}



int mydown_chunked_skip(u8 *buff, int chunkedsize) {
    int     t;

    if(!buff) return(0);
    for(t = 0; t < chunkedsize; t++) {
        if((buff[t] != '\r') && (buff[t] != '\n')) break;
    }
    if(t) {
        chunkedsize -= t;
        memmove(buff, buff + t, chunkedsize);
    }
    return(chunkedsize);
}



int mydown_unzip(z_stream *z, u8 *in, int inlen, u8 **outx, int *outxlen) {
    int     zerr,
            outsz;
    u8      *out;

    if(!in) return(0);
    if(inlen <= 0) return(0);

    out     = *outx;
    outsz   = *outxlen;

    z->next_in   = in;
    z->avail_in  = inlen;

    z->total_out = 0;
    for(;;) {
        z->next_out  = out   + z->total_out;
        z->avail_out = outsz - z->total_out;

        zerr = inflate(z, Z_NO_FLUSH);
        if(zerr == Z_STREAM_END) break;
        if((zerr != Z_OK) && (zerr != Z_BUF_ERROR)) {
            fprintf(stderr, "\nError: zlib error %d\n", zerr);
            z->total_out = MYDOWN_ERROR;
            break;
        }

        if(!z->avail_in) break;

        outsz += (inlen << 1);      // inlen * 2 should be enough each time
        out = realloc(out, outsz);
        if(!out) {
            outsz       = 0;
            z->total_out = MYDOWN_ERROR;
            break;
        }
    }

    *outx    = out;
    *outxlen = outsz;
    return(z->total_out);
}



u8 *mydown_tmpnam(void) {
    FILE    *fd;
    int     i;
    u8      *ret;

    ret = malloc(32 + 1);
    for(i = 1; ; i++) {
        sprintf(ret, "%u.myd", i);  // enough small for any OS (8.3 too)
        fd = fopen(ret, "rb");      // check real existence of the file
        if(!fd) break;
        fclose(fd);
    }
    return(ret);
}



int mydown_sscanf_hex(u8 *data, int datalen) {
    int     i,
            ret;

    if(!data) return MYDOWN_ERROR;
    for(i = 0; i < datalen; i++) {
        if((data[i] == '\r') || (data[i] == '\n')) break;
    }
    if(i == datalen) return MYDOWN_ERROR;

    if(sscanf(data, "%x", &ret) != 1) return MYDOWN_ERROR;
    return(ret);
}



int mydown_timeout(SSL *ssl_sd, int sd, int secs) {
    struct  timeval tout;
    fd_set  fdr;
    int     err;

    if(secs > 0) {
#ifdef MYDOWN_SSL
        if(ssl_sd) {
            // just a work-around since the solution is using BIO
            if(SSL_pending(ssl_sd) > 0) return 0;
            // return 0;    // would be good to remove timeout on SSL without BIO
        }
#endif
        tout.tv_sec  = secs;
        tout.tv_usec = 0;
        FD_ZERO(&fdr);
        FD_SET(sd, &fdr);
        err = select(sd + 1, &fdr, NULL, NULL, &tout);
        if(err < 0) return MYDOWN_ERROR; //std_err();
        if(!err) return MYDOWN_ERROR;
    }
    return 0;
}



int mydown_block_recv(SSL *ssl_sd, int sd, u8 *data, int len, int timeout) {
    if(!timeout) timeout = MYDOWN_MAXTIMEOUT;
    if(mydown_timeout(ssl_sd, sd, timeout) < 0) return MYDOWN_ERROR;
    return(mydown_recv(ssl_sd, sd, data, len));
}



void mydown_showstatus(u64 fsize, u64 ret, u64 oldret, int timediff) {
    int     vis;

    if(fsize) {
        vis = ((u64)ret * (u64)100) / (u64)fsize;
        fprintf(stderr, "%3u%%", (vis < 100) ? vis : 100);
    }
    fprintf(stderr, "   %12"PRIu64, ret);
    if(ret > 0) {
        if(timediff) fprintf(stderr, "   %-10u", (u32)(((((u64)ret - (u64)oldret) * (u64)1000) / (u64)timediff) / 1024));
    }
    fprintf(stderr, "\r");
}



u8 *mydown_base64_encode(u8 *data, int *size) {
    int     len;
    u8      *buff,
            *p,
            a,
            b,
            c;
    static const u8 base[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
    };

    if(!data) return(NULL);
    if(!size || (*size < 0)) {      // use size -1 for auto text size!
        len = strlen(data);
    } else {
        len = *size;
    }
    buff = calloc(((len / 3) << 2) + 6, 1);
    if(!buff) return(NULL);

    p = buff;
    do {
        a     = data[0];
        b     = data[1];
        c     = data[2];
        *p++  = base[(a >> 2) & 63];
        *p++  = base[(((a &  3) << 4) | ((b >> 4) & 15)) & 63];
        *p++  = base[(((b & 15) << 2) | ((c >> 6) &  3)) & 63];
        *p++  = base[c & 63];
        data += 3;
        len  -= 3;
    } while(len > 0);
    *p = 0;

    for(; len < 0; len++) *(p + len) = '=';

    if(size) *size = p - buff;
    return(buff);
}



struct addrinfo *mydown_resolv(struct addrinfo *ret, char *host, int port, void *ai_addr) {
    mydown_dns_db_t *dns        = NULL;
    struct addrinfo Hints,
                    *AddrInfo   = NULL,
                    *AI         = NULL;
    int     i;
    char    tmp[32];    // for the port

    struct addrinfo tmp_ret;
    if(!ret) ret = &tmp_ret;
    memset(ret, 0, sizeof(struct addrinfo));
    if(ai_addr) memset(ai_addr, 0, sizeof(mydown_dns_db[0].ai_addr));

    if(!host) return NULL;

    // something seen during some tests, non-compliant and may be removed in future
    // http://host.com|127.0.0.1/test
    u8  *p = strchr(host, '|');
    if(p) host = p + 1;

    if(port < 0) port = 80; // just in case

    sprintf(tmp, "%d", port);
    memset(&Hints, 0, sizeof(struct addrinfo));
    Hints.ai_flags      = AI_NUMERICHOST;   // try without name resolution
    Hints.ai_family     = AF_UNSPEC;
    Hints.ai_socktype   = SOCK_STREAM;
    Hints.ai_protocol   = IPPROTO_TCP;
    if(!getaddrinfo(host, tmp, &Hints, &AddrInfo)) AI = AddrInfo;
    if(!AI) {
        // IP failed, try the local cached hostnames, ignore any DNS Client or OS caching system
        for(i = 0; i < mydown_dns_db_max; i++) {
            if(mydown_dns_db[i].host && !stricmp(host, mydown_dns_db[i].host)) {
                AI = &mydown_dns_db[i].peer;
                // we have the IP but the port may be different!
                     if(AI->ai_family == AF_INET)  ((struct sockaddr_in  *)AI->ai_addr)->sin_port  = htons(port);
                else if(AI->ai_family == AF_INET6) ((struct sockaddr_in6 *)AI->ai_addr)->sin6_port = htons(port);
                else AI = NULL; // let's use getaddrinfo again
                break; //goto quit;
            }
        }
    }
    if(!AI) {
        Hints.ai_flags  = 0;
        if(!getaddrinfo(host, tmp, &Hints, &AddrInfo)) AI = AddrInfo;
    }
    // >= Vista
    #ifndef AI_ALL
    #define AI_ALL      0x0100
    #endif
    #ifndef AI_V4MAPPED
    #define AI_V4MAPPED 0x0800
    #endif
    if(!AI) {
        Hints.ai_flags  = AI_ALL;
        if(!getaddrinfo(host, tmp, &Hints, &AddrInfo)) AI = AddrInfo;
    }
    if(!AI) {
        fprintf(stderr, "\nError: Unable to resolve hostname (%s)\n\n", host);
        //goto quit;
    } else {
        // for(; AI != NULL; AI = AI->ai_next) { ... }  // just use the first available

        // remember ai_addr when freeing AddrInfo
        if(Hints.ai_flags & AI_NUMERICHOST) {
            AI = mydown_ai_addr_copy(ret, AI, ai_addr);
        } else {
            if(!mydown_dns_db_max) memset(&mydown_dns_db, 0, sizeof(mydown_dns_db));
            if(mydown_dns_db_add >= MYDOWN_MAXDNS) mydown_dns_db_add = 0;    // add
            dns = &mydown_dns_db[mydown_dns_db_add];
            i = strlen(host) + 1;
            dns->host = realloc(dns->host, i);
            memcpy(dns->host, host, i);
            AI = mydown_ai_addr_copy(&dns->peer, AI, dns->ai_addr);
            mydown_dns_db_add++;
            if(mydown_dns_db_max < MYDOWN_MAXDNS) mydown_dns_db_max++;
        }
    }
//quit:
    if(AI) {
        AI->ai_canonname = NULL;    // just in case
        AI->ai_next = NULL;
        if(AI != ret) memcpy(ret, AI, sizeof(struct addrinfo)); // AI points to dns->peer
    } else {
        ret = NULL;
    }
    if(AddrInfo) freeaddrinfo(AddrInfo);
    return ret;
}



in_addr_t mydown_resolv_legacy(char *host) {
    mydown_dns_db_t    *dns;
    struct      hostent *hp;
    in_addr_t   host_ip;
    int         i;

    if(!host) return(INADDR_NONE);

    // something seen during some tests, non-compliant and may be removed in future
    // http://host.com|127.0.0.1/test
    u8  *p = strchr(host, '|');
    if(p) host = p + 1;

    host_ip = inet_addr(host);
    if(host_ip == htonl(INADDR_NONE)) {

        for(i = 0; i < mydown_dns_db_max; i++) {    // search
            if(!stricmp(host, mydown_dns_db[i].host)) return ((struct sockaddr_in *)mydown_dns_db[i].peer.ai_addr)->sin_addr.s_addr;
        }

        hp = gethostbyname(host);
        if(!hp) {
            fprintf(stderr, "\nError: Unable to resolve hostname (%s)\n\n", host);
            return(INADDR_NONE);
        }
        host_ip = *(in_addr_t *)(hp->h_addr);

        if(!mydown_dns_db_max) memset(&mydown_dns_db, 0, sizeof(mydown_dns_db));
        if(mydown_dns_db_add == MYDOWN_MAXDNS) mydown_dns_db_add = 0;    // add
        dns = &mydown_dns_db[mydown_dns_db_add];
        i = strlen(host) + 1;
        dns->host = realloc(dns->host, i);
        memcpy(dns->host, host, i);
        ((struct sockaddr_in *)&dns->peer.ai_addr)->sin_addr.s_addr = host_ip;
        mydown_dns_db_add++;
        if(mydown_dns_db_max < MYDOWN_MAXDNS) mydown_dns_db_max++;
    }
    return(host_ip);
}



char *mydown_stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;

      for (start = (char *)String; *start; start++)
      {
            /* find start of pattern in string */
            for ( ; (*start && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if (!*start)
                  return 0;

            pptr = (char *)Pattern;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if (!*pptr)
                        return (start);
            }
      }
      return 0;
}



int mydown_create_dir(u8 *name) {
    int     i;

    if(!name) return 0;
    // name is already an allocated buffer so feel free to edit it
    for(i = 0; name[i]; i++) {
        if(strchr("\\/", name[i])) {
            name[i] = 0;
            make_dir(name);
            name[i] = PATHSLASH;
        }
    }
    return 0;
}



int mydown_check_is_dir(u8 *fname) {
    struct stat xstat;

    if(!fname) return(1);
    if(stat(fname, &xstat) < 0) return(0);
    if(!S_ISDIR(xstat.st_mode)) return(0);
    return(1);
}



int mydown_file_exists(u8 *fname) {
    FILE    *fd;

    if(!fname) return 0;

    // stdin/stdout ???
    if(!strcmp(fname, "-")) return 1;

    // needed for symlinks to folders
    if(mydown_check_is_dir(fname)) return 0;

    fd = fopen(fname, "rb");
    if(!fd) return 0;
    fclose(fd);
    return 1;
}



int mydown_get_yesno(u8 *data) {
    u8      tmp[16];

    if(!data) {
        if(!fgets(tmp, sizeof(tmp), stdin)) return(0);
        data = tmp;
    }
    return(tolower(data[0]));
}



u8 *mydown_rename_auto(int cnt, u8 *name) {
    int     i,
            namelen = 0,
            extlen  = 0;
    u8      tmp[1 + 8 + 1],
            *p,
            *ext;

    if(!name) return NULL;
    namelen = strlen(name);
    ext = strrchr(name, '.');
    if(ext) *ext++ = 0;
    else     ext = name + strlen(name);
    namelen = strlen(name);
    extlen  = strlen(ext);
    sprintf(tmp, "_%08x", cnt - 1);
    p = NULL;
    if((cnt > 1) && (namelen >= (1 + 8))) {
        p = name + namelen - (1 + 8);
        if(strcmp(p, tmp)) p = NULL;
    }
    if(!p) {
        p = ext + 1 + 8;
        for(i = extlen /* - 1 I want the final NULL delimiter too */; i >= 0; i--) {
            p[i] = ext[i];
        }
        if(extlen) ext += 1 + 8;
        p = name + namelen;
    }
    sprintf(p, "_%08x", cnt);
    if(extlen) ext[-1] = '.';
    return(name);
}



int mydown_check_overwrite(u8 *fname, int check_if_present_only, int resume, int *asked) {
    int     c;

    if(asked) *asked = 0;
    if(mydown_force_overwrite > 0) return(0);
    if(mydown_force_overwrite < 0) return(-1);
    if(!fname) return(0);
    if(!mydown_file_exists(fname)) return(0);
    if(check_if_present_only) return(-1);
    if(mydown_force_rename) return(-2);
    
    if(asked) *asked = 1;
    printf(
        "\n"
        "- The following output file already exists:\n"
        "  %s\n"
        "  Do you want to %s it?\n"
        "    y = %s (you can use also the 'o' key)\n"
        "    n = skip (default, just press RETURN)\n"
        "    a = %s all the files without asking\n"
        "    r = automatically rename the files with the same name\n"
        "    s = skip all the existent files without asking\n"
        "  \n",
        fname,
        resume ? "resume" : "ovewrite",
        resume ? "resume" : "ovewrite",
        resume ? "resume" : "ovewrite");
    c = mydown_get_yesno(NULL);
    if(c == 'y') return(0);
    if(c == 'o') return(0); // Overwrite
    if(c == 'a') {
        mydown_force_overwrite = 1;
        return(0);
    }
    if(c == 'r') {
        mydown_force_rename = 1;
        return(-2);
    }
    if((c == 's') || (c == '0')) {  // O and 0 (zero) are too similar {
        mydown_force_overwrite = -1;
        return(-1);
    }
    return(-1);
}



// currently websocket is just a placeholder but it works well

#define MYDOWN_OPCODE_CONTINUATION  0
#define MYDOWN_OPCODE_TEXT          1
#define MYDOWN_OPCODE_BINARY        2
#define MYDOWN_OPCODE_CLOSE         8
#define MYDOWN_OPCODE_PING          9
#define MYDOWN_OPCODE_PONG          10



int mydown_recv_websocket(SSL *ssl_sd, int sd, u8 *data, int datasz, int *ret_opcode) {
    int i;
    u8  tmp[1];
    u8  c;

    if(ret_opcode) *ret_opcode = 0;

    if(mydown_recv(ssl_sd, sd, tmp, 1) != 1) return -1;
    c = tmp[0];
    //int frame_fin                   = (c >> 7) & 1;
    //int frame_rsv1                  = (c >> 6) & 1;
    //int frame_rsv2                  = (c >> 5) & 1;
    //int frame_rsv3                  = (c >> 4) & 1;
    int frame_opcode                = (c >> 0) & ((1<<4)-1);
    //int frame_opcode_cont           = frame_opcode == 0;
    //int frame_opcode_non_control    = (frame_opcode >= 1) || (frame_opcode <= 7);
    //int frame_opcode_control        = (frame_opcode >= 8) || (frame_opcode <= 0xf);

    if(mydown_recv(ssl_sd, sd, tmp, 1) != 1) return -1;
    c = tmp[0];
    int frame_masked                = (c >> 7) & 1;
    u64 frame_payload_length        = (c >> 0) & ((1<<7)-1);
    if(frame_payload_length >= 126) {
        frame_payload_length = 0;
        i = (frame_payload_length >= 127) ? 8 : 2;
        while(i--) {
            if(mydown_recv(ssl_sd, sd, tmp, 1) != 1) return -1;
            frame_payload_length = (u64)tmp[0] << (u64)(i * 8);
        }
    }

    u8  frame_mask[4];
    for(i = 0; i < 4; i++) {
        frame_mask[i] = 0;
        if(frame_masked) if(mydown_recv(ssl_sd, sd, frame_mask + i, 1) != 1) return -1;
    }

    if(data) {
        for(i = 0; i < frame_payload_length; i++) {
            if(mydown_recv(ssl_sd, sd, (i < datasz) ? (data + i) : tmp, 1) != 1) return -1;
        }
        if(i < datasz) data[i] = 0;

        if(frame_opcode == MYDOWN_OPCODE_CLOSE) {
            int error = 0;
            if(datasz == 1) error = data[0];
            else if(datasz >= 2) error = (data[0] << 8) | data[1];
            frame_payload_length = -error;
        }
    }

    if(ret_opcode) *ret_opcode = frame_opcode;
    return frame_payload_length;
}



int mydown_send_websocket(SSL *ssl_sd, int sd, u8 *data, int datasz, int opcode) {
    int i;
    u8  tmp[1];

    if(datasz < 0) return 0;
    if(!datasz) return 0;   // no need to send anything?

    int frame_fin                   = 1;
    tmp[0] = (frame_fin << 7) | (opcode & ((1<<4)-1));
    if(mydown_send(ssl_sd, sd, tmp, 1) != 1) return -1;

    int frame_masked                = 1;    // mandatory for clients
    tmp[0] = (frame_masked << 7);
    if(datasz <= 125) {
        tmp[0] |= datasz & ((1<<7)-1);
    } else if(datasz <= 0xffff) {
        tmp[0] |= 126;
    } else {
        tmp[0] |= 127;
    }
    if(mydown_send(ssl_sd, sd, tmp, 1) != 1) return -1;
    if(datasz > 125) {
        i = (datasz > 0xffff) ? 8 : 2;
        while(i--) {
            tmp[0] = (u64)datasz >> (u64)(i * 8);
            if(mydown_send(ssl_sd, sd, tmp, 1) != 1) return -1;
        }
    }

    u8  frame_mask[4];  // just a placeholder
    for(i = 0; i < 4; i++) {
        frame_mask[i] = 0;
        if(frame_masked) if(mydown_send(ssl_sd, sd, frame_mask + i, 1) != 1) return -1;
    }

    if(data) {
        /* // not implemented due to possible static const data, alternative is sending one byte per time
        if(frame_masked) {
            for(i = 0; i < datasz; i++) {
                data[i] ^= frame_mask[i % 4];
            }
        }
        */
        if(mydown_send(ssl_sd, sd, data, datasz) < 0) return -1;
    }

    return datasz;
}


