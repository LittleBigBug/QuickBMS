/*---------------------------------------------------------------------------*
  Project:  NitroSDK - tools - nitroCompLib
  File:     nitroCompLib.c

  Copyright 2003-2006 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: nitroCompLib.c,v $
  Revision 1.14  01/18/2006 02:11:30  kitase_hirotake
  do-indent

  Revision 1.13  01/05/2006 03:43:40  takano_makoto
  Fixed the bug when executing 3 byte compression on end data.

  Revision 1.12  11/29/2005 05:23:48  takano_makoto
  Increased speed of LZ compression

  Revision 1.11  06/29/2005 04:25:17  takano_makoto
  Fixed the problem for when there is only one type of value for Huffman compression.

  Revision 1.10  03/03/2005 10:20:07  takano_makoto
  Revised to 4-byte align the end of output

  Revision 1.9  03/02/2005 07:57:15  takano_makoto
  Revised to skip unnecessary searches

  Revision 1.8  02/28/2005 05:26:27  yosizaki
  do-indent.

  Revision 1.7  10/08/2004 04:31:52  takano_makoto
  Fixed bug in debug extraction routine.

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include "nitroCompLib.h"

#undef _DEBUG
#ifdef _DEBUG
#define new DEBUG_NEW
#define DEBUG_PRINT
// #define DEBUG_PRINT_DIFFFILT
#define DEBUG_PRINT_RL
// #define DEBUG_PRINT_HUFF
// #define DEBUG_PRINT_LZ
// #define DEBUG_PRINT_DATAMATCH
#endif


#ifdef __cplusplus
#define EXTERN extern "C"
#define STATIC
#else
#define EXTERN
#define STATIC static
#endif


#ifdef DEBUG_PRINT
#define dbg_printf fprintf
#else
#define dbg_printf dummy
#endif

#ifdef DEBUG_PRINT_DIFFFILT
#define dbg_printf_dif fprintf
#else
#define dbg_printf_dif dummy
#endif

#ifdef DEBUG_PRINT_RL
#define dbg_printf_rl fprintf
#else
#define dbg_printf_rl dummy
#endif

#ifdef DEBGU_PRINT_HUFF
#define dbg_printf_huff fprintf
#else
#define dbg_printf_huff dummy
#endif

#ifdef DEBGU_PRINT_LZ
#define dbg_printf_lz fprintf
#else
#define dbg_printf_lz dummy
#endif

#ifdef DEBUG_PRINT_DATAMATCH
#define dbg_printf_match fprintf
#else
#define dbg_printf_match dummy
#endif

void dummy(void *fp, ...)
{
}

#define DIFF_CODE_HEADER        (0x80)
#define RL_CODE_HEADER          (0x30)
#define LZ_CODE_HEADER          (0x10)
#define HUFF_CODE_HEADER        (0x20)
#define CODE_HEADER_MASK        (0xF0)



//==================================================================================
// Global variable declarations
//==================================================================================
static u8 *pCompBuf[2];                // Double buffer to use during compression process
static u8 compBufNo = 1;               // Indicates a valid double buffer
static char *pCompList;                // Current reference point in compList

//==================================================================================
// Prototype Declaration
//==================================================================================
STATIC u32 RawWrite(u8 *srcp, u32 size, u8 *dstp);
STATIC u32 DiffFiltWrite(u8 *srcp, u32 size, u8 *dstp, u8 diffBitSize);
STATIC u32 RLCompWrite(u8 *srcp, u32 size, u8 *dstp);
STATIC u32 LZCompWrite(u8 *srcp, u32 size, u8 *dstp, u8 lzSearchOffset);
STATIC u32 HuffCompWrite(u8 *srcp, u32 size, u8 *dstp, u8 huffBitSize);

STATIC s32 RawRead(u8 *srcp, u32 size, u8 *dstp);
EXTERN s32 DiffFiltRead(u8 *srcp, u32 size, u8 *dstp, u8 diffBitSize);
EXTERN s32 RLCompRead(u8 *srcp, u32 size, u8 *dstp);
EXTERN s32 LZCompRead(u8 *srcp, u32 size, u8 *dstp);
EXTERN s32 HuffCompRead(u8 *srcp, u32 size, u8 *dstp, u8 huffBitSize);

/*
//==================================================================================
// Functions for DLL
//==================================================================================
EXTERN BOOL WINAPI DllMain( HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}
*/

//----------------------------------------------------------------------------------
//  Secure memory region to place compressed data
//    Secure a region that is twice the size the data before it is compressed.
//----------------------------------------------------------------------------------
EXTERN u8 *__stdcall nitroCompMalloc(u32 size)
{
    return (u8 *)malloc(size * 3 + 512);
}

//----------------------------------------------------------------------------------
//  Free the memory region where the data was held after it was compressed.
//----------------------------------------------------------------------------------
EXTERN void __stdcall nitroCompFree(u8 *p)
{
    if (p != NULL)
    {
        free((void *)p);
        p = NULL;
    }
}

//------------------------------------------------------------
//  Data compression
//------------------------------------------------------------
EXTERN u32 __stdcall nitroCompress(u8 *srcp, u32 size, u8 *dstp, char *compList, u8 rawHeaderFlag)
{
    u32     dataSize, nextDataSize;    // Compression data size (bytes)
    u8     *pReadBuf;                  // Pointer to the beginning address of compressed data.
    u8      bitSize;                   // Unit to apply difference filter, Huffman encoding.
    char    str[16];
    u16     i, j;
    u8      lzSearchOffset;

//  pCompBuf[0] = (u8 *)malloc(size*2 + 4 + 256*2);     // Worst case is size*2 + 4 + 256*2
//  pCompBuf[1] = (u8 *)malloc(size*2 + 4 + 256*2);     // additional compression and data headers could make this insufficient.
    pCompBuf[0] = (u8 *)malloc(size * 3 + 256 * 2);
    pCompBuf[1] = (u8 *)malloc(size * 3 + 256 * 2);
    pReadBuf = pCompBuf[0];
    compBufNo = 1;                     // Important!!  If you do not do this, you will have problems the second time you call nitroCompress.

    // malloc check
    if (pCompBuf[0] == NULL || pCompBuf[1] == NULL)
    {
        fprintf(stderr, "Error: Memory is not enough.\n");
        exit(1);
    }

    dataSize = size;

    // Process to add a NULL header (pseudo header for data before it is compressed)
    if (rawHeaderFlag)
    {
        dataSize += 4;
        *(u32 *)pReadBuf = size << 8 | 0;       // data header
        memcpy(&pReadBuf[4], srcp, size);
    }
    else
    {
        memcpy(pReadBuf, srcp, size);
    }

    pCompList = compList;              // Points to the array that stores the compression order.

    // Loop while there are elements in the array that stores the compression order.
    while (1)
    {
        switch (*pCompList)
        {
        case 'd':
            {
                pCompList++;           // 8 or 16 comes after 'd'
                str[0] = *pCompList;
                if (*pCompList == '1')
                {
                    pCompList++;
                    str[1] = *pCompList;
                    str[2] = '\n';
                }
                bitSize = atoi(str);   // Stores in units that are applicable to a difference filter.
                str[0] = str[1] = '\n';

                dbg_printf(stderr, "nitroCompress  Diff %d\n", bitSize);

                if ((bitSize == 16) && (dataSize & 0x01))
                {
                    fprintf(stderr, "16-bit differencial filter must be 2-byte allignment.\n");
                    exit(1);
                }
                nextDataSize = DiffFiltWrite(pReadBuf, dataSize, pCompBuf[compBufNo], bitSize);
            }
            break;

        case 'r':
            {
                dbg_printf(stderr, "nitroCompress  RL\n");

                nextDataSize = RLCompWrite(pReadBuf, dataSize, pCompBuf[compBufNo]);
            }
            break;

        case 'l':
            {
                pCompList++;
                i = 0;
                while (isdigit(*pCompList))
                {
                    str[i] = *pCompList;
                    pCompList++;
                    i++;
                    if (i == 15)
                    {
                        break;
                    }
                }
                str[i] = '\n';
                pCompList--;
                lzSearchOffset = (u8)atoi(str); // Truncate to round large values.
                for (j = 0; j < i; j++)
                {
                    str[j] = '\n';
                }
                dbg_printf(stderr, "nitroCompress  L %d\n", lzSearchOffset);

                nextDataSize = LZCompWrite(pReadBuf, dataSize, pCompBuf[compBufNo], lzSearchOffset);
            }
            break;

        case 'h':
            {
                pCompList++;           // 4 or 8 comes after 'h'
                str[0] = *pCompList;
                str[1] = '\n';
                bitSize = atoi(str);   // 4 or 8
                str[0] = '\n';

                dbg_printf(stderr, "nitroCompress  Huff %d\n", bitSize);

                nextDataSize = HuffCompWrite(pReadBuf, dataSize, pCompBuf[compBufNo], bitSize);
            }
            break;
            //-----------------------------------------
            // Compression end (*CompTypeBufp is NULL)
        default:
            {
                dbg_printf(stderr, "nitroCompress  raw\n");

                RawWrite(pReadBuf, dataSize, dstp);
                if (pCompBuf[0] != NULL)
                {
                    free((void *)pCompBuf[0]);
                    pCompBuf[0] = NULL;
                }
                if (pCompBuf[1] != NULL)
                {
                    free(pCompBuf[1]);
                    pCompBuf[1] = NULL;
                }
                return dataSize;

            }
        }
        // One more loop
        pReadBuf = pCompBuf[compBufNo];
        compBufNo ^= 0x01;
        dataSize = nextDataSize;
        pCompList++;
    }
}


//===========================================================================
//  Copy compression data
//===========================================================================
STATIC u32 RawWrite(u8 *srcp, u32 size, u8 *dstp)
{
    u32     i;

    dbg_printf(stderr, "RawWrite\tsize=%d\n\n", size);

    size = (size + 0x3) & ~0x3;
    for (i = 0; i < size - 1; i++)
    {
        *dstp = *srcp;
        dstp++;
        srcp++;
    }
    *dstp = *srcp;

    return size;
}


//===========================================================================
//  Difference filter
//===========================================================================
STATIC u32 DiffFiltWrite(u8 *srcp, u32 size, u8 *dstp, u8 diffBitSize)
{
    u32     DiffCount;                 // Number of bytes of compressed data
    u32     i;

    u16    *src16p = (u16 *)srcp;
    u16    *dst16p = (u16 *)dstp;

    dbg_printf_dif(stderr, "DiffFiltWrite\tsize=%d\tdiffBitSize=%d\n", size, diffBitSize);

    *(u32 *)dstp = size << 8 | (DIFF_CODE_HEADER | diffBitSize / 8);    // data header

    DiffCount = 4;

    if (diffBitSize == 8)
    {
#ifdef DEBUG_PRINT_DIFFFILT
        for (i = 0; i < 16; i++)
        {
            dbg_printf_dif(stderr, "srcp[%d] = %x\n", i, srcp[i]);
        }
#endif
        dstp[DiffCount] = srcp[0];     // No difference for beginning data only
        DiffCount++;
        for (i = 1; i < size; i++, DiffCount++)
        {
            dbg_printf_dif(stderr, "dstp[%x] = srcp[%d]-srcp[%d] = %x - %x = %x\n",
                           DiffCount, i, i - 1, srcp[i], srcp[i - 1], srcp[i] - srcp[i - 1]);

            dstp[DiffCount] = srcp[i] - srcp[i - 1];    // Store difference data
        }
    }
    else                               // 16 bit size
    {
        dst16p[DiffCount / 2] = src16p[0];
        DiffCount += 2;
        for (i = 1; i < size / 2; i++, DiffCount += 2)
        {
            dst16p[DiffCount / 2] = src16p[i] - src16p[i - 1];
        }
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((DiffCount + i) & 0x3)
    {
        dstp[DiffCount + i] = 0;
        i++;
    }

    return DiffCount;
}


//===========================================================================
//  Run length encoding (bytes)
//===========================================================================
STATIC u32 RLCompWrite(u8 *srcp, u32 size, u8 *dstp)
{
    u32     RLDstCount;                // Number of bytes of compressed data
    u32     RLSrcCount;                // Processed data volume of the compression target data (in bytes)
    u8      RLCompFlag;                // 1 if performing run length encoding
    u8      runLength;                 // Run length
    u8      rawDataLength;             // Length of data not run
    u32     i;

    u8     *startp;                    // Point to the start of compression target data for each process loop

    dbg_printf_rl(stderr, "RLCompWrite\tsize=%d\n", size);

    //  Data header (The size after decompression)
    *(u32 *)dstp = size << 8 | RL_CODE_HEADER;  // data header
    RLDstCount = 4;

    RLSrcCount = 0;
    rawDataLength = 0;
    RLCompFlag = 0;

    while (RLSrcCount < size)
    {
        startp = &srcp[RLSrcCount];    // Set compression target data

        for (i = 0; i < 128; i++)      // Data volume that can be expressed in 7 bits is 0 to 127
        {
            // Reach the end of the compression target data
            if (RLSrcCount + rawDataLength >= size)
            {
                rawDataLength = (u8)(size - RLSrcCount);
                break;
            }

            if (RLSrcCount + rawDataLength + 2 < size)
            {
                if (startp[i] == startp[i + 1] && startp[i] == startp[i + 2])
                {
                    RLCompFlag = 1;
                    break;
                }
            }
            rawDataLength++;
        }

        // Store data that is not encoded
        // If the 8th bit of the data length storage byte is 0, the data series that is not encoded.
        // The data length is x - 1, so 0-127 becomes 1-128.
        if (rawDataLength)
        {
            dstp[RLDstCount++] = rawDataLength - 1;     // Store "data length - 1" (7 bits)
            for (i = 0; i < rawDataLength; i++)
            {
                dstp[RLDstCount++] = srcp[RLSrcCount++];
            }
            rawDataLength = 0;
        }

        // Run Length Encoding
        if (RLCompFlag)
        {
            runLength = 3;
            for (i = 3; i < 128 + 2; i++)
            {
                // Reach the end of the data for compression
                if (RLSrcCount + runLength >= size)
                {
                    runLength = (u8)(size - RLSrcCount);
                    break;
                }

                // If run is interrupted
                if (srcp[RLSrcCount] != srcp[RLSrcCount + runLength])
                {
                    break;
                }
                // Run continues
                runLength++;
            }

            // If the 8th bit of the data length storage byte is 1, the data series that is encoded.
            dstp[RLDstCount++] = 0x80 | (runLength - 3);        // Add 3, and store 3 to 130
            dstp[RLDstCount++] = srcp[RLSrcCount];
            RLSrcCount += runLength;
            RLCompFlag = 0;
        }
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((RLDstCount + i) & 0x3)
    {
        dstp[RLDstCount + i] = 0;
        i++;
    }

    return RLDstCount;
}

//===========================================================================
//  LZ77 compression
//===========================================================================
STATIC u8 SearchLZ(const u8 *nextp, u32 remainSize, u16 *offset);

static u16 windowPos;
static u16 windowLen;

static s16 LZOffsetTable[4096];
static s16 LZByteTable[256];
static s16 LZEndTable[256];


STATIC void LZInitTable(void)
{
    u16     i;

    for (i = 0; i < 256; i++)
    {
        LZByteTable[i] = -1;
        LZEndTable[i] = -1;
    }
    windowPos = 0;
    windowLen = 0;
}

STATIC void SlideByte(const u8 *srcp)
{
    s16     offset;
    u8      in_data = *srcp;
    u16     insert_offset;

    if (windowLen == 4096)
    {
        u8      out_data = *(srcp - 4096);
        if ((LZByteTable[out_data] = LZOffsetTable[LZByteTable[out_data]]) == -1)
        {
            LZEndTable[out_data] = -1;
        }
        insert_offset = windowPos;
    }
    else
    {
        insert_offset = windowLen;
    }

    offset = LZEndTable[in_data];
    if (offset == -1)
    {
        LZByteTable[in_data] = insert_offset;
    }
    else
    {
        LZOffsetTable[offset] = insert_offset;
    }
    LZEndTable[in_data] = insert_offset;
    LZOffsetTable[insert_offset] = -1;

    if (windowLen == 4096)
    {
        windowPos = (u16)((windowPos + 1) % 0x1000);
    }
    else
    {
        windowLen++;
    }
}

STATIC void LZSlide(const u8 *srcp, u32 n)
{
    u32     i;

    for (i = 0; i < n; i++)
    {
        SlideByte(srcp++);
    }
}


/*---------------------------------------------------------------------------*
  Name:         MI_CompressLZ

  Description:  Function that performs LZ77 compression 

  Arguments:    srcp            Pointer to compression source data
                size            Size of compression source data
                dstp            Pointer to destination for compressed data
                                Buffer must be larger than size of compression source data.

  Returns:      The data size after compression.
                If compressed data is larger than original data, compression is terminated and 0 gets returned.
 *---------------------------------------------------------------------------*/
STATIC u32 LZCompWrite(u8 *srcp, u32 size, u8 *dstp, u8 lzSearchOffset)
{
    u32     LZDstCount;                // Number of bytes of compressed data
    u8      LZCompFlags;               // Flag series indicating whether there is a compression
    u8     *LZCompFlagsp;              // Point to memory regions storing LZCompFlags
    u16     lastOffset;                // Offset to matching data (the longest matching data at the time)
    u8      lastLength;                // Length of matching data (the longest matching data at the time)
    u8      i;
    u32     dstMax=0;

    *(u32 *)dstp = size << 8 | LZ_CODE_HEADER;  // data header
    dstp += 4;
    LZDstCount = 4;
    dstMax = size;
    LZInitTable();

    while (size > 0)
    {
        LZCompFlags = 0;
        LZCompFlagsp = dstp++;         // Designation for storing flag series
        LZDstCount++;

        // Since flag series is stored as 8-bit data, loop eight times
        for (i = 0; i < 8; i++)
        {
            LZCompFlags <<= 1;         // No meaning for the first time (i=0)
            if (size <= 0)
            {
                // When the end is reached, quit after shifting flag to the end. 
                continue;
            }

            if ((lastLength = SearchLZ(srcp, size, &lastOffset)))
            {
                // Enabled Flag if compression is possible 
                LZCompFlags |= 0x1;

                // Divide offset into upper 4 bits and lower 8 bits and store
                *dstp++ = (u8)((lastLength - 3) << 4 | (lastOffset - 1) >> 8);
                *dstp++ = (u8)((lastOffset - 1) & 0xff);
                LZDstCount += 2;
                LZSlide(srcp, lastLength);
                srcp += lastLength;
                size -= lastLength;
            }
            else
            {
                // No compression
                LZSlide(srcp, 1);
                *dstp++ = *srcp++;
                size--;
                LZDstCount++;
            }
        }                              // Complete eight loops
        *LZCompFlagsp = LZCompFlags;   // Store flag series
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((LZDstCount + i) & 0x3)
    {
        *dstp++ = 0;
        i++;
    }

    return LZDstCount;
}


//--------------------------------------------------------
// Searches for the longest matching string from the slide window with LZ77 Compression.
//  Arguments:    startp              Pointer to starting position of data
//                nextp               Pointer to data where search will start
//                remainSize          Size of remaining data
//                offset              Pointer to region storing matched offset
//  Return   :    TRUE if matching string is found.
//                FALSE if not found.
//--------------------------------------------------------
STATIC u8 SearchLZ(const u8 *nextp, u32 remainSize, u16 *offset)
{
    const u8 *searchp;
    const u8 *headp, *searchHeadp;
    u16     maxOffset;
    u8      maxLength = 2;
    u8      tmpLength;
    s32     w_offset;

    if (remainSize < 3)
    {
        return 0;
    }

    w_offset = LZByteTable[*nextp];

    while (w_offset != -1)
    {
        if (w_offset < windowPos)
        {
            searchp = nextp - windowPos + w_offset;
        }
        else
        {
            searchp = nextp - windowLen - windowPos + w_offset;
        }

        /* This isn't needed, but it does make it a little faster.*/
        if (*(searchp + 1) != *(nextp + 1) || *(searchp + 2) != *(nextp + 2))
        {
            w_offset = LZOffsetTable[w_offset];
            continue;
        }

        if (nextp - searchp < 2)
        {
            // VRAM is accessed in units of 2 bytes (since sometimes data is read from VRAM),
            // so the search must start 2 bytes prior to the search target.
            // 
            // Since the offset is stored in 12 bits, the value is 4096 or less
            break;
        }
        tmpLength = 3;
        searchHeadp = searchp + 3;
        headp = nextp + 3;

        while (((u32)(headp - nextp) < remainSize) && (*headp == *searchHeadp))
        {
            headp++;
            searchHeadp++;
            tmpLength++;

            // Since the data length is stored in 4 bits, the value is 18 or less (3 is added)
            if (tmpLength == (0xF + 3))
            {
                break;
            }
        }
        if (tmpLength > maxLength)
        {
            // Update the maximum-length offset
            maxLength = tmpLength;
            maxOffset = (u16)(nextp - searchp);
            if (maxLength == (0xF + 3))
            {
                // This is the largest matching length, so end search.
                break;
            }
        }
        w_offset = LZOffsetTable[w_offset];
    }

    if (maxLength < 3)
    {
        return 0;
    }
    *offset = maxOffset;
    return maxLength;
}


//===========================================================================
//  Huffman encoding
//===========================================================================
#define HUFF_END_L  0x80
#define HUFF_END_R  0x40

STATIC void HuffUpdateParentDepth(u16 leftNo, u16 rightNo);
STATIC void HuffMakeCode(u16 nodeNo, u32 paHuffCode);
STATIC u8 HuffCountHWord(u16 nodeNo);
STATIC void HuffMakeHuffTree(u16 rootNo);
STATIC void HuffMakeSubsetHuffTree(u16 huffTreeNo, u8 rightNodeFlag);
STATIC u8 HuffRemainingNodeCanSetOffset(u8 costHWord);
STATIC void HuffSetOneNodeOffset(u16 huffTreeNo, u8 rightNodeFlag);

typedef struct
{
    u16     No;                        // Data No.
    u32     Freq;                      // Frequency of occurrence
    s16     PaNo;                      // Parent No.
    s16     ChNo[2];                   // Child No (0: left side, 1: right side)
    u16     PaDepth;                   // Parent node depth
    u16     LeafDepth;                 // Depth to leaf
    u32     HuffCode;                  // Huffman code
    u8      Bit;                       // Node's bit data
    u16     HWord;                     // For each intermediate node, the amount of memory needed to store the subtree that has the node as its root in HuffTree
}
HuffData;

static HuffData HuffTable[512];
static HuffData HuffTableInitData = { 0, 0, 0, {-1, -1}, 0, 0, 0, 0, 0 };

static u8 HuffTreeTop;                 // The number for HuffTreeTop
static u8 HuffTree[256][2];
typedef struct
{
    u8      leftOffsetNeed;            // 1 if offset to left child node is required
    u8      rightOffsetNeed;           // The right child node No.
    u16     leftNodeNo;                // The left child node No.
    u16     rightNodeNo;               // The right child node No.
}
HuffTreeCtrlData;
static HuffTreeCtrlData HuffTreeCtrl[256];
static HuffTreeCtrlData HuffTreeCtrlInitData = { 1, 1, 0, 0 };

static u16 sHuffDataNum;               // The type of data to be encoded: 16 if 4-bit encoding. 256 if 8-bit encoding


STATIC u32 HuffCompWrite(u8 *srcp, u32 size, u8 *dstp, u8 huffBitSize)
{
    u32     HuffDstCount;              // Number of bytes of compressed data
    u8      tmp;
    u16     nodeNum=0;                   // Number of valid nodes
    u16     tableTop;                  // When creating table, the table top No.
    s32     leftNo, rightNo;           // Node number for creating binary tree
    u32     i, ii, iii;
    u8      srcTmp;
    u32     bitStream = 0;
    u32     streamLength = 0;
    u16     rootNo;                    // Binary tree's root No.

    dbg_printf_huff(stderr, "HuffCompWrite\tsize=%d\thuffBitSize=%d\n", size, huffBitSize);

    sHuffDataNum = 1 << huffBitSize;   // 8->256, 4->16
    tableTop = sHuffDataNum;

    // Initialize table
    //  Subscript:   0  ~ 15(255)    : Information on encoding target data (8-bit encoding)
    //              16 ~ 31(511)    : Information for creating binary tree
    for (i = 0; i < (u16)(sHuffDataNum * 2); i++)
    {
        HuffTable[i] = HuffTableInitData;
        HuffTable[i].No = (u16)i;
    }

    // Check frequency of occurrence
    if (huffBitSize == 8)
    {
        for (i = 0; i < size; i++)
        {
            HuffTable[srcp[i]].Freq++; // 8-bit encoding
        }
    }
    else
    {
        for (i = 0; i < size; i++)
        {                              // 4-bit encoding
//          tmp =  srcp[i] & 0x0f;          HuffTable[tmp].Freq++;  // Store from low 4-bits forward
//          tmp = (srcp[i] & 0xf0) >> 4;    HuffTable[tmp].Freq++;
            tmp = (srcp[i] & 0xf0) >> 4;
            HuffTable[tmp].Freq++;     // Store from upper 4 bits forward // Either is OK
            tmp = srcp[i] & 0x0f;
            HuffTable[tmp].Freq++;     // The problem is the encoding
        }
    }

    // Create tree table
    leftNo = rightNo = -1;
    while (1)
    {
        // Search for two subtree nodes with low Freq value. At least one should be found.
        // Search child node (left)
        for (i = 0; i < tableTop; i++)
        {
            if ((HuffTable[i].Freq != 0) && (HuffTable[i].PaNo == 0))
            {
                leftNo = i;
                break;
            }
        }
        for (i = (u16)leftNo; i < tableTop; i++)
        {
            if ((HuffTable[i].Freq != 0) &&
                (HuffTable[i].PaNo == 0) && (HuffTable[i].Freq < HuffTable[leftNo].Freq))
            {
                leftNo = i;
            }
        }
        // Search child node (right)
        for (i = 0; i < tableTop; i++)
        {
            if ((HuffTable[i].Freq != 0) && (HuffTable[i].PaNo == 0) && (i != leftNo))
            {
                rightNo = i;
                break;
            }
        }
        for (i = (u16)rightNo; i < tableTop; i++)
        {
            if ((HuffTable[i].Freq != 0) &&
                (HuffTable[i].PaNo == 0) &&
                (HuffTable[i].Freq < HuffTable[rightNo].Freq) && (i != leftNo))
            {
                rightNo = i;
            }
        }
        // If only one, then end table creation
        if (rightNo < 0)
        {
            // When only one type of value exists, then create one node that takes the same value for both 0 and 1.
            if (tableTop == sHuffDataNum)
            {
                HuffTable[tableTop].Freq = HuffTable[leftNo].Freq;
                HuffTable[tableTop].ChNo[0] = (s16)leftNo;
                HuffTable[tableTop].ChNo[1] = (s16)leftNo;
                HuffTable[tableTop].LeafDepth = 1;
                HuffTable[leftNo].PaNo = (s16)tableTop;
                HuffTable[leftNo].Bit = 0;
                HuffTable[leftNo].PaDepth = 1;
            }
            else
            {
                tableTop--;
            }
            rootNo = tableTop;
            nodeNum = (rootNo - sHuffDataNum + 1) * 2 + 1;
            break;
        }

        // Create vertex that combines left subtree and right subtree
        HuffTable[tableTop].Freq = HuffTable[leftNo].Freq + HuffTable[rightNo].Freq;
        HuffTable[tableTop].ChNo[0] = (s16)leftNo;
        HuffTable[tableTop].ChNo[1] = (s16)rightNo;
        if (HuffTable[leftNo].LeafDepth > HuffTable[rightNo].LeafDepth)
        {
            HuffTable[tableTop].LeafDepth = HuffTable[leftNo].LeafDepth + 1;
        }
        else
        {
            HuffTable[tableTop].LeafDepth = HuffTable[rightNo].LeafDepth + 1;
        }

        HuffTable[leftNo].PaNo = HuffTable[rightNo].PaNo = tableTop;
        HuffTable[leftNo].Bit = 0;
        HuffTable[rightNo].Bit = 1;

        HuffUpdateParentDepth((u16)leftNo, (u16)rightNo);

        tableTop++;
        leftNo = rightNo = -1;
    }

    // Generate Huffman code (In HuffTable[i].HuffCode)
    HuffMakeCode(rootNo, 0x00);        // The Huffman code is the code with HuffCode's lower bit masked for PaDepth bits

    // For each intermediate node, calculate the amount of memory needed to store the subtree that has this node as the root in HuffTree.
    HuffCountHWord(rootNo);

    dbg_printf_huff(stderr, "No Freq PaNo ChNo[0] ChNo[1] PaDepth LeafDepth  HuffCode Bit HWord\n");
#ifdef DEBUG_PRINT_HUFF
    for (i = 0; i < (u16)(sHuffDataNum * 2); i++)
    {
        dbg_printf_huff(stderr, "%2d %4d %4d   %4d %4d\t%d\t%d\t0x%2x\t%d\t%d\n",
                        HuffTable[i].No,
                        HuffTable[i].Freq,
                        HuffTable[i].PaNo,
                        HuffTable[i].ChNo[0],
                        HuffTable[i].ChNo[1],
                        HuffTable[i].PaDepth,
                        HuffTable[i].LeafDepth,
                        HuffTable[i].HuffCode, H uffTable[i].Bit, HuffTable[i].HWord);
    }
#endif

    // Create HuffTree
    HuffMakeHuffTree(rootNo);
    HuffTree[0][0] = --HuffTreeTop;

    dbg_printf_huff(stderr, "rootNo = %d\n", rootNo);
    dbg_printf_huff(stderr, "                 0  1\n");
#ifdef DEBUG_PRINT_HUFF
    for (i = 0; i < 256; i++)
    {
        dbg_printf_huff(stderr, "HuffTree[%3d] = %2x %2x\n", i, HuffTree[i][0], HuffTree[i][1]);
    }
#endif

    // data header
    *(u32 *)dstp = size << 8 | HUFF_CODE_HEADER | huffBitSize;
    HuffDstCount = 4;

    for (i = 0; i < (u16)((HuffTreeTop + 1) * 2); i++)  // Tree table
    {
        dstp[HuffDstCount++] = ((u8 *)HuffTree)[i];
    }

    // Align to 4-byte boundary
    //   Data0 used for alignment is included in data size (as per the decoder algorithm)
    while (HuffDstCount & 0x3)
    {
        if (HuffDstCount & 0x1)
        {
            HuffTreeTop++;
            dstp[4] = dstp[4] + 1;
        }
        dstp[HuffDstCount++] = 0;
    }

    // Huffman encoding
    for (i = 0; i < size; i++)
    {                                  // Data compression
        if (huffBitSize == 8)
        {                              // 8-bit Huffman
            bitStream = (bitStream << HuffTable[srcp[i]].PaDepth) | HuffTable[srcp[i]].HuffCode;
            streamLength += HuffTable[srcp[i]].PaDepth;
            for (ii = 0; ii < streamLength / 8; ii++)
            {
                dstp[HuffDstCount++] = (u8)(bitStream >> (streamLength - (ii + 1) * 8));
            }
            streamLength %= 8;
        }
        else                           // 4-bit Huffman
        {
            for (ii = 0; ii < 2; ii++)
            {
//              if (ii) srcTmp = srcp[i] & 0x0f;    // Lower 4 bits
//              else    srcTmp = srcp[i] >> 4;      // Upper 4 bits
                if (ii)
                {
                    srcTmp = srcp[i] >> 4;      // Upper 4 bits
                }
                else
                {
                    srcTmp = srcp[i] & 0x0F;    // Lower 4 bits
                }
                bitStream = (bitStream << HuffTable[srcTmp].PaDepth) | HuffTable[srcTmp].HuffCode;
                streamLength += HuffTable[srcTmp].PaDepth;
                for (iii = 0; iii < streamLength / 8; iii++)
                {
                    dstp[HuffDstCount++] = (u8)(bitStream >> (streamLength - (iii + 1) * 8));
                }
                streamLength %= 8;
            }
        }
    }
    if (streamLength != 0)
    {
        dstp[HuffDstCount++] = (u8)(bitStream << (8 - streamLength));
    }

    // Align to 4-byte boundary
    //   Data0 for alignment is included in data size
    //   This is special to Huffman encoding! Data is stored after the alignment-boundary data in order to convert to little endian.
    while (HuffDstCount & 0x3)
    {
        dstp[HuffDstCount++] = 0;
    }

    for (i = 1 + (HuffTreeTop + 1) * 2 / 4; i < (HuffDstCount / 4) + 1; i++)
    {                                  // Little endian conversion
        tmp = dstp[i * 4 + 0];
        dstp[i * 4 + 0] = dstp[i * 4 + 3];
        dstp[i * 4 + 3] = tmp;         // Swap
        tmp = dstp[i * 4 + 1];
        dstp[i * 4 + 1] = dstp[i * 4 + 2];
        dstp[i * 4 + 2] = tmp;         // Swap
    }

    return HuffDstCount;
}



//-----------------------------------------------------------------------
// Creating table of Huffman Code
//-----------------------------------------------------------------------
STATIC void HuffMakeHuffTree(u16 rootNo)
{
    u16     i, tmp;
    s16     costHWord, tmpCostHWord;   // Make subtree code table for most-costly node when subtree code table has not been created.
    s16     costOffsetNeed, tmpCostOffsetNeed;
    s16     costMaxKey, costMaxRightFlag=0;       // Information for specifying the least costly node from HuffTree
    u8      offsetNeedNum;
    u8      tmpKey=0, tmpRightFlag;

    // Initialize HuffTreeCtrl
    for (i = 0; i < 256; i++)
    {
        HuffTree[i][0] = HuffTree[i][1] = 0;
    }
    for (i = 0; i < 256; i++)
    {
        HuffTreeCtrl[i] = HuffTreeCtrlInitData;
    }
    HuffTreeTop = 1;
    costOffsetNeed = 0;

    HuffTreeCtrl[0].leftOffsetNeed = 0; // Do not use (used as table size)
    HuffTreeCtrl[0].rightNodeNo = rootNo;

    while (1)                          // Until return
    {
        // Calculate the number of nodes required for setting offset
        offsetNeedNum = 0;
        for (i = 0; i < HuffTreeTop; i++)
        {
            if (HuffTreeCtrl[i].leftOffsetNeed)
            {
                offsetNeedNum++;
            }
            if (HuffTreeCtrl[i].rightOffsetNeed)
            {
                offsetNeedNum++;
            }
        }

        // Search for node with greatest cost
        costHWord = -1;
        costMaxKey = -1;
        tmpKey = 0;
        tmpRightFlag = 0;

        for (i = 0; i < HuffTreeTop; i++)
        {

            tmpCostOffsetNeed = HuffTreeTop - i;

            // Evaluate cost of left child node
            if (HuffTreeCtrl[i].leftOffsetNeed)
            {
                tmpCostHWord = HuffTable[HuffTreeCtrl[i].leftNodeNo].HWord;

                if ((tmpCostHWord + offsetNeedNum) > 64)
                {
                    goto leftCostEvaluationEnd;
                }
                if (!HuffRemainingNodeCanSetOffset((u8)tmpCostHWord))
                {
                    goto leftCostEvaluationEnd;
                }
                if (tmpCostHWord > costHWord)
                {
                    costMaxKey = i;
                    costMaxRightFlag = 0;
                }
                else if ((tmpCostHWord == costHWord) && (tmpCostOffsetNeed > costOffsetNeed))
                {
                    costMaxKey = i;
                    costMaxRightFlag = 0;
                }
            }
          leftCostEvaluationEnd:{
            }

            // Evaluate cost of right child node
            if (HuffTreeCtrl[i].rightOffsetNeed)
            {
                tmpCostHWord = HuffTable[HuffTreeCtrl[i].rightNodeNo].HWord;

                if ((tmpCostHWord + offsetNeedNum) > 64)
                {
                    goto rightCostEvaluationEnd;
                }
                if (!(HuffRemainingNodeCanSetOffset((u8)tmpCostHWord)))
                {
                    goto rightCostEvaluationEnd;
                }
                if (tmpCostHWord > costHWord)
                {
                    costMaxKey = i;
                    costMaxRightFlag = 1;
                }
                else if ((tmpCostHWord == costHWord) && (tmpCostOffsetNeed > costOffsetNeed))
                {
                    costMaxKey = i;
                    costMaxRightFlag = 1;
                }
            }
          rightCostEvaluationEnd:{
            }
        }

        // Store entire subtree in HuffTree
        if (costMaxKey >= 0)
        {
            HuffMakeSubsetHuffTree((u8)costMaxKey, (u8)costMaxRightFlag);
            goto nextTreeMaking;
        }
        else
        {
            // Search for largest node with required offset
            for (i = 0; i < HuffTreeTop; i++)
            {
                tmp = 0;
                tmpRightFlag = 0;
                if (HuffTreeCtrl[i].leftOffsetNeed)
                {
                    tmp = HuffTable[HuffTreeCtrl[i].leftNodeNo].HWord;
                }
                if (HuffTreeCtrl[i].rightOffsetNeed)
                {
                    if (HuffTable[HuffTreeCtrl[i].rightNodeNo].HWord > tmp)
                    {
                        tmpRightFlag = 1;
                    }
                }
                if ((tmp != 0) || (tmpRightFlag))
                {
                    HuffSetOneNodeOffset((u8)i, tmpRightFlag);
                    goto nextTreeMaking;
                }
            }
        }
        return;
      nextTreeMaking:{
        }
    }
}

//-----------------------------------------------------------------------
// Store entire subtree in HuffTree
//-----------------------------------------------------------------------
STATIC void HuffMakeSubsetHuffTree(u16 huffTreeNo, u8 rightNodeFlag)
{
    u8      i;

    i = HuffTreeTop;
    HuffSetOneNodeOffset(huffTreeNo, rightNodeFlag);

    if (rightNodeFlag)
    {
        HuffTreeCtrl[huffTreeNo].rightOffsetNeed = 0;
    }
    else
    {
        HuffTreeCtrl[huffTreeNo].leftOffsetNeed = 0;
    }

    while (i < HuffTreeTop)
    {
        if (HuffTreeCtrl[i].leftOffsetNeed)
        {
            HuffSetOneNodeOffset(i, 0);
            HuffTreeCtrl[i].leftOffsetNeed = 0;
        }
        if (HuffTreeCtrl[i].rightOffsetNeed)
        {
            HuffSetOneNodeOffset(i, 1);
            HuffTreeCtrl[i].rightOffsetNeed = 0;
        }
        i++;
    }
}

//-----------------------------------------------------------------------
// Check if there is any problems with HuffTree construction if subtree of obtained data size is decompressed.
//-----------------------------------------------------------------------
STATIC u8 HuffRemainingNodeCanSetOffset(u8 costHWord)
{
    u8      i;
    s16     capacity;

    capacity = 64 - costHWord;

    // The offset value is larger for smaller values of i, so you should calculate without sorting, with i=0 -> HuffTreeTop
    for (i = 0; i < HuffTreeTop; i++)
    {
        if (HuffTreeCtrl[i].leftOffsetNeed)
        {
            if ((HuffTreeTop - i) <= capacity)
            {
                capacity--;
            }
            else
            {
                return 0;
            }
        }
        if (HuffTreeCtrl[i].rightOffsetNeed)
        {
            if ((HuffTreeTop - i) <= capacity)
            {
                capacity--;
            }
            else
            {
                return 0;
            }
        }
    }

    return 1;
}

//-----------------------------------------------------------------------
// Create Huffman code table for one node
//-----------------------------------------------------------------------
STATIC void HuffSetOneNodeOffset(u16 huffTreeNo, u8 rightNodeFlag)
{
    u16     nodeNo;
    u8      offsetData = 0;

    if (rightNodeFlag)
    {
        nodeNo = HuffTreeCtrl[huffTreeNo].rightNodeNo;
        HuffTreeCtrl[huffTreeNo].rightOffsetNeed = 0;
    }
    else
    {
        nodeNo = HuffTreeCtrl[huffTreeNo].leftNodeNo;
        HuffTreeCtrl[huffTreeNo].leftOffsetNeed = 0;
    }

    // Left child node
    if (HuffTable[HuffTable[nodeNo].ChNo[0]].LeafDepth == 0)
    {
        offsetData |= 0x80;
        HuffTree[HuffTreeTop][0] = (u8)HuffTable[nodeNo].ChNo[0];
        HuffTreeCtrl[HuffTreeTop].leftNodeNo = (u8)HuffTable[nodeNo].ChNo[0];
        HuffTreeCtrl[HuffTreeTop].leftOffsetNeed = 0;   // Offset no longer required
    }
    else
    {
        HuffTreeCtrl[HuffTreeTop].leftNodeNo = HuffTable[nodeNo].ChNo[0];       // Offset required
    }

    // Right child node
    if (HuffTable[HuffTable[nodeNo].ChNo[1]].LeafDepth == 0)
    {
        offsetData |= 0x40;
        HuffTree[HuffTreeTop][1] = (u8)HuffTable[nodeNo].ChNo[1];
        HuffTreeCtrl[HuffTreeTop].rightNodeNo = (u8)HuffTable[nodeNo].ChNo[1];
        HuffTreeCtrl[HuffTreeTop].rightOffsetNeed = 0;  // Offset no longer required
    }
    else
    {
        HuffTreeCtrl[HuffTreeTop].rightNodeNo = HuffTable[nodeNo].ChNo[1];      // Offset required
    }

    offsetData |= (u8)(HuffTreeTop - huffTreeNo - 1);
    HuffTree[huffTreeNo][rightNodeFlag] = offsetData;

    HuffTreeTop++;
}


//-----------------------------------------------------------------------
// When creating binary tree and when combining subtrees, add 1 to the depth of every node in the subtree.
//-----------------------------------------------------------------------
STATIC void HuffUpdateParentDepth(u16 leftNo, u16 rightNo)
{
    HuffTable[leftNo].PaDepth++;
    HuffTable[rightNo].PaDepth++;

    if (HuffTable[leftNo].LeafDepth != 0)
    {
        HuffUpdateParentDepth(HuffTable[leftNo].ChNo[0], HuffTable[leftNo].ChNo[1]);
    }
    if (HuffTable[rightNo].LeafDepth != 0)
    {
        HuffUpdateParentDepth(HuffTable[rightNo].ChNo[0], HuffTable[rightNo].ChNo[1]);
    }
}

//-----------------------------------------------------------------------
// Create Huffman code
//-----------------------------------------------------------------------
STATIC void HuffMakeCode(u16 nodeNo, u32 paHuffCode)
{
    HuffTable[nodeNo].HuffCode = (paHuffCode << 1) | HuffTable[nodeNo].Bit;

    if (HuffTable[nodeNo].LeafDepth != 0)
    {
        HuffMakeCode(HuffTable[nodeNo].ChNo[0], HuffTable[nodeNo].HuffCode);
        HuffMakeCode(HuffTable[nodeNo].ChNo[1], HuffTable[nodeNo].HuffCode);
    }
}

//-----------------------------------------------------------------------
// Data volume required of intermediate node to create HuffTree
//-----------------------------------------------------------------------
STATIC u8 HuffCountHWord(u16 nodeNo)
{
    u8      leftHWord, rightHWord;

    switch (HuffTable[nodeNo].LeafDepth)
    {
    case 0:
        return 0;
    case 1:
        leftHWord = rightHWord = 0;
        break;
    default:
        leftHWord = HuffCountHWord(HuffTable[nodeNo].ChNo[0]);
        rightHWord = HuffCountHWord(HuffTable[nodeNo].ChNo[1]);
        break;
    }

    HuffTable[nodeNo].HWord = leftHWord + rightHWord + 1;
    return leftHWord + rightHWord + 1;
}

//##############################################################################################
//##############################################################################################
// Expansion related functions follow
//##############################################################################################
//##############################################################################################

//==================================================================================
// Expand Raw data
//==================================================================================
STATIC s32 RawRead(u8 *srcp, u32 size, u8 *dstp)
{
//  memcpy(dstp, srcp, size);
    u32     i;

    for (i = 0; i < size - 1; i++)
    {
        *dstp = *srcp;
        dstp++;
        srcp++;
    }
    *dstp = *srcp;

    return size;
}

//==================================================================================
// Expand difference compression data
//==================================================================================
EXTERN s32 DiffFiltRead(u8 *srcp, u32 size, u8 *dstp, u8 diffBitSize)
{
    s32     DiffCount = 0;             // Expanded data size in bytes
    u32     i;

    u16    *src16p = (u16 *)srcp;
    u16    *dst16p = (u16 *)dstp;

    if (diffBitSize == 8)
    {
#ifdef DEBUG_PRINT_DIFFFILT
        for (i = 0; i < 16; i++)
        {
            dbg_printf_dif(stderr, "srcp[%d] = %x\n", i, srcp[i]);
        }
#endif
        dstp[DiffCount] = srcp[0];     // No difference for beginning data only
        DiffCount++;
        for (i = 1; i < size; i++, DiffCount++)
        {
            dbg_printf_dif(stderr, "dstp[%x] = srcp[%d]+dstp[%d] = %x + %x = %x\n",
                           DiffCount, i, i - 1, srcp[i], dstp[i - 1], srcp[i] - dstp[i - 1]);
            dstp[DiffCount] = srcp[i] + dstp[i - 1];    // Store difference data
        }
    }
    else                               // 16 bit size
    {
        dst16p[DiffCount / 2] = src16p[0];
        DiffCount += 2;
        for (i = 1; i < size / 2; i++, DiffCount += 2)
            dst16p[DiffCount / 2] = src16p[i] + dst16p[i - 1];
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((DiffCount + i) & 0x3)
    {
        dstp[DiffCount + i] = 0;
        i++;
    }

    return DiffCount;
}

//==================================================================================
// Expanding run length compressed data
//==================================================================================
EXTERN s32 RLCompRead(u8 *srcp, u32 size, u8 *dstp)
{
    u32     RLDstCount;                // Expanded data size in bytes
    u32     RLSrcCount;                // Size of data that is targeted for expansion--after processing (bytes)
    u8      length;                    // Data length
    u32     i;

    RLSrcCount = 0;
    RLDstCount = 0;
    while (RLDstCount < size)
    {
        if (srcp[RLSrcCount] & 0x80)   // Decoding (run length encoded)
        {
            length = srcp[RLSrcCount++] & 0x7f; // Store data length (since 3 is added, this should be considered to be +3.)
            dstp[RLDstCount++] = srcp[RLSrcCount];
            dstp[RLDstCount++] = srcp[RLSrcCount];
            dstp[RLDstCount++] = srcp[RLSrcCount];
            for (i = 3; i < (u8)(length + 3); i++)
            {
                dstp[RLDstCount++] = srcp[RLSrcCount];
            }
            RLSrcCount++;
        }
        else                           // Copy raw data (not run length encoded)
        {
            length = srcp[RLSrcCount++];        //  (Same as srcp[RLSrcCount] & 0x7f )
            for (i = 0; i < (u8)(length + 1); i++)
            {                          //  +1 because data length is stored -1.
                dstp[RLDstCount] = srcp[RLSrcCount];
                RLDstCount++;
                RLSrcCount++;
            }
        }
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((RLDstCount + i) & 0x3)
    {
        dstp[RLDstCount + i] = 0;
        i++;
    }

    return RLDstCount;
}

//==================================================================================
// Expand LZ77 compressed data
//==================================================================================
EXTERN s32 LZCompRead(u8 *srcp, u32 size, u8 *dstp)
{
    u32     LZDstCount;                // Expanded data size in bytes
    u32     LZSrcCount;                // Size of data that is targeted for expansion--after processing (bytes)
    u8      compFlags;                 // Flag sequence indicating whether or not compressed
    u8      length;                    // Target data length (3 is added)
    u16     offset;                    // Match data offset -1 (Always 2 or more)
    u8     *startp;                    // Match data start address
    u32     i, j;

    LZSrcCount = 0;
    LZDstCount = 0;
    offset = 0;
    length = 0;

    while (LZDstCount < size)
    {
        compFlags = srcp[LZSrcCount++];
        for (i = 0; i < 8; i++)
        {
            if (compFlags & 0x80)      // Compressed
            {
                length = (srcp[LZSrcCount] >> 4) + 3;   // Upper 4 bits. 3 is added.  
                offset = (srcp[LZSrcCount] & 0x0f) << 8;        // Lower 4 bits. When offset, 11-8th bit
                LZSrcCount++;
                offset |= srcp[LZSrcCount];
                offset++;
                LZSrcCount++;
                compFlags <<= 1;

                // Extraction process
                startp = &dstp[LZDstCount - offset];
                for (j = 0; j < length; j++)
                {
                    dstp[LZDstCount++] = startp[j];
                }
            }
            else                       // Not compressed
            {
                dstp[LZDstCount++] = srcp[LZSrcCount++];
                compFlags <<= 1;
            }
            // end when size reached
            if (LZDstCount >= size)
            {
                break;
            }
        }
    }

    // Align to 4-byte boundary
    //   Does not include Data0 used for alignment as data size
    i = 0;
    while ((LZDstCount + i) & 0x3)
    {
        dstp[LZDstCount + i] = 0;
        i++;
    }
    return LZDstCount;
}

//==================================================================================
// Expanding Huffman encoded data
//==================================================================================
EXTERN s32 HuffCompRead(u8 *srcp, u32 size, u8 *dstp, u8 huffBitSize)
{
    u16     treeSize;                  // HuffTree size * 2
    u32     HuffSrcCount;              // Size of data that is targeted for expansion--after processing (bytes)
    u32     HuffDstCount;              // Extracted data
    u32     currentBitStream;
    u8      currentBit;
    u16     i;
    u16     treeAddr;
    u8      treeData;
    u8      preTreeData;
    u8      isUpper4bits = 0;

    treeSize = ((*srcp) + 1) * 2;
    HuffSrcCount = treeSize;           // Get beginning of data
    HuffDstCount = 0;
    treeAddr = 1;
    preTreeData = srcp[1];

    dbg_printf_huff(stderr, "HuffSrcCount = %d\n", HuffSrcCount);

    //  Extraction process
    while (1)                          // until return
    {
        currentBitStream = srcp[HuffSrcCount++];
        currentBitStream |= srcp[HuffSrcCount++] << 8;
        currentBitStream |= srcp[HuffSrcCount++] << 16;
        currentBitStream |= srcp[HuffSrcCount++] << 24;

        for (i = 0; i < 32; i++)
        {
            currentBit = (u8)(currentBitStream >> 31);
            currentBitStream <<= 1;

            if (((currentBit == 0) && (preTreeData & 0x80)) ||
                ((currentBit == 1) && (preTreeData & 0x40)))
            {
                if (huffBitSize == 8)
                {
                    treeData = srcp[(treeAddr * 2) + currentBit];       // code data
                    dstp[HuffDstCount++] = treeData;
                }
                else if (isUpper4bits)
                {
                    treeData |= (srcp[(treeAddr * 2) + currentBit]) << 4;
                    dstp[HuffDstCount++] = treeData;
                    isUpper4bits = 0;
                }
                else
                {
                    treeData = srcp[(treeAddr * 2) + currentBit];
                    isUpper4bits = 1;
                }

                if (HuffDstCount >= size)
                {
                    return HuffDstCount;
                }

                treeAddr = 1;
                preTreeData = srcp[1];
            }
            else
            {
                preTreeData = srcp[(treeAddr * 2) + currentBit];        // offset data
                treeAddr += (preTreeData & 0x3f) + 1;
            }
        }
    }
}

//==================================================================================
// Data expansion control function (Because it auto expands, it will not run if at the end there is no header for raw data expansion.)
//==================================================================================
EXTERN u32 __stdcall nitroDecompress(u8 *srcp, u32 size, u8 *dstp, s8 depth)
{
    // rawData      // Data header
    // *(u32 *)pReadBuf = size << 8 | 0;
    //                      [i+3] [i+2] [i+1](size)  |  [0000 0000]
    // DiffFilt
    // *(u32 *)dstp     = size << 8 | 0x80 | diffBitSize/8;
    //                      [i+3] [i+2] [i+1](size)  |  [1000 00XX]
    // RL
    // *(u32 *)dstp     = size << 8 | 0x30;
    //                      [i+3] [i+2] [i+1](size)  |  [0011 0000]
    // LZ77
    // *(u32 *)dstp     = size << 8 | 0x10;
    //                      [i+3] [i+2] [i+1](size)  |  [0001 0000]
    // Huffman
    // *(u32 *)dstp     = size << 8 | 0x20 | huffBitSize;
    //                      [i+3] [i+2] [i+1](size)  |  [0010 XX00]
    u32     header;
    u32     srcSize, dstSize = size;
    u32     memSize = size * 3 + 256 * 2;
    u8     *pReadBuf;                  // Pointer to the beginning address of compressed data.
    s8      curDepth = 0;
    s8      targetDepth;

    pCompBuf[0] = (u8 *)malloc(memSize);
    pCompBuf[1] = (u8 *)malloc(memSize);
    pReadBuf = pCompBuf[0];

    // malloc check
    if (pCompBuf[0] == NULL || pCompBuf[1] == NULL)
    {
        fprintf(stderr, "Error: Memory is not enough.\n");
        exit(1);
    }

    compBufNo = 1;
    memcpy(pReadBuf, srcp, size);

    if (depth < 1)
    {
        targetDepth = -1;
    }
    else
    {
        targetDepth = depth;
    }
    dbg_printf(stderr, "nitroCompress    \t(Compressed   size      is 0x%x)\n", size);

    while (1)
    {
        // End condition in the case that targetDepth is specified
        if (curDepth == targetDepth)
        {
            dbg_printf(stderr, "nitroDecompress  Raw \t(Decompressed size will be 0x%x)\n",
                       dstSize);
            dstSize = RawRead(pReadBuf, srcSize, dstp);

            if (pCompBuf[0] != NULL)
            {
                free(pCompBuf[0]);
                pCompBuf[0] = NULL;
            }
            if (pCompBuf[1] != NULL)
            {
                free(pCompBuf[1]);
                pCompBuf[1] = NULL;
            }
            return dstSize;
        }

        header = *(u32 *)pReadBuf;
        srcSize = header >> 8;         // Size does not include the header. Passes to the extraction function without the header.

        if (memSize < srcSize)
        {
            memSize = srcSize * 3 + 256 * 2;
            pCompBuf[0] = (u8 *)realloc(pCompBuf[0], memSize);
            pCompBuf[1] = (u8 *)realloc(pCompBuf[1], memSize);
            pReadBuf = pCompBuf[compBufNo ^ 0x1];

        }

        switch (header & CODE_HEADER_MASK)
        {
        case DIFF_CODE_HEADER:
            {
                dbg_printf(stderr, "nitroDecompress  Diff %d \t(Decompressed size will be 0x%x)\n",
                           ((u8)header & 0x03) * 8, srcSize);
                dstSize =
                    DiffFiltRead(&pReadBuf[4], srcSize, pCompBuf[compBufNo],
                                 ((u8)header & 0x03) * 8);
            }
            break;
        case HUFF_CODE_HEADER:
            {
                dbg_printf(stderr, "nitroDecompress  Huff %d \t(Decompressed size will be 0x%x)\n",
                           ((u8)header & 0x0f), srcSize);
                dstSize =
                    HuffCompRead(&pReadBuf[4], srcSize, pCompBuf[compBufNo], (u8)header & 0x0f);
            }
            break;
        case LZ_CODE_HEADER:
            {
                dbg_printf(stderr, "nitroDecompress  LZ \t(Decompressed size will be 0x%x)\n",
                           srcSize);
                dstSize = LZCompRead(&pReadBuf[4], srcSize, pCompBuf[compBufNo]);
            }
            break;
        case RL_CODE_HEADER:
            {
                dbg_printf(stderr, "nitroDecompress  RL \t(Decompressed size will be 0x%x)\n",
                           srcSize);
                dstSize = RLCompRead(&pReadBuf[4], srcSize, pCompBuf[compBufNo]);
            }
            break;
        default:
            {
                dbg_printf(stderr, "nitroDecompress  Raw \t(Decompressed size will be 0x%x)\n",
                           srcSize);

                dstSize = RawRead(&pReadBuf[4], srcSize, dstp);
                if (pCompBuf[0] != NULL)
                {
                    free(pCompBuf[0]);
                    pCompBuf[0] = NULL;
                }
                if (pCompBuf[1] != NULL)
                {
                    free(pCompBuf[1]);
                    pCompBuf[1] = NULL;
                }
                return dstSize;
            }
        }
        // One more loop
        pReadBuf = pCompBuf[compBufNo];
        compBufNo ^= 0x01;
        srcSize = dstSize;
        curDepth++;
    }
}

//==================================================================================
// Output memory content in hex
//==================================================================================
EXTERN void __stdcall debugMemPrint(FILE * fp, u8 *str, u32 size)
{
    u32     i = 0;

    while (str)
    {
        fprintf(fp, "%4lx:\t0x%2x\n", i, *str);
        str++;
        i++;
        if (i >= size)
        {
            break;
        }
    }
}

//==================================================================================
// Output memory content in binary
//==================================================================================
EXTERN void __stdcall debugMemBitPrint(FILE * fp, u8 *str, u32 size)
{
    u32     i = 0;
    u8      j;

    while (str)
    {
        if (i >= size)
        {
            break;
        }

        fprintf(fp, "%4lx:\t0x%2x\t(binary\t", i, *str);
        for (j = 0; j < 8; j++)
        {
            fprintf(fp, "%d", *str >> (7 - j) & 0x01);
        }
        fprintf(fp, " )\n");
        str++;
        i++;
    }
}

//==================================================================================
// Compares data before compression and after expansion. (If expansion is done correctly, will output "DATA match".)
//==================================================================================
EXTERN int __stdcall matchingCheck(u8 *srcp, u32 srcSize, u8 *dstp, u32 dstSize)
{
    u32     minSize, i;
    u8      dataMatchFlag = 1;
    u8      sizeMatchFlag;

    sizeMatchFlag = (srcSize == dstSize);
    if (srcSize < dstSize)
    {
        minSize = srcSize;
    }
    else
    {
        minSize = dstSize;
    }

    for (i = 0; i < minSize; i++)
    {
        dbg_printf_match(stderr, "src[%3x], dst[%3x] = %2x , %2x", i, i, srcp[i], dstp[i]);
        if (srcp[i] != dstp[i])
        {
            dataMatchFlag = 0;
            dbg_printf_match(stderr, "\t; mismatch here!");
        }
        dbg_printf_match(stderr, "\n");
    }

    if (sizeMatchFlag)
    {
        fprintf(stderr, "\nSIZE match.\n");
    }
    else
    {
        fprintf(stderr, "\nSIZE mismatch!\n");
    }

    if (dataMatchFlag)
    {
        fprintf(stderr, "DATA match.\n");
    }
    else
    {
        fprintf(stderr, "DATA mismatch!\n");
    }

    if (dataMatchFlag && sizeMatchFlag)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
