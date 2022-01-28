#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/lzham_codec/include/lzham.h"



#define lzham_set_settings(X) \
    int     m_dict_size_log2 = LZHAM_MAX_DICT_SIZE_LOG2_X86, \
            m_table_update_rate = 0, \
            m_##X##compress_flags = 0, \
            m_table_max_update_interval = 0, \
            m_table_update_interval_slow_rate = 0; \
            \
    if(parameters) { \
        get_parameter_numbers(parameters, \
            &m_dict_size_log2, &m_table_update_rate, &m_##X##compress_flags, &m_table_max_update_interval, &m_table_update_interval_slow_rate, NULL); \
    }



int lzham_unpack(unsigned char *in, int insz, unsigned char *out, int outsz, u8 *parameters) {
    lzham_decompress_params par;
    lzham_z_ulong   tmp;
    int     i       = 0,
            j       = 0,
            z       = 0,
            do_scan = 1;

    lzham_set_settings(de)

    if(parameters) {
        do_scan = 0;
        goto doit;
    }

    for(i = 0;; i++) {
        // do not use LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED
             if(i == 0) m_decompress_flags = 0;
        else if(i == 1) m_decompress_flags = LZHAM_DECOMP_FLAG_READ_ZLIB_STREAM;
        else break;

        for(j = LZHAM_MAX_DICT_SIZE_LOG2_X86; j >= LZHAM_MIN_DICT_SIZE_LOG2; j--) {     // 8 wrong files in content.tim of abbeygames 
        //for(j = LZHAM_MIN_DICT_SIZE_LOG2; j <= LZHAM_MAX_DICT_SIZE_LOG2_X86; j++) {     // 5 wrong files in content.tim of abbeygames
            m_dict_size_log2 = j;

            for(z = 0;; z++) {
                     if(z == 0) m_table_update_rate = LZHAM_DEFAULT_TABLE_UPDATE_RATE;  // 0 is just the same
                else if(z == 1) m_table_update_rate = LZHAM_FASTEST_TABLE_UPDATE_RATE;
                else if(z == 2) m_table_update_rate = LZHAM_SLOWEST_TABLE_UPDATE_RATE;
                else break;

doit:
                memset(&par, 0, sizeof(par));
                par.m_struct_size = sizeof(par);
                par.m_dict_size_log2    = m_dict_size_log2;
                par.m_decompress_flags  = m_decompress_flags;
                par.m_table_update_rate = m_table_update_rate;
                par.m_table_max_update_interval = m_table_max_update_interval;
                par.m_table_update_interval_slow_rate = m_table_update_interval_slow_rate;

                size_t          otmp = outsz;
                lzham_uint32    crc  = 0;
                if(lzham_decompress_memory(&par, out, &otmp, in, insz, &crc)
                    == LZHAM_DECOMP_STATUS_SUCCESS) return otmp;

                if(!do_scan) goto quit;
            }
        }
    }

quit:
    // last chance
    tmp = outsz;
    if(lzham_z_uncompress(out, &tmp, in, insz)
        == LZHAM_Z_OK) return tmp;
    return -1;
}



int lzham_pack(unsigned char *in, int insz, unsigned char *out, int outsz, u8 *parameters) {
    lzham_compress_params par;

    lzham_set_settings()

    memset(&par, 0, sizeof(par));
    par.m_struct_size = sizeof(par);
    par.m_level = LZHAM_COMP_LEVEL_UBER;
    par.m_max_helper_threads = -1;

    par.m_dict_size_log2 = m_dict_size_log2,
    par.m_table_update_rate = m_table_update_rate;
    par.m_compress_flags = m_compress_flags;
    par.m_table_max_update_interval = m_table_max_update_interval;
    par.m_table_update_interval_slow_rate = m_table_update_interval_slow_rate;

    size_t          otmp = outsz;
    lzham_uint32    crc  = 0;
    if(lzham_compress_memory(&par, out, &otmp, in, insz, &crc)
        == LZHAM_COMP_STATUS_SUCCESS) return otmp;

    // last chance
    lzham_z_ulong   tmp = outsz;
    if(lzham_z_compress2(out, &tmp, in, insz, LZHAM_Z_UBER_COMPRESSION)
        == LZHAM_Z_OK) return tmp;
    return -1;
}


