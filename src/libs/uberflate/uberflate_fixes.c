/*
    Copyright 2013 Luigi Auriemma

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

#ifdef WIN32    // Windows only, Linux maybe via wine

static const u32    uberflate_patch_sign    = 0xdef1a7ed;



// source code here... I'm obligated to use it in this way otherwise
// the C compiler will add stack protections and other stuff



/*
static inline void * __memset ( void *dest, int fill, size_t len ) {
	void *discard_D;
	size_t discard_c;
	__asm__ __volatile__ ( "rep stosb"
			       : "=&D" ( discard_D ), "=&c" ( discard_c )
			       : "0" ( dest ), "1" ( len ), "a" ( fill )
			       : "memory" );
	return dest;
}
HANDLE WINAPI uberflate_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) {
    register unsigned char
            *p,
            *l,
            *f;

    // memset
    //p = (void *)lpFindFileData;
    //l = p + sizeof(WIN32_FIND_DATA);
    //while(p < l) *p++ = 0;
    __memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATA));

    lpFindFileData->dwFileAttributes    = FILE_ATTRIBUTE_NORMAL;
    lpFindFileData->nFileSizeLow        = uberflate_patch_sign;   // sign

    // strrchr
    for(p = (void *)lpFileName; *p; p++);
    for(--p; (void *)p >= (void *)lpFileName; p--) {
        if((*p == '\\') || (*p == '/')) {
            p++;
            break;
        }
    }

    // strncpy
    f = lpFindFileData->cFileName;
    l = f + MAX_PATH - 1;
    while((f < l) && *p) {
        *f++ = *p++;
    }
    *f = 0;
    return((HANDLE)0xdeadc0de); // different than INVALID_HANDLE_VALUE
    // yeah or the compiler will kill the uberflate_kzip_exe_patch - uberflate_FindFirstFile solution
}
*/
static const u8 uberflate_FindFirstFile[] =
    "\x55\x89\xe5\x57\x53\x8b\x55\x0c\x89\xd7\xb9\x40\x01\x00\x00\x31"
    "\xc0\xf3\xaa\xc7\x02\x80\x00\x00\x00\xc7\x42\x20" "\xed\xa7\xf1\xde"
    "\x8b\x45\x08\xeb\x01\x40\x80\x38\x00\x75\xfa\xeb\x0f\x8a\x08\x80"
    "\xf9\x2f\x74\x05\x80\xf9\x5c\x75\x03\x40\xeb\x06\x48\x3b\x45\x08"
    "\x73\xeb\x8d\x4a\x2c\x81\xc2\x2f\x01\x00\x00\xeb\x04\x88\x19\x41"
    "\x40\x39\xd1\x73\x06\x8a\x18\x84\xdb\x75\xf2\xc6\x01\x00\xb8\xde"
    "\xc0\xad\xde\x5b\x5f\x5d\xc2\x08\x00";



/*
DWORD WINAPI GetFileSize(HANDLE h, PDWORD hs) {
    if(hs) *hs = 0;
    return(uberflate_patch_sign);
}
*/
static const u8 uberflate_GetFileSize[] =
    "\x55\x89\xe5\x8b\x45\x0c\x85\xc0\x74\x06\xc7\x00\x00\x00\x00\x00"
    "\xb8" "\xed\xa7\xf1\xde" "\x5d\xc2\x08\x00";



/*
UINT WINAPI GetTempFileNameA(LPCSTR PathName, LPCSTR PrefixString, UINT Unique, LPSTR FileName) {
    char    *myname = (void *)static_name,
            *p,
            *f;

    f = myname;
    for(p = FileName;; p++, f++) {
        *p = *f;
        if(!*f) break;
    }
    return(1);
}
*/
static const u8 uberflate_GetTempFileNameA[] =
    "\x55\x89\xe5\x53\xba" "\xed\xa7\xf1\xde" "\x90\x31\xc0\x8a\x0c\x02\x8b"
    "\x5d\x14\x88\x0c\x03\x40\x80\x7c\x02\xff\x00\x75\xef\xb8\x01\x00"
    "\x00\x00\x5b\x5d\xc2\x10\x00";



// yeah I know that after this modification kzip_exe remains
// modified but it shouldn't affect the program



static int uberflate_patch(u8 *exe, int exe_off, const u8 *func, int func_len, void *value) {
    void    *filesize;

    filesize = uberflate_memmem(func, (void *)&uberflate_patch_sign, func_len, 4);
    if(!filesize) return(-1);
    memcpy(exe + exe_off, func, func_len);
    *(u32 *)(exe + exe_off + (filesize - (void *)func)) = (u32)value;
    return(exe_off);
}



static void uberflate_patch_jmp(u8 *exe, int off, int jmp_off) {
    exe[off++] = 0x90;
    exe[off++] = 0xe8;
    *(u32 *)(exe + off) = jmp_off - (off + 4);
}



    /*
    kzip uses FindFirstFile for creating a list of files and then loading them,
    additionally it also converts \ in /
    */
static int uberflate_kzip_exe_patch(int insz) {
    u32     exe_patch_offset;

    exe_patch_offset = uberflate_patch(kzip_exe, 0x4000 /* 0x24d */, uberflate_FindFirstFile, sizeof(uberflate_FindFirstFile) - 1, (void *)insz);
    uberflate_patch_jmp(kzip_exe, 0x3265, exe_patch_offset);
    memset(kzip_exe + 0x340c, 0x90, 1 + 1 + 6 + 2 + 6 + 1 + 6); // FindNextFile/FindClose
    kzip_exe[0x3232] = '\\';
    return(0);
}



    /*
    deflopt is for sure not the best example of good programming and
    fixing/improving it was quite time consuming:
    - instead of making printf(fmt) in the debugging function it does
        vsprintf(buff, fmt); printf(buff);
    - it does even things like:
        sprintf(tmp, "%I64", n);
        printf("blah %s blah", tmp);
    - there are many \ to / conversions
    - it uses the FindFirstFile method like kzip
    - for scanning the files searching PK sign it does the following:
        for(len = 0x4000; len < size; len <<= 1) {
            fseek(fd, 0, SEEK_SET);
            fread(buff, 1, len, fd);    // yeah it rescans the same parts again!
            scan sign in buffer
        }
    - it uses a temporary file for the output and the copies it on the output file
    - more...
    */

static int uberflate_deflopt_exe_patch(u8 *fname, int insz) {
    static const int    rva = 0x00400c00;   // just to make it on the fly (.text)
    u32     exe_patch_offset;

    // no banner
    deflopt_exe[0xB5A8] = 0;

    // vsprintf + printf = no no
    //memset(deflopt_exe + 0x7C13, 0x90, 18);
    //memset(deflopt_exe + 0x7C2e, 0x90, 2);
    deflopt_exe[0x004087E0 - rva] = 0xc3;

    deflopt_exe[0x00402C0E - rva] = 0xeb;

    exe_patch_offset = uberflate_patch(deflopt_exe, 0x00402c10 - rva, uberflate_FindFirstFile, sizeof(uberflate_FindFirstFile) - 1, (void *)insz);
    uberflate_patch_jmp(deflopt_exe, 0x00404572 - rva, exe_patch_offset);
    uberflate_patch_jmp(deflopt_exe, 0x00404672 - rva, exe_patch_offset);

    memset(deflopt_exe + (0x0040481a - rva), 0x90, 1 + 1 + 6 + 2 + 6 + 1 + 6);  // FindNextFile/FindClose

    exe_patch_offset = uberflate_patch(deflopt_exe, 0x00403b59 - rva, uberflate_GetFileSize, sizeof(uberflate_GetFileSize) - 1, (void *)insz);
    uberflate_patch_jmp(deflopt_exe, 0x0040141d - rva, exe_patch_offset);
    uberflate_patch_jmp(deflopt_exe, 0x00401a94 - rva, exe_patch_offset);

    int     t = (0x004087E1 - rva) + (sizeof(uberflate_GetTempFileNameA) - 1);
    strcpy(deflopt_exe + t, fname);
    exe_patch_offset = uberflate_patch(deflopt_exe, 0x004087E1 - rva, uberflate_GetTempFileNameA, sizeof(uberflate_GetTempFileNameA) - 1, (void *)(rva + t));
    uberflate_patch_jmp(deflopt_exe, 0x00403d88 - rva, exe_patch_offset);

    deflopt_exe[0x00404408 - rva] = 0xeb;
    deflopt_exe[0x00404419 - rva] = '\\';
    deflopt_exe[0x0040446a - rva] = '\\';
    deflopt_exe[0xB97e] = '\\';
    deflopt_exe[0xB9D8] = '\\';
    deflopt_exe[0x00404606 - rva] = '\\';
    deflopt_exe[0xB9C5] = '\\';
    return(0);
}



static int uberflate_defluff_exe_patch(int insz) {
    return(0);
}

#endif

