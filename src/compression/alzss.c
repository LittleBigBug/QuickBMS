// by Elijah H. Brolijah
// https://github.com/Brolijah/Aqualead_LZSS

/*
## About
This tool can be used to decompress an Aqualead LZSS file. These files are recognizable by their header magic "ALLZ". Aqualead's ALLZ compression is described in their documentation as a modified version of LZSS with some of their own tweaks and optimizations.

For the record, this is just one format out of 15-ish provided by the Aqualead framework. I am only aware of small handful of games to make use of these formats, though I'm sure there are others. To name a few, Groove Coaster for Steam, Pandora's Tower (Wii), and Zanki Zero: Last Beginning.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
//#include "types.h"
#include "aqualead_types.h"

#include <stdint.h>
typedef uint8_t    u8;
typedef uint32_t  u32;
typedef int32_t   s32;

size_t ALLZ_Decode(u8** ptr_dst, u8* src, size_t srcSize);
s32 ALLZ_Setup_EncFlags(s32* encFlags, u8 alFlag, u8** ptr_encoded_data);
s32 ALLZ_AnalyzeBlock(s32* encFlags, u8 alFlag, u8** ptr_encoded_data);

/**
 * @brief ALLZ_Decode
 *      Decompresses the packed file data of an Aqualead LZSS file and writes
 *      to the supplied destination.
 * @param ptr_dst
 *      Pointer-to-pointer to your destination buffer. This is a double pointer
 *      so that the caller does not need to allocate it before calling.
 * @param src
 *      Pointer to the Aqualead compressed file data.
 * @param srcSize
 *      Size of the Aqualead compressed file in bytes.
 * @return
 *      On SUCCESS, length of decompressed data.
 *      On ERROR, returns zero. An error indicates either
 *          (1) Analysis of the compressed file passed EOF
 *          (2) Did not finish decoding the entire file.
 */
size_t ALLZ_Decode(u8** ptr_dst, u8* src, size_t srcSize)
{
    // aqualead file metadata
    u8  alarFlag1;
    u8  alarFlag2;
    u8  allzFlag1;
    u8  allzFlag2;
    u32 fullSize;
    // local variables
    u8* dst;                /// pointer to destination buffer
    u8* encoded_src;        /// used in memory to recall data position
    u8* encoded_eof;        /// used to break out of function
    u8* decoded_eof;        /// used to break out of function
    s32 lzssFlags[2];       /// Some 8-byte variable (probably supposed to be a struct of two integers)
    s32 disp_offset;        /// offset of displaced bytes needed duplicate
    s32 disp_length;        /// length of displacement to duplicate
    u8* duplicates = NULL;  /// pointer to displaced bytes
    // These three are lacking proper names
    s32 local_var1 = 0;     /// Used during ALLZ FLAG 2 decode. Used to copy unencoded bytes.
    s32 local_var3 = 0;     /// Used during ALLZ FLAG 1 decode. Gets assigned to disp_offset.
    s32 local_var5 = 0;     /// Used during ALAR FLAG 2 decode. Gets assigned to disp_length.
    u8  tempAlFlag;     /// Temporary Aqualead Flag for decoding.
    s32 tempEncFlag;    /// Temporary copy of lzssFlags[0] used to escape loops.
    u8* src_temp;       /// Temporary copy of encoded_src used to copy duplicate unencoded bytes.

    // initialize default values and read ALLZ header
    alarFlag1   = *( u8*)(src + 0x04);
    alarFlag2   = *( u8*)(src + 0x05);
    allzFlag1   = *( u8*)(src + 0x06);
    allzFlag2   = *( u8*)(src + 0x07);
    fullSize    = *(u32*)(src + 0x08);
    encoded_src =  ( u8*)(src + 0x0C); // src + 12
    lzssFlags[0] = 0;
    lzssFlags[1] = 0;

    if(!(*ptr_dst)) // stupid-proofing in case the caller didn't malloc a buffer
    {
        *ptr_dst = malloc(fullSize);
        memset(*ptr_dst, 0, fullSize);
    }
    dst = *ptr_dst;
    decoded_eof = dst + fullSize;
    encoded_eof = src + srcSize;

    /* printf("Aqualead LZSS File Metadata:\n"
           "  ALAR Flag1 = %d\n"
           "  ALAR Flag2 = %d\n"
           "  ALLZ Flag1 = %d\n"
           "  ALLZ Flag2 = %d\n"
           "  Full size  = %u\n\n",
           alarFlag1, alarFlag2, allzFlag1, allzFlag2, fullSize); */

    if( !alarFlag1 )
    {
        ALLZ_AnalyzeBlock((s32*)lzssFlags, 1, &encoded_src);
    }

    local_var1 = 1 + ALLZ_Setup_EncFlags((s32*)lzssFlags, allzFlag2, &encoded_src);
    do {
        *dst++ = *encoded_src++;
    } while(--local_var1 != 0);

    local_var3  = ALLZ_Setup_EncFlags((s32*)lzssFlags, allzFlag1, &encoded_src);
    disp_offset = local_var3 + 1;
    local_var5  = ALLZ_Setup_EncFlags((s32*)lzssFlags, alarFlag2, &encoded_src);
    disp_length = local_var5 + 3;

    if(encoded_src <= encoded_eof)
    {
        while((dst + disp_length) < decoded_eof)
        {
            while(lzssFlags[1] == 0)
            {
                lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                lzssFlags[1] += 8;
            }

            tempAlFlag = 0;
            tempEncFlag = lzssFlags[0];
            lzssFlags[0] >>= 1;
            lzssFlags[1] -= 1;

            if( !(tempEncFlag & 0x01) )
            {
    // DECODE WITH ALLZ FLAG 2
                tempAlFlag = allzFlag2;
                while(1)
                {
                    while(lzssFlags[1] == 0)
                    {
                        lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                        lzssFlags[1] += 8;
                    }
                    tempEncFlag = lzssFlags[0];
                    lzssFlags[0] >>= 1;
                    lzssFlags[1] -= 1;
                    if ( !(tempEncFlag & 0x01) ) break;
                    ++tempAlFlag;
                }
                local_var1 = ALLZ_AnalyzeBlock((s32*)lzssFlags, tempAlFlag, &encoded_src);
                if(tempAlFlag > allzFlag2)
                {
                    local_var1 += ((1 << (tempAlFlag - allzFlag2)) - 1) << allzFlag2;
                }
                ++local_var1;
                if((dst + disp_length + local_var1) >= decoded_eof) // finish copying bytes and return
                {
                    if(disp_length)
                    {
                        duplicates = dst - disp_offset;
                        do {
                            *dst++ = *duplicates++;
                        } while(--disp_length != 0);
                    }

                    if(local_var1)
                    {
                        do {
                            *dst++ = *encoded_src++;
                        } while(--local_var1 != 0);
                    }

                    break; // while((dst + disp_length) < decoded_eof)
                }

    // DECODE WITH ALLZ FLAG 1
                src_temp = encoded_src;
                tempAlFlag = allzFlag1;
                encoded_src += local_var1;
                while(1)
                {
                    while(lzssFlags[1] == 0)
                    {
                        lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                        lzssFlags[1] += 8;
                    }
                    tempEncFlag = lzssFlags[0];
                    lzssFlags[0] >>= 1;
                    lzssFlags[1] -= 1;
                    if ( !(tempEncFlag & 0x01) ) break;
                    ++tempAlFlag;
                }
                local_var3 = ALLZ_AnalyzeBlock((s32*)lzssFlags, tempAlFlag, &encoded_src);
                if(tempAlFlag > allzFlag1)
                {
                    local_var3 += (((1 << (tempAlFlag - allzFlag1)) - 1) << allzFlag1);
                }
    // DECODE WITH ALAR FLAG 2
                tempAlFlag = alarFlag2;
                while(1)
                {
                    while(lzssFlags[1] == 0)
                    {
                        lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                        lzssFlags[1] += 8;
                    }
                    tempEncFlag = lzssFlags[0];
                    lzssFlags[0] >>= 1;
                    lzssFlags[1] -= 1;
                    if ( !(tempEncFlag & 0x01) ) break;
                    ++tempAlFlag;
                }
                local_var5 = ALLZ_AnalyzeBlock((s32*)lzssFlags, tempAlFlag, &encoded_src);
                if(tempAlFlag > alarFlag2)
                {
                    local_var5 += (((1 << (tempAlFlag - alarFlag2)) - 1) << alarFlag2);
                }
    // COPY OVER DECODED BYTES
                if(disp_length)
                {
                    duplicates = dst - disp_offset;
                    do {
                        *dst++ = *duplicates++;
                    } while(--disp_length != 0);
                }

                if(local_var1)
                {
                    do {
                        *dst++ = *src_temp++;
                    } while(--local_var1 != 0);
                }

            } else {
    // DECODE WITH ALLZ FLAG 1
                tempAlFlag = allzFlag1;
                while(1)
                {
                    while(lzssFlags[1] == 0)
                    {
                        lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                        lzssFlags[1] += 8;
                    }
                    tempEncFlag = lzssFlags[0];
                    lzssFlags[0] >>= 1;
                    lzssFlags[1] -= 1;
                    if ( !(tempEncFlag & 0x01) ) break;
                    ++tempAlFlag;
                }
                local_var3 = ALLZ_AnalyzeBlock((s32*)lzssFlags, tempAlFlag, &encoded_src);
                if(tempAlFlag > allzFlag1)
                {
                    local_var3 += (((1 << (tempAlFlag - allzFlag1)) - 1) << allzFlag1);
                }
    // DECODE WITH ALAR FLAG 2
                tempAlFlag = alarFlag2;
                while(1)
                {
                    while(lzssFlags[1] == 0)
                    {
                        lzssFlags[0] |= *encoded_src++ << lzssFlags[1];
                        lzssFlags[1] += 8;
                    }
                    tempEncFlag = lzssFlags[0];
                    lzssFlags[0] >>= 1;
                    lzssFlags[1] -= 1;
                    if( !(tempEncFlag & 0x01) ) break;
                    ++tempAlFlag;
                }
                local_var5 = ALLZ_AnalyzeBlock((s32*)lzssFlags, tempAlFlag, &encoded_src);
                if(tempAlFlag > alarFlag2)
                {
                    local_var5 += (((1 << (tempAlFlag - alarFlag2)) - 1) << alarFlag2);
                }
    // COPY OVER DECODED BYTES
                if(disp_length)
                {
                    duplicates = dst - disp_offset;
                    do {
                        *dst++ = *duplicates++;
                    } while(--disp_length != 0);
                }
            }

            disp_offset = local_var3 + 1;
            disp_length = local_var5 + 3;
            if(encoded_src > encoded_eof)
            {
                //printf("ERROR: decode():%d : encoded_src passed encoded_eof\n", __LINE__);
                return 0;
            }
        }

        if(disp_length)
        {
            duplicates = dst - disp_offset;
            do {
                *dst++ = *duplicates++;
            } while (--disp_length != 0);
        }
    }

    /* printf("DEBUG:\n"
           "  decoded_dst = 0x%08X\n"
           "  decoded_eof = 0x%08X\n\n", dst, decoded_eof); */

    return (dst == decoded_eof) ? fullSize : 0;
}

s32 ALLZ_AnalyzeBlock(s32* encFlags, u8 alFlag, u8** ptr_encoded_data)
{
    u32 tempFlag;
    while(encFlags[1] < alFlag)
    {
        encFlags[0] |= *(*ptr_encoded_data)++ << encFlags[1];
        encFlags[1] += 8;
    }

    tempFlag = encFlags[0];
    encFlags[0] >>= alFlag;
    encFlags[1] -= alFlag;
    return (tempFlag & ((1 << alFlag) - 1));
}

s32 ALLZ_Setup_EncFlags(s32* encFlags, u8 alFlag, u8** ptr_encoded_data)
{
    u32 tempAlFlag = alFlag;
    u32 tempEncFlag;
    s32 result;
    while(1)
    {
        while(encFlags[1] == 0)
        {
            encFlags[0] |= *(*ptr_encoded_data)++ << encFlags[1];
            encFlags[1] += 8;
        }
        tempEncFlag = encFlags[0];
        encFlags[0] >>= 1;
        encFlags[1] -= 1;

        if( !(tempEncFlag & 0x01) ) break;
        ++tempAlFlag;
    }

    result = ALLZ_AnalyzeBlock(encFlags, tempAlFlag, ptr_encoded_data);
    if( tempAlFlag > alFlag )
        result += ((1 << (tempAlFlag - alFlag)) - 1) << alFlag;
    return result;
}
