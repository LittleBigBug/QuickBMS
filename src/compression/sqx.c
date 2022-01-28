// doesn't seem to work!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __CX_DINO_H
#undef __SQX_OS_WIN32__
#define __SQX_OS_SIMPLE_32BIT_DOS__
#include "../libs/sqx/Format/SQX1/Source/uarde.c"

int sqx1_decoder(unsigned char *in, int insz, unsigned char *out, int outsz, int has_compflags) {
    sqx_in   = in;
    sqx_inl  = in + insz;
    sqx_out  = out,
    sqx_outl = out + outsz;

    /*
    example of sqx1 archive: main_head and file

    00000000 get     usHeadCRC  0x00000688 2
    00000002 get     cHeadType  0x00000052 1
    00000003 get     usHeadFlags 0x00002004 2
    00000005 get     usHeadSize 0x00000019 2
    00000007 getdstr cHeadID    "-sqx-" 5
    0000000c get     cArcerVer  0x0000000b 1
    0000000d get     usArcVolNum 0x00000000 2
    0000000f get     u32ArcVolId 0x00000000 4
    00000013 get     usReserved1 0x00000000 2
    00000015 get     usReserved2 0x00000000 2
    00000017 get     usReserved3 0x00000000 2
    00000019 get     usHeadCRC  0x0000b915 2
    0000001b get     cHeadType  0x00000044 1
    0000001c get     usHeadFlags 0x00002740 2
    0000001e get     usHeadSize 0x0000002d 2
    00000020 get     cCompFlags 0x00000000 1
    00000021 get     usExtraFlags 0x00000000 2
    00000023 get     cOsFilesys 0x00000013 1
    00000024 get     cArcerExVer 0x0000000b 1
    00000025 get     cArcMethod 0x00000004 1
    00000026 get     u32FileCRC32 0x0e61eb75 4
    0000002a get     u32FileAttr 0x00000020 4
    0000002e get     u32FileTime 0x2cb5187a 4
    00000032 get     u32CompSize 0x00000160 4
    00000036 get     u32OrigSize 0x000003eb 4
    0000003a get     usFileNameLen 0x0000000a 2
    */

    memset(&ArcStruct, 0, sizeof(ArcStruct));
    memset(&DeltaCtrl, 0, sizeof(DeltaCtrl));
    memset(&ProgressStruct, 0, sizeof(ProgressStruct));

    ArcStruct.ArchiveStatus = SQX_EC_OK;

    memset(&IoStruct, 0, sizeof(IoStruct));
	IoStruct.uCFileCRC = 0;
	IoStruct.u32FileCRC = 0;
	IoStruct.cCompMethod = 0;
	IoStruct.cCompFlags = 0;
	IoStruct.u32FileTime = 0;
	IoStruct.l64OrigSize = outsz;
	IoStruct.l64CompSize = insz;
	IoStruct.l64ToFlush = 1;
	IoStruct.usExtCompressor = 0 | SQX_DELTA_TYPE_FLAG; // ??? useless
	IoStruct.lSubCompressor = 0;
	IoStruct.lImageLastCount = 0;
	IoStruct.lImageSize = 0;

    /*
    ARC_NODE ALNode;
    memset(&ALNode, 0, sizeof(ALNode));
	ALNode.lTagged = OPTION_NONE;
	ALNode.lCryptFlag = OPTION_NONE;
	ALNode.szFName[0] = 0;
	ALNode.l64NewSize = 0;
	ALNode.l64OrigSize = outsz;
	ALNode.lFileTime = 0;
	ALNode.lAttr = ~OS_ATTRIBUTE_DIRECTORY;
	ALNode.lVersion = 0;
	ALNode.uFileCRC = 0;
	ALNode.lHostOS = 0;
	ALNode.lMethod = SQX_C1_BEST;
	ALNode.lFlags = ~SQX_SOLID_FILE_FLAG; // maximum dictionary size, non solid
	ALNode.lBlockType = 0;
	ALNode.lCompFlags = has_compflags ? (~OPTION_NONE) : OPTION_NONE;
	ALNode.lTotalBlockSize = 0;
	ALNode.l64FileHdrStart = 0;
    */

    InitDecompressor();
    //DecodeFile(ALNode.lCompFlags,ALNode.lFlags & SQX_SOLID_FILE_FLAG,(1U << (((ALNode.lFlags >> 8) & 15) + 15)));
    DecodeFile(0, 0, 1U << ((7 /*crash wit > 9*/ & 15) + 15));
    FreeDecompressor();

    return sqx_out - out;
}

