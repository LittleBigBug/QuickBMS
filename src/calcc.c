// code from my calcc tool, but limited in some functionalities
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;
typedef uint64_t    u64;



    #define NTI     u64
    #define NTF     double
    #define NTSZ    64



int calcc_findop(int c) {
    return(
        (c == '~') || (c == '!') ||
        (c == '<') || (c == '>') ||
        (c == 'l') || (c == 'r') ||
        (c == '^') || (c == '&') || (c == '|') ||
        (c == 'p') || (c == 'v') ||
        (c == 's') || (c == 'w') ||
        (c == '*') || (c == '/') || (c == '%') ||
        (c == '+') || (c == '-') ||
        (c == '=') ||
        (c == '(') || (c == ')') ||
        !c);
}



NTI calcc_rol(NTI n1, NTI n2) {
    return((n1 << n2) | (n1 >> (NTSZ - n2)));
}



NTI calcc_ror(NTI n1, NTI n2) {
    return((n1 >> n2) | (n1 << (NTSZ - n2)));
}



NTI calcc_bitswap(NTI n1, NTI n2) {
    NTI     out,
            rem = 0;

    if(n2 < NTSZ) {
        rem = n1 & (((NTI)-1) ^ (((NTI)1 << n2) - 1));
    }

    for(out = 0; n2; n2--) {
        out = (out << 1) | (n1 & 1);
        n1 >>= 1;
    }
    return(out | rem);
}



NTI calcc_byteswap(NTI n1, NTI n2) {
    NTI     out,
            rem = 0;

    if(n2 < (NTSZ >> 3)) {
        rem = n1 & (((NTI)-1) ^ (((NTI)1 << (n2 << 3)) - 1));
    }

    for(out = 0; n2; n2--) {
        out = (out << 8) | (n1 & 0xff);
        n1 >>= 8;
    }
    return(out | rem);
}



    // the following function should be part of the
    // ffmpeg project but I have not verified it
NTI calcc_mysqrt(NTI num) {
     NTI    ret    = 0,
            ret_sq = 0,
            b;
     int    s;

     for(s = (NTSZ >> 1) - 1; s >= 0; s--) {
         b = ret_sq + ((NTI)1 << (s << 1)) + ((ret << s) << 1);
         if(b <= num) {
             ret_sq = b;
             ret += (NTI)1 << s;
         }
     }
     return(ret);
}



    // optimized from http://www.hackersdelight.org/HDcode/iexp.cc
NTI calcc_power(NTI n1, NTI n2) {
    NTI     out = 1;

    for(;;) {
        if(n2 & 1) out *= n1;
        n2 >>= 1;
        if(!n2) break;
        n1 *= n1;
    }
    return(out);
}




NTI calcc_radix(NTI n1, NTI n2) {
    NTI     i,
            old,    // due to the
            new;    // lack of bits

    if(!n1 || !n2) return(0);

    if(n2 == 2) return(calcc_mysqrt(n1)); // fast way

    for(i = old = 1; ; i <<= 1) {   // faster???
        new = calcc_power(i, n2);
        if((new > n1) || (new < old)) break;
        old = new;
    }

    for(i >>= 1; ; i++) {
        new = calcc_power(i, n2);
        if((new > n1) || (new < old)) break;
        old = new;
    }
    return(i - 1);
}



NTI calcc_ip2num(char *ip) {
    NTI     out = 0;
    int     i,
            len;
    unsigned oct[6];

    for(i = 0; i < 6; i++) {
        oct[i] = 0;
    }

    len = sscanf(ip,
        "%u.%u.%u.%u.%u.%u",
        &oct[0], &oct[1], &oct[2], &oct[3], &oct[4], &oct[5]);

    for(i = 0; i < 6; i++) {
        oct[i] &= 0xff;  // needed!
    }

    for(i = 0; i < len; i++) {
        out = (out << 8) | oct[i];
    }
    return(out);
}



NTI calcc_readdouble(char *data) {
    NTI     out;
    double  db;
    float   fl;

    fl = atof(data);
    db = atof(data);
    out = 0;
    if(sizeof(out) == sizeof(db)) {
        memcpy(&out, &db, sizeof(out));
    } else {
        memcpy(&out, &fl, sizeof(out));
    }
    return(out);
}



NTI calcc_readbase(char *data, int size) {

    static const u8 b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const u8 b32_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    static const u8 hex_table[] = "0123456789abcdef";

    NTI     num;
    int     sign = 0;
    u8      c,
            *p,
            *table;

    if((size > 0) && (size <= 16)) {
        table = (u8 *)hex_table;
    } else if(size == 32) {
        table = (u8 *)b32_table;
    } else if(size == 64) {
        table = (u8 *)b64_table;
    } else {
        return(0);
    }
    if(*data == '-') {  // useless in calcc but can useful in other programs
        sign = 1;
        data++;
    }
    for(num = 0; *data; data++) {
        c = *data;      // already tolower()
        if((c == 'x') && (size == 16)) continue;
        p = memchr(table, c, size);
        if(!p) break;
        num = (num * size) + (p - table);
    }
    if(sign) num = -num;
    return(num);
}



NTI calcc_readtime(char *data) {
    NTI     out = 0;
    int     ss = 0,
            mm = 0,
            hh = 0;

    sscanf(
        data,
        "%02d:%02d:%02d",
        &hh,
        &mm,
        &ss);

    // hh %= 24;    // maximum freedom of using bigger values
    // mm %= 60;
    // ss %= 60;
    out = (hh * 60 * 60) + (mm * 60) + ss;
    if(!out) out = time(NULL);
    return(out);
}



NTI calcc_get_num(char *p, NTI left) {
    NTI     num;
    int     dot;
    char    *l;

    if(calcc_findop(*p)) return(0);

    dot = 0;        // auto IP
    for(l = p; !calcc_findop(*l); l++) {
        if(*l == '.') {
            dot++;
            if(dot == 3) return(calcc_ip2num(p));
        }
    }
    if((dot == 1) || strchr(p, ',')) return(calcc_readdouble(p));

    if((*p == '0') && (p[1] == 'x')) p++;  // hex

    num = 0;    // compatibility
    switch(*p) {
        case 'x':
        case '$':
        case 'h': num = calcc_readbase(p + 1, 16);            break;
        case 'o': num = calcc_readbase(p + 1, 8);             break;
        case 'b': num = calcc_readbase(p + 1, 2);             break;
        case 'i': num = calcc_ip2num(p + 1);                  break;
        case 'q': num = calcc_readbase(p + 1, 4);             break;
        case 't': num = calcc_readtime(p + 1);                break;
        case 'd': num = calcc_readbase(p + 1, 10) * 86400;    break;
        case 'm': num = calcc_readbase(p + 1, 10) * 2629744;  break;  // 28,29,30 and 31
        case 'y': num = calcc_readbase(p + 1, 10) * 31556926; break;
        case 'c': {
            num = calcc_readbase(p + 1, 10) * left;
            num /= 100;
            break;
        }
        //case '\"':  // impossible because " gets replaced by '
        case '\'': {
            for(++p; *p && (*p != '\''); p++) {
                num <<= 8;
                num |= *p;
            }
            break;
        }
        default:  num = calcc_readbase(p, 10);                break;
    }
    return(num);
}



int calcc(char *input) {

    #define calcc_MOV(X,Y)    memmove(X, Y, slen - (Y - buff));

    NTI     left,
            right,
            tot;
    NTF     ntftmp;
    double  totdb;
    float   totfl;
    int     op,
            len,
            slen;
    char    //buff[READSZ * 9],   // should be enough
            tmp[NTSZ + 1],
            *data,
            *pl,
            *pr,
            *p,
            *l,
            *special;

    static char *buff = NULL;
    buff = realloc((strlen(input) * 9) + 1);
    if(!buff) return 0;
    strcpy(buff, input);

        //mytolower(buff);  don't use it or "" will not work

        for(p = data = buff; *p; p++) {
            if(*p == '\"') *p = '\'';
            if(*p == '\'') {
                for(l = p; *l; l++) {
                    if(*l == '\"') *l = '\'';
                    *data++ = *l;
                    if((l != p) && (*l == '\'')) break;
                }
                p = l;
                continue;
            }

            *p = tolower(*p);
            if((*p == '\r') || (*p == '\n')) {
                break;
            } else if(*p <= ' ') {
                continue;
            } else if((*p == '[') || (*p == '{')) {
                *data++ = '(';
            } else if((*p == ']') || (*p == '}')) {
                *data++ = ')';
            } else if((*p == '<') && (p[1] == '<')) {
                continue;
            } else if((*p == '>') && (p[1] == '>')) {
                continue;
            } else if((*p == '&') && (p[1] == '&')) {
                continue;
            } else if((*p == '|') && (p[1] == '|')) {
                continue;
            } else {
                *data++ = *p;
            }
        }
        *data = 0;
        slen = (data - buff) + 1;   // NULL included!!!
        /*
        if(!strcmp(buff, "?") || !strcmp(buff, "help")) {   // they are already lower
            help();
        } else if(!strcmp(buff, "q") || !strcmp(buff, "quit")) {   // they are already lower
            exit(0);
        }
        */

        for(;;) {
            data = buff;

            #ifdef DEBUG
            printf("DEBUG: %s\n", data);
            #endif

            special = NULL;
            if(*data == '\'') {
                special = data;
                for(++data; *data && (*data != '\''); data++);
                if(*data) data++;
            }

            pr = strchr(data, ')');
            if(pr) {
                for(pl = pr; (pl >= data) && (*pl != '('); pl--);
                data = pl + 1;
                if(data == buff) {
                    printf("- no opened parenthesis found\n");
                    *data = 0;
                    break;
                }

                for(p = data; !calcc_findop(*p); p++);
                if(p == pr) {   // find (num)
                    memmove(pl, pl + 1, pr - (pl + 1));
                    calcc_MOV(pr - 1, pr + 1);
                    slen -= 2;
                    continue;
                }
                *pr = 0;
            } else if((pl = strchr(data, '('))) {
                printf("- no closed parenthesis found\n");
                *data = 0;
                break;
            }

            #define BLA(x)  if((p = strchr(data, x))) { op = x;
            #define BLAH(x) BLA(x) }

            // priority based only on the operator
            // I don't consider the left/right position at the moment
            // the user MUST ever use the parenthesis
            BLA('~')
                calcc_MOV(p + 1, p);
                slen++;
                if(!pr) pr = p;
                pr++;
                *p++ = '0';
            } else BLA('!')
                calcc_MOV(p + 1, p);
                slen++;
                if(!pr) pr = p;
                pr++;
                *p++ = '0';
            }
            else BLAH('<')
            else BLAH('>')
            else BLAH('l')
            else BLAH('r')
            else BLAH('&')
            else BLAH('^')
            else BLAH('|')
            else BLAH('p')
            else BLAH('v')
            else BLAH('s')
            else BLAH('w')
            else BLAH('*')
            else BLAH('/')
            else BLAH('%')
            else BLA('-')
                if(p[1] == '-') {
                    l = p + 2;
                    calcc_findop(*l) ? (p[1] = '1') : (*p = '1');
                }
            }
            else BLA('+')
                if(p[1] == '+') {
                    l = p + 2;
                    calcc_findop(*l) ? (p[1] = '1') : (*p = '1');
                }
            }
            else BLAH('=')
            else break;

            #undef BLA
            #undef BLAH

            for(l = p - 1; (l >= data) && !calcc_findop(*l); l--);

            if(special) l = special - 1;
            left  = calcc_get_num(++l, 1);
            right = calcc_get_num(++p, left);

            switch(op) {
                case '~': tot = right ^ ((NTI)-1);      break;
                case '!': tot = right ? 0 : 1;          break;
                case '<': { // x86 fix!
                    tot = ((right >= NTSZ) ? 0 : (left << right));
                    break;
                }
                case '>': { // x86 fix!
                    tot = ((right >= NTSZ) ? 0 : (left >> right));
                    break;
                }
                case 'l': tot = calcc_rol(left, right);       break;
                case 'r': tot = calcc_ror(left, right);       break;
                case '^': tot = left ^ right;           break;
                case '&': tot = left & right;           break;
                case '|': tot = left | right;           break;
                case 'p': tot = calcc_power(left, right);     break;
                case 'v': tot = calcc_radix(left, right);     break;
                case 's': tot = calcc_byteswap(left, right);  break;
                case 'w': tot = calcc_bitswap(left, right);   break;
                case '*': tot = left * right;           break;
                case '/': {
                    if(!right) {
                        printf("division by zero\n");
                        tot = 0;
                    } else {
                        tot = left / right;
                    }
                    break;
                }
                case '%': {
                    if(!right) {
                        printf("division by zero\n");
                        tot = 0;
                    } else {
                        tot = left % right;
                    }
                    break;
                }
                case '+': tot = left + right;           break;
                case '-': tot = left - right;           break;
                case '=': tot = right;                  break;
                default:  tot = 0;                      break;
            }

            if(pr) *pr = ')';

            while(!calcc_findop(*p)) p++;

            //len = sprintf(tmp, "h%s", calcc_showbase(tot, 16, 0));
            len = sprintf(tmp, "h%", calcc_showbase(tot, 16, 0));
            if(len != (p - l)) {
                calcc_MOV(l + len, p);
                slen += (l + len) - p;
            }
            memcpy(l, tmp, len);
        }

        if(!*buff) return 0; //continue;

        tot = calcc_get_num(buff, 1);

    return tot;
}


