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
// QuickBMS enum, defines, global variables and so on

enum {
    QUICKBMS_OK                 = 0,    // success
    QUICKBMS_ERROR_UNKNOWN      = 1,    // any error
    QUICKBMS_ERROR_MEMORY       = 2,    // unable to allocate memory, memory errors
    QUICKBMS_ERROR_FILE_READ    = 3,    // impossible to read/seek input file
    QUICKBMS_ERROR_FILE_WRITE   = 4,    // impossible to write output file
    QUICKBMS_ERROR_COMPRESSION  = 5,    // errors related to file compression
    QUICKBMS_ERROR_ENCRYPTION   = 6,    // errors related to file encryption
    QUICKBMS_ERROR_DLL          = 7,    // any external dll or executable
    QUICKBMS_ERROR_BMS          = 8,    // anything related the BMS script and language
    QUICKBMS_ERROR_ARGUMENTS    = 9,    // quickbms arguments (argc, argv)
    QUICKBMS_ERROR_FOLDER       = 10,   // problems with the input/output folders
    QUICKBMS_ERROR_USER         = 11,   // termination caused by the user
    QUICKBMS_ERROR_EXTRA        = 12,   // extra IO input/output
    QUICKBMS_ERROR_UPDATE       = 13,   // update feature
    //
    QUICKBMS_ERROR_DUMMY
};



enum {
    CMD_NONE = 0,
    CMD_CLog,
    CMD_Do,
    CMD_FindLoc,
    CMD_For,
    CMD_ForTo,  // for an easy handling of For
    CMD_Get,
    CMD_GetDString,
    CMD_GoTo,
    CMD_IDString,
    CMD_ImpType,
    CMD_Log,
    CMD_Math,
    CMD_Next,
    CMD_Open,
    CMD_SavePos,
    CMD_Set,
    CMD_While,
    CMD_String,
    CMD_CleanExit,
    CMD_If,
    CMD_Else,
    CMD_Elif,   // added by me
    CMD_EndIf,
    CMD_GetCT,
    CMD_ComType,
    CMD_ReverseLong,
        // added by me
    CMD_Endian,
    CMD_FileXOR,        // similar job done also by Encryption
    CMD_FileRot13,      // similar job done also by Encryption
    CMD_FileCrypt,      // experimental and useless
    CMD_Break,
    CMD_Strlen,         // not necessary (implemented in Set)
    CMD_GetVarChr,
    CMD_PutVarChr,
    CMD_Debug,          // only for debugging like -v, so not necessary
    CMD_Padding,        // useful but not necessary, can be done with GoTo
    CMD_Append,
    CMD_Encryption,
    CMD_Print,
    CMD_GetArray,
    CMD_PutArray,
    CMD_SortArray,
    CMD_SearchArray,
    CMD_StartFunction,
    CMD_CallFunction,
    CMD_EndFunction,
    CMD_ScanDir,        // not needed for the extraction jobs
    CMD_CallDLL,
    CMD_Put,            // not needed for the extraction jobs
    CMD_PutDString,     // not needed for the extraction jobs
    CMD_PutCT,          // not needed for the extraction jobs
    CMD_GetBits,        // rarely useful
    CMD_PutBits,        // rarely useful
    CMD_ReverseShort,   // rarely useful
    CMD_ReverseLongLong,// rarely useful
    CMD_Prev,           // like i--
    CMD_XMath,          // one line math
    CMD_NameCRC,        // name hashing
    CMD_Codepage,
    CMD_SLog,
    CMD_Continue,
    CMD_Label,
    CMD_Reimport,
    CMD_If_Return,      // internal usage
    CMD_DirectoryExists,
    CMD_FEof,
    CMD_NOP
};



#define ISNUMTYPE(X)    ((X > 0) || (X == BMS_TYPE_ASIZE))
enum {  // the value is referred to their size which makes the job faster, numbers are positive and the others are negative!
    BMS_TYPE_NONE               = 0,    // never change this value
    BMS_TYPE_BYTE               = 1,
    BMS_TYPE_SHORT              = 2,
    BMS_TYPE_THREEBYTE          = 3,
    BMS_TYPE_LONG               = 4,
    BMS_TYPE_LONGLONG           = 8,
    BMS_TYPE_STRING             = -1,
    BMS_TYPE_ASIZE              = -2,
    BMS_TYPE_PURETEXT           = -3,
    BMS_TYPE_PURENUMBER         = -4,
    BMS_TYPE_TEXTORNUMBER       = -5,
    BMS_TYPE_FILENUMBER         = -6,
        // added by me
    BMS_TYPE_FILENAME           = -1000,
    BMS_TYPE_BASENAME           = -1001,
    BMS_TYPE_EXTENSION          = -1002,
    BMS_TYPE_UNICODE            = -1003,
    BMS_TYPE_BINARY             = -1004,
    BMS_TYPE_LINE               = -1005,
    BMS_TYPE_FULLNAME           = -1006,
    BMS_TYPE_CURRENT_FOLDER     = -1007,
    BMS_TYPE_FILE_FOLDER        = -1008,
    BMS_TYPE_INOUT_FOLDER       = -1009,
    BMS_TYPE_BMS_FOLDER         = -1010,
    BMS_TYPE_ALLOC              = -1011,
    BMS_TYPE_COMPRESSED         = -1012,
    BMS_TYPE_FLOAT              = -1013,
    BMS_TYPE_DOUBLE             = -1014,
    BMS_TYPE_LONGDOUBLE         = -1015,
    BMS_TYPE_VARIABLE           = -1016,    // c & 0x80
    BMS_TYPE_VARIABLE2          = -1017,    // unreal index numbers
    BMS_TYPE_VARIANT            = -1018,
    BMS_TYPE_BITS               = -1019,
    BMS_TYPE_TIME               = -1020,
    BMS_TYPE_TIME64             = -1021,
    BMS_TYPE_CLSID              = -1022,
    BMS_TYPE_IPV4               = -1023,
    BMS_TYPE_IPV6               = -1024,
    //BMS_TYPE_ASM                = -1025,
    BMS_TYPE_VARIABLE3          = -1026,
    BMS_TYPE_SIGNED_BYTE        = -1027,
    BMS_TYPE_SIGNED_SHORT       = -1028,
    BMS_TYPE_SIGNED_THREEBYTE   = -1029,
    BMS_TYPE_SIGNED_LONG        = -1030,
    BMS_TYPE_VARIABLE4          = -1031,
    BMS_TYPE_VARIABLE5          = -1032,
    BMS_TYPE_FILEPATH           = -1033,
    BMS_TYPE_FULLBASENAME       = -1034,
    BMS_TYPE_TO_UNICODE         = -1035,
    BMS_TYPE_TCC                = -1036,
    BMS_TYPE_VARIABLE6          = -1037,
    BMS_TYPE_VARIABLE7          = -1038,
    BMS_TYPE_UNICODE32          = -1039,
    BMS_TYPE_EXE_FOLDER         = -1040,
        //
    BMS_TYPE_ASM16              = -2000,
    BMS_TYPE_ASM64              = -2001,
    BMS_TYPE_ASM_ARM            = -2002,
    BMS_TYPE_ASM_ARM_THUMB      = -2003,
    BMS_TYPE_ASM_ARM64          = -2004,
    BMS_TYPE_ASM_MIPS           = -2005,
    BMS_TYPE_ASM_MIPS64         = -2006,
    BMS_TYPE_ASM_PPC            = -2007,
    BMS_TYPE_ASM_PPC64          = -2008,
    BMS_TYPE_ASM_SPARC          = -2009,
    BMS_TYPE_ASM_SYSZ           = -2010,
    BMS_TYPE_ASM_XCORE          = -2011,
    BMS_TYPE_ASM                = -2012,
        //
    BMS_TYPE_REGEX              = -3000,
    BMS_TYPE_PROMPT             = -3001,
        //
    BMS_TYPE_UNKNOWN            = -10000,
        // nop
    BMS_TYPE_NOP
};



enum {
    APPEND_MODE_NONE            = 0,
    APPEND_MODE_APPEND          = 1,
    APPEND_MODE_OVERWRITE       = 2,
    APPEND_MODE_INSERT          = 3,    // sort of logical AND of APPEND and OVERWRITE
    APPEND_MODE_BEFORE          = -1
};



/*
if you add a new compression algorithm remember to modify the following files:
- comtype.h -> QUICKBMS_COMP()
- perform.c -> perform_compression
*/

// the full list of compression algorithms is visible by using
// quickbms.exe -U

#define QUICKBMS_COMP_ENUM
#include "comtype.h"
#undef  QUICKBMS_COMP_ENUM

#define QUICKBMS_COMP_LIST
#include "comtype.h"
#undef  QUICKBMS_COMP_LIST



#define QUICK_COMP_CASE(X) \
    break; \
    case COMP_##X:  set_int3((const void *)COMP_##X, (const void *)in, (const void *)zsize, (const void *)out, (const void *)size);



#define QUICK_CRYPT_CASE(X) \
    if(X) { \
        if(datalen < 0) return 0; \
        set_int3((const void *)X, (const void *)data, (const void *)datalen, (const void *)NULL, (const void *)NULL);



//#pragma pack(1)   // never enable this



enum {
    LZMA_FLAGS_NONE         = 0,
    LZMA_FLAGS_86_HEADER    = 1,
    LZMA_FLAGS_86_DECODER   = 2,
    LZMA_FLAGS_EFS          = 4,
    LZMA_FLAGS_PROP0        = 0x1000,
    LZMA_FLAGS_NOP
};



typedef struct {
    void    *info;
    u8      *data;
    u_int   size;
} data_t;



typedef struct {
    u8      active;
    int     vars;
    int     *var;           // example: idx of i and j
    int     arrays;
    data_t  *array;         // list of arrays containing the various values of i:j
} sub_variable_t;



#define MAX_REIMPORT_MATH_OPS   8   // apparently doesn't eat much memory, even 256 takes only one megabyte
typedef struct {
    u_int   offset;     // offsets are u_int because quickbms is limited to 32bit, quickbms_4gb_files doesn't have this limit
    i32     fd;
    i32     type;
    i32     math_ops;   // history of operations
    i32     math_op   [MAX_REIMPORT_MATH_OPS];  // i32 necessary for -1000
    int     math_value[MAX_REIMPORT_MATH_OPS];
    i32     math_opbck[MAX_REIMPORT_MATH_OPS];
    int     use_filexor;
} variable_reimport_t;



typedef struct {
    // for optimizing the usage of the memory I use a static buffer and an allocated pointer used if
    // the static buffer is not big enough
    // pros: fast and avoids memory consumption with xalloc
    // cons: wastes memory, moreover with -9 (compared with the allocated only version)

#ifndef QUICKBMS_VAR_STATIC
    union {
#endif
    u8      *name;          // name of the variable, it can be also a fixed number since "everything" is handled as a variable
    u8      *name_alloc;
#ifndef QUICKBMS_VAR_STATIC
    };
#endif
#ifdef QUICKBMS_VAR_STATIC
    u8      name_static[VAR_NAMESZ + 1];
#endif

#ifndef QUICKBMS_VAR_STATIC
    union {
#endif
    u8      *value;         // content of the variable
    u8      *value_alloc;
#ifndef QUICKBMS_VAR_STATIC
    };
#endif
#ifdef QUICKBMS_VAR_STATIC
    u8      value_static[VAR_VALUESZ + 1];
#endif

    int     value32;        // number

#ifndef QUICKBMS_VAR_STATIC
    union {
#endif
    u_int   size;           // used for avoiding to waste realloc too much, not so much important and well used in reality
    u_int   real_size;      // work-around to avoid to "touch" the size value
#ifndef QUICKBMS_VAR_STATIC
    };
#endif

    i8      isnum;          // 1 if it's a number, 0 if a string. -1 for floats
    i8      constant;       // 1 if the variable is a fixed number and not a "real" variable
    i8      binary;         // 1 if the variable is binary
    i8      reserved;

    double  float64;

    sub_variable_t  *sub_var;

    variable_reimport_t reimport;   // alternative reimport mode
} variable_t;



typedef struct {
    int     var[MAX_ARGS];  // pointer to a variable
    int     num[MAX_ARGS];  // simple number
    u8      *str[MAX_ARGS]; // fixed string
    u8      type;           // type of command to execute
    u8      *debug_line;    // used with -v
    i32     bms_line_number;
    u32     mask;           // used only in idstring and findloc but may be useful in the future
} command_t;



#define FDBITS \
    u8      bitchr; \
    u8      bitpos; \
    u_int   bitoff;



typedef struct {
    u8      byte;
    u8      idx;    // it's necessary to save the memory although idx can be truncated
    u8      flags;
    u8      *name;
} hexhtml_t;



#include "extra/utlist.h"



typedef struct parsing_debug_t {
    u_int   offset;
    u_int   end_offset;
    double  entropy;
    struct parsing_debug_t *next;
    struct parsing_debug_t *prev;
} parsing_debug_t;



typedef struct {
    FILE    *fd;
    u8      *fullname;      // just the same input filename, like c:\myfile.pak or ..\..\myfile.pak
    u8      *filename;      // input filename only, like myfile.pak
    u8      *basename;      // input basename only, like myfile
    u8      *fileext;       // input extension only, like pak
    u8      *filepath;
    u8      *fullbasename;
    FDBITS
    hexhtml_t   *hexhtml;
    u_int   hexhtml_size;
    u_int   coverage;       // experimental coverage
    parsing_debug_t *parsing_debug;
    //
    void    *sd;            // socket operations
    void    *pd;            // process memory operations
    void    *ad;            // audio operations
    void    *vd;            // video operations
    void    *md;            // Windows messages operations
    u8      temporary_file;
    i32     invalid_reimport2;
    u_int   tail_toc_offset;
    u_int   tail_toc_size;
} filenumber_t;



typedef struct {
    u8      *data;
    u_int   pos;
    u_int   size;
    u_int   maxsize;
    FDBITS
    hexhtml_t   *hexhtml;
    u_int   hexhtml_size;
    u_int   coverage;       // experimental coverage
} memory_file_t;



typedef struct {
    u_int       allocated_elements;
    u_int       elements;
    variable_t  *var;
} array_t;



typedef struct {
    u8      *name;
    //u64     offset; // unused at the moment
    u64     size;
} files_t;



typedef struct {
    u32     g1;
    u16     g2;
    u16     g3;
    u8      g4;
    u8      g5;
    u8      g6;
    u8      g7;
    u8      g8;
    u8      g9;
    u8      g10;
    u8      g11;
} clsid_t;



filenumber_t    g_filenumber[MAX_FILES + 1]     = {{0}};
variable_t      g_variable_main[MAX_VARS + 1]   = {{0}};
variable_t      *g_variable = g_variable_main;  // remember to reinitialize it every time (to avoid problems with callfunction)
command_t       g_command[MAX_CMDS + 1]         = {{{0}}};
memory_file_t   g_memory_file[MAX_FILES + 1]    = {{0}};
array_t         g_array[MAX_ARRAYS + 1]         = {{0}};



int             evp_do_final    = 0;    // outside #ifndef
#ifndef DISABLE_SSL
HMAC_CTX        *hmac_ctx       = NULL;
EVP_CIPHER_CTX  *evp_ctx        = NULL;
EVP_MD_CTX      *evpmd_ctx      = NULL;
BF_KEY          *blowfish_ctx   = NULL;
typedef struct {
    AES_KEY     ctx;
    u8          ivec[AES_BLOCK_SIZE];
    u8          ecount[AES_BLOCK_SIZE];
	unsigned    num;
    int         type;
} aes_ctr_ctx_t;
enum {
    aes_ctr_ctx_ctr,
    aes_ctr_ctx_ige,
    aes_ctr_ctx_bi_ige,
    aes_ctr_ctx_heat,
};
aes_ctr_ctx_t   *aes_ctr_ctx    = NULL;
aes_ctr_ctx_t   *zip_aes_ctx    = NULL;
typedef struct {
    BIGNUM  *n;
    BIGNUM  *e;
    BIGNUM  *c;
    BN_CTX  *bn_tmp;
    BIGNUM  *r;
    int     zed;
} modpow_context;
modpow_context *modpow_ctx = NULL;
#endif
tea_context     *tea_ctx        = NULL;
xtea_context    *xtea_ctx       = NULL;
xxtea_context   *xxtea_ctx      = NULL;
swap_context    *swap_ctx       = NULL;
math_context    *math_ctx       = NULL;
xmath_context   *xmath_ctx      = NULL;
random_context  *random_ctx     = NULL;
xor_context     *xor_ctx        = NULL;
rot_context     *rot_ctx        = NULL;
rotate_context  *rotate_ctx     = NULL;
reverse_context *reverse_ctx    = NULL;
flip_context    *flip_ctx       = NULL;
inc_context     *inc_ctx        = NULL;
charset_context *charset_ctx    = NULL;
charset_context *charset2_ctx   = NULL;
TWOFISH_context *twofish_ctx    = NULL;
SEED_context    *seed_ctx       = NULL;
serpent_context_t *serpent_ctx  = NULL;
ICE_KEY         *ice_ctx        = NULL; // must be not allocated
Rotorobj        *rotor_ctx      = NULL;
ssc_context     *ssc_ctx        = NULL;
wincrypt_context *wincrypt_ctx  = NULL;
bcrypt_context  *bcrypt_ctx     = NULL;
cunprot_context *cunprot_ctx    = NULL;
u32             *zipcrypto_ctx  = NULL;
u32             *threeway_ctx   = NULL;
void            *skipjack_ctx   = NULL;
ANUBISstruct    *anubis_ctx     = NULL;
aria_ctx_t      *aria_ctx       = NULL;
u32             *crypton_ctx    = NULL;
u32             *frog_ctx       = NULL;
gost_ctx_t      *gost_ctx       = NULL;
int             lucifer_ctx     = 0;
u32             *mars_ctx       = NULL;
u32             *misty1_ctx     = NULL;
NOEKEONstruct   *noekeon_ctx    = NULL;
seal_ctx_t      *seal_ctx       = NULL;
safer_key_t     *safer_ctx      = NULL;
int             kirk_ctx        = -1;
u8              *pc1_128_ctx    = NULL;
u8              *pc1_256_ctx    = NULL;
sph_context     *sph_ctx        = NULL;
u32             *mpq_ctx        = NULL;
#ifndef DISABLE_MCRYPT
    MCRYPT      mcrypt_ctx      = NULL;
#endif
u32             *rc6_ctx        = NULL;
xor_prev_next_context *xor_prev_next_ctx = NULL;
typedef struct {
    void    *openssl_rsa_private;
    void    *openssl_rsa_public;
#ifndef DISABLE_TOMCRYPT
    rsa_key tomcrypt_rsa;
#endif
    int     is_tomcrypt;
    u8      *ivec;  // currently unused
    int     ivecsz;
} rsa_context;
rsa_context     *rsa_ctx        = NULL;
#ifndef DISABLE_TOMCRYPT
    typedef struct {
        int     idx;
        int     cipher;
        int     hash;
        int     prng;
        u8      *key;
        int     keysz;
        u8      *ivec;      // allocated
        int     ivecsz;
        u8      *nonce;     // allocated
        int     noncelen;
        u8      *header;    // allocated
        int     headerlen;
        u8      *tweak;     // allocated
    } TOMCRYPT;
    TOMCRYPT    *tomcrypt_ctx   = NULL;
#endif
crc_context     *crc_ctx        = NULL;
u8              *execute_ctx    = NULL;
u8              *calldll_ctx    = NULL;
typedef struct {
    int     algo;
    u8      *key;           // allocated
    int     keysz;
    u8      *ivec;          // allocated
    int     ivecsz;
} ecrypt_context;
ecrypt_context  *ecrypt_ctx     = NULL;
enum {
#define QUICKBMS_ECRYPT_defs
#include "encryption/ecrypt.h"
#undef QUICKBMS_ECRYPT_defs
};
ISAAC_ctx_t     *isaac_ctx      = NULL;
int             isaac_vernam_ctx    = 0;
int             isaac_caesar_ctx    = 0;
int             hsel_ctx            = 0;
int             rng_ctx             = 0;
u8              *g_oodlenetwork_state       = NULL;
int             g_oodlenetwork_state_size   = 0;
u8              *g_oodlenetwork_shared      = NULL;
int             g_oodlenetwork_shared_size  = 0;
u8              *molebox_ctx        = NULL;
replace_ctx_t   *replace_ctx        = NULL;
arc4_context    *rc4_ctx            = NULL;
int             d3des_ctx           = 0;
chacha20_ctx    *chacha_ctx         = NULL;
typedef struct {
    int     size;
    u64     hash1;
    u64     hash2;
    u8      *key;
    int     keysz;
} myhash_context;
myhash_context  *spookyhash_ctx     = NULL;
int             murmurhash_ctx      = 0;
myhash_context  *xxhash_ctx         = NULL;
typedef struct {
    int     algo;
    int     openssl;
    u8      *key;
    int     keysz;
    u8      *ivec;
    int     ivecsz;
    int     iter;
    int     hash;
} PBKDF_ctx_t;
PBKDF_ctx_t     *PBKDF_ctx          = NULL;



enum {
    DEBUG_OUTPUT_JSON,
    DEBUG_OUTPUT_CSV,
    DEBUG_OUTPUT_YAML,
    DEBUG_OUTPUT_XML,
    DEBUG_OUTPUT_C,
    //
    DEBUG_OUTPUT_ERROR,
};
typedef struct {
    int     format;
    u8      *ext;
    u8      *filename;
    int     level;
    FILE    *fd;
} g_debug_output_t;
g_debug_output_t    *g_debug_output = NULL;



quickiso_ctx_t  *g_quickiso         = NULL;
quickzip_ctx_t  *g_quickzip         = NULL;
FILE    *g_listfd                   = NULL;
int     g_codepage_default          = -1;   // necessary for the -P option and the bms initialization
int     g_last_cmd                  = 0,
        g_codepage                  = -1,   // ok
        g_bms_line_number           = 0,
        g_extracted_files           = 0,
        g_extracted_files2          = 0,    // used only for -f #NUM
        g_extracted_logs            = 0,
        g_reimported_files          = 0,
        g_reimported_files_skip     = 0,
        g_reimported_files_404      = 0,
        g_reimported_logs           = 0,
        g_endian                    = MYLITTLE_ENDIAN,
        g_list_only                 = 0,
        g_force_overwrite           = 0,
        g_force_rename              = 0,
        g_verbose                   = 0,
        g_quiet                     = 0,
        g_quick_gui_exit            = 0,
        g_compression_type          = COMP_ZLIB,
        g_comtype_dictionary_len    = 0,
        g_comtype_scan              = 0,
        g_encrypt_mode              = 0,
        g_append_mode               = APPEND_MODE_NONE,
        g_temporary_file_used       = 0,
        g_quickbms_version          = 0,
        g_decimal_notation          = 1,    // myitoa is a bit slower (due to the %/) but is better for some strings+num combinations
        g_mex_default               = 0,
        g_script_uses_append        = 0,
        g_write_mode                = 0,
        g_input_total_files         = 0,
        g_endian_killer             = 0,
        g_void_dump                 = 0,
        g_reimport                  = 0,
        g_enable_hexhtml            = 0,
        g_continue_anyway           = 0,
        g_yes                       = 0,
        g_int3                      = 0,
        g_is_gui                    = 0,
        g_memfile_reimport_name     = -1,
        g_lame_add_var_const_workaround = 0,
        g_reimport_zero             = 0,
        g_keep_temporary_file       = 0,
        g_decimal_names             = 0,
        g_ignore_comp_errors        = 0,
        g_force_cstring             = 0,
        g_quickbms_outname          = 0,
        g_parsing_debug             = 0,
        g_ipc_web_api               = 0,
        g_replace_fdnum0            = 0,
        g_extracted_file_tree_view_mode = -1,
        g_quickbms_dll              = 0,
        g_extraction_hash           = 0,
        g_force_output_pos          = 0,    // used in reimport mode, don't reset it
        g_reimport_shrink_enlarge   = 0,
        g_force_utf16               = 0,    // currently used only in slog
        g_log_filler_char           = -1,
        g_slog_id                   = 0,
        g_c_structs_allowed         = 0;
        //g_min_int                   = 1 << ((sizeof(int) << 3) - 1),
        //g_max_int                   = (u_int)(1 << ((sizeof(int) << 3) - 1)) - 1;
u8      g_current_folder[PATHSZ + 1]= "",  // just the current folder when the program is launched
        g_bms_folder[PATHSZ + 1]    = "",
        g_bms_script[PATHSZ + 1]    = "",
        g_exe_folder[PATHSZ + 1]    = "",
        g_file_folder[PATHSZ + 1]   = "",
        g_temp_folder[PATHSZ + 1]   = "",
        g_output_folder[PATHSZ + 1] = "",
        **g_filter_files            = NULL,     // the wildcard
        **g_filter_in_files         = NULL,     // the wildcard
        *g_comtype_dictionary       = NULL,
        *g_quickbms_execute_file    = NULL,
        *g_force_output             = NULL,
        *g_compare_folder           = NULL;
int     EXTRCNT_idx                 = -1,
        BytesRead_idx               = -1,
        NotEOF_idx                  = -1,
        SOF_idx                     = -1,
        EOF_idx                     = -1;

typedef struct {
    int     cmd;
    u8      *key;
    u_int   *pos;   // it was int but failed in post_fseek_actions with negative value
    u_int   size;
    int     fd;     // MAX_FILES+1 for all the files (default)
} filexor_t;
#define RESET_FILEXOR   {0, NULL, NULL, 0, MAX_FILES + 1}
filexor_t   g_filexor_reset = RESET_FILEXOR;    // no easy way to reset a struct after declaration
filexor_t   g_filexor       = RESET_FILEXOR;
filexor_t   g_filerot       = RESET_FILEXOR;
filexor_t   g_filecrypt     = RESET_FILEXOR;



// experimental input and output
int     enable_sockets              = 0,
        enable_process              = 0,
        enable_audio                = 0,
        enable_video                = 0,
        enable_winmsg               = 0,
        enable_calldll              = 0,
        enable_execute_pipe         = 0;

// alternative -r -r mode, they are just variable indexes
int     g_reimport2_offset          = -1,
        g_reimport2_zsize           = -1,
        g_reimport2_size            = -1,
        g_reimport2_xsize           = -1;



#ifdef WIN32
    OSVERSIONINFO   g_osver         = {0};
#endif



//#pragma pack()

