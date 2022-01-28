/*
    Copyright 2009-2021 Luigi Auriemma

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



int get_my_endian(void);
u16 swap16le(u16 n);
u16 swap16be(u16 n);
int myisdigitstr(u8 *str);



#ifndef CP_UTF8
#define CP_UTF8 65001
#endif



// libiconv takes one megabyte when linked statically, so I prefer to use the Win32 API
#ifdef USE_LIBICONV
    #include <iconv.h>
#endif



static const int max_utf8_char_size = 5;    // 4 for utf8 and 5 for a very rare case of utf7, better stay safe and using 5



// NEVER use sizeof(wchar_t) because on Linux it's 32bit instead of 16!
// sizeof(wchar_t) -> sizeof(u16)



// https://msdn.microsoft.com/en-us/library/windows/desktop/dd317756%28v=vs.85%29.aspx
typedef struct {
    int     identifier;
    char    *name;
    char    *information;
} charset_codepage_t;

charset_codepage_t  g_charset_codepage[] = {
    { 037, "IBM037", "IBM EBCDIC US-Canada" },
    { 437, "IBM437", "OEM United States" },
    { 500, "IBM500", "IBM EBCDIC International" },
    { 708, "ASMO-708", "Arabic (ASMO 708)" },
    { 709, "", "Arabic (ASMO-449+, BCON V4)" },
    { 710, "", "Arabic - Transparent Arabic" },
    { 720, "DOS-720", "Arabic (Transparent ASMO); Arabic (DOS)" },
    { 737, "ibm737", "OEM Greek (formerly 437G); Greek (DOS)" },
    { 775, "ibm775", "OEM Baltic; Baltic (DOS)" },
    { 850, "ibm850", "OEM Multilingual Latin 1; Western European (DOS)" },
    { 852, "ibm852", "OEM Latin 2; Central European (DOS)" },
    { 855, "IBM855", "OEM Cyrillic (primarily Russian)" },
    { 857, "ibm857", "OEM Turkish; Turkish (DOS)" },
    { 858, "IBM00858", "OEM Multilingual Latin 1 + Euro symbol" },
    { 860, "IBM860", "OEM Portuguese; Portuguese (DOS)" },
    { 861, "ibm861", "OEM Icelandic; Icelandic (DOS)" },
    { 862, "DOS-862", "OEM Hebrew; Hebrew (DOS)" },
    { 863, "IBM863", "OEM French Canadian; French Canadian (DOS)" },
    { 864, "IBM864", "OEM Arabic; Arabic (864)" },
    { 865, "IBM865", "OEM Nordic; Nordic (DOS)" },
    { 866, "cp866", "OEM Russian; Cyrillic (DOS)" },
    { 869, "ibm869", "OEM Modern Greek; Greek, Modern (DOS)" },
    { 870, "IBM870", "IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2" },
    { 874, "windows-874", "ANSI/OEM Thai (ISO 8859-11); Thai (Windows)" },
    { 875, "cp875", "IBM EBCDIC Greek Modern" },
    { 932, "shift_jis", "ANSI/OEM Japanese; Japanese (Shift-JIS)" },
    { 936, "gb2312", "ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)" },
    { 949, "ks_c_5601-1987", "ANSI/OEM Korean (Unified Hangul Code)" },
    { 950, "big5", "ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)" },
    { 1026, "IBM1026", "IBM EBCDIC Turkish (Latin 5)" },
    { 1047, "IBM01047", "IBM EBCDIC Latin 1/Open System" },
    { 1140, "IBM01140", "IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)" },
    { 1141, "IBM01141", "IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)" },
    { 1142, "IBM01142", "IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)" },
    { 1143, "IBM01143", "IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)" },
    { 1144, "IBM01144", "IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)" },
    { 1145, "IBM01145", "IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)" },
    { 1146, "IBM01146", "IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)" },
    { 1147, "IBM01147", "IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)" },
    { 1148, "IBM01148", "IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)" },
    { 1149, "IBM01149", "IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)" },
    { 1200, "utf-16", "Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications" },
    { 1201, "unicodeFFFE", "Unicode UTF-16, big endian byte order; available only to managed applications" },
    { 1250, "windows-1250", "ANSI Central European; Central European (Windows)" },
    { 1251, "windows-1251", "ANSI Cyrillic; Cyrillic (Windows)" },
    { 1252, "windows-1252", "ANSI Latin 1; Western European (Windows)" },
    { 1253, "windows-1253", "ANSI Greek; Greek (Windows)" },
    { 1254, "windows-1254", "ANSI Turkish; Turkish (Windows)" },
    { 1255, "windows-1255", "ANSI Hebrew; Hebrew (Windows)" },
    { 1256, "windows-1256", "ANSI Arabic; Arabic (Windows)" },
    { 1257, "windows-1257", "ANSI Baltic; Baltic (Windows)" },
    { 1258, "windows-1258", "ANSI/OEM Vietnamese; Vietnamese (Windows)" },
    { 1361, "Johab", "Korean (Johab)" },
    { 10000, "macintosh", "MAC Roman; Western European (Mac)" },
    { 10001, "x-mac-japanese", "Japanese (Mac)" },
    { 10002, "x-mac-chinesetrad", "MAC Traditional Chinese (Big5); Chinese Traditional (Mac)" },
    { 10003, "x-mac-korean", "Korean (Mac)" },
    { 10004, "x-mac-arabic", "Arabic (Mac)" },
    { 10005, "x-mac-hebrew", "Hebrew (Mac)" },
    { 10006, "x-mac-greek", "Greek (Mac)" },
    { 10007, "x-mac-cyrillic", "Cyrillic (Mac)" },
    { 10008, "x-mac-chinesesimp", "MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)" },
    { 10010, "x-mac-romanian", "Romanian (Mac)" },
    { 10017, "x-mac-ukrainian", "Ukrainian (Mac)" },
    { 10021, "x-mac-thai", "Thai (Mac)" },
    { 10029, "x-mac-ce", "MAC Latin 2; Central European (Mac)" },
    { 10079, "x-mac-icelandic", "Icelandic (Mac)" },
    { 10081, "x-mac-turkish", "Turkish (Mac)" },
    { 10082, "x-mac-croatian", "Croatian (Mac)" },
    { 12000, "utf-32", "Unicode UTF-32, little endian byte order; available only to managed applications" },
    { 12001, "utf-32BE", "Unicode UTF-32, big endian byte order; available only to managed applications" },
    { 20000, "x-Chinese_CNS", "CNS Taiwan; Chinese Traditional (CNS)" },
    { 20001, "x-cp20001", "TCA Taiwan" },
    { 20002, "x_Chinese-Eten", "Eten Taiwan; Chinese Traditional (Eten)" },
    { 20003, "x-cp20003", "IBM5550 Taiwan" },
    { 20004, "x-cp20004", "TeleText Taiwan" },
    { 20005, "x-cp20005", "Wang Taiwan" },
    { 20105, "x-IA5", "IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)" },
    { 20106, "x-IA5-German", "IA5 German (7-bit)" },
    { 20107, "x-IA5-Swedish", "IA5 Swedish (7-bit)" },
    { 20108, "x-IA5-Norwegian", "IA5 Norwegian (7-bit)" },
    { 20127, "us-ascii", "US-ASCII (7-bit)" },
    { 20261, "x-cp20261", "T.61" },
    { 20269, "x-cp20269", "ISO 6937 Non-Spacing Accent" },
    { 20273, "IBM273", "IBM EBCDIC Germany" },
    { 20277, "IBM277", "IBM EBCDIC Denmark-Norway" },
    { 20278, "IBM278", "IBM EBCDIC Finland-Sweden" },
    { 20280, "IBM280", "IBM EBCDIC Italy" },
    { 20284, "IBM284", "IBM EBCDIC Latin America-Spain" },
    { 20285, "IBM285", "IBM EBCDIC United Kingdom" },
    { 20290, "IBM290", "IBM EBCDIC Japanese Katakana Extended" },
    { 20297, "IBM297", "IBM EBCDIC France" },
    { 20420, "IBM420", "IBM EBCDIC Arabic" },
    { 20423, "IBM423", "IBM EBCDIC Greek" },
    { 20424, "IBM424", "IBM EBCDIC Hebrew" },
    { 20833, "x-EBCDIC-KoreanExtended", "IBM EBCDIC Korean Extended" },
    { 20838, "IBM-Thai", "IBM EBCDIC Thai" },
    { 20866, "koi8-r", "Russian (KOI8-R); Cyrillic (KOI8-R)" },
    { 20871, "IBM871", "IBM EBCDIC Icelandic" },
    { 20880, "IBM880", "IBM EBCDIC Cyrillic Russian" },
    { 20905, "IBM905", "IBM EBCDIC Turkish" },
    { 20924, "IBM00924", "IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)" },
    { 20932, "EUC-JP", "Japanese (JIS 0208-1990 and 0212-1990)" },
    { 20936, "x-cp20936", "Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)" },
    { 20949, "x-cp20949", "Korean Wansung" },
    { 21025, "cp1025", "IBM EBCDIC Cyrillic Serbian-Bulgarian" },
    { 21027, "", "(deprecated)" },
    { 21866, "koi8-u", "Ukrainian (KOI8-U); Cyrillic (KOI8-U)" },
    { 28591, "iso-8859-1", "ISO 8859-1 Latin 1; Western European (ISO)" },
    { 28592, "iso-8859-2", "ISO 8859-2 Central European; Central European (ISO)" },
    { 28593, "iso-8859-3", "ISO 8859-3 Latin 3" },
    { 28594, "iso-8859-4", "ISO 8859-4 Baltic" },
    { 28595, "iso-8859-5", "ISO 8859-5 Cyrillic" },
    { 28596, "iso-8859-6", "ISO 8859-6 Arabic" },
    { 28597, "iso-8859-7", "ISO 8859-7 Greek" },
    { 28598, "iso-8859-8", "ISO 8859-8 Hebrew; Hebrew (ISO-Visual)" },
    { 28599, "iso-8859-9", "ISO 8859-9 Turkish" },
    { 28603, "iso-8859-13", "ISO 8859-13 Estonian" },
    { 28605, "iso-8859-15", "ISO 8859-15 Latin 9" },
    { 29001, "x-Europa", "Europa 3" },
    { 38598, "iso-8859-8-i", "ISO 8859-8 Hebrew; Hebrew (ISO-Logical)" },
    { 50220, "iso-2022-jp", "ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)" },
    { 50221, "csISO2022JP", "ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)" },
    { 50222, "iso-2022-jp", "ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)" },
    { 50225, "iso-2022-kr", "ISO 2022 Korean" },
    { 50227, "x-cp50227", "ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)" },
    { 50229, "", "ISO 2022 Traditional Chinese" },
    { 50930, "", "EBCDIC Japanese (Katakana) Extended" },
    { 50931, "", "EBCDIC US-Canada and Japanese" },
    { 50933, "", "EBCDIC Korean Extended and Korean" },
    { 50935, "", "EBCDIC Simplified Chinese Extended and Simplified Chinese" },
    { 50936, "", "EBCDIC Simplified Chinese" },
    { 50937, "", "EBCDIC US-Canada and Traditional Chinese" },
    { 50939, "", "EBCDIC Japanese (Latin) Extended and Japanese" },
    { 51932, "euc-jp", "EUC Japanese" },
    { 51936, "EUC-CN", "EUC Simplified Chinese; Chinese Simplified (EUC)" },
    { 51949, "euc-kr", "EUC Korean" },
    { 51950, "", "EUC Traditional Chinese" },
    { 52936, "hz-gb-2312", "HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)" },
    { 54936, "GB18030", "Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)" },
    { 57002, "x-iscii-de", "ISCII Devanagari" },
    { 57003, "x-iscii-be", "ISCII Bengali" },
    { 57004, "x-iscii-ta", "ISCII Tamil" },
    { 57005, "x-iscii-te", "ISCII Telugu" },
    { 57006, "x-iscii-as", "ISCII Assamese" },
    { 57007, "x-iscii-or", "ISCII Oriya" },
    { 57008, "x-iscii-ka", "ISCII Kannada" },
    { 57009, "x-iscii-ma", "ISCII Malayalam" },
    { 57010, "x-iscii-gu", "ISCII Gujarati" },
    { 57011, "x-iscii-pa", "ISCII Punjabi" },
    { 65000, "utf-7", "Unicode (UTF-7)" },
    { 65001, "utf-8", "Unicode (UTF-8)" },
    //
    { 0, "CP_ACP", "" },
    { 1, "CP_OEM", "" },
    { 1, "CP_OEMCP", "" },
    { 2, "CP_MACCP", "" },
    { 3, "CP_THREAD_ACP", "" },
    { 42, "CP_SYMBOL", "" },
    { 65000, "CP_UTF7", "" },
    { 65001, "CP_UTF8", "" },
    { 65000, "UTF7", "" },
    { 65001, "UTF8", "" },
    { -1, NULL, NULL }
};




u8 *get_codepage_from_id(int id) {
    int     i;
    for(i = 0; g_charset_codepage[i].identifier >= 0; i++) {
        if(g_charset_codepage[i].identifier == id) {
            return g_charset_codepage[i].name;
        }
    }
    return NULL;
}



int get_codepage_from_string(u8 *name) {
    int     i;
    while(*name && ((*name <= ' ') || (*name == '_') || (*name == '-'))) name++;
    for(i = 0; g_charset_codepage[i].identifier >= 0; i++) {
        if(!stricmp(name, g_charset_codepage[i].name)) {
            return g_charset_codepage[i].identifier;
        }
    }
    if(!strnicmp(name, "cp", 2)) {
        return get_codepage_from_string(name + 2);
    }
    if(myisdigitstr(name)) {
        return myatoi(name);
    }
    return -1;
}



// not compatible with big endian utf32
int _utf8_to_utf16_chr(u8 *in, int insz, wchar_t *wc, int handle_endianess, int mycodepage, int utf32) {
    int     len = -1;
    wchar_t t16;

    if(!wc) wc = &t16;
    *wc = 0;

    if(insz < 0) insz = strlen(in);
    if(!insz) return 0;
    if(insz > max_utf8_char_size) insz = max_utf8_char_size;    //otherwise it goes in a sort of endless loop, a char is max 4 bytes

#ifdef WIN32
    static const int    codepage[] = {
                CP_UTF8,
                //CP_OEMCP,
                CP_ACP,
                -1
            };
    int     cp;
    if(mycodepage > 0) {
        cp = -1;    // automatic scan if it fails (cp++ is 0)
        goto doit;
    }
    for(cp = 0; codepage[cp] >= 0; cp++) {
        mycodepage = codepage[cp];
doit:
        *wc = 0;
        int     i;
        for(i = 1; i <= insz; i++) {
            if(MultiByteToWideChar(mycodepage, MB_ERR_INVALID_CHARS /*necessary or this solution will not work*/, in, i, (void *)wc, 1 /*chars not bytes!*/)) {
                len = i;
                break;
            }
        }
        if((len > 0) && (*wc != 0xfffd)) break;
    }
    if(len > insz) len = -1;
#endif
#ifdef USE_LIBICONV
    if(len <= 0) {
        static  iconv_t ic      = NULL;
        static  int old_endian  = -1;
        if(g_endian != old_endian) {
            old_endian = g_endian;
            if(ic) iconv_close(ic);
            //ic = iconv_open((g_endian == MYLITTLE_ENDIAN) ? "UTF-16LE" : "UTF-16BE", "UTF-8");   // utf8 to utf16
            ic = iconv_open(utf32 ? "UTF-32LE" : "UTF-16LE", "UTF-8");   // correct, not tested on big endian CPU
        }
        if(ic != (iconv_t)(-1)) {
            u16     wc16    = 0;
            u32     wc32    = 0;
            size_t  il  = insz,
                    ol  = utf32 ? sizeof(wc32) : sizeof(wc16) /*sizeof(wchar_t)*/;
            char    *ip = in,
                    *op = utf32 ? (void *)&wc32 : (void *)&wc16;
            if(iconv(ic, &ip, &il, &op, &ol) >= 0) {
                len = ip - (char *)in;
            }
            *wc = utf32 ? wc32 : wc16;
        }
    }
#endif
    if(len <= 0) {
        len = mbtowc(wc, in, insz);
    }
    if(len <= 0) {
        len = 1;
        *wc = in[0];
    }
    if(handle_endianess) {
        if(utf32) {
            // endianess does NOT work with utf32 because wc is wchar_t (16 bit on Windows)
            if(g_endian == MYLITTLE_ENDIAN) *wc = swap32le(*wc);
            else                            *wc = swap32be(*wc);
        } else {
            if(g_endian == MYLITTLE_ENDIAN) *wc = swap16le(*wc);
            else                            *wc = swap16be(*wc);
        }
    }
    return len; // input size
}

int utf8_to_utf16_chr(u8 *in, int insz, u16 *wc, int handle_endianess, int mycodepage) {
    wchar_t wc_ret;
    int ret = _utf8_to_utf16_chr(in, insz, &wc_ret, handle_endianess, mycodepage, 0);
    if(wc) *wc = wc_ret;
    return ret;
}

int utf8_to_utf32_chr(u8 *in, int insz, u32 *wc, int handle_endianess, int mycodepage) {
    wchar_t wc_ret;
    int ret = _utf8_to_utf16_chr(in, insz, &wc_ret, handle_endianess, mycodepage, 1);
    if(wc) *wc = wc_ret;
    return ret;
}



int _utf16_to_utf8_chr(u32 wc, u8 *out, int outsz, int handle_endianess, int mycodepage, int utf32) {
    int     len = -1;
    wchar_t wct;

    if(handle_endianess) {
        if(utf32) {
            if(g_endian == MYLITTLE_ENDIAN) wc = swap32le(wc);
            else                            wc = swap32be(wc);
        } else {
            if(g_endian == MYLITTLE_ENDIAN) wc = swap16le(wc);
            else                            wc = swap16be(wc);
        }
    }
    wct = wc;
#ifdef WIN32
    if(mycodepage < 0) mycodepage = CP_UTF8;
    len = WideCharToMultiByte(mycodepage, 0, (void *)&wct, 1 /*chars not bytes!*/, out, outsz, NULL, NULL);
    if(len > outsz) len = -1;
#endif
#ifdef USE_LIBICONV
    if(len <= 0) {
        static  iconv_t ic      = NULL;
        static  int old_endian  = -1;
        if(g_endian != old_endian) {
            old_endian = g_endian;
            if(ic) iconv_close(ic);
            //ic = iconv_open("UTF-8", (g_endian == MYLITTLE_ENDIAN) ? "UTF-16LE" : "UTF-16BE");   // utf16 to utf8
            ic = iconv_open("UTF-8", utf32 ? "UTF-32LE" : "UTF-16LE"); // wct is already endian-converted, so no need of LE/BE, not tested on big endian CPU
        }
        if(ic != (iconv_t)(-1)) {
            u16     wc16    = 0;
            u32     wc32    = 0;
            size_t  il  = utf32 ? sizeof(wc32) : sizeof(wc16) /*sizeof(wchar_t)*/,
                    ol  = outsz;
            char    *ip = utf32 ? (void *)&wc32 : (void *)&wc16,
                    *op = out;
            if(iconv(ic, &ip, &il, &op, &ol) >= 0) {
                len = op - (char *)out;
            }
            wct = utf32 ? wc32 : wc16;
        }
    }
#endif
    if(len <= 0) {
        len = wctomb(out, wct);
    }
    if(len <= 0) {
        len = 1;
        out[0] = wct;
    }
    return len; // output size
}

int utf16_to_utf8_chr(u16 wc, u8 *out, int outsz, int handle_endianess, int mycodepage) {
    return _utf16_to_utf8_chr(wc, out, outsz, handle_endianess, mycodepage, 0);
}

int utf32_to_utf8_chr(u32 wc, u8 *out, int outsz, int handle_endianess, int mycodepage) {
    return _utf16_to_utf8_chr(wc, out, outsz, handle_endianess, mycodepage, 1);
}



// yes, this is like a sort of duplicate of utf8_to_utf16_chr... it should be rewritten probably
int utf8_to_utf16(u8 *in, int insz, u8 *out, int outsz, int rev) {
    int     i   = 0,
            o   = 0,
            t   = 0,
            len = 0,
            mycodepage = g_codepage;
    wchar_t wc  = 0;

    if(!out) return -1;
    if(outsz < 0) return -1;
    if(insz < 0) insz = strlen(in);

#ifdef WIN32
    if(!rev) {
        int     codepage;
        for(codepage = 0;; codepage++) {
            if(codepage == 0) mycodepage = g_codepage;
            if(mycodepage < 0) codepage++;
                 if(codepage == 1) mycodepage = CP_UTF8;
            else if(codepage == 2) mycodepage = CP_ACP;
            else break;
            t = outsz / sizeof(u16) /*sizeof(wchar_t)*/;
            o = MultiByteToWideChar(mycodepage, 0, in, insz, (void *)out, t);
            if(o > t) o = -1;
            o *= sizeof(u16) /*sizeof(wchar_t)*/;
            for(i = 0; i < o; i += sizeof(u16) /*sizeof(wchar_t)*/) {
                wc = ((wchar_t *)(out + i))[0];
                if(wc == 0xfffd) break;
                if(g_endian == MYLITTLE_ENDIAN) ((wchar_t *)(out + i))[0] = swap16le(wc);
                else                            ((wchar_t *)(out + i))[0] = swap16be(wc);
            }
            if(i < o) continue;
            break;
        }
    } else {
        t = insz / sizeof(u16) /*sizeof(wchar_t)*/;
        o = WideCharToMultiByte((g_codepage < 0) ? CP_UTF8 : g_codepage, 0, (void *)in, t, out, outsz, NULL, NULL);
        if(o > t) o = -1;
    }
    if(o <= 0)
#endif
    {
#ifdef USE_LIBICONV
    iconv_t ic;
    if(!rev) ic = iconv_open((g_endian == MYLITTLE_ENDIAN) ? "UTF-16LE" : "UTF-16BE", "UTF-8");   // utf8 to utf16
    else     ic = iconv_open("UTF-8", (g_endian == MYLITTLE_ENDIAN) ? "UTF-16LE" : "UTF-16BE");   // utf16 to utf8
    if(ic != (iconv_t)(-1)) {

        size_t  il  = insz,
                ol  = outsz;
        char    *ip = in,
                *op = out;
        do {
            len = iconv(ic, &ip, &il, &op, &ol);
        } while(len > 0);
        iconv_close(ic);
        o = op - (char *)out;

    } else
#endif
    {

        o = 0;
        if(!rev) {  // 8 to 16
            for(i = 0; i < insz; i += len) {
                len = mbtowc(&wc, in + i, insz - i);
                if(len <= 0) {
                    wc = in[i];
                    len = 1;
                }
                if(g_endian == MYLITTLE_ENDIAN) {
                    out[o++] = wc & 0xff;
                    out[o++] = (wc >> 8) & 0xff;
                } else {
                    out[o++] = (wc >> 8) & 0xff;
                    out[o++] = wc & 0xff;
                }
            }
        } else {    // 16 to 8
            len = sizeof(u16) /*sizeof(wchar_t)*/;
            for(i = 0; (i + len) <= insz; i += len) {
                if(g_endian == MYLITTLE_ENDIAN) {
                    wc = in[i] | (in[i + 1] << 8);
                } else {
                    wc = (in[i] << 8) | in[i + 1];
                }
                t = wctomb(out + o, wc);
                if(t <= 0) {
                    out[o] = wc;
                    t = 1;
                }
                o += t;
            }
        }
    }
    }

    int ret = o;    // without the last NULL

    out[o++] = 0;
    if(!rev) out[o++] = 0;

    return ret;
}



// tested with little and big endian in reimport mode (SLog)
u8 *_set_utf8_to_unicode(u8 *input, int input_size, int *ret_size, int utf32) {
    static int  buffsz  = 0;
    static u8   *buff   = NULL;
    u32         wc32;
    u16         wc16;
    int         i,
                t,
                wcsz    = utf32 ? sizeof(wc32) : sizeof(wc16);
    u8          *p,
                *l;

    if(!input) input = "";
    if(input_size < 0) input_size = strlen(input);

    p = input;
    l = input + input_size;
    i = 0;
    while(p < l) {
        if(utf32)   t = utf8_to_utf32_chr(p, l - p, &wc32, 1, g_codepage);
        else        t = utf8_to_utf16_chr(p, l - p, &wc16, 1, g_codepage);
        if(t <= 0) break;
        p += t;
        //if(!wc) break;    // give freedom to have 0x00 in SLog unicode
        if((i + wcsz) >= buffsz) {
            buffsz += (STRINGSZ * wcsz);
            buff = realloc(buff, buffsz + wcsz);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        if(utf32)   *(u32 *)(buff + i) = wc32;
        else        *(u16 *)(buff + i) = wc16;
        i += wcsz;
    }
    if(ret_size) *ret_size = i;
    //buff = realloc(buff, i + wcsz);    // do NOT activate or will crash
    if(utf32)   *(u32 *)(buff + i) = 0;
    else        *(u16 *)(buff + i) = 0;

    //static const int    set_utf8_to_unicode_zeroes = 4; // ???
    //buff = realloc(buff, i + set_utf8_to_unicode_zeroes);
    //if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
    //memset(buff + i, 0, set_utf8_to_unicode_zeroes);
    return buff;
}

u8 *set_utf8_to_unicode(u8 *input, int input_size, int *ret_size) {
    return _set_utf8_to_unicode(input, input_size, ret_size, 0);
}

u8 *set_utf8_to_unicode32(u8 *input, int input_size, int *ret_size) {
    return _set_utf8_to_unicode(input, input_size, ret_size, 1);
}



u8 *_set_unicode_to_utf8(u8 *input, int input_size, int *ret_size, int utf32) {
    static int  buffsz  = 0;
    static u8   *buff   = NULL;
    int     i,
            x,
            c,
            len,
            unicnt  = 0,
            wcsz    = utf32 ? sizeof(u32) : sizeof(u16),
            unicode = utf32 ? -1 : 0;   // compatible with fgetss
    u32     wc      = 0;
    u8      *p,
            *l;
    u8      tmp[32];

    if(input_size < 0) {
        if(utf32) {
            for(input_size = 0; *(u32 *)(input + input_size); input_size += sizeof(u32));
        } else {
            for(input_size = 0; *(u16 *)(input + input_size); input_size += sizeof(u16));
        }
    }

    p = input;
    l = input + input_size;

    i = 0;
    for(;;) {
        if(p >= l) c = 0;
        else       c = *p++;
        // no c check here

        // shared with fgetss
            if(!unicnt) wc = 0;
            if(g_endian == MYLITTLE_ENDIAN) {
                wc |= (c << (8 * unicnt));
            } else {
                wc |= (c << (8 * (wcsz - (unicnt + 1))));
            }
            unicnt++;
            if(unicnt < wcsz) continue;
            unicnt = 0;
            if(unicode > 0) len = utf16_to_utf8_chr(wc, tmp, sizeof(tmp), 0, g_codepage);
            else            len = utf32_to_utf8_chr(wc, tmp, sizeof(tmp), 0, g_codepage);

        //if((len == 1) && (tmp[0] == 0x00)) break;
        for(x = 0; x < len; x++) if(tmp[x]) break;
        if(x >= len) break;

        if((i + len) >= buffsz) {
            buffsz += len + STRINGSZ;
            buff = realloc(buff, buffsz + max_utf8_char_size);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        memcpy(buff + i, tmp, len);
        i += len;
    }
    if(ret_size) *ret_size = i;
    if(!buff) buff = malloc(1);   // useful
    buff[i] = 0;

    // do not enable or crash... ???
    //static const int set_unicode_to_utf8_zeroes = 4; // ???
    //buff = realloc(buff, i + set_unicode_to_utf8_zeroes);
    //memset(buff + i, 0, set_unicode_to_utf8_zeroes);
    return buff;
}

u8 *set_unicode_to_utf8(u8 *input, int input_size, int *ret_size) {
    return _set_unicode_to_utf8(input, input_size, ret_size, 0);
}

u8 *set_unicode32_to_utf8(u8 *input, int input_size, int *ret_size) {
    return _set_unicode_to_utf8(input, input_size, ret_size, 1);
}



// if backup_g_codepage is active then every conversion will be wrong
wchar_t *native_utf8_to_unicode(u8 *input) {
    //int     backup_g_codepage   = g_codepage;
    int     backup_g_endian     = g_endian;
    //g_codepage  = CP_UTF8;
    g_endian    = get_my_endian();
    wchar_t *retw = (wchar_t *)set_utf8_to_unicode(input, -1, NULL);
    //g_codepage  = backup_g_codepage;
    g_endian    = backup_g_endian;
    return retw;
}
u8 *native_unicode_to_utf8(wchar_t *input) {
    //int     backup_g_codepage   = g_codepage;
    int     backup_g_endian     = g_endian;
    //g_codepage  = CP_UTF8;
    g_endian    = get_my_endian();
    u8      *ret = set_unicode_to_utf8((u8 *)input, -1, NULL);
    //g_codepage  = backup_g_codepage;
    g_endian    = backup_g_endian;
    return ret;
}



void set_codepage(void) {
    if(g_codepage < 0) {
        setlocale(LC_CTYPE, "C.utf8");
#ifdef WIN32
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
#endif
    } else {
        u8  tmp[20];
        sprintf(tmp, ".%d", (i32)g_codepage);
        setlocale(LC_CTYPE, tmp);
#ifdef WIN32
        SetConsoleCP(g_codepage);
        SetConsoleOutputCP(g_codepage);
#endif
    }
}



void set_g_codepage(u8 *str, int strn) {
    int     t;
    if(!str) str = "UTF8";
    if(!str[0] || myisdigitstr(str)) {
        if(strn >= 0) g_codepage = strn;
        else          g_codepage = myatoi(str);
    } else {
        t = get_codepage_from_string(str);
        if(t >= 0) g_codepage = t;
    }
    set_codepage();
}



wchar_t *mywstrcpy(wchar_t *dst, wchar_t *src) {
    int     i;
    if(!dst) return NULL;
    if(!src) src = L"";
    for(i = 0; (dst[i] = src[i]); i++);
    return dst;
}



wchar_t *mywstrncpy(wchar_t *dst, wchar_t *src, int num) {
    int     i;
    if(num < 0) return mywstrcpy(dst, src);
    if(!num) return NULL;
    if(!dst) return NULL;
    if(!src) src = L"";
    num--;  // reserve space for final NUL (not done by original implementation)
    for(i = 0; (i < num) && (dst[i] = src[i]); i++);
    dst[i] = 0; // overwrites the NUL or add a new one
    return dst;
}



int mywstrcmp(wchar_t *a, wchar_t *b) {
    int     i;
    if(!a && !b) return 0;
    if(!a) return 1;
    if(!b) return -1;
    for(i = 0; a[i] && b[i] && (a[i] == b[i]); i++);
    if(!a[i] && !b[i]) return 0;
    if(b[i]) return 1;
    if(a[i]) return -1;
    return 0;
}



int mywstricmp(wchar_t *a, wchar_t *b) {
    int     i;
    if(!a && !b) return 0;
    if(!a) return 1;
    if(!b) return -1;
    for(i = 0; a[i] && b[i] && (tolower(a[i]) == tolower(b[i])); i++);
    if(!a[i] && !b[i]) return 0;
    if(b[i]) return 1;
    if(a[i]) return -1;
    return 0;
}

