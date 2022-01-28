
/* Dicky - public domain */

#ifndef __DICKY_P_H__
#define __DICKY_P_H__ 1

typedef struct LOLFSM_ {
    signed char seen_l;
    signed char seen_o;
    signed char seen_m;
    signed char seen_d;
    signed char seen_r;
    signed char seen_b;
    signed char seen_i;
    signed char seen_t;
    signed char seen_e;
} LOLFSM;

typedef struct OutputBuffer_ {
    unsigned char *buffer;
    size_t pos;
    size_t sizeof_buffer;
    size_t chunk_size;
    int quartet;
} OutputBuffer;

typedef struct InputBuffer_ {
    const unsigned char *buffer;
    size_t pos;
    size_t sizeof_buffer;
    int quartet;
} InputBuffer;

enum LOLFSM_State {
    C_HOLD = -1,
    C_SPACE = 0,
    C_LOL = 13,
    C_MDR = 14,
    C_BITE = 15,
};

static const unsigned char ctable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 4, 10, 1, 0, 10, 5, 11, 0, 5, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 3, 1, 10, 5, 9, 4, 5, 6, 7, 8, 8, 1, 2, 6, 9, 10, 11,
    12, 5, 5, 6, 4, 10, 0, 0, 0, 0, 0, 0, 1, 2, 10, 3, 1, 10, 5, 9, 4,
    5, 6, 7, 8, 8, 1, 2, 6, 9, 10, 11, 12, 5, 5, 6, 4, 10, 0, 4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 10, 1, 1, 4, 4, 10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 6, 0, 0, 4, 1, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 10, 1, 1, 1, 1, 4, 4, 4, 4, 1, 8, 1, 1, 1, 1,
    1, 10, 1, 12, 12, 12, 12, 4, 2, 2, 1, 1, 1, 1, 1, 1, 0, 10, 1, 1,
    1, 1, 4, 4, 4, 4, 1, 8, 1, 1, 1, 1, 1, 0, 1, 12, 12, 12, 12, 4, 2,
    4
};

static const char * const dtable_[] = {
    "\x20",
    "\x41\x45\x4f\x61\x65\x6f\xa3\xa4\xba\xc0\xc1\xc2\xc3\xc4\xc5"
      "\xc8\xc9\xca\xcb\xd0\xd2\xd3\xd4\xd5\xd6\xd8\xe0\xe1\xe2\xe3\xe4\xe5"
      "\xe8\xe9\xea\xeb\xf0\xf2\xf3\xf4\xf5\xf6\xf8",
    "\x42\x50\x62\x70\xde\xdf\xfe",
    "\x44\x64",
    "\x49\x59\x69\x79\x7c\xa1\xa5\xa6\xb9\xcc\xcd\xce\xcf\xdd\xec\xed"
      "\xee\xef\xfd\xff",
    "\x47\x4a\x56\x57\x67\x6a\x76\x77",
    "\x4b\x51\x58\x6b\x71\x78\xb6",
    "\x4c\x6c",
    "\x4d\x4e\x6d\x6e\xd1\xf1",
    "\x48\x52\x68\x72",
    "\x43\x46\x53\x5a\x63\x66\x73\x7a\xa2\xa7\xc7\xd7\xe7",
    "\x54\x74",
    "\x55\x75\xb5\xd9\xda\xdb\xdc\xf9\xfa\xfb\xfc"    
};

static const unsigned char * const * const dtable =
    (const unsigned char * const *) dtable_;

#define AVG_COMPRESSION_RATIO 2U

#endif
