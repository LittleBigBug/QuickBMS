/*
    Copyright 2018-2021 Luigi Auriemma

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

#ifdef WIN32
    #include <windows.h>
    #include <winsock.h>
    #define PIPE_PREFIX "\\\\.\\"
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #define PIPE_PREFIX "/tmp/"
#endif



/*
web API usage example with mydown:
mydown -o OUTPUT_FILE -C INPUT_FILE "http://127.0.0.1:1234/compress?algo=zlib&size=OUTPUT_SIZE"
*/



typedef struct {
    u8      *data;      // whole data received
    u32     size;       // size
    u32     eoheader;   // where the header terminates
} ipc_client_request_t;

typedef struct {
    u8      *key;
    u8      *val;
    i32     valsz;      // just a placeholder
    int     val32;      // "int" is 64bit compatible on quickbms_4gb_files
} ipc_keyval_t;



static const i32    web_ipc_multithread     = 0;    // quickbms is single-threaded so do NOT enable this... never. works only with the web api
static const i32    web_ipc_no_keepalive    = 1;    // makes everything more simple, in fact without multi-threading there will be N connections with the first still open
static const i32    IPC_BUFFSZ              = 4096;
static const i32    do_FlushFileBuffers     = 0;

static const u8     web_ipc_help[] =
    "<html>"
    "<head>"
    "<title>QuickBMS %s experimental web API interface</title>"
    "</head>"
    "<body>"

    "This is just an example page for web browsers (multipart/form-data).<br>"
    "The API is better to be used with arguments in the URI and file sent as application/octet-stream, example:<br>"
    "POST http://127.0.0.1:1234/compress?algo=zlib&size=1028<br>"
    "<br><hr><br>"

    "/compress<br>"
    "<form action=\"/compress\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"text\" name=\"algo\" placeholder=\"zlib\"> Compression algorithm<br>"
    "<input type=\"number\" name=\"size\" placeholder=\"0\"> Decompressed size, need exact size (0 is for recompression only)<br>"
    "<input type=\"number\" name=\"dict\" placeholder=\"0\"> Optional dictionary, use backslash as escape for binary data<br>"
    "<input type=\"file\" name=\"data\"> Raw compressed data<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/crypt<br>"
    "<form action=\"/crypt\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"text\" name=\"algo\" placeholder=\"aes\"> Encryption algorithm<br>"
    "<input type=\"text\" name=\"key\" placeholder=\"\"> Key, use backslash as escape for binary data<br>"
    "<input type=\"text\" name=\"ivec\" placeholder=\"\"> Ivec, use backslash as escape for binary data<br>"
    "<input type=\"number\" name=\"mode\" placeholder=\"0\"> mode: 0=decrypt, 1=encrypt<br>"
    "<input type=\"checkbox\" name=\"hash\" value=\"1\"> set if this is an hash or crc algorithm<br>"
    "<input type=\"file\" name=\"data\"> Raw encrypted data<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/script<br>"
    "<form action=\"/script\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"checkbox\" name=\"verbose\" value=\"1\"> verbose<br>"
    "<input type=\"file\" name=\"data\"> quickbms script to load and test<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/file (test with decompression and size check, requires /script called before)<br>"
    "<form action=\"/file\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"hidden\" name=\"test\" value=\"1\">"
    "<input type=\"checkbox\" name=\"verbose\" value=\"1\"> verbose<br>"
    "<input type=\"file\" name=\"data\"> input file to test with the previous quickbms script<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/file (list only, requires /script called before)<br>"
    "<form action=\"/file\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"hidden\" name=\"list\" value=\"1\">"
    "<input type=\"checkbox\" name=\"verbose\" value=\"1\"> verbose<br>"
    "<input type=\"file\" name=\"data\"> input file to test with the previous quickbms script<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/tree (requires both /script and /file called before)<br>"
    "<form action=\"/tree\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"number\" name=\"mode\" value=\"5\"> view mode: "myhelp_option_t"<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "/math<br>"
    "<form action=\"/math\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"text\" name=\"var1\" placeholder=\"\"> first number<br>"
    "<input type=\"text\" name=\"op\" placeholder=\"\"> operator<br>"
    "<input type=\"text\" name=\"var2\" placeholder=\"\"> second number<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"

    "<!--"  // commented
    "/calldll<br>"
    "<form action=\"/calldll\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"text\" name=\"dll\" placeholder=\"\"> DLL name (full path or in same folder of quickbms.exe)<br>"
    "<input type=\"text\" name=\"func\" placeholder=\"\"> function or offset<br>"
    "<input type=\"text\" name=\"conv\" placeholder=\"\"> calling convention (stdcall, cdecl and so on)<br>"
    "<input type=\"text\" name=\"arg\" placeholder=\"\"> all the arguments to pass to the function, example: \"arg 1\" arg2 #INPUT# #INPUT_SIZE#<br>"
    "<input type=\"file\" name=\"data\"> optional file to use with #INPUT#/#INPUT_SIZE# in arguments<br>"
    "<input type=\"submit\">"
    "</form>"
    "<br><hr><br>"
    "-->"   // commented

    "/quit<br>"
    "<form action=\"/quit\" method=\"post\" enctype=\"multipart/form-data\">"
    "<input type=\"submit\">"
    "</form>"

    "<br><hr><br>"

    "</body>"
    "</html>";



static const u8     web_ipc_icons_folder[]  = "\x47\x49\x46\x38\x39\x61\x14\x00\x16\x00\xc2\x00\x00\xff\xff\xff\xff\xcc\x99\xcc\xff\xff\x99\x66\x33\x33\x33\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x21\xfe\x4e\x54\x68\x69\x73\x20\x61\x72\x74\x20\x69\x73\x20\x69\x6e\x20\x74\x68\x65\x20\x70\x75\x62\x6c\x69\x63\x20\x64\x6f\x6d\x61\x69\x6e\x2e\x20\x4b\x65\x76\x69\x6e\x20\x48\x75\x67\x68\x65\x73\x2c\x20\x6b\x65\x76\x69\x6e\x68\x40\x65\x69\x74\x2e\x63\x6f\x6d\x2c\x20\x53\x65\x70\x74\x65\x6d\x62\x65\x72\x20\x31\x39\x39\x35\x00\x21\xf9\x04\x01\x00\x00\x02\x00\x2c\x00\x00\x00\x00\x14\x00\x16\x00\x00\x03\x54\x28\xba\xdc\xfe\x30\xca\x49\x59\xb9\xf8\xce\x12\xba\xef\x45\xc4\x7d\x64\xa6\x29\xc5\x40\x7a\x6a\x89\x06\x43\x2c\xc7\x2b\x1c\x8e\xf5\x1a\x13\x57\x9e\x0f\x3c\x9c\x8f\x05\xec\x0d\x49\x45\xe1\x71\x67\x3c\xb2\x82\x4e\x22\x34\xda\x49\x52\x61\x56\x98\x56\xc5\xdd\xc2\x78\x82\xd4\x6c\x3c\x26\x80\xc3\xe6\xb4\x7a\xcd\x23\x2c\x4c\xf0\x8c\x3b\x01\x00\x3b";
static const u8     web_ipc_icons_unknown[] = "\x47\x49\x46\x38\x39\x61\x14\x00\x16\x00\xc2\x00\x00\xff\xff\xff\xcc\xff\xff\x99\x99\x99\x33\x33\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x21\xfe\x4e\x54\x68\x69\x73\x20\x61\x72\x74\x20\x69\x73\x20\x69\x6e\x20\x74\x68\x65\x20\x70\x75\x62\x6c\x69\x63\x20\x64\x6f\x6d\x61\x69\x6e\x2e\x20\x4b\x65\x76\x69\x6e\x20\x48\x75\x67\x68\x65\x73\x2c\x20\x6b\x65\x76\x69\x6e\x68\x40\x65\x69\x74\x2e\x63\x6f\x6d\x2c\x20\x53\x65\x70\x74\x65\x6d\x62\x65\x72\x20\x31\x39\x39\x35\x00\x21\xf9\x04\x01\x00\x00\x01\x00\x2c\x00\x00\x00\x00\x14\x00\x16\x00\x00\x03\x68\x38\xba\xbc\xf1\x30\x0c\x40\xab\x9d\x23\xbe\x69\x3b\xcf\x11\x07\x08\x02\x61\x9a\x22\xb8\x51\xe5\x79\x8a\x80\xca\xb5\x2e\x4a\x0c\x37\xb1\x02\xae\x30\xd6\xa7\x9d\xab\xd2\x23\xe9\x24\x9d\x0a\x8d\x40\x2a\xed\x2c\x4b\x13\xeb\x08\xfb\xd5\x7c\xbf\x27\x05\x88\xcd\x22\x2d\xbd\x8e\xf3\x4b\x3c\x25\xc7\xd5\x24\x94\xda\x19\x8a\xd9\x60\xf3\x5b\xcb\x93\xaf\xe9\x6a\x25\x3c\x3f\x97\x0c\x9a\x80\x81\x24\x38\x11\x40\x86\x41\x01\x09\x00\x3b";
static const u8     web_ipc_icons_binary[]  = "\x47\x49\x46\x38\x39\x61\x14\x00\x16\x00\xc2\x00\x00\xff\xff\xff\xcc\xff\xff\xcc\xcc\xcc\x99\x99\x99\x33\x33\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x21\xfe\x4e\x54\x68\x69\x73\x20\x61\x72\x74\x20\x69\x73\x20\x69\x6e\x20\x74\x68\x65\x20\x70\x75\x62\x6c\x69\x63\x20\x64\x6f\x6d\x61\x69\x6e\x2e\x20\x4b\x65\x76\x69\x6e\x20\x48\x75\x67\x68\x65\x73\x2c\x20\x6b\x65\x76\x69\x6e\x68\x40\x65\x69\x74\x2e\x63\x6f\x6d\x2c\x20\x53\x65\x70\x74\x65\x6d\x62\x65\x72\x20\x31\x39\x39\x35\x00\x21\xf9\x04\x01\x00\x00\x01\x00\x2c\x00\x00\x00\x00\x14\x00\x16\x00\x00\x03\x69\x48\xba\xbc\xf1\x30\x10\x40\xab\x9d\x24\xbe\x59\x0a\x10\x5d\x21\x00\x13\xa9\x71\x5e\x07\xa8\xa5\x09\xa1\x6b\xea\xb5\xae\x14\xdf\x6a\x41\xe8\xfc\x76\x83\x9d\x51\x68\xe8\xb3\x18\x29\x83\xe4\xa0\xe0\x03\x8a\x6e\x15\x25\xd3\x96\x93\x1d\x97\xbe\x2a\xd4\x82\xa5\x5a\x55\xc6\xee\xc4\x29\xf4\x84\xa7\xb4\xe3\x39\xeb\x21\x5f\xd1\x38\xeb\xda\x1b\x7f\xb3\xeb\x73\x98\x3b\xaf\xee\x8b\x09\x4a\x81\x82\x49\x3b\x11\x43\x87\x88\x53\x09\x00\x3b";
static const u8     web_ipc_icons_back[]    = "\x47\x49\x46\x38\x39\x61\x14\x00\x16\x00\xc2\x00\x00\xff\xff\xff\xcc\xff\xff\x99\x99\x99\x66\x66\x66\x33\x33\x33\x00\x00\x00\x00\x00\x00\x00\x00\x00\x21\xfe\x4e\x54\x68\x69\x73\x20\x61\x72\x74\x20\x69\x73\x20\x69\x6e\x20\x74\x68\x65\x20\x70\x75\x62\x6c\x69\x63\x20\x64\x6f\x6d\x61\x69\x6e\x2e\x20\x4b\x65\x76\x69\x6e\x20\x48\x75\x67\x68\x65\x73\x2c\x20\x6b\x65\x76\x69\x6e\x68\x40\x65\x69\x74\x2e\x63\x6f\x6d\x2c\x20\x53\x65\x70\x74\x65\x6d\x62\x65\x72\x20\x31\x39\x39\x35\x00\x21\xf9\x04\x01\x00\x00\x01\x00\x2c\x00\x00\x00\x00\x14\x00\x16\x00\x00\x03\x4b\x18\xba\xdc\xfe\x23\x10\xf2\x6a\x10\x33\x53\xbb\xa2\xce\xdc\xf5\x69\x9c\x37\x4e\x16\x76\x82\x90\x74\x06\x05\x37\x2a\x45\x1c\x6e\x58\x6d\x2f\xfb\x92\xeb\x35\x46\x50\xf1\x03\xea\x78\xc6\xa4\x91\xa6\x6c\x16\x28\x4e\xa7\x20\xda\x8c\xc0\xa8\x3a\x82\x40\x18\xd5\x56\x92\xde\xd0\x55\xfc\xe8\x91\xcf\x8b\x04\x00\x3b";
static const u8     web_ipc_icons_blank[]   = "\x47\x49\x46\x38\x39\x61\x14\x00\x16\x00\xa1\x00\x00\xff\xff\xff\xcc\xff\xff\x00\x00\x00\x00\x00\x00\x21\xfe\x4e\x54\x68\x69\x73\x20\x61\x72\x74\x20\x69\x73\x20\x69\x6e\x20\x74\x68\x65\x20\x70\x75\x62\x6c\x69\x63\x20\x64\x6f\x6d\x61\x69\x6e\x2e\x20\x4b\x65\x76\x69\x6e\x20\x48\x75\x67\x68\x65\x73\x2c\x20\x6b\x65\x76\x69\x6e\x68\x40\x65\x69\x74\x2e\x63\x6f\x6d\x2c\x20\x53\x65\x70\x74\x65\x6d\x62\x65\x72\x20\x31\x39\x39\x35\x00\x21\xf9\x04\x01\x00\x00\x01\x00\x2c\x00\x00\x00\x00\x14\x00\x16\x00\x00\x02\x13\x8c\x8f\xa9\xcb\xed\x0f\xa3\x9c\xb4\xda\x8b\xb3\xde\xbc\xfb\x0f\x86\x49\x01\x00\x3b";



i32 g_socket_printf_sd = 0;
i32 ipc_handle_command_chunked_send(i32 sd, u8 *data, i32 size) {
    i32 t;
    u8  tmp[32];
    if(size > 0) {  // 0 is the terminator
        t = sprintf(tmp, "%x\r\n", size);
        if(mysend(NULL, sd, tmp,    t   ) < 0) return -1;
        if(mysend(NULL, sd, data,   size) < 0) return -1;
        if(mysend(NULL, sd, "\r\n", 2   ) < 0) return -1;
    }
    return size;
}
#define socket_printf_ \
    __builtin_va_list __local_argv; __builtin_va_start( __local_argv, __format ); \
    __retval = vspr(&buff, __format, __local_argv ); \
    __builtin_va_end( __local_argv ); \
    return ipc_handle_command_chunked_send(g_socket_printf_sd, buff, __retval);
i32 /*__cdecl*/ socket_printf(const char *__format, ...) {
    static  u8  *buff = NULL;
    register i32 __retval;
    socket_printf_
}
i32 /*__cdecl*/ socket_fprintf(FILE *__stream, const char *__format, ...) {
    static  u8  *buff = NULL;
  register i32 __retval;
    if((__stream == stdout) || (__stream == stderr)) {
    socket_printf_
    }
  __builtin_va_list __local_argv; __builtin_va_start( __local_argv, __format );
  __retval = /*__mingw_*/vfprintf( __stream, __format, __local_argv );
  __builtin_va_end( __local_argv );
  return __retval;
}



i32 ipc_handle_command_chunked(i32 sd, i32 init, u8 *content_type) {
    i32     len,
            ret     = 0;
    u8      buff[300];

    if(init) {

        if(!content_type) content_type = "text/plain";

        len = sprintf(buff,
            "HTTP/1.1 %d OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Encoding: identity\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            "Pragma: no-cache\r\n"
            "Expires: 0\r\n"
            "Connection: %s\r\n"
            //"Keep-Alive: timeout=5, max=100\r\n"
            ,
            200,
            content_type,
            web_ipc_no_keepalive ? "close" : "keep-alive"    // no problem, if the connection is "close" it will be terminated automatically
        );
        buff[len++] = '\r';
        buff[len++] = '\n';
        ret = mysend(NULL, sd, buff, len);

        g_socket_printf_sd = sd;
        real_printf  = socket_printf;
        real_fprintf = socket_fprintf;

    } else {

        len = sprintf(buff, "%x\r\n\r\n", 0);
        ret = mysend(NULL, sd, buff, len);

        g_socket_printf_sd = 0;
        real_printf  = backup_real_printf;
        real_fprintf = backup_real_fprintf;

    }
    return ret;
}



u8 *ipc_handle_command(i32 sd, ipc_keyval_t *kv, u8 *in, i32 zsize, i32 *ret_size, u8 **content_type) {
    i32     i,
            t,
            size        = 0,
            ret         = -1,
            chunked     = 0,
            verbose_bck = g_verbose,
            alternative_output  = 0,
            id;
    u8      *out        = NULL,
            *tmp        = NULL,
            *in_bck     = in;

    if(ret_size) *ret_size = 0;
    if(content_type) *content_type = NULL;
    if(!kv) goto quit;
    if(!in || (zsize < 0)) goto quit;   // do NOT quit with zsize 0 because it's used in many situations

    g_verbose = 0;  // to avoid problems with -v and /file

    ipc_keyval_t    argv[32];
    memset(argv, 0, sizeof(argv));

    if(stristr(kv[0].key, "compress")) {

        for(i = 1; kv[i].key; i++) {
                   if(!stricmp(kv[i].key, "algo") || !stricmp(kv[i].key, "comtype")) {
                argv[1].val = kv[i].val;

            } else if(stristr(kv[i].key, "dict")) {
                if(stristr(kv[i].key, "64")) kv[i].val32 = unbase64(kv[i].val, -1, kv[i].val, -1);
                else                         kv[i].val32 = cstring( kv[i].val, kv[i].val, -1, NULL, NULL);
                argv[2].val   = kv[i].val;
                argv[3].val32 = kv[i].val32;

            } else if(!stricmp(kv[i].key, "size") || !stricmp(kv[i].key, "decompressed_size") || !stricmp(kv[i].key, "output_size")) {
                size = myatoi(kv[i].val);
            }
        }

        // mandatory arguments and checks
        if(!argv[1].val || !argv[1].val[0]) argv[1].val = "zlib";   // necessary or the default comtype is copy!

        if(CMD_ComType_func(0, argv[1].val, argv[2].val, argv[3].val32) < 0) goto quit;

        if(size <= 0) size = MAXZIPLEN(zsize);
        out = realloc(out, size + 1);
        if(!out) goto quit;

        int     outsize = size;
        ret = perform_compression(in, zsize, &out, size, &outsize, 0);



    } else if(stristr(kv[0].key, "crypt")) {

        for(i = 1; kv[i].key; i++) {
                   if(!stricmp(kv[i].key, "algo") || stristr(kv[i].key, "cryption")) {
                argv[1].val = kv[i].val;

            } else if(stristr(kv[i].key, "key")) {
                if(stristr(kv[i].key, "64")) kv[i].val32 = unbase64(kv[i].val, -1, kv[i].val, -1);
                else                         kv[i].val32 = cstring( kv[i].val, kv[i].val, -1, NULL, NULL);
                argv[2].val   = kv[i].val;
                argv[3].val32 = kv[i].val32;

            } else if(stristr(kv[i].key, "ivec")) {
                if(stristr(kv[i].key, "64")) kv[i].val32 = unbase64(kv[i].val, -1, kv[i].val, -1);
                else                         kv[i].val32 = cstring( kv[i].val, kv[i].val, -1, NULL, NULL);
                argv[4].val   = kv[i].val;
                argv[5].val32 = kv[i].val32;

            } else if(!stricmp(kv[i].key, "mode") || !stricmp(kv[i].key, "encrypt")) {
                argv[6].val32 = myatoi(kv[i].val);

            } else if(!stricmp(kv[i].key, "hash") || !stricmp(kv[i].key, "crc") || !stricmp(kv[i].key, "checksum")) {
                alternative_output = 1;
            }
        }

        // mandatory arguments
        if(!argv[1].val) argv[1].val = "aes";

        ret = quickbms_encryption(argv[1].val, argv[2].val, argv[3].val32, argv[4].val, argv[5].val32, argv[6].val32, in, zsize);
        if(ret < 0) goto quit;
        out = in;

        if(alternative_output) {
            out = NULL;
            ret = spr(&out,
                "QUICKBMS_HEXHASH %s\n"
                "QUICKBMS_CRC 0x%"PRIx"\n",
                (get_var_from_name("QUICKBMS_HEXHASH", -1) >= 0) ? get_var  (get_var_from_name("QUICKBMS_HEXHASH", -1)) : (u8*)"",
                (get_var_from_name("QUICKBMS_CRC",     -1) >= 0) ? get_var32(get_var_from_name("QUICKBMS_CRC",     -1)) : 0
            );
        }



    } else if(stristr(kv[0].key, "script")) {   // useless, probably just for testing the script... don't know

        for(i = 1; kv[i].key; i++) {
                   if(stristr(kv[i].key, "verbose")) {
                g_verbose = myatoi(kv[i].val) ? 1 : 0;
            }
        }

        if(ipc_handle_command_chunked(sd, 1, NULL) < 0) goto quit;
        chunked = 1;

        i32 cmd = 0;
        bms_init(0);
        cmd = parse_bms(NULL, in, cmd, 0);

        printf("\n%d commands parsed from the script\n", cmd);



    } else if(!stricmp(kv[0].key, "file")) { // useless, probably just for testing the script... don't know

        for(i = 1; kv[i].key; i++) {
                   if(stristr(kv[i].key, "list")) {
                g_list_only = 1;

            } else if(stristr(kv[i].key, "test")) {
                g_void_dump = 1;    // default

            } else if(stristr(kv[i].key, "verbose")) {
                g_verbose = myatoi(kv[i].val) ? -1 : 0;
            }
        }

        if(ipc_handle_command_chunked(sd, 1, NULL) < 0) goto quit;
        chunked = 1;

        memory_file_t   *memfile    = NULL;
        memfile = &g_memory_file[-g_replace_fdnum0];
        FREE(memfile->data);
        memset(memfile, 0, sizeof(memory_file_t));
        memfile->data = in;
        memfile->size = zsize;

        time_t  benchmark;
        benchmark = time(NULL);

        start_bms(-1, 0, 0, NULL, NULL, NULL, NULL);

        quickbms_statistics(benchmark);
        if(memfile->data != in) {
            fprintf(stderr, "\nError: memfile->data (%p) != in (%p)\n", memfile->data, in);
            STD_ERR(QUICKBMS_ERROR_EXTRA);
        }
        memfile->data = NULL;   // it's not a real memory_file!
        bms_init(1);    // at the end of the process



    } else if(!stricmp(kv[0].key, "tree")) { // useless, probably just for testing the script... don't know

        id = 0;

        for(i = 1; kv[i].key; i++) {
                   if(stristr(kv[i].key, "mode")) {
                g_extracted_file_tree_view_mode = myatoi(kv[i].val);

            } else if(stristr(kv[i].key, "id")) {
                id = myatoi(kv[i].val);
            }
        }

        tmp = NULL;
        switch(g_extracted_file_tree_view_mode) {
            case extracted_file_tree_view_json1:
            case extracted_file_tree_view_json2: tmp = "application/jso";   break;
            case extracted_file_tree_view_web:   tmp = "text/html";         break;
            default: break;
        }
        if(ipc_handle_command_chunked(sd, 1, tmp) < 0) goto quit;
        chunked = 1;

        if(g_extracted_file_tree_view_mode >= 0) {
            if(!id) {
                extracted_file_tree_free(&g_extracted_file_tree);
                extracted_file_tree_build(&g_extracted_file_tree);
            }
            extracted_file_tree_view(g_extracted_file_tree, 0, g_extracted_file_tree_view_mode, id);
        }



    } else if(!stricmp(kv[0].key, "quickbms")) {

        // will be implemented in future maybe
        // but currently /tree with web visualization does a similar job


    } else if(!stricmp(kv[0].key, "math")) {

        t = 1;
        for(i = 1; kv[i].key; i++) {
                   if(!stricmp(kv[i].key, "var")) {
                if(t <  1) t = 1;
                if(t == 2) t = 3;
                if(t >  4) t = 4;
                argv[t].val32 = myatoi(kv[i].val);
                t++;

            } else if(stristr(kv[i].key, "1")) {
                argv[1].val32 = myatoi(kv[i].val);
                t = 2;

            } else if(stristr(kv[i].key, "op")) {
                argv[2].val32 = kv[i].val[0];

            } else if(stristr(kv[i].key, "2")) {
                argv[3].val32 = myatoi(kv[i].val);
                t = 4;
            }
        }
        t = math_operations(-1, argv[1].val32, argv[2].val32, argv[3].val32, 1);
        ret = spr(&out, "%"PRId"\n", t);
        if(content_type) *content_type = "text/plain";



    } else if(!stricmp(kv[0].key, "endian")) {  // some rare algorithms "may" require a different endianess...

        for(i = 1; kv[i].key; i++) {
                   if(stristr(kv[i].key, "big")) {
                g_endian = MYBIG_ENDIAN;

            } else if(stristr(kv[i].key, "little") || stristr(kv[i].key, "net")) {
                g_endian = MYLITTLE_ENDIAN;

            } else if(stristr(kv[i].key, "swap")) {
                swap_endian();
            }
        }



    } else if(!stricmp(kv[0].key, "calldll")) { // not really implemented, just a placeholder

        for(i = 1; kv[i].key; i++) {
                   if(stristr(kv[i].key, "dll")) {
                argv[1].val = kv[i].val;

            } else if(stristr(kv[i].key, "func") || stristr(kv[i].key, "off")) {
                argv[2].val = kv[i].val;

            } else if(stristr(kv[i].key, "conv")) {
                argv[3].val = kv[i].val;

            } else if(stristr(kv[i].key, "ret")) {
                argv[4].val = kv[i].val;

            } else if(stristr(kv[i].key, "arg")) {
                argv[5].val = kv[i].val;

            }
        }

        if(argv[5].val && stristr(argv[5].val, "#OUTPUT#")) {
            size = zsize;
            out = realloc(out, size + 1);
            if(!out) goto quit;
        }

        argv[4].val = "ret";
        t = spr(&tmp, "\"%s\" \"%s\" \"%s\" \"%s\" %s", argv[1].val, argv[2].val, argv[3].val, argv[4].val, argv[5].val);
        ret = quickbms_calldll_pipe(tmp, in, zsize, out, size);
        FREE(tmp)

        //ret = getvarnum("ret", -1);
        if(argv[5].val && stristr(argv[5].val, "#OUTPUT#")) {
            // ret may be the size of the output
        } else {
            // ret is probably a number, strings 
            ret = getvarnum("ret", -1);
            ret = spr(&out, "%"PRId"\n", ret);
            if(content_type) *content_type = "text/plain";
        }



    } else if(!stricmp(kv[0].key, "quit") || !stricmp(kv[0].key, "exit") || !stricmp(kv[0].key, "cleanexit")) {

        myexit(-2); // -2 instead of -1 because that's used for other things



    } else if(!kv[0].key || !kv[0].key[0] || !stricmp(kv[0].key, "help")) {

        ret = spr(&out, (u8 *)web_ipc_help, VER);
        if(content_type) *content_type = "text/html";



    } else if(!strnicmp(kv[0].key, "icons/", 6)) {
        tmp = NULL;
        if(!stricmp(kv[0].key + 6, "folder.gif"))   { tmp = (u8*)web_ipc_icons_folder;  ret = sizeof(web_ipc_icons_folder) - 1; }
        if(!stricmp(kv[0].key + 6, "unknown.gif"))  { tmp = (u8*)web_ipc_icons_unknown; ret = sizeof(web_ipc_icons_unknown) - 1; }
        if(!stricmp(kv[0].key + 6, "binary.gif"))   { tmp = (u8*)web_ipc_icons_binary;  ret = sizeof(web_ipc_icons_binary) - 1; }
        if(!stricmp(kv[0].key + 6, "back.gif"))     { tmp = (u8*)web_ipc_icons_back;    ret = sizeof(web_ipc_icons_back) - 1; }
        if(!stricmp(kv[0].key + 6, "blank.gif"))    { tmp = (u8*)web_ipc_icons_blank;   ret = sizeof(web_ipc_icons_blank) - 1; }
        if(tmp) {
            out = realloc(out, ret);
            memcpy(out, tmp, ret);
            if(content_type) *content_type = "image/gif";
        }



    } else {

        // nothing, for example images, icons and so on

    }

quit:
    if(ret < 0) {
        if(out != in) {
            FREE(out)
        }
    } else {
        if(ret_size) *ret_size = ret;
    }
    if(chunked) {
        ipc_handle_command_chunked(sd, 0, NULL);
        if(content_type && *content_type) *content_type = NULL;
    } else {
        if(content_type && !*content_type) *content_type = "application/octet-stream";
    }
    // restore
    g_verbose   = verbose_bck;
    g_list_only = 0;
    g_void_dump = 1;
    if(in_bck != in) {  // in case I forget to not touch "in"
        fprintf(stderr, "\nError: in_bck (%p) != in (%p)\n", in_bck, in);
        STD_ERR(QUICKBMS_ERROR_EXTRA);
    }
    return out;
}



i32 ipc_bind_socket(struct sockaddr_in *peer) {
    static struct linger ling = {1,1};
    static i32  on  = 1;
    i32     sd;

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sd < 0) return(-2);
    if(peer->sin_port) {
        if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))
          < 0) STD_ERR(QUICKBMS_ERROR_EXTRA);
    }
    if(bind(sd, (struct sockaddr *)peer, sizeof(struct sockaddr_in))
      < 0) return(-6);
    listen(sd, SOMAXCONN);
    setsockopt(sd, SOL_SOCKET, SO_LINGER,    (char *)&ling, sizeof(ling));
    setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&on,   sizeof(on));
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&on,   sizeof(on));
    return(sd);
}



ipc_keyval_t *web_ipc_uri(u8 *req_name) {
    ipc_keyval_t *kv = NULL;
    i32     argc,
            do_argv = 0;
    u8      *fname,
            *p,
            *v,
            *l;

    if(!req_name) return(NULL);
    fname = req_name;

    if((strlen(fname) > 2) && ((fname[1] == ':') || strchr("\\/", fname[0]))) {
        p = mystrrchrs(fname, "\\/");
        if(!p) p = strrchr(fname, ':');
        if(p) p++;
        else  p = fname;   // ???
        mymemmove(fname, p, -1);
    }

    for(p = fname; *p; p++) {
        if(strchr("?#", *p)) break;
    }
    if(*p == '?') do_argv = 1;
    *p = 0;

    p++;
    argc = 0;
    kv = realloc(kv, (argc + 1) * sizeof(ipc_keyval_t));
    memset(&kv[argc], 0, sizeof(ipc_keyval_t));
    kv[argc].key = mystrdup_simple(fname);
    argc++;
    if(do_argv) {
        for(;;) {
            for(l = p; *l && (*l != '&'); l++);
            if(!*l) l = NULL;
            else *l = 0;
            v = strchr(p, '=');
            if(v) *v++ = 0;
            kv = realloc(kv, (argc + 1) * sizeof(ipc_keyval_t));
            memset(&kv[argc], 0, sizeof(ipc_keyval_t));
            kv[argc].key = mystrdup_simple(p);
            kv[argc].val = mystrdup_simple(v);
            argc++;
            if(!l) break;
            p = l + 1;
        }
    }
    kv = realloc(kv, (argc + 1) * sizeof(ipc_keyval_t));
    memset(&kv[argc], 0, sizeof(ipc_keyval_t));
    return kv;
}



// ipc_client_request_t has not been used on purpose for reusing the function in other possible contexts
i32 ipc_client_REQUEST_UPDATE(i32 sd, u8 **out, u32 *outsz, u8 *in, i32 insz) {
    if(insz < 0) return 0;  // insz 0 is valid due to the +1 delimiter
    if(!out) return -1;
    // no integer overflow checks, this is a testing tool
    *out = realloc(*out, *outsz + insz + 1);
    if(!*out) return -1;
    if(sd > 0) {
        i32     i, t;
        for(i = 0; i < insz; i += t) {
            t = myrecv(NULL, sd, *out + *outsz + i, insz - i);
            if(t <= 0) return -1;
        }
    } else {
        if(in) memcpy(*out + *outsz, in, insz);
        else   memset(*out + *outsz, 0,  insz);
    }
    *outsz += insz;
    (*out)[*outsz] = 0;
    return insz;
}



// code from onlywebs.c
quick_thread(web_ipc_server, i32 sd) {

    #define IPC_LAME_FREE { \
        FREE(boundary) \
        FREE(command) \
        FREE(req_name) \
        FREE(ipc_client_request.data) \
        ipc_client_request.eoheader = 0; \
        ipc_client_request.size = 0; \
        if(kv) { \
            for(t = 0; kv[t].key; t++) { \
                FREE(kv[t].key) \
                FREE(kv[t].val) \
            } \
            FREE(kv) \
        } \
    }

    ipc_keyval_t    *kv     = NULL;
    ipc_client_request_t    ipc_client_request  = {NULL,0,0};
    i32     i               = 0,
            t               = 0,
            len             = 0,
            buffsz          = 0,
            keepalive       = 0,
            first_byte_pos  = 0,
            last_byte_pos   = 0,
            cseq            = 0,
            http_code       = 200,
            etag            = 0;    // incremental
    u16     port            = 0;
    u8      tmp[32]         = "",
            *buff           = NULL,
            *p              = NULL,
            *s              = NULL,
            *l              = NULL,
            *req_name       = NULL,
            *command        = NULL,
            *content_type   = NULL,
            *boundary       = NULL;

    buffsz = IPC_BUFFSZ;
    buff = malloc(buffsz + 1);
    if(!buff) goto quit;

    etag = time(NULL);
//redo:
for(;;) {
    etag++;  // useful for etag

    IPC_LAME_FREE

            len             = 0;
            keepalive       = 0;
            first_byte_pos  = 0;
            last_byte_pos   = 0;    //g_last_byte_pos;
            http_code       = 200;
            content_type    = NULL; // static

    {
        do {
            if(buffsz <= len) { // so in any case buff is allocated with some bytes
                buffsz += IPC_BUFFSZ;
                buff = realloc(buff, buffsz + 1);
                if(!buff) goto quit;
            }

            // this is a testing tool so it's useful to have a big timeout or nothing at all
            if(timeout(sd, 300) < 0) goto quit;

            t = myrecv(NULL, sd, buff + len, 1);

            if(t <= 0) goto quit;
            len += t;
            buff[len] = 0;

            {
                l = strstr(buff, "\r\n\r\n");
                if(l) {
                    l += 4;
                } else {
                    l = strstr(buff, "\n\n");
                    if(l) l += 2;
                }
            }
        } while(!l);

        ipc_client_request.eoheader = l - buff; //len;
        if(ipc_client_REQUEST_UPDATE(0, &(ipc_client_request.data), &(ipc_client_request.size), buff, len) < 0) goto quit;

        // handle Content-Length
        s = stristr(buff, "\nContent-Length:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            len = atoi(s);
            if(len < 0) goto quit;
            if(ipc_client_REQUEST_UPDATE(sd, &(ipc_client_request.data), &(ipc_client_request.size), NULL, len) < 0) goto quit;
        }

        s = stristr(buff, "\nTransfer-Encoding:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            if(!strnicmp(s, "chunk", 5)) {
                for(;;) {
                    for(i = 0; i < sizeof(tmp);) {
                        if(myrecv(NULL, sd, tmp + i, 1) <= 0) goto quit;
                        i++;
                        if(tmp[i - 1] == '\n') break;   // \r\n
                    }
                    if(i >= sizeof(tmp)) goto quit; // invalid chunk

                    if(ipc_client_REQUEST_UPDATE(0, &(ipc_client_request.data), &(ipc_client_request.size), tmp, i) < 0) goto quit;

                    sscanf(tmp, "%x", &len);
                    if(len < 0) goto quit;
                    len += 2;   // \r\n
                    if(ipc_client_REQUEST_UPDATE(sd, &(ipc_client_request.data), &(ipc_client_request.size), NULL, len) < 0) goto quit;
                    if(len <= 2) break; // len 0 + 2
                }
            }
        }

        s = stristr(buff, "\nConnection:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            if(tolower(*s) == 'k') keepalive = 1;
        }

        s = stristr(buff, "\nRange:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            last_byte_pos  = 0;
            first_byte_pos = 0;
            sscanf(s, "bytes=%d-%d", &first_byte_pos, &last_byte_pos);
            if(first_byte_pos < 0) first_byte_pos = 0;
            if(last_byte_pos  < 0) last_byte_pos  = 0;
            if(first_byte_pos || last_byte_pos) http_code = 206;
        }

        s = stristr(buff, "\nCSeq:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            cseq = atoi(s);
        }

        s = stristr(buff, "\nContent-Type:");
        if(s) {
            s = strchr(s, ':') + 1;
            while(*s && (*s <= ' ')) s++;
            if(stristr(s, "multipart/form-data")) {
                content_type = "multipart/form-data";
                s = stristr(s, "boundary=");
                if(s) {
                    spr(&boundary, "--%s", s + 9);
                    delimit(boundary);
                }
            }
        }

        for(p = buff; *p && (*p != '\r') && (*p != '\n'); p++);
        *p = 0;

        {
            // remove "HTTP/"
            for(--p; (p >= buff) && (*p > ' '); p--)
            *p = 0;
        }
        while((p >= buff) && (*p <= ' ')) *p-- = 0;

        mydown_hex2uri(buff);

        // skip possible spaces
        p = buff;
        while(*p && (*p <= ' ')) p++;

        // skip the command
        command = p;
        while(*p && (*p > ' ')) p++;
        if(*p) {
            *p++ = 0;
            while(*p && strchr("\\/ \t", *p)) p++;
        }
        //if(!*p) p = "\"\"";   // NEVER enable!
        req_name = p;

        command = mystrdup_simple(command);  // needed
        if(g_verbose) {
            printf("- command: %s\n", command);
        }

        //p = mystrrchrs(req_name, "\\/");
        //if(p) req_name = p + 1;

        p = req_name;
        while(*p) {
            l = strstr(p, "..");
            if(!l) break;
            p = l + 2;
        }
        while(*p && strchr("\\/", *p)) p++;
        req_name = p;

        // HTTP proxy
        p = strstr(req_name, "://");
        if(p) {
            for(s = req_name; s < p; s++) {
                if(!myisalnum(*s)) break;
            }
            if(s >= p) {
                l = strchr(p + 3, '/');
                if(l) mymemmove(req_name, l + 1, -1);
                else  req_name[0] = 0;
            }
        }

        {
            p = strchr(req_name, ':');
            if(p) mymemmove(req_name, p + 1, -1);
        }

        // moved in hex2uri
        //for(p = req_name; *p; p++) {
        //    if(strchr(":;%&?#", *p)) *p = 0;
        //}

        //if(strlen(req_name) > MAX_NAME) req_name[MAX_NAME] = 0;
        //req_name = strip_quotes(req_name);
        //set_pathslash(req_name);

        req_name = mystrdup_simple(req_name);    // needed
        /*if(g_verbose)*/ {
            printf("- URI: /%s\n", req_name);
        }
    }

    kv = web_ipc_uri(req_name);
    if(!kv) goto quit;

    u8      *in     = ipc_client_request.data + ipc_client_request.eoheader;
    i32     zsize   = ipc_client_request.size - ipc_client_request.eoheader;

    if(content_type && !stricmp(content_type, "multipart/form-data")) {
        p = in;
        l = NULL;
        u8  *limit = in + zsize;
        i32 argc = 1;
        i32 file_is_loaded = 0;
        while(p < limit) {
            l = mymemmem(p, boundary, limit - p, -1);
            if(!l) break;
            p = l;

            l = mymemmem(p, "\r\n", limit - p, 2);
            if(!l) break;
            p = l + 2;

            // Content-Disposition
            content_type = NULL;    // used to identify the form with the file
            u8 *name = NULL;
            for(;;) {
                l = mymemmem(p, "\r\n", limit - p, 2);
                if(!l) break;
                *l = 0;
                if(p == l) {
                    p = l + 2;
                    break;
                }
                if(!strcmpx(p, "Content-Type:")) {
                    content_type = p;
                }
                if(!name) {
                    name = stristr(p, "name=\"");
                    if(name) {
                        name += 6;
                        for(s = name; *s && (*s != '\"'); s++);
                        *s = 0;
                    }
                }
                p = l + 2;
            }
            if(!l) break;

            // data
            l = mymemmem(p, boundary, limit - p, -1);
            if(!l) break;
            l -= 2; // \r\n
            if(l < p) break;
            if(content_type && !file_is_loaded) {
                file_is_loaded = 1;
                t = limit - p;  // total size
                zsize = l - p;  // data size
                mymemmove(in, p, t);
                in[zsize] = 0;  // necessary and it's ok, we have \r\n and use mymemmem
                p = in;
                l = p + zsize;
                limit = p + t;
            } else {
                kv = realloc(kv, (argc + 1) * sizeof(ipc_keyval_t));
                memset(&kv[argc], 0, sizeof(ipc_keyval_t));
                mystrdup(&(kv[argc].key), name);
                delimit(kv[argc].key);
                malloc_copy((void **)&kv[argc].val, p, kv[argc].valsz = l - p);
                //delimit(kv[argc].val);    // don't delimit it (it's already done by l-2), keep it open for possible multiple files
                argc++;
            }
            p = l;
        }
        kv = realloc(kv, (argc + 1) * sizeof(ipc_keyval_t));
        memset(&kv[argc], 0, sizeof(ipc_keyval_t));
        if(!file_is_loaded) {
            zsize = ((l && (l >= p)) ? l : limit) - p;
            mymemmove(in, p, zsize);
            in[zsize] = 0;
        }
    }

    /*if(g_verbose)*/ {
        printf("Content %p %d\n", in, (i32)zsize);
        for(i = 0; kv[i].key; i++) {
            printf("arg%d: %s =", (i32)i, kv[i].key);
            if(kv[i].val) printf(" %s", kv[i].val);
            printf("\n");
        }
    }

    u8      *out    = NULL;
    i32     size    = 0;
    out = ipc_handle_command(sd, kv, in, zsize, &size, &content_type);

    // the idea is to give the possibility of using the chunked encoding with textual data
    // and also the normal content-length way, that's meant to give 100% compatibility with
    // any client, even the simplest one (chunked encoding is not that simple)
    if(content_type) {
            static const u8 supported_options[] =
                    "OPTIONS, SETUP, GET, HEAD, POST, DELETE, PUT";

        printf("%d output bytes sent to browser\n", size);

        len = sprintf(buff,
            "HTTP/1.1 %d OK\r\n"
            "CSeq: %d\r\n"
            "Public: %s\r\n"
            "Allow: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-length: %d\r\n"
            "Connection: %s\r\n"
            //"Keep-Alive: timeout=5, max=100\r\n"
            "ETag: %u\r\n"
            "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            "Pragma: no-cache\r\n"
            "Expires: 0\r\n"
            "%s",
            http_code,
            cseq,
            supported_options,
            supported_options,
            content_type,
            (size > 0) ? size : 0,
            web_ipc_no_keepalive ? "close" : (keepalive ? "keep-alive" : "close"),
            etag,
            !stricmp(content_type, "application/octet-stream") ? "Content-Disposition: attachment; filename=\"output.dat\"\r\n" : ""
        );
        buff[len++] = '\r';
        buff[len++] = '\n';
        if(mysend(NULL, sd, buff, len) < 0) goto quit;
    }
    if(out) {
        t = 0;
        if(size > 0) t = mysend(NULL, sd, out, size);
        if(out != in) {
            // "in" points to another buffer, in the middle of the buffer
            FREE(out) // in case the output buffer is the same of the input like with encryption
        }
        if(t < 0) goto quit;
    }

//end_of_connection:
    if(g_verbose) fputc('\n', stdout);
    if(!web_ipc_no_keepalive) break;
    if(!keepalive) break;
}

quit:
    if(g_verbose) fputc('\n', stdout);
    IPC_LAME_FREE
    FREE(buff)
    if(sd > 0) close(sd);
    return(0);
}



#ifdef WIN32
    // nothing to do
#else
    #ifndef INVALID_HANDLE_VALUE    // linux
        #define INVALID_HANDLE_VALUE    0
    #endif
    #define GENERIC_READ    O_RDONLY
    #define GENERIC_WRITE   O_WRONLY
    #define GENERIC_ALL     O_RDWR
#define ERROR_MORE_DATA 234L
#define CREATE_NEW	1
#define CREATE_ALWAYS	2
#define OPEN_EXISTING	3
#define OPEN_ALWAYS	4
#define FILE_SHARE_READ 			0x00000001
#define FILE_SHARE_WRITE			0x00000002
#define FILE_SHARE_DELETE			0x00000004
#define FILE_ATTRIBUTE_NORMAL			0x00000080
#define PIPE_ACCESS_DUPLEX      O_RDWR      //3
#define PIPE_ACCESS_INBOUND     O_RDONLY    //1
#define PIPE_ACCESS_OUTBOUND    O_WRONLY    //2
#define PIPE_TYPE_BYTE	0
#define PIPE_TYPE_MESSAGE	4
#define PIPE_READMODE_BYTE	0
#define PIPE_READMODE_MESSAGE	2
#define PIPE_WAIT	0
#define PIPE_NOWAIT	1
#define PIPE_CLIENT_END 0
#define PIPE_SERVER_END 1
#define PIPE_UNLIMITED_INSTANCES 255
#define MAILSLOT_NO_MESSAGE			((DWORD)-1)
#define MAILSLOT_WAIT_FOREVER			((DWORD)-1)
#define NMPWAIT_NOWAIT	1
#define NMPWAIT_WAIT_FOREVER	((DWORD)-1)
#define NMPWAIT_USE_DEFAULT_WAIT	0
    HANDLE CreateFile(char *lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void *lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, void *hTemplateFile) {
        return open(lpFileName, dwDesiredAccess);
    }
    BOOL FlushFileBuffers(HANDLE hFile) {
        return 1;   // TRUE
    }
    HANDLE CreateNamedPipe(char *lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, void *lpSecurityAttributes) {
        HANDLE  h;
        h = mkfifo(lpName, 0666);
        if((h < 0) && (errno != EEXIST)) {
            h = INVALID_HANDLE_VALUE;
        } else {
            h = open(lpName, dwOpenMode);
            if(h < 0) {
                h = INVALID_HANDLE_VALUE;
            }
        }
        return h;
    }
    BOOL ReadFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToRead, DWORD *lpNumberOfBytesRead, void *lpOverlapped) {
        i32     ret;
        if(lpNumberOfBytesRead) *lpNumberOfBytesRead = 0;
        ret = read(hFile, lpBuffer, nNumberOfBytesToRead);
        if(ret < 0) return 0;   // FALSE
        if(lpNumberOfBytesRead) *lpNumberOfBytesRead = ret;
        return 1;   // TRUE
    }
    BOOL WriteFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToWrite, DWORD *lpNumberOfBytesWrite, void *lpOverlapped) {
        i32     ret;
        if(lpNumberOfBytesWrite) *lpNumberOfBytesWrite = 0;
        ret = write(hFile, lpBuffer, nNumberOfBytesToWrite);
        if(ret < 0) return 0;   // FALSE
        if(lpNumberOfBytesWrite) *lpNumberOfBytesWrite = ret;
        return 1;   // TRUE
    }
    BOOL CloseHandle(HANDLE hObject) {
        return close(hObject);
    }
    BOOL GetMailslotInfo(HANDLE  hMailslot, DWORD *lpMaxMessageSize, DWORD *lpNextSize, DWORD *lpMessageCount, DWORD *lpReadTimeout) {
        if(lpNextSize) {
            if(lpMaxMessageSize) *lpNextSize = *lpMaxMessageSize;
            else                 *lpNextSize = 0x7fffffff;  // ???
        }
        if(lpMessageCount) *lpMessageCount = 1;
        if(lpReadTimeout) *lpReadTimeout = 0;
        return 1;   // TRUE
    }
    DWORD GetLastError(void) {
        return 0;
    }
    void SetLastError(DWORD dwErrCode) {}
    BOOL GetNamedPipeHandleState(HANDLE  hNamedPipe, DWORD *lpState, DWORD *lpCurInstances, DWORD *lpMaxCollectionCount, DWORD *lpCollectDataTimeout, char *lpUserName, DWORD nMaxUserNameSize) {
        return 1;   // TRUE
    }
    BOOL ConnectNamedPipe(HANDLE hNamedPipe, void *lpOverlapped) {
        return 1;   // TRUE
    }
    BOOL DisconnectNamedPipe(HANDLE hNamedPipe) {
        return 1;   // TRUE
    }
    HANDLE CreateMailslot(char *lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, void *lpSecurityAttributes) {
        return CreateNamedPipe(lpName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    }
#endif



HANDLE namedpipe_create(u8 *name, i32 message_mode, i32 buffsz) {
    HANDLE  h;

    printf("- create named pipe file: %s\n", name);
#ifdef WIN32
    h = CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    } else
#endif
    {
        h = CreateNamedPipe(
            name,
            PIPE_ACCESS_DUPLEX,
            (message_mode ? (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE) : (PIPE_TYPE_BYTE | PIPE_READMODE_BYTE))
            | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            buffsz,
            buffsz,
            NMPWAIT_USE_DEFAULT_WAIT,
            NULL);
    }
    if(h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "\nError: unable to create named pipe %s\n", name);
        return INVALID_HANDLE_VALUE;
    }
    return h;
}



// the code seems complex because it's meant to work in any situation and condition
i32 namedpipe_read(HANDLE h, u8 *buff, i32 buffsz, i32 message_mode) {
    DWORD   dw,
            cbMessage,
            cMessage;
    i32     i,
            t,
            is_string;

    is_string   = 0;
    if(buffsz < 0) {
        is_string = 1;
        buffsz = -buffsz;
    }

    i = 0;
    if(message_mode < 0) {
        do {
            cbMessage   = 0;
            cMessage    = 0;
            dw          = buffsz;   // for Linux
            if(!GetMailslotInfo(h, &dw, &cbMessage, &cMessage, NULL)) return -1;
        } while(!cbMessage || (cbMessage == MAILSLOT_NO_MESSAGE) || !cMessage);
        if((i32)cbMessage < 0) return -1;
        if(cbMessage < buffsz) buffsz = cbMessage;
        while(i < buffsz) {
            if(do_FlushFileBuffers) FlushFileBuffers(h);
            dw = 0;
            if(!ReadFile(h, buff + i, buffsz - i, &dw, NULL)) return -1;
            if(!dw) break;
            i += dw;
        }
        u8      c;
        for(t = i; t < cbMessage; t += dw) {
            if(do_FlushFileBuffers) FlushFileBuffers(h);
            dw = 0;
            if(!ReadFile(h, &c, 1, &dw, NULL)) return -1;
        }
    } else {
        message_mode &= PIPE_READMODE_MESSAGE;
        while(i < buffsz) {
            if(do_FlushFileBuffers) FlushFileBuffers(h);
            if(message_mode) SetLastError(0);
            dw = 0;
            t = ReadFile(h, buff + i, message_mode ? (buffsz - i) : 1, &dw, NULL);
            if(!dw) break;
            i += dw;
            if(message_mode) {
                if(t) break;
                if(GetLastError() != ERROR_MORE_DATA) return -1;
            } else {
                if(!t) break;
                if(is_string) {
                    // only byte mode requires the string handling, I prefer to
                    // not use it in message mode for possible future improvements
                    // like sending multiple script lines in the command
                    if((i > 0) && (!buff[i - 1] || (buff[i - 1] == '\n'))) {
                        buff[--i] = 0;
                        break;
                    }
                }
            }
        }
    }
    if(is_string) buff[i] = 0;  // necessary
    return i;
}



// WriteFile was enough but I want to be sure that it works in any condition
i32 namedpipe_write(HANDLE h, u8 *buff, i32 buffsz) {
    DWORD   dw;
    i32     i,
            t;

    i = 0;
    while(i < buffsz) {
        dw = 0;
        if(!WriteFile(h, buff + i, buffsz - i, &dw, NULL)) return -1;
        if(do_FlushFileBuffers) FlushFileBuffers(h);
        i += dw;
    }
    return i;
}



/*
Named pipe example:
send: "comtype zlib"
send: "302"
send: 302 bytes of compressed data
send: "1028"
recv: "1028"
recv: 1028 bytes of decompressed data

Both PIPE_READMODE_BYTE and PIPE_READMODE_MESSAGE seem to work correctly,
maybe append a '\n' (line-feed) to the strings for being sure that they get
parsed in any condition, for example "comtype zlib\n".

Mailslots are one-way, they can't be used like named pipes:
CreateFile GENERIC_WRITE    + WriteFile
CreateMailslot              + ReadFile

Don't use input strings longer than 4Kb, files are limited to 2Gb since more
would be useless for any IPC that doesn't use shared memory.
*/
quick_thread(pipeslot_ipc_server, i32 ipc_mode) {
    HANDLE  h       = INVALID_HANDLE_VALUE,
            ret_h   = INVALID_HANDLE_VALUE;
    DWORD   status,
            dw,
            cbMessage,
            cMessage;
    i32     len,
            cmd,
            zsize,
            size,
            ret,
            t;
    u8      buff[IPC_BUFFSZ + 1],
            *in     = NULL,
            *out    = NULL,
            *ret_p  = NULL,
            path_to_name_separator  =
#ifdef WIN32
            PATHSLASH
#else
            '_'
#endif
            ;

    status = 0;
    if(ipc_mode <= 1) {
        // named pipe: 0 for byte, 1 for message
        sprintf(buff, "%s%s%c%s%s", PIPE_PREFIX, "pipe", path_to_name_separator, "quickbms", ipc_mode ? "" : "_byte");
        // \\.\pipe\quickbms\byte and \\.\pipe\quickbms can coexist but this code is meant
        // to be compatible as much as possible, even in the case it will be used on non-Windows in future
        h = namedpipe_create(buff, ipc_mode, IPC_BUFFSZ);
        if(h == INVALID_HANDLE_VALUE) return 0;
        printf("- IPC%d Named Pipe %s has been created\n", ipc_mode, buff);
        GetNamedPipeHandleState(h, &status, NULL, NULL, NULL, NULL, 0);
    } else {
        // mailslot
        sprintf(buff, "%s%s%c%s%c%s", PIPE_PREFIX, "mailslot", path_to_name_separator, "quickbms", path_to_name_separator, "send");
        h = CreateMailslot(buff, 0, MAILSLOT_WAIT_FOREVER, NULL);
        if(h == INVALID_HANDLE_VALUE) return 0;
        printf("- IPC%d Mailslot %s has been created\n", ipc_mode, buff);
#ifdef WIN32
        status = -1;    // just for recognizing mailslots
#endif  // leave status 0 on Linux
    }

    for(;;) {
        if(ipc_mode <= 1) {
            t = ConnectNamedPipe(h, NULL);
        } else {
            cbMessage   = 0;    // never trust the API
            cMessage    = 0;
            dw          = IPC_BUFFSZ;   // for Linux
            t = GetMailslotInfo(h, &dw, &cbMessage, &cMessage, NULL);
            if(!cMessage) {
                sleepms(100);   // necessary
                continue;
            }
        }
        if(!t) break;   // bad error
        printf("- IPC%d connection\n", ipc_mode);

        // command
        len = namedpipe_read(h, buff, -IPC_BUFFSZ, status);
        if(len <= 0) goto quit;
        printf("- IPC%d recv command: %s\n", ipc_mode, buff);

        bms_init(1);
        cmd = 0;
        cmd = parse_bms(NULL, buff, cmd, 0);
        CMD.type = CMD_NONE;

        // input size
        len = namedpipe_read(h, buff, -IPC_BUFFSZ, status);
        if(len <= 0) goto quit;
        printf("- IPC%d recv input size: %s\n", ipc_mode, buff);
        zsize = myatoi(buff);

        if(zsize <= 0) goto quit;
        in = realloc(in, zsize + 1);    // +1 is useful because I may implement other commands in future
        if(!in) goto quit;

        // input
        len = namedpipe_read(h, in, zsize, status);
        if(len <= 0) goto quit;
        zsize = len;
        in[zsize] = 0;
        printf("- IPC%d recv input: %d\n", ipc_mode, zsize);

        ret = -1;
        if(g_command[0].type == CMD_ComType) {

            // output size
            len = namedpipe_read(h, buff, -IPC_BUFFSZ, status);
            if(len <= 0) goto quit;
            printf("- IPC%d recv output size: %s\n", ipc_mode, buff);
            size = myatoi(buff);

            if(size <= 0) size = zsize;
            out = realloc(out, size);
            if(!out) goto quit;

            int     outsize = size;
            ret = perform_compression(in, zsize, &out, size, &outsize, 0);
            ret_p = out;

        } else if(g_command[0].type == CMD_Encryption) {

            ret = perform_encryption(in, zsize);
            ret_p = in;

        }

        ret_h = INVALID_HANDLE_VALUE;
        if(ret >= 0) {   // both negative and zero are often an error, but it's possible to compress a zero bytes file
            ret_h = h;
            if(ipc_mode == 2) { // mailslot is one-way
                sprintf(buff, "%s%s%c%s%c%s", PIPE_PREFIX, "mailslot", path_to_name_separator, "quickbms", path_to_name_separator, "recv");
                ret_h = CreateFile(buff, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if(ret_h == INVALID_HANDLE_VALUE) goto quit;
            }

            len = sprintf(buff, "%d\r\n", ret);
            printf("- IPC%d send output size: %s\n", ipc_mode, buff);
            if(namedpipe_write(ret_h, buff, len) < 0) goto quit;

            printf("- IPC%d send output: %d\n", ipc_mode, ret);
            if(namedpipe_write(ret_h, ret_p, ret) < 0) goto quit;
        }

quit:
        printf("- IPC%d close\n", ipc_mode);
        FlushFileBuffers(h);    // mandatory?
        if(ipc_mode <= 1) {
            DisconnectNamedPipe(h);
        }
        if((ret_h != h) && (ret_h != INVALID_HANDLE_VALUE)) {
            CloseHandle(ret_h);
        }
    }

    if(h != INVALID_HANDLE_VALUE) CloseHandle(h);
    FREE(in)
    FREE(out)
    printf("- IPC%d terminated\n", ipc_mode);
    return 0;
}



/*
includes all the available IPC methods:
- web API
- named pipe
- clipboard?
*/
int quickbms_ipc(int port) {
    struct  sockaddr_in peer,
                        peerl;
    files_t *files;
    i32     i,
            j,
            t,
            sd,
            sa,
            psz;
    u8      *tmp,
            *p,
            *s,
            *virtual_name,
            *name;

#ifdef WIN32
    WSADATA    wsadata;
    WSAStartup(MAKEWORD(1,0), &wsadata);
#endif

    // named pipe (byte), not thread-safe
    quick_threadx(pipeslot_ipc_server, (void *)0);
    sleepms(100);

    // named pipe (message), not thread-safe
    quick_threadx(pipeslot_ipc_server, (void *)1);
    sleepms(100);

    // mailslot, not thread-safe
    quick_threadx(pipeslot_ipc_server, (void *)2);
    sleepms(100);

    if(port <= 0) {
        printf("- named pipes and mailslot running, no web API available\n");
        // nothing to run so let's just keep the main thread sleeping
        for(;;) {
            sleepms(60 * 1000);
        }
    }

    // web API
    memset(&peerl, 0, sizeof(peerl));
    peerl.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY;
    peerl.sin_port        = myhtons(port);  // htons() annoyance on Linux...
    peerl.sin_family      = AF_INET;

    printf("\n- try listening on port %d\n", (i32)port);
    sd = ipc_bind_socket(&peerl);
    if(sd < 0) STD_ERR(QUICKBMS_ERROR_EXTRA);

    printf("\n- waiting connections on http://127.0.0.1:%d/\n\n", (i32)port);

    for(;;) {
        memset(&peer, 0, sizeof(peer));
        psz = sizeof(struct sockaddr_in);
        sa = accept(sd, (struct sockaddr *)&peer, &psz);
        if(sa < 0) STD_ERR(QUICKBMS_ERROR_EXTRA);

        printf("\n%s : %d\n",
            inet_ntoa(peer.sin_addr), (i32)myntohs(peer.sin_port)); // ntohs() annoyance on Linux...

        if(web_ipc_multithread) {
            if(!quick_threadx(web_ipc_server, (void *)sa)) close(sa);
        } else {
                              web_ipc_server(         sa);
        }
    }

    close(sd);
    return(0);
}

