/*
    Copyright 2010 Luigi Auriemma

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

// http://www.totalcmd.net/directory/packer.html
// http://www.ghisler.com/plugins.htm

#ifdef WIN32
#include "wcxhead.h"
#define WCX_PROC(X) X = (void *)GetProcAddress(hlib, #X)



// unicode bits:
// 00 = ascii   <- ascii
// 01 = ascii   <- unicode
// 10 = unicode <- ascii
// 11 = unicode <- unicode
char *wcx_strcpy(void *dst, void *src, int max, int unicode) {
    char    *p,
            *l,
            *s;

    s = (char *)src;
    p = (char *)dst;
    l = (char *)dst + max - 1;
    if(unicode & 2) l--;
    while(p < l) {
        if(!*s) break;
        *p++ = *s++;
        if(unicode & 1) s++;
        if(unicode & 2) *p++ = 0;
    }
    if(unicode & 2) *p++ = 0;
    *p = 0;
    return(dst);
}



/*
Packer plugins functions:
"OpenArchiveW"
"CloseArchive"
"ReadHeader"
"ReadHeaderEx"
"ReadHeaderExW"
"ProcessFile"
"ProcessFileW"
"SetChangeVolProc"
"SetChangeVolProcW"
"SetProcessDataProc"
"SetProcessDataProcW"
"PkSetCryptCallback"
"PkSetCryptCallbackW"
"PackFiles"
"PackFilesW"
"DeleteFiles"
"DeleteFilesW"
"GetPackerCaps"
"ConfigurePacker"
"StartMemPack"
"StartMemPackW"
"PackToMem"
"DoneMemPack"
"CanYouHandleThisFile"
"CanYouHandleThisFileW"
"PackSetDefaultParams"
*/
static HANDLE (*__stdcall OpenArchive) (tOpenArchiveData *ArchiveData) = NULL;
static HANDLE (*__stdcall OpenArchiveW) (tOpenArchiveDataW *ArchiveDataW) = NULL;
static int (*__stdcall ReadHeader) (HANDLE hArcData, tHeaderData *HeaderData) = NULL;
static int (*__stdcall ReadHeaderEx) (HANDLE hArcData, tHeaderDataEx *HeaderDataEx) = NULL;
static int (*__stdcall ReadHeaderExW) (HANDLE hArcData, tHeaderDataExW *HeaderDataExW) = NULL;
static int (*__stdcall ProcessFile) (HANDLE hArcData, int Operation, char *DestPath, char *DestName) = NULL;
static int (*__stdcall ProcessFileW) (HANDLE hArcData, int Operation, WCHAR *DestPath, WCHAR *DestName) = NULL;
static int (*__stdcall CloseArchive) (HANDLE hArcData) = NULL;
static void (*__stdcall PkSetCryptCallback) (tPkCryptProc pPkCryptProc,int CryptoNr,int Flags) = NULL;
static void (*__stdcall PkSetCryptCallbackW) (tPkCryptProcW pPkCryptProcW,int CryptoNr,int Flags) = NULL;
static void (*__stdcall SetChangeVolProc) (HANDLE hArcData, tChangeVolProc pChangeVolProc) = NULL;
static void (*__stdcall SetChangeVolProcW) (HANDLE hArcData, tChangeVolProcW pChangeVolProcW) = NULL;
static void (*__stdcall SetProcessDataProc) (HANDLE hArcData, tProcessDataProc pProcessDataProc) = NULL;
static void (*__stdcall SetProcessDataProcW) (HANDLE hArcData, tProcessDataProcW pProcessDataProcW) = NULL;
static int __stdcall pChangeVolProc(char *ArcName, int Mode)      { return 1; }
static int __stdcall pChangeVolProcW(WCHAR *ArcName, int Mode)    { return 1; }
static int __stdcall pProcessDataProc(char *FileName, int Size)   { return 1; }
static int __stdcall pProcessDataProcW(WCHAR *FileName, int Size) { return 1; }
static int __stdcall pPkCryptProc(int CryptoNumber, int mode, char *ArchiveName, char *Password, int maxlen) {
    static  char    cache[256] = "";
    if(mode == PK_CRYPT_SAVE_PASSWORD) {
        wcx_strcpy(cache, Password, sizeof(cache), 0);
    } else {
        wcx_strcpy(Password, cache, maxlen, 0);
    }
    return 0;  //FS_FILE_OK
}
static int __stdcall pPkCryptProcW(int CryptoNumber, int mode, WCHAR *ArchiveName, WCHAR *Password, int maxlen) {
    static  WCHAR   cache[256] = L"";
    if(mode == PK_CRYPT_SAVE_PASSWORD) {
        wcx_strcpy(cache, Password, sizeof(cache), 3);
    } else {
        wcx_strcpy(Password, cache, maxlen, 3);
    }
    return 0;  //FS_FILE_OK
}



// work-around for stupid plugins
u_int wcx_namecrc(unsigned char *data) {
    static u_int   *crctable = NULL;
    u_int   crc;

    if(!crctable) { // the the zlib table
        crctable = (u_int *)get_crc_table();
    }
    for(crc = 0; *data; data++) {
        //crc = ((crc >> 1) + ((crc & 1) << 31)) + *data;
        crc = crctable[(*data ^ crc) & 0xff] ^ (crc >> 8);
    }
    return(crc);
}



int wcx(char *plugin, char *input_file) {
    static WCHAR    tmpw[PATHSZ + 1] = L""; // declared as statics
    static char     tmp[PATHSZ + 1]  = "",  // for not using the stack (and used in QuickBMS compatibility)
                    fullpath[PATHSZ + 1]  = "";
    static HMODULE  hlib    = NULL;
    static HANDLE   hnd     = NULL;
    static int      workaroundsz = 0,
                    fullpath_len = 0;
    static u_int    *workaround  = NULL;    // work-around for stupid plugins
    tOpenArchiveData    oad;
    tOpenArchiveDataW   oadw;
    tHeaderDataExW      hdxw;
    tHeaderDataEx       hdx;
    tHeaderData         hd;
    int     attr,
            offset,
            size,
            extract,
            tot,
            i;
    u_int   crc;
    char    *fname      = NULL;

    if(!plugin && input_file) { // QuickBMS first loads the input_file and then the bms/plugin
        getcwd(tmp, PATHSZ);    // save the current folder for input_file
        return 0;
    }

    if(plugin && input_file) {
        hlib = LoadLibrary(plugin);
        if(!hlib) return -1;
        WCX_PROC(OpenArchive);
        WCX_PROC(OpenArchiveW);
        WCX_PROC(ReadHeader);
        WCX_PROC(ReadHeaderEx);
        WCX_PROC(ReadHeaderExW);
        WCX_PROC(ProcessFile);
        WCX_PROC(ProcessFileW);
        WCX_PROC(CloseArchive);
        WCX_PROC(PkSetCryptCallback);
        WCX_PROC(PkSetCryptCallbackW);
        WCX_PROC(SetChangeVolProc);
        WCX_PROC(SetChangeVolProcW);
        WCX_PROC(SetProcessDataProc);
        WCX_PROC(SetProcessDataProcW);

        memset(&oad,  0, sizeof(oad));
        memset(&oadw, 0, sizeof(oadw));
        memset(&hdxw, 0, sizeof(hdxw));
        memset(&hdx,  0, sizeof(hdx));
        memset(&hd,   0, sizeof(hd));

        getcwd((char *)fullpath, PATHSZ);   // avoid to waste another buffer
        chdir(tmp); // restore previous folder used by input_file

        hnd = 0;
        if(OpenArchive) {
            oad.ArcName  = input_file;
            oad.OpenMode = g_list_only ? PK_OM_LIST : PK_OM_EXTRACT;
            hnd = OpenArchive(&oad);
        } else if(OpenArchiveW) {
            wcx_strcpy(tmpw, input_file, sizeof(tmpw), 2);
            oadw.ArcName  = tmpw;
            oadw.OpenMode = g_list_only ? PK_OM_LIST : PK_OM_EXTRACT;
            hnd = OpenArchiveW(&oadw);
        }
        chdir((char *)fullpath);    // restore previous folder
        if(!hnd) {
            printf("\nError: the file %s is not available or is not supported by this WCX plugin\n", input_file);
            myexit(QUICKBMS_ERROR_EXTRA);
        }

        if(PkSetCryptCallback)  PkSetCryptCallback(pPkCryptProc, 0, PK_CRYPTOPT_MASTERPASS_SET);
        if(PkSetCryptCallbackW) PkSetCryptCallbackW(pPkCryptProcW, 0, PK_CRYPTOPT_MASTERPASS_SET);
        if(SetChangeVolProc)    SetChangeVolProc(hnd, pChangeVolProc);
        if(SetChangeVolProcW)   SetChangeVolProcW(hnd, pChangeVolProcW);
        if(SetProcessDataProc)  SetProcessDataProc(hnd, pProcessDataProc);
        if(SetProcessDataProcW) SetProcessDataProcW(hnd, pProcessDataProcW);
        return 0;
    }

    getcwd((char *)fullpath, PATHSZ);   // another boring compatibility fix for bugged plugins
    fullpath_len = strlen(fullpath);
    fullpath[fullpath_len] = PATHSLASH;
    fullpath_len++;

    tot = 0;
    for(;;) {
        if(ReadHeaderEx) {
            if(ReadHeaderEx(hnd, &hdx)) break;
            fname = hdx.FileName;
            size  = hdx.UnpSize;
            if(size <= 0) size = hdx.PackSize;
            attr  = hdx.FileAttr;
        } else if(ReadHeader) {
            if(ReadHeader(hnd, &hd)) break;
            fname = hd.FileName;
            size  = hd.UnpSize;
            if(size <= 0) size = hd.PackSize;
            attr  = hd.FileAttr;
        } else if(ReadHeaderExW) {
            if(ReadHeaderExW(hnd, &hdxw)) break;
            wcx_strcpy(tmp, hdxw.FileName, sizeof(tmp), 1);
            fname = tmp;
            size  = hdxw.UnpSize;
            if(size <= 0) size = hdxw.PackSize;
            attr  = hdxw.FileAttr;
        } else {
            break;
        }

        if(check_wildcards(fname, g_filter_files) < 0) attr = 0x10;  //skip the file

        extract = PK_SKIP;
        if(attr & 0x10) {   // directory
            //if(!g_list_only) create_dir(fname, 1, 0, 0, 1);  // useless
        } else {
            // GAUP and Total Commander are senseless:
            // indeed the correct method is using PK_EXTRACT for the files we want to
            // extract and PK_SKIP for the others BUT gaup will continue to extract
            // ever the same file if we use this correct method.
            // so TC maintains the list of files it needs to extract in a memory array
            // and after each ReadHeader it checks the one returned by the plugin with
            // the file it expects... useful in its situation, senseless for the rest
            // of the world.
            // the problem exists only with PK_EXTRACT and seems only with gaup_pro.wcx
            // so all the work-arounds I have adopted here are for fixing the stupid
            // behaviour of this plugin!

            crc = wcx_namecrc(fname);
            for(i = 0; i < tot; i++) {
                if(crc == workaround[i]) break;
            }
            if(i >= tot) {  // the file has not been extracted yet
                tot++;  // myalloc is buffered so there is no problem of performance
                myalloc((void *)&workaround, tot * sizeof(u_int), &workaroundsz);
                workaround[i] = crc;

                offset = (int)SetFilePointer(hnd, 0, NULL, FILE_CURRENT);   // not real offset!
                if(offset == (u_int)-1LL) offset = 0;

                printf("  %08x %-10u %s\n", offset, size, fname);
                if(g_listfd) {
                    fprintf(g_listfd, "  %08x %-10u %s\n", offset, size, fname);
                    fflush(g_listfd);
                }
                if(g_list_only) {
                    g_extracted_files++;
                } else {
                    fname = create_dir(fname, 1, 0, 0, 1);
                    if(!check_overwrite(fname, 0)) {
                        extract = PK_EXTRACT;
                        g_extracted_files++;
                    }
                }
            }
        }

        wcx_strcpy(fullpath + fullpath_len, fname, sizeof(fullpath) - fullpath_len, 0);
        fname = fullpath;

        if(ProcessFile) {
            ProcessFile(hnd, extract, NULL, fname);
        } else if(ProcessFileW) {
            wcx_strcpy(tmpw, fname, sizeof(tmpw), 2);   // needed for security
            ProcessFileW(hnd, extract, NULL, tmpw);
        } else {
            break;
        }
    }

    if(hnd) {
        CloseArchive(hnd);
        hnd = NULL;
    }
    if(hlib) {
        FreeLibrary(hlib);
        hlib = NULL;
    }
    return(tot);
}

#else
int wcx(char *plugin, char *input_file) {
    printf("\nError: Total Commander WCX plugins not supported on this platform\n");
    myexit(QUICKBMS_ERROR_EXTRA);
    return -1;
}
#endif

