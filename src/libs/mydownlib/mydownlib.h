/*
mydownlib
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

Note that this library has been written for being used in my tools,
so there is no manual except some comments in the code.
Note also that some of the strange behaviours of showhead, resume and
onlyifdiff are caused by keeping mydown_http2file intact for
backward compatibility (so new features have been added in pre-existent
options).


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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



#define MYDOWN_VER                          "0.4a"
#define MYDOWN_ERROR                        (-1LL)
#define MYDOWN_OPTION_ONLY_IF_DIFFERENT     1
#define MYDOWN_OPTION_DOSSL                 30
#define MYDOWN_OPTION_ASK_OVERWRITE         32
#define MYDOWN_OPTION_INTERNAL_KEEPALIVE    64



typedef struct {
    uint64_t    from;           // download from byte, use -1 for the latest tot bytes
    uint64_t    tot;            // download tot bytes
    int         showhead;       // show the http header and stop:
                                //   0      no
                                //   1      yes
                                //   2      get all the remote file information (solution for backward compatibility!)
    int         resume;         // resume a download:
                                //   0      no
                                //   1      yes
                                //   2/3    considers the filename as FILE * (solution for backward compatibility!))
    int         onlyifdiff;     // mydownlib flag (the onlyifdiff name was an old name left for backward compatibility):
                                //   0                                download the remote file in any case (without checking if a local one already exists)
                                //   MYDOWN_OPTION_ONLY_IF_DIFFERENT  download only if the local file is different than the remote one
                                //   MYDOWN_OPTION_DOSSL              SSL stuff used internally, do not set it
                                //   MYDOWN_OPTION_ASK_OVERWRITE      ask to overwrite the local file if it already exists
                                //   MYDOWN_OPTION_INTERNAL_KEEPALIVE will be used an internal database with a socket for each host, useful for performances with multiple hosts
    uint8_t     *user;          // username for authentication
    uint8_t     *pass;          // password for authentication
    uint8_t     *referer;       // referer string
    uint8_t     *useragent;     // user-agent string
    uint8_t     *cookie;        // cookie string
    uint8_t     *more_http;     // additional http parameters
    int         verbose;        // verbosity:
                                //   -1     quiet
                                //   0      normal
                                //   1      verbose
                                //   &2     received header
                                //   &4     sent header
                                //   7      verbose + sent + received header
    uint8_t     **filedata;     // use it if you want to store the downloaded file in memory,
                                // if showhead is 2 then filedata will contain the name of the remote file
                                // remember to set your buffer to NULL before passing it (mydownlib uses realloc)
    int         *keep_alive;    // keep-alive socket, remember to initialize it to 0
    int         timeout;        // seconds of timeout
    int         *ret_code;      // HTTP code from the server
    int         onflyunzip;     // unpack the file on the fly if possible (usually gzipped)
    uint8_t     *content;       // data to post
    uint64_t    contentsize;    // optional size of content, by default it's auto guessed via strlen()
    uint8_t     *get;           // the type of request, like GET or POST or HEAD or anything else
    uint8_t     *proxy;         // proxy (HTTP and CONNECT method)
    uint16_t    proxy_port;     // proxy port
    uint64_t    *recv_bytes;    // how many bytes we have received, useful in case of errors (ret == MYDOWN_ERROR)
    FILE        *fd;            // ignore the output file name and uses this file descriptor
} mydown_options;



uint64_t mydown(                // ret: file size
    uint8_t     *myurl,         // the URL
                                // can be like http://aluigi.org/mytoolz/mydown.zip
                                // or http://user:pass@host:port/blabla/blabla.php?file=1
    uint8_t     *filename,      // NULL for automatic filename or forced like "test.txt"
                                // if showhead is 2 filename will be considered a uint8_t ** containing the
                                // pointer to a filename that will be returned by mydown:
                                //    char *filename = NULL;
                                //    opt.showhead = 2;
                                //    mydown("http://HOST/file", &filename, &opt);
    mydown_options *opt         // the above structure for your options
);



/*
mydown_http2file was created to have the possibility of calling mydown without
creating the mydown_options structure, the downside was that upgrading the
library meant changing the prototype of mydown_http2file.
So some versions ago I added a mydown_options argument so that the prototype
will no longer change and additional options can be passed in that structure
*/
uint64_t mydown_http2file(      // ret: file size
    int         *sock,          // socket for keep-alive
    int         timeout,        // seconds of timeout
    uint8_t     *host,          // hostname or IP
    uint16_t    port,           // port
    uint8_t     *user,          // username
    uint8_t     *pass,          // password
    uint8_t     *referer,       // Referer
    uint8_t     *useragent,     // User-Agent
    uint8_t     *cookie,        // Cookie
    uint8_t     *more_http,     // additional http parameters (ex: mycookie: blabla\r\npar: val\r\n)
    int         verbose,        // verbose
    uint8_t     *getstr,        // URI
    FILE        *fd,            // file descriptor
    uint8_t     *filename,      // force filename
    int         showhead,       // show headers
    int         onlyifdiff,     // download only if differs from local file
    int         resume,         // resume
    uint64_t    from,           // download from byte
    uint64_t    tot,            // download tot bytes
    uint64_t    *filesize,      // for storing file size
    uint8_t     **filedata,     // use it if you want to store the downloaded file in memory
    int         *ret_code,      // HTTP code from the server
    int         onflyunzip,     // on fly decompression
    uint8_t     *content,       // data to post
    uint64_t    contentsize,    // optional size of content (default is auto)   // note that it's not a true 64bit
    uint8_t     *get,           // request
    uint8_t     *proxy,
    uint16_t    proxy_port,
    mydown_options *opt         // set it to NULL, it's an experimental idea
);


