#ifdef WIN32
    #include <winsock.h>

    #define close   closesocket
    #define sleep   Sleep
    #define sleepms sleep
    #define ONESEC  1000
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>

    #define sleepms(X)  usleep(X * 1000)
    #define ONESEC  1
#endif

#ifndef DISABLE_SSL
    #include <openssl/ssl.h>    // link with libssl.a libcrypto.a -lgdi32
#else                           // on linux: gcc -o stcppipe stcppipe.c -lssl -lcrypto -lpthread
    #define SSL     char
    #define SSL_read(A,B,C)     0
    #define SSL_write(A,B,C)    0
    typedef void SSL_CTX;
    typedef void SSL_METHOD;
    //typedef void SSL;
#endif



#ifndef SO_EXCLUSIVEADDRUSE
    #define SO_EXCLUSIVEADDRUSE ((u_int)(~SO_REUSEADDR))
#endif
#ifndef TCP_NODELAY
    #define TCP_NODELAY 0x0001
#endif

#define SSL_COMPAT(X)   SSL_CTX_set_cipher_list(X, SSL_TXT_ALL); \
                        SSL_CTX_set_options(X, SSL_OP_ALL);



#define IPPROTO_QUICKBMS    0x10000
#define IPPROTO_HTTP    (IPPROTO_QUICKBMS + 80)
#define IPPROTO_HTTPS   (IPPROTO_QUICKBMS + 443)



typedef struct {
    struct sockaddr_in  peer;
    int     sd;
    int     sa;
    int     proto;
    u8      *host;
    i32     port;
    int     bind_mode;
    int     pos;
    void    *prev;
    void    *next;
    u8      *data;  // used only for http and https
    int     datasz;
//#ifndef DISABLE_SSL
    i32     dossl;
    u8      *ssl_cert_file;
    u8      *ssl_method_type;
    SSL_CTX     *ctx_sd;
    SSL_METHOD  *ssl_method;
    SSL         *ssl_sd;
//#endif
} socket_file_t;



static  socket_file_t   *socket_file    = NULL;

u8      *ssl_cert_pass  = NULL;   // not set at the moment



static const u8 SSL_CERT_X509[] =   // x509 –in input.crt –inform PEM –out output.crt –outform DER
"\x30\x82\x03\x07\x30\x82\x02\x70\xa0\x03\x02\x01\x02\x02\x09\x00"
"\x85\x3a\x6e\x0a\xa4\x3c\x6b\xec\x30\x0d\x06\x09\x2a\x86\x48\x86"
"\xf7\x0d\x01\x01\x05\x05\x00\x30\x61\x31\x0b\x30\x09\x06\x03\x55"
"\x04\x06\x13\x02\x55\x53\x31\x0b\x30\x09\x06\x03\x55\x04\x08\x14"
"\x02\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04\x07\x14\x02\x22\x22"
"\x31\x0b\x30\x09\x06\x03\x55\x04\x0a\x14\x02\x22\x22\x31\x0b\x30"
"\x09\x06\x03\x55\x04\x0b\x14\x02\x22\x22\x31\x0b\x30\x09\x06\x03"
"\x55\x04\x03\x14\x02\x22\x22\x31\x11\x30\x0f\x06\x09\x2a\x86\x48"
"\x86\xf7\x0d\x01\x09\x01\x16\x02\x22\x22\x30\x1e\x17\x0d\x30\x39"
"\x30\x31\x30\x34\x30\x33\x31\x34\x33\x33\x5a\x17\x0d\x31\x30\x30"
"\x31\x30\x34\x30\x33\x31\x34\x33\x33\x5a\x30\x61\x31\x0b\x30\x09"
"\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x0b\x30\x09\x06\x03\x55"
"\x04\x08\x14\x02\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04\x07\x14"
"\x02\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04\x0a\x14\x02\x22\x22"
"\x31\x0b\x30\x09\x06\x03\x55\x04\x0b\x14\x02\x22\x22\x31\x0b\x30"
"\x09\x06\x03\x55\x04\x03\x14\x02\x22\x22\x31\x11\x30\x0f\x06\x09"
"\x2a\x86\x48\x86\xf7\x0d\x01\x09\x01\x16\x02\x22\x22\x30\x81\x9f"
"\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03"
"\x81\x8d\x00\x30\x81\x89\x02\x81\x81\x00\xc5\xe3\x3f\x2d\x8f\x98"
"\xc2\x2a\xef\x71\xea\x40\x21\x54\x3f\x08\x62\x9c\x7b\x39\x22\xfd"
"\xda\x80\x1f\x21\x3e\x8d\x68\xcf\x8e\x6b\x70\x98\x95\x2c\x1e\x4e"
"\x79\x39\x45\xf5\xa3\xd9\x20\x54\x85\x79\x36\xf5\x08\xbe\xa0\xa6"
"\x03\x80\x60\x21\xd6\xbc\xde\xf8\xed\xe8\x73\x02\x96\x84\xcb\xb4"
"\xff\x72\x89\xf4\x56\x41\xf6\x28\xf6\x6b\x9f\x0c\x1d\xe0\x9b\x21"
"\xcb\x86\x08\xdf\x6b\xc1\x8a\xd6\xa3\x52\x2f\xfa\xd8\x5a\x2c\x86"
"\x52\x0d\x75\x2d\xf6\x17\x11\xa7\x17\xad\xc2\x3b\xd8\x0f\xcf\xb7"
"\x2b\x2c\x8a\xc4\xcd\x2d\x94\xe4\x15\x75\x02\x03\x01\x00\x01\xa3"
"\x81\xc6\x30\x81\xc3\x30\x1d\x06\x03\x55\x1d\x0e\x04\x16\x04\x14"
"\x00\x6b\x12\xa2\xb9\x10\x90\xe4\xe5\xe8\xff\xec\x5c\x24\x44\xee"
"\xed\xc1\x66\xb7\x30\x81\x93\x06\x03\x55\x1d\x23\x04\x81\x8b\x30"
"\x81\x88\x80\x14\x00\x6b\x12\xa2\xb9\x10\x90\xe4\xe5\xe8\xff\xec"
"\x5c\x24\x44\xee\xed\xc1\x66\xb7\xa1\x65\xa4\x63\x30\x61\x31\x0b"
"\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x0b\x30\x09\x06"
"\x03\x55\x04\x08\x14\x02\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04"
"\x07\x14\x02\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04\x0a\x14\x02"
"\x22\x22\x31\x0b\x30\x09\x06\x03\x55\x04\x0b\x14\x02\x22\x22\x31"
"\x0b\x30\x09\x06\x03\x55\x04\x03\x14\x02\x22\x22\x31\x11\x30\x0f"
"\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x09\x01\x16\x02\x22\x22\x82"
"\x09\x00\x85\x3a\x6e\x0a\xa4\x3c\x6b\xec\x30\x0c\x06\x03\x55\x1d"
"\x13\x04\x05\x30\x03\x01\x01\xff\x30\x0d\x06\x09\x2a\x86\x48\x86"
"\xf7\x0d\x01\x01\x05\x05\x00\x03\x81\x81\x00\x33\xb1\xd0\x31\x04"
"\x17\x67\xca\x54\x72\xbc\xb7\x73\x5a\x8f\x1b\x23\x25\x7d\xcb\x23"
"\xae\x1b\x9b\xd2\x92\x80\x09\x5d\x20\x24\xd2\x73\x6f\xe7\x5a\xaf"
"\x9e\xd0\xdd\x50\x61\x96\xbf\x7c\x2d\xa1\x0a\xc4\x88\xf7\xe0\xc6"
"\xc3\x04\x35\x6f\xac\xd5\xd1\xfd\x55\xab\x6c\x99\xc7\x66\x72\xb8"
"\x70\x22\xcb\xd3\x8c\xa7\x18\x17\x2e\x25\x2f\x33\x5c\x57\x82\x67"
"\x0e\x29\xeb\x81\x74\xd3\xa3\x54\xfa\x08\xba\x87\x50\x18\xab\xc5"
"\x15\x69\xce\x4a\x73\x3b\xee\x12\x4d\x1c\x63\x11\x9b\xdf\x4d\xa1"
"\x38\x0d\xb6\x1d\xfb\xd6\xb8\x5b\xc2\x10\xd9";

static const u8 SSL_CERT_RSA[] =    // rsa –in input.key –inform PEM –out output.key –outform DER
"\x30\x82\x02\x5b\x02\x01\x00\x02\x81\x81\x00\xc5\xe3\x3f\x2d\x8f"
"\x98\xc2\x2a\xef\x71\xea\x40\x21\x54\x3f\x08\x62\x9c\x7b\x39\x22"
"\xfd\xda\x80\x1f\x21\x3e\x8d\x68\xcf\x8e\x6b\x70\x98\x95\x2c\x1e"
"\x4e\x79\x39\x45\xf5\xa3\xd9\x20\x54\x85\x79\x36\xf5\x08\xbe\xa0"
"\xa6\x03\x80\x60\x21\xd6\xbc\xde\xf8\xed\xe8\x73\x02\x96\x84\xcb"
"\xb4\xff\x72\x89\xf4\x56\x41\xf6\x28\xf6\x6b\x9f\x0c\x1d\xe0\x9b"
"\x21\xcb\x86\x08\xdf\x6b\xc1\x8a\xd6\xa3\x52\x2f\xfa\xd8\x5a\x2c"
"\x86\x52\x0d\x75\x2d\xf6\x17\x11\xa7\x17\xad\xc2\x3b\xd8\x0f\xcf"
"\xb7\x2b\x2c\x8a\xc4\xcd\x2d\x94\xe4\x15\x75\x02\x03\x01\x00\x01"
"\x02\x81\x80\x59\x45\x5c\x11\xf4\xae\xc8\x21\x50\x65\xc6\x74\x69"
"\xd4\xb4\x9e\xd6\xc5\x9a\xfd\x3a\xa0\xe4\x7a\x5a\x10\xc8\x44\x48"
"\xdd\x21\x75\xac\x94\xd8\xee\xcf\x39\x3d\x8c\xad\xd7\xd3\xb3\xb6"
"\xd7\x0a\x63\x95\x7c\x53\x16\x94\x28\x70\x79\xf0\x64\x33\x98\x7e"
"\xca\x33\xa0\x97\x38\x01\xe9\x06\x9b\x5c\x15\x3d\x89\xa3\x40\x2a"
"\x54\xb1\x79\x15\xf1\x7c\xfd\x18\xca\xdf\x53\x42\x6c\x8a\x0b\xc1"
"\x18\x70\xea\x7e\x00\x64\x07\x84\x37\xf2\x1b\xf5\x2a\x22\xe9\xd6"
"\xfa\x03\xc6\x7f\xaa\xc8\xa2\xa3\x67\x2a\xd3\xdd\xae\x36\x47\xc1"
"\x4f\x13\xe1\x02\x41\x00\xec\x61\x11\xbf\xcd\x87\x03\xa6\x87\xc9"
"\x2f\x1d\x80\xc1\x73\x5f\x19\xe7\x7c\xb9\x67\x7e\x49\x58\xbf\xab"
"\xd8\x37\x29\x22\x69\x79\xa4\x06\xcd\xac\x5f\x9e\xba\x12\x77\xf8"
"\x3e\xd2\x6a\x06\xb5\x90\xe4\xfa\x23\x86\xff\x41\x1b\x10\xbe\xe4"
"\x9d\x29\x75\x7c\xe6\x49\x02\x41\x00\xd6\x50\x40\xfc\xc9\x49\xad"
"\x69\x55\xc7\xa3\x5d\x51\x05\x5b\x41\x2b\xd2\x5a\x74\xf8\x15\x49"
"\x06\xf0\x1a\x6f\x7d\xb6\x65\x17\xa0\x64\xff\x7a\xd6\x99\x54\x0d"
"\x53\x95\x9f\x6c\x43\xde\x27\x1b\xe9\x24\x13\x43\xd5\xda\x22\x85"
"\x1d\xa7\x55\xa5\x4d\x0f\x5e\x45\xcd\x02\x40\x51\x92\x4d\xe5\xba"
"\xaf\x54\xfb\x2a\xf0\xaa\x69\xab\xfd\x16\x2b\x43\x6d\x37\x05\x64"
"\x49\x98\x56\x20\x0e\xd5\x56\x73\xc3\x84\x52\x8d\xe0\x2b\x29\xc8"
"\xf5\xa5\x90\xaa\x05\xe8\xe8\x03\xde\xbc\xd9\x7b\xab\x36\x87\x67"
"\x9e\xb8\x10\x57\x4f\xdd\x4c\x69\x56\xe8\xc1\x02\x40\x27\x02\x5a"
"\xa1\xe8\x9d\xa1\x93\xef\xca\x33\xe1\x33\x73\x2f\x26\x10\xac\xec"
"\x4c\x28\x2f\xef\xa7\xf4\xa2\x4b\x32\xed\xb5\x3e\xf4\xb2\x0d\x92"
"\xb5\x67\x19\x56\x87\xa5\x4f\x6c\x6c\x7a\x0e\x52\x55\x40\x7c\xc5"
"\x37\x32\xca\x5f\xc2\x83\x07\xe2\xdb\xc0\xf5\x5e\xed\x02\x40\x1b"
"\x88\xf3\x29\x8d\x6b\xdb\x39\x4c\xa6\x96\x6a\xd7\x6b\x35\x85\xde"
"\x1c\x2c\x3f\x0c\x8d\xff\xf5\xc1\xeb\x25\x3c\x56\x63\xaa\x03\xe3"
"\x10\x24\x87\x98\xd4\x73\x62\x4a\x51\x3b\x01\x9a\xda\x73\xf2\xcd"
"\xd6\xbb\xe3\x3e\x37\xb3\x19\xd9\x82\x91\x07\xdf\xd0\xa9\x80";



int mysend(SSL *ssl_sd, int sd, u8 *data, int datasz) {
    if(ssl_sd) return(SSL_write(ssl_sd, data, datasz));
    return(send(sd, data, datasz, 0));
}



int myrecv(SSL *ssl_sd, int sd, u8 *data, int datasz) {
    if(ssl_sd) return(SSL_read(ssl_sd, data, datasz));
    return(recv(sd, data, datasz, 0));
}



int pem_passwd_cb(char *buf, int num, int rwflag, void *userdata) {
    return(sprintf(buf, "%s", ssl_cert_pass));
}



void sock_err(void) {
#ifdef WIN32
    char    *error;

    switch(WSAGetLastError()) {
        case 10004: error = "Interrupted system call"; break;
        case 10009: error = "Bad file number"; break;
        case 10013: error = "Permission denied"; break;
        case 10014: error = "Bad address"; break;
        case 10022: error = "Invalid argument (not bind)"; break;
        case 10024: error = "Too many open files"; break;
        case 10035: error = "Operation would block"; break;
        case 10036: error = "Operation now in progress"; break;
        case 10037: error = "Operation already in progress"; break;
        case 10038: error = "Socket operation on non-socket"; break;
        case 10039: error = "Destination address required"; break;
        case 10040: error = "Message too long"; break;
        case 10041: error = "Protocol wrong type for socket"; break;
        case 10042: error = "Bad protocol option"; break;
        case 10043: error = "Protocol not supported"; break;
        case 10044: error = "Socket type not supported"; break;
        case 10045: error = "Operation not supported on socket"; break;
        case 10046: error = "Protocol family not supported"; break;
        case 10047: error = "Address family not supported by protocol family"; break;
        case 10048: error = "Address already in use"; break;
        case 10049: error = "Can't assign requested address"; break;
        case 10050: error = "Network is down"; break;
        case 10051: error = "Network is unreachable"; break;
        case 10052: error = "Net dropped connection or reset"; break;
        case 10053: error = "Software caused connection abort"; break;
        case 10054: error = "Connection reset by peer"; break;
        case 10055: error = "No buffer space available"; break;
        case 10056: error = "Socket is already connected"; break;
        case 10057: error = "Socket is not connected"; break;
        case 10058: error = "Can't send after socket shutdown"; break;
        case 10059: error = "Too many references, can't splice"; break;
        case 10060: error = "Connection timed out"; break;
        case 10061: error = "Connection refused"; break;
        case 10062: error = "Too many levels of symbolic links"; break;
        case 10063: error = "File name too long"; break;
        case 10064: error = "Host is down"; break;
        case 10065: error = "No Route to Host"; break;
        case 10066: error = "Directory not empty"; break;
        case 10067: error = "Too many processes"; break;
        case 10068: error = "Too many users"; break;
        case 10069: error = "Disc Quota Exceeded"; break;
        case 10070: error = "Stale NFS file handle"; break;
        case 10091: error = "Network SubSystem is unavailable"; break;
        case 10092: error = "WINSOCK DLL Version out of range"; break;
        case 10093: error = "Successful WSASTARTUP not yet performed"; break;
        case 10071: error = "Too many levels of remote in path"; break;
        case 11001: error = "Host not found"; break;
        case 11002: error = "Non-Authoritative Host not found"; break;
        case 11003: error = "Non-Recoverable errors: FORMERR, REFUSED, NOTIMP"; break;
        case 11004: error = "Valid name, no data record of requested type"; break;
        default: error = strerror(errno); break;
    }
    fprintf(stderr, "\nError: %s\n", error);
    myexit(QUICKBMS_ERROR_EXTRA);
#else
    STD_ERR(QUICKBMS_ERROR_EXTRA);
#endif
}



void socket_close_sd(int *sd) {
    if(sd && *sd) {
        close(*sd);
        *sd = 0;
    }
}



int socket_close(socket_file_t *sockfile) {
#ifndef DISABLE_SSL
    if(sockfile->dossl) {
        if(sockfile->ssl_sd) {
            SSL_shutdown(sockfile->ssl_sd);
            SSL_free(sockfile->ssl_sd);
            sockfile->ssl_sd = NULL;
        }
        if(sockfile->ctx_sd) {
            SSL_CTX_free(sockfile->ctx_sd);
            sockfile->ctx_sd = NULL;
        }
    }
#endif
    socket_close_sd(&sockfile->sd);
    return 0;
}



int create_socket(socket_file_t *sockfile) {
    static int  first_time  = 1;
    static int  size        = 0xffff;
    static struct linger ling = {1,1};
    static int  on = 1;
    int     sd  = -1,
            i;

    int sd_already_set = sockfile->sd;
    struct sockaddr_in *peer = &sockfile->peer;

    for(;;) {
        if(sd_already_set > 0) {
            sd = sd_already_set;
            return(sd); // added for quickbms, do NOT use break or ssl will not work
        }

        if((sockfile->proto == IPPROTO_HTTP) || (sockfile->proto == IPPROTO_HTTPS)) {
            sockfile->pos = 0;
            mydown_options  opt;
            memset(&opt, 0, sizeof(opt));
            opt.verbose     = -1;
            opt.keep_alive  = &sd;
            if(!opt.keep_alive) sd = QUICKBMS_MAX_INT(sd);  // in case the above is not set we need a lame positive placeholder
            /* upload?
            opt.get         = "GET";
            opt.content     = NULL;
            opt.contentsize = 0;
            */
            opt.useragent   = "Mozilla/4.0";
            opt.filedata    = &sockfile->data;
            opt.onlyifdiff  = MYDOWN_OPTION_INTERNAL_KEEPALIVE;
            sockfile->datasz = mydown(sockfile->host, NULL, &opt);
            if(sockfile->datasz < 0) goto quit;
        }

        if(sockfile->proto >= IPPROTO_QUICKBMS) break;

        for(i = 0; i < 5; i++) {
            if(sockfile->proto < 0) {
                sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            } else if(!sockfile->proto) {
                sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            } else {
                sd = socket(AF_INET, SOCK_RAW, sockfile->proto);
            }
            if(sd > 0) break;
            sleepms(500);
        }

        if(peer) fprintf(stderr, "- %s : %d\n", inet_ntoa(peer->sin_addr), (i32)ntohs(peer->sin_port));

        // SO_LINGER makes the sending a bit slower because it really sends
        // the whole full data and is sure almost at 100% that it's received
        setsockopt(sd, SOL_SOCKET, SO_LINGER,    (char *)&ling, sizeof(ling));
        setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&on,   sizeof(on));
        setsockopt(sd, SOL_SOCKET, SO_SNDBUF,    (char *)&size, sizeof(size));  // useless
        if(sockfile->proto >= 0) break;   // packets

        setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&on,   sizeof(on));
        if(!peer) break;

        if(peer->sin_addr.s_addr == INADDR_ANY) {
            if(bind(sd, (struct sockaddr *)peer, sizeof(struct sockaddr_in))
              < 0) goto quit; //sock_err();
            if(sockfile->proto < 0) listen(sd, SOMAXCONN);
            break;
        } else {
            if(!connect(sd, (struct sockaddr *)peer, sizeof(struct sockaddr_in))) {
                if(first_time) first_time = 0;
                break;
            }
        }
        if(first_time) goto quit; //sock_err();
        socket_close_sd(&sd);
        sd_already_set = -1;
        sleepms(500);
    }
    sockfile->sd = sd;  // just to be sure

#ifndef DISABLE_SSL
    if(sockfile->dossl) {
        // it's made to keep compatibility with the old numeric-only format, don't touch the order
        if(stristr(sockfile->ssl_method_type, "23"))    sockfile->ssl_method = (SSL_METHOD *)SSLv23_method();
        if(stristr(sockfile->ssl_method_type, "2"))
        // no longer exists in OpenSSL
        //#ifndef OPENSSL_NO_SSL2
        //                                                sockfile->ssl_method = (SSL_METHOD *)SSLv2_method();
        //#else
                                                        sockfile->ssl_method = (SSL_METHOD *)SSLv23_method();
        //#endif
        if(stristr(sockfile->ssl_method_type, "3"))
        #ifndef OPENSSL_NO_SSL3_METHOD
                                                        sockfile->ssl_method = (SSL_METHOD *)SSLv3_method();
        #else
                                                        sockfile->ssl_method = (SSL_METHOD *)SSLv23_method();
        #endif
        if(stristr(sockfile->ssl_method_type, "dtls"))  sockfile->ssl_method = (SSL_METHOD *)DTLSv1_method();
        if(stristr(sockfile->ssl_method_type, "1"))     sockfile->ssl_method = (SSL_METHOD *)TLSv1_method();
        if(!sockfile->ssl_method_type)                  sockfile->ssl_method = (SSL_METHOD *)SSLv23_method();

        if(sockfile->bind_mode) { // input is SSL
            sockfile->ctx_sd = SSL_CTX_new(sockfile->ssl_method);
            if(!sockfile->ctx_sd) goto quit;
            SSL_COMPAT(sockfile->ctx_sd)

            if(ssl_cert_pass) SSL_CTX_set_default_passwd_cb(sockfile->ctx_sd, pem_passwd_cb);
            if(sockfile->ssl_cert_file) {
                if(!SSL_CTX_use_certificate_chain_file(sockfile->ctx_sd, sockfile->ssl_cert_file) ||
                   !SSL_CTX_use_PrivateKey_file(sockfile->ctx_sd, sockfile->ssl_cert_file, SSL_FILETYPE_PEM) ||
                   !SSL_CTX_use_PrivateKey_file(sockfile->ctx_sd, sockfile->ssl_cert_file, SSL_FILETYPE_ASN1) ||
                   !SSL_CTX_use_certificate_file(sockfile->ctx_sd, sockfile->ssl_cert_file, SSL_FILETYPE_PEM) ||
                   !SSL_CTX_use_certificate_file(sockfile->ctx_sd, sockfile->ssl_cert_file, SSL_FILETYPE_ASN1)
                ) {
                    fprintf(stderr, "\n"
                        "Error: problems with the loading of the certificate file\n"
                        "       check if the certificate you specified is in PEM format and the\n"
                        "       password for the private key and the choosed SSL method are correct\n"
                        "- the following is a quick example for creating a quick certificate:\n"
                        "   openssl req -x509 -days 365 -newkey rsa:1024 -keyout cert.pem -out cert.pem\n"
                        "  add -nodes for a passwordless certificate\n");
                    myexit(QUICKBMS_ERROR_EXTRA);
                }
            } else {
                if(!SSL_CTX_use_certificate_ASN1(sockfile->ctx_sd, sizeof(SSL_CERT_X509) - 1, SSL_CERT_X509) ||
                   !SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA, sockfile->ctx_sd, SSL_CERT_RSA, sizeof(SSL_CERT_RSA) - 1)) {
                    fprintf(stderr, "\nError: problems with the loading of the certificate in memory\n");
                    myexit(QUICKBMS_ERROR_EXTRA);
                }
            }
            SSL_CTX_set_verify_depth(sockfile->ctx_sd, 1);  // #if (OPENSSL_VERSION_NUMBER < 0x00905100L)

            sockfile->ssl_sd = SSL_new(sockfile->ctx_sd);
            if(!sockfile->ssl_sd) goto quit;
            #ifdef SSL_set_tlsext_host_name
            SSL_set_tlsext_host_name(sockfile->ssl_sd, sockfile->host);
            #endif
            SSL_set_fd(sockfile->ssl_sd, sockfile->sd);
            if(SSL_accept(sockfile->ssl_sd) < 0) goto quit;

        } else {

            sockfile->ctx_sd = SSL_CTX_new(sockfile->ssl_method);
            if(!sockfile->ctx_sd) goto quit;
            SSL_COMPAT(sockfile->ctx_sd)

            sockfile->ssl_sd = SSL_new(sockfile->ctx_sd);
            SSL_set_fd(sockfile->ssl_sd, sockfile->sd);
            if(SSL_connect(sockfile->ssl_sd) < 0) goto quit;
        }
    }
#endif

    return(sockfile->sd);
quit:
    socket_close(sockfile);
    return -1;
}



int timeout(int sock, int secs) {
    struct  timeval tout;
    fd_set  fd_read;

    tout.tv_sec  = secs;
    tout.tv_usec = 0;
    FD_ZERO(&fd_read);
    FD_SET(sock, &fd_read);
    if(select(sock + 1, &fd_read, NULL, NULL, &tout)
      <= 0) return -1;
    return 0;
}



u32 resolv(char *host) {
    struct  hostent *hp;
    u32     host_ip;

    host_ip = inet_addr(host);
    if(host_ip == INADDR_NONE) {
        hp = gethostbyname(host);
        if(!hp) {
            fprintf(stderr, "\nError: Unable to resolv hostname (%s)\n", host);
            myexit(QUICKBMS_ERROR_EXTRA);
        } else host_ip = *(u32 *)hp->h_addr;
    }
    return(host_ip);
}



int socket_common(socket_file_t *sockfile) {
    struct sockaddr_in  peerl;
    int     sd,
            psz;

    if(sockfile->proto >= IPPROTO_QUICKBMS) {
        // do nothing with special types, currently only IPPROTO_HTTP and IPPROTO_HTTPS
    } else if(!sockfile->peer.sin_addr.s_addr && !sockfile->peer.sin_port) {
        sockfile->peer.sin_addr.s_addr  = resolv(sockfile->host);
        if(!sockfile->peer.sin_addr.s_addr) {
            sockfile->bind_mode = 1;
            sockfile->peer.sin_addr.s_addr  = INADDR_ANY;
        }
        sockfile->peer.sin_port     = htons(sockfile->port);
        sockfile->peer.sin_family   = AF_INET;
    }

    sockfile->sd = create_socket(sockfile);
    if(sockfile->sd < 0) return -1; //sock_err();

    sd = sockfile->sd;
    if(sockfile->bind_mode && (sockfile->proto < 0)) {
        if(!sockfile->sa) {
            psz = sizeof(struct sockaddr_in);
            sockfile->sa = accept(sockfile->sd, (struct sockaddr *)&peerl, &psz);
            if(sockfile->sa < 0) return -1; //sock_err();
        }
        sd = sockfile->sa;
    }
    return(sd);
}



socket_file_t *socket_open(u8 *fname) {
    static  int init_socket     = 0;
    socket_file_t   *sockfile   = NULL,
                    *sockfile_tmp;
    i32     force_new       = 0;
    i32     host_n          = 0;
    u8      host[1024+1]    = "",
            proto[16]       = "";

    if(!strstr(fname, "://")) return NULL;

    sockfile_tmp = calloc(1, sizeof(socket_file_t));
    if(!sockfile_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);

    sockfile_tmp->ssl_method_type = "23";

    sscanf(fname,
        "%10[^:]://%n%1024[^:,\\/?&]:%d,%d,%u",
        proto,
        &host_n,
        host,
        &sockfile_tmp->port,
        &sockfile_tmp->dossl,
        &force_new);

         if(!stricmp(proto, "tcp"))     sockfile_tmp->proto = -1;
    else if(!stricmp(proto, "udp"))     sockfile_tmp->proto = 0;
    else if(!stricmp(proto, "raw"))     sockfile_tmp->proto = IPPROTO_RAW;
    else if(!stricmp(proto, "icmp"))    sockfile_tmp->proto = IPPROTO_ICMP;
    else if(!stricmp(proto, "udp_raw")) sockfile_tmp->proto = IPPROTO_UDP;
    else if(!stricmp(proto, "tcp_raw")) sockfile_tmp->proto = IPPROTO_TCP;
    else if(!stricmp(proto, "ssl"))   { sockfile_tmp->proto = -1;               sockfile_tmp->dossl = 1; }  // ssl23
    else if(!stricmp(proto, "ssl3"))  { sockfile_tmp->proto = -1;               sockfile_tmp->dossl = 1;    sockfile_tmp->ssl_method_type = "3";    }
    else if(!stricmp(proto, "ssl2"))  { sockfile_tmp->proto = -1;               sockfile_tmp->dossl = 1;    sockfile_tmp->ssl_method_type = "2";    }
    else if(!stricmp(proto, "dtls"))  { sockfile_tmp->proto = -1;               sockfile_tmp->dossl = 1;    sockfile_tmp->ssl_method_type = "dtls"; }
    else if(!stricmp(proto, "tls1"))  { sockfile_tmp->proto = -1;               sockfile_tmp->dossl = 1;    sockfile_tmp->ssl_method_type = "tls1"; }
    else if(!stricmp(proto, "http"))  { sockfile_tmp->proto = IPPROTO_HTTP;     sockfile_tmp->host = mystrdup_simple(fname); }
    else if(!stricmp(proto, "https")) { sockfile_tmp->proto = IPPROTO_HTTPS;    sockfile_tmp->host = mystrdup_simple(fname); }
    else {
        // example: 17://
        sockfile_tmp->proto = myatoi(proto);
        if((sockfile_tmp->proto <= 0) || (sockfile_tmp->proto > 0xff)) {
            //sockfile_tmp->proto = -1;
            FREE(sockfile_tmp);
            return NULL;
        }
    }
    if(!host[0]) {
        FREE(sockfile_tmp);
        return NULL;
    }

    if(!enable_sockets) {
        fprintf(stderr,
            "\n"
            "Error: the script uses network sockets, if you are SURE about the genuinity of\n"
            "       this script\n"
            "\n"
            "         you MUST use the -n or -network option at command-line.\n"
            "\n"
            "       note that the usage of the sockets allows QuickBMS to send and receive\n"
            "       data to and from other computers so you MUST really sure about the\n"
            "       script you are using and what you are doing.\n"
            "       this is NOT a feature for extracting files!\n");
        myexit(QUICKBMS_ERROR_EXTRA);
    }
    if(!init_socket) {
        #ifdef WIN32
        WSADATA    wsadata;
        WSAStartup(MAKEWORD(1,0), &wsadata);
        #endif
        #ifndef DISABLE_SSL
        SSL_library_init();
        //SSL_load_error_strings();
        #endif
        init_socket = 1;
    }

    if(sockfile_tmp->proto >= IPPROTO_QUICKBMS) {
        // do nothing
    } else if(sockfile_tmp->port <= 0) {
        fprintf(stderr, "\nError: the specified port is invalid (%d)\n", sockfile_tmp->port);
        myexit(QUICKBMS_ERROR_EXTRA);
    }

    if(!sockfile_tmp->host) sockfile_tmp->host = mystrdup_simple(host);

    for(sockfile = socket_file; sockfile; sockfile = sockfile->next) {
        if(
            (sockfile->proto == sockfile_tmp->proto) &&
            !stricmp(sockfile->host, sockfile_tmp->host) &&
            (sockfile->port == sockfile_tmp->port)
        ) {
            if(force_new && sockfile->sd) {
                socket_close_sd(&sockfile->sa);
                socket_close_sd(&sockfile->sd);
            }
            FREE(sockfile_tmp->host);
            FREE(sockfile_tmp);
            sockfile_tmp = NULL;
            break;
        }
    }
    if(!sockfile) {
        if(!socket_file) {
            socket_file = sockfile_tmp;
            sockfile = socket_file;
        } else {
            // get the last element
            for(sockfile = socket_file;; sockfile = sockfile->next) {
                if(sockfile->next) continue;
                sockfile->next = sockfile_tmp;
                sockfile_tmp->prev = sockfile;
                sockfile = sockfile_tmp;
                break;
            }
        }
    }

    int sd = socket_common(sockfile);
    if(sd < 0) return NULL;
    return(sockfile);
}



int socket_read(socket_file_t *sockfile, u8 *data, int size) {
    int     sd,
            t,
            len,
            psz;

    sd = socket_common(sockfile);
    if(sd < 0) return -1;

    if(sockfile->proto < 0) {
        for(len = 0; len < size; len += t) {
            t = myrecv(sockfile->ssl_sd, sd, data + len, size - len);
            if(t <= 0) break;
        }
    } else if((sockfile->proto == IPPROTO_HTTP) || (sockfile->proto == IPPROTO_HTTPS)) {
        if((sockfile->datasz > 0) && (sockfile->pos < sockfile->datasz)) {
            len = sockfile->datasz - sockfile->pos;
            if(len > size) len = size;
            memcpy(data, sockfile->data + sockfile->pos, len);
        } else {
            len = -1;
        }
        size = len; // necessary for the "sockfile->proto >= 0" later!
    } else {
        psz = sizeof(struct sockaddr_in);
        len = recvfrom(sd, data, size, 0, (struct sockaddr *)&sockfile->peer, &psz);
    }
    if(g_verbose && (len > 0)) show_dump(2, data, len, stdout);
    if(sockfile->proto >= 0) {
        len = size; // lame way to avoid errors? long story about packets
    }
    if(len > 0) sockfile->pos += len;
    return len;
}



int socket_write(socket_file_t *sockfile, u8 *data, int size) {
    int     sd,
            len;

    sd = socket_common(sockfile);
    if(sd < 0) return -1;

    if(g_verbose && (size > 0)) show_dump(2, data, size, stdout);

    if(sockfile->proto < 0) {
        len = mysend(sockfile->ssl_sd, sd, data, size);
    } else if((sockfile->proto == IPPROTO_HTTP) || (sockfile->proto == IPPROTO_HTTPS)) {
        len = -1;   // unsupported
    } else {
        // an udp socket in listening mode can't send data because there is no destination!
        if((sockfile->peer.sin_addr.s_addr == INADDR_ANY) || (sockfile->peer.sin_addr.s_addr == INADDR_NONE)) return size;
        len = sendto(sd, data, size, 0, (struct sockaddr *)&sockfile->peer, sizeof(struct sockaddr_in));
    }
    //if(len != size) sock_err();
    if(len > 0) sockfile->pos += len;
    return len;
}

