/*
    Copyright 2009-2018 Luigi Auriemma

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

// QuickBMS types conversions



static const char   *g_months[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec", NULL };



int put_type_variable(int fdnum, u_int num) {
    int     i   = 0;
    u8      tmp[32];

    do {
        tmp[i++] = num & 0x7f;
        num >>= 7;
    } while(num);

    for(--i; i >= 0; i--) {
        if(i) tmp[i] |= 0x80;
        if(myfw(fdnum, &tmp[i], 1) < 0) return -1;
    }
    return 0;
}



int put_type_variable3(int fdnum, u_int num) {
    int     i   = 0,
            j;
    u8      tmp[32];

    do {
        tmp[i++] = num & 0x7f;
        num >>= 7;
    } while(num);

    for(j = 0; j < i; j++) {
        if(j == (i - 1)) tmp[j] |= 0x80;
        if(myfw(fdnum, &tmp[j], 1) < 0) return -1;
    }
    return 0;
}



int put_type_variable4(int fdnum, u_int num) {
    u8      c;

    do {
        c = num & 0x7f;
        num >>= 7;
        if(num) c |= 0x80;
        if(myfw(fdnum, &c, 1) < 0) return -1;
    } while(num);
    return 0;
}



int put_type_variable5(int fdnum, u_int num) {
  u8 firstByte = 0;
  u8 mask = 0x80;
  u8    tmp[1];
  int i;
  for (i = 0; i < 8; i++)
  {
    if (num < (((u64)(1) << (u64)( 7  * (u64)(i + 1)))))
    {
      firstByte |= (u8)(num >> (u64)(8 * i));
      break;
    }
    firstByte |= mask;
    mask >>= 1;
  }
  tmp[0] = firstByte;
  if(myfw(fdnum, tmp, 1) < 0) return -1;
  for (;i > 0; i--)
  {
    tmp[0] = num;
    if(myfw(fdnum, tmp, 1) < 0) return -1;
    num >>= 8;
  }
    return 0;
}



int put_type_variable6(int fdnum, u_int num) {
    FDBITS

    int     t;
    t = get_var_from_name("ValueMax", -1);
    t = (t < 0) ? (((u_int)(-1)) >> 1) : get_var32(t);
    u_int   n = 0;
    u_int   mask;
    if(t >= 2) {
        for(mask = 1; ((n + mask) < t) && mask; mask *= 2) {
            if(num & mask) {
                if(fd_write_bits(1, 1, &bitchr, &bitpos, fdnum, NULL) < 0) return -1;
                n += mask;
            } else {
                if(fd_write_bits(0, 1, &bitchr, &bitpos, fdnum, NULL) < 0) return -1;
            }
        }
    }
    return 0;
}



int put_type_variable7(int fdnum, u_int num) {
    u_int   rem = num;
    do {
        u8  n = rem & 0x7f;
        rem >>= 7;
        n <<= 1;
        if(rem) n |= 1;
        if(myfw(fdnum, &n, 1) < 0) return -1;
    } while(rem);
    return 0;
}



int unreal_index(int fdnum) {
    int     result = 0;
    u8      b0,
            b1,
            b2,
            b3,
            b4;

    b0 = fgetxx(fdnum, 1, NULL);
    if(b0 & 0x40) {
        b1 = fgetxx(fdnum, 1, NULL);
        if(b1 & 0x80) {
            b2 = fgetxx(fdnum, 1, NULL);
            if(b2 & 0x80) {
                b3 = fgetxx(fdnum, 1, NULL);
                if(b3 & 0x80) {
                    b4 = fgetxx(fdnum, 1, NULL);
                    result = b4;
                }
                result = (result << 7) | (b3 & 0x7f);
            }
            result = (result << 7) | (b2 & 0x7f);
        }
        result = (result << 7) | (b1 & 0x7f);
    }
    result = (result << 6) | (b0 & 0x3f);
    if(b0 & 0x80) result = -result;
    return(result);
}



int make_unreal_index(int number, u8 *index_num) {
    int     len  = 0,
            sign = 0;

    if(number < 0) {
        number = -number;
        sign = -1;
    }

    len++;
    index_num[0] = (number & 0x3f);
    number >>= 6;
    if(number) {
        len++;
        index_num[0] += 0x40;
        index_num[1] = (number & 0x7f);
        number >>= 7;
        if(number) {
            len++;
            index_num[1] += 0x80;
            index_num[2] = (number & 0x7f);
            number >>= 7;
            if(number) {
                len++;
                index_num[2] += 0x80;
                index_num[3] = (number & 0x7f);
                number >>= 7;
                if(number) {
                    len++;
                    index_num[3] += 0x80;
                    index_num[4] = number;
                }
            }
        }
    }
    if(sign) index_num[0] += 0x80;
    return(len);
}



typedef struct {
    u16     Year;
    u16     Month;
    u16     Day;
    u16     Hour;
    u16     Minute;
    u16     Second;
    u16     Milliseconds;
    u16     Weekday;
} MY_PTIME_FIELDS;



// from Wine source code
// http://source.winehq.org/source/dlls/ntdll/time.c#L115
void myRtlTimeToTimeFields(u64 *liTime, MY_PTIME_FIELDS *TimeFields) {
    #define TICKSPERSEC        10000000
    #define TICKSPERMSEC       10000
    #define SECSPERDAY         86400
    #define SECSPERHOUR        3600
    #define SECSPERMIN         60
    //#define MINSPERHOUR        60
    //#define HOURSPERDAY        24
    #define EPOCHWEEKDAY       1  /* Jan 1, 1601 was Monday */
    #define DAYSPERWEEK        7
    //#define EPOCHYEAR          1601
    //#define DAYSPERNORMALYEAR  365
    //#define DAYSPERLEAPYEAR    366
    //#define MONSPERYEAR        12
    #define DAYSPERQUADRICENTENNIUM (365 * 400 + 97)
    //#define DAYSPERNORMALCENTURY (365 * 100 + 24)
    #define DAYSPERNORMALQUADRENNIUM (365 * 4 + 1)

    i32 SecondsInDay;
    u32 cleaps, years, yearday, months;
    u32 Days;
    u64 Time;

    /* Extract millisecond from time and convert time into seconds */
    TimeFields->Milliseconds =
        (u16) (( *liTime % TICKSPERSEC) / TICKSPERMSEC);
    Time = *liTime / TICKSPERSEC;

    /* The native version of RtlTimeToTimeFields does not take leap seconds
     * into account */

    /* Split the time into days and seconds within the day */
    Days = Time / SECSPERDAY;
    SecondsInDay = Time % SECSPERDAY;

    /* compute time of day */
    TimeFields->Hour = (u16) (SecondsInDay / SECSPERHOUR);
    SecondsInDay = SecondsInDay % SECSPERHOUR;
    TimeFields->Minute = (u16) (SecondsInDay / SECSPERMIN);
    TimeFields->Second = (u16) (SecondsInDay % SECSPERMIN);

    /* compute day of week */
    TimeFields->Weekday = (u16) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

    /* compute year, month and day of month. */
    cleaps=( 3 * ((4 * Days + 1227) / DAYSPERQUADRICENTENNIUM) + 3 ) / 4;
    Days += 28188 + cleaps;
    years = (20 * Days - 2442) / (5 * DAYSPERNORMALQUADRENNIUM);
    yearday = Days - (years * DAYSPERNORMALQUADRENNIUM)/4;
    months = (64 * yearday) / 1959;
    /* the result is based on a year starting on March.
     * To convert take 12 from Januari and Februari and
     * increase the year by one. */
    if( months < 14 ) {
        TimeFields->Month = months - 1;
        TimeFields->Year = years + 1524;
    } else {
        TimeFields->Month = months - 13;
        TimeFields->Year = years + 1525;
    }
    /* calculation of day of month is based on the wonderful
     * sequence of INT( n * 30.6): it reproduces the 
     * 31-30-31-30-31-31 month lengths exactly for small n's */
    TimeFields->Day = yearday - (1959 * months) / 64 ;
}



u8 *time64_to_strtime(int fdnum) {
    static u8   buff[32];
    MY_PTIME_FIELDS    tm;
    u64     tmp64;
    u32     tmp1,
            tmp2;

    // tmp64 in reality are 2 32bit as used on Windows
    if(g_endian == MYLITTLE_ENDIAN) {
        tmp2 = fgetxx(fdnum, 4, NULL);
        tmp1 = fgetxx(fdnum, 4, NULL);
    } else {
        tmp1 = fgetxx(fdnum, 4, NULL);
        tmp2 = fgetxx(fdnum, 4, NULL);
    }
    tmp64 = (((u64)tmp1) << 32) | ((u64)tmp2);
    myRtlTimeToTimeFields(&tmp64, &tm);

    sprintf(
        buff,
        "%02d %s %d %02d:%02d:%02d",
        tm.Day,
        g_months[(tm.Month - 1) % 12],
        tm.Year,
        tm.Hour, tm.Minute, tm.Second);
    return buff;
}



u8 *time_to_strtime(int fdnum) {
    struct  tm  *tmx;
    time_t  datex;
    static  char    buff[32];
    u32     num;
    num = fgetxx(fdnum, 4, NULL);

    datex = num;
    tmx = gmtime(&datex);
    if(!tmx) return("none");
    sprintf(
        buff,
        "%02d %s %d %02d:%02d:%02d",
        tmx->tm_mday,
        g_months[tmx->tm_mon % 12],
        1900 + tmx->tm_year,
        tmx->tm_hour, tmx->tm_min, tmx->tm_sec);
    return buff;
}



/*
time_t filetime_to_timet(FILETIME *ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
*/



int strtime_to_time(u8 *str, u32 *ret32, u64 *ret64) {
    int     i;
    i32     day     = 0,
            month   = 0,
            year    = 0,
            hour    = 0,
            min     = 0,
            sec     = 0;
    u8      s1[32]  = "",
            s2[32]  = "";

    if(ret32) *ret32 = 0;
    if(ret64) *ret64 = 0;
    if(!str || !str[0]) return -1;
    sscanf(str,
        "%20[^ ,/] %20[^ ,/] %d %d:%d:%d",
        s1, s2, &year,
        &hour, &min, &sec);

    month = -1;
    for(i = 0; g_months[i]; i++) { if(stristr(s1, g_months[i])) { month = i; break; } }
    if(month >= 0) {
        day = atoi(s2);
    } else {
        for(i = 0; g_months[i]; i++) { if(stristr(s2, g_months[i])) { month = i; break; } }
        day = atoi(s1);
    }
    if(day > 31) {
        i = day;
        day = year;
        year = i;
    }

    /*
    SYSTEMTIME  st;
    FILETIME    ft;
    st.wYear = year;
    st.wMonth = month;
    st.wDayOfWeek = 0;
    st.wDay = day;
    st.wHour = hour;
    st.wMinute = min;
    st.wSecond = sec;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, &ft);
    if(ret64) *ret64 = *(u64 *)&ft;
    if(ret32) {
        // I don't know why but it will be a month - 1... mah
        // I have already verified with strftime and it confirms the problem
        st.wMonth = month + 1;
        SystemTimeToFileTime(&st, &ft);
        *ret32 = filetime_to_timet(&ft);
    }
    */
    struct tm   tp;
    u64     t64;
    u32     t32;
    memset(&tp, 0, sizeof(tp));
    tp.tm_sec = sec;
    tp.tm_min = min;
    tp.tm_hour = hour;
    tp.tm_mday = day;
    tp.tm_mon = month;
    tp.tm_year = year - 1900;
    t32 = mktime(&tp);
    tp.tm_mon = month - 1;  // same problem as before
    t64 = mktime(&tp);
    t64 += (u64)11644473600ULL;
    t64 *= (u64)10000000ULL;
    if(ret32) *ret32 = t32;
    if(ret64) *ret64 = t64;
    return 0;
}



u8 *bytes2clsid(int fdnum) {
    static u8   ret[64];
    clsid_t clsid;

    if(myfr(fdnum, (void *)&clsid, sizeof(clsid_t), TRUE) < 0) return NULL;

    // endianess
    clsid.g1    = getxx((void *)&clsid.g1, 4);
    clsid.g2    = getxx((void *)&clsid.g2, 2);
    clsid.g3    = getxx((void *)&clsid.g3, 2);

    sprintf(ret,
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        clsid.g1,
        clsid.g2, clsid.g3,
        clsid.g4, clsid.g5,
        clsid.g6, clsid.g7, clsid.g8, clsid.g9, clsid.g10, clsid.g11);
    return ret;
}



int clsid2bytes(int fdnum, u8 *data) {
    clsid_t clsid;
    u32     g1, g2, g3, g4, g5, g6, g7, g8, g9, g10, g11;

    if(!data) return -1;
    memset(&clsid, 0, sizeof(clsid_t));
    sscanf(data,
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &g1, &g2, &g3, &g4, &g5, &g6, &g7, &g8, &g9, &g10, &g11);

    clsid.g1  = g1;
    clsid.g2  = g2;
    clsid.g3  = g3;
    clsid.g4  = g4;
    clsid.g5  = g5;
    clsid.g6  = g6;
    clsid.g7  = g7;
    clsid.g8  = g8;
    clsid.g9  = g9;
    clsid.g10 = g10;
    clsid.g11 = g11;

    // endianess
    clsid.g1    = getxx((void *)&clsid.g1, 4);
    clsid.g2    = getxx((void *)&clsid.g2, 2);
    clsid.g3    = getxx((void *)&clsid.g3, 2);

    return(myfw(fdnum, (void *)&clsid, sizeof(clsid_t)));
}



int quickbms_asm(int fdnum, u8 *pasm) {
    t_asmmodel  am;
    int     len;
    u8      errtext[TEXTLEN];

    if(!pasm) return -1;

    /*
    lowercase   = 1;
    extraspace  = 1;
    showmemsize = 1;
    */

    len = Assemble(pasm, 0 /*0x00400000*/, &am, 0, 0, errtext);
    if(len <= 0) return -1;
    return(myfw(fdnum, am.code, len));
}



u8 *quickbms_disasm(int fdnum, int type) {
    static u8   out[256];
    u8          in[32];
    int     i,
            c;

    out[0] = 0;

#ifdef ENABLE_BEAENGINE
    DISASM  disasm;
    int     len;

    memset(&disasm, 0, sizeof(disasm));
    disasm.Archi        = 0;
    disasm.Options      = NoTabulation | NasmSyntax | PrefixedNumeral;
    disasm.EIP          = (UIntPtr)in;
    disasm.VirtualAddr  = myftell(fdnum);

    for(disasm.SecurityBlock = 1;; disasm.SecurityBlock++) {
        if(disasm.SecurityBlock >= sizeof(in)) return NULL;
        len = myfgetc(fdnum);
        if(len < 0) return NULL;
        in[disasm.SecurityBlock - 1] = len;
        len = Disasm(&disasm);
        if(len == OUT_OF_BLOCK) continue;
        if(len == UNKNOWN_OPCODE) {
            mystrcpy(out, "unknown", sizeof(out));
        } else {
            mystrcpy(out, disasm.CompleteInstr, sizeof(out));
        }
        break;
    }
#elif ENABLE_OLLYDBG
    static t_config    asm_cfg;
    static int  init = 0;
    t_disasm da;

    if(!init) {
        init = 1;
        Preparedisasm();
        memset(&asm_cfg, 0, sizeof(asm_cfg));
        asm_cfg.lowercase = 0;            // Force lowercase display
        asm_cfg.tabarguments = 0;         // Tab between mnemonic and arguments
        asm_cfg.extraspace = 0;           // Extra space between arguments
        asm_cfg.putdefseg = 0;            // Display default segments in listing
        asm_cfg.showmemsize = 1;          // Always show memory size
        asm_cfg.shownear = 0;             // Show NEAR modifiers
        asm_cfg.shortstringcmds = 0;      // Use short form of string commands
        asm_cfg.sizesens = 0;             // How to decode size-sensitive mnemonics
    }

    int offset = myftell(fdnum);
    for(i = 0;; i++) {
        if(i >= sizeof(in)) return NULL;
        c = myfgetc(fdnum);
        if(c < 0) return NULL;
        in[i] = c;
        //int len = 
        Disasm(in, i + 1, offset, &da, DA_TEXT, &asm_cfg, NULL);
        //if(len <= 0) return NULL; // probably it's better to keep it disabled and trusting da.errors only

        // never disable the following!
        if(da.errors == DAE_NOERR) {
            mystrcpy(out, da.result, sizeof(out));
            break;
        }

        if(da.errors & DAE_BADCMD) break;
    }
#else
    static csh  handle      = 0;
    static int  last_type   = -1;

    cs_insn *insn   = NULL;
    int     count;

    if(type != last_type) {
        last_type = type;
        cs_close(&handle);

        int     arch = CS_ARCH_X86;
        int     mode = CS_MODE_32;

        switch(type) {
        case BMS_TYPE_ASM16:        arch = CS_ARCH_X86;     mode = CS_MODE_16;      break;
        case BMS_TYPE_ASM64:        arch = CS_ARCH_X86;     mode = CS_MODE_64;      break;
        case BMS_TYPE_ASM_ARM:      arch = CS_ARCH_ARM;     mode = CS_MODE_ARM;     break;
        case BMS_TYPE_ASM_ARM_THUMB:arch = CS_ARCH_ARM;     mode = CS_MODE_THUMB;   break;
        case BMS_TYPE_ASM_ARM64:    arch = CS_ARCH_ARM64;   mode = CS_MODE_ARM;     break;
        case BMS_TYPE_ASM_MIPS:     arch = CS_ARCH_MIPS;    mode = CS_MODE_MIPS32;  break;
        case BMS_TYPE_ASM_MIPS64:   arch = CS_ARCH_MIPS;    mode = CS_MODE_MIPS64;  break;
        case BMS_TYPE_ASM_PPC:      arch = CS_ARCH_PPC;     mode = CS_MODE_32;      break;
        case BMS_TYPE_ASM_PPC64:    arch = CS_ARCH_PPC;     mode = CS_MODE_64;      break;
        case BMS_TYPE_ASM_SPARC:    arch = CS_ARCH_SPARC;   mode = 0;               break;
        case BMS_TYPE_ASM_SYSZ:     arch = CS_ARCH_SYSZ;    mode = 0;               break;
        case BMS_TYPE_ASM_XCORE:    arch = CS_ARCH_XCORE;   mode = 0;               break;
        case BMS_TYPE_ASM:
        default:                    arch = CS_ARCH_X86;     mode = CS_MODE_32;      break;
        }

        if(g_endian == MYLITTLE_ENDIAN) {
            mode += CS_MODE_LITTLE_ENDIAN;
        } else {
            mode += CS_MODE_BIG_ENDIAN;
        }

        if(cs_open(arch, mode, &handle) != CS_ERR_OK) return NULL;
        //cs_option(handle, CS_OPT_DETAIL, CS_OPT_OFF);         // already default
        //cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);  // if you like it...
    }

    int offset = myftell(fdnum);
    for(i = 0;; i++) {
        if(i >= sizeof(in)) return NULL;
        c = myfgetc(fdnum);
        if(c < 0) return NULL;
        in[i] = c;
        count = cs_disasm(handle, in, i + 1, offset, 1, &insn);
        if(count > 0) {
            int t = snprintf(out, sizeof(out), "%s %s", insn[0].mnemonic, insn[0].op_str);
            if((t < 0) || (t > sizeof(out))) out[sizeof(out) - 1] = 0;
            cs_free(insn, count);
            break;
        }
    }
#endif

    return out;
}



u8 *ipv6_to_string(int fdnum) {
    static u8   ret[64];
    int     i,
            len;
    u16     t16;
    u8      ipv6[16];

    if(myfr(fdnum, ipv6, sizeof(ipv6), TRUE) < 0) return NULL;

    len = 0;
    for(i = 0; i < sizeof(ipv6); i += 2) {
        if(i) len += sprintf(ret + len, ":");
        t16 = (ipv6[i] << 8) | ipv6[i + 1];
        len += sprintf(ret + len, "%04x", t16);
    }
    return ret;
}



int string_to_ipv6(int fdnum, u8 *s) {
    int     i,
            len;
    u16     t16;
    u8      ipv6[16];

    if(!s) return -1;

    memset(ipv6, 0, sizeof(ipv6));

    i = 0;
    if((s[0] == ':') && (s[1] == ':')) {
        s += 2;
        i = sizeof(ipv6) - 4;
        ipv6[6] = 0xff;
        ipv6[7] = 0xff;
    }
    for(; i < sizeof(ipv6); i += 2) {
        if(!*s) break;
        t16 = readbase(s, 16, &len);
        ipv6[i]     = t16 >> 8;
        ipv6[i + 1] = t16;
        s += len;
        if(*s == ':') s++;
    }
    return(myfw(fdnum, ipv6, sizeof(ipv6)));
}



u8 *get_dos_time(void) {
    struct  tm  *tmx;
    time_t  datex;
    static  char    buff[32];

    datex = time(NULL);
    tmx = gmtime(&datex);
    if(!tmx) return("none");
    sprintf(
        buff,
        "%02d-%3.3s-%02d  %02d:%02d",
        tmx->tm_mday,
        g_months[tmx->tm_mon % 12],
        (1900 + tmx->tm_year) % 100,
        tmx->tm_hour, tmx->tm_min);
    return buff;
}



u8 *get_ls_time(void) {
    struct  tm  *tmx;
    time_t  datex;
    static  char    buff[32];

    datex = time(NULL);
    tmx = gmtime(&datex);
    if(!tmx) return("none");
    sprintf(
        buff,
        "%3.3s %2d %02d:%02d",
        g_months[tmx->tm_mon % 12],
        tmx->tm_mday,
        tmx->tm_hour, tmx->tm_min);
    return buff;
}


