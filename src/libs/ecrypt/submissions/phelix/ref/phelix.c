/*
** Phelix implementation
**
** Public domain code.  Author:  Doug Whiting, Hifn, 2005
*/
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#ifdef ECRYPT_API
#include "ecrypt-sync-ae.h"
#endif

#include "phelix.h"

#ifndef BSWAP       /* make sure that we have a BSWAP that works */
#if     defined(ECRYPT_LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN_)
#define BSWAP(x)    (x)
#elif   defined(ECRYPT_BIG_ENDIAN)    || defined(_BIG_ENDIAN_)
#define BSWAP(x)    SWAP32(x)
#else   /* completely portable: slow, but it works */
#define GET8(x,i) ((u32b)(((u08b *) & (x))[i]))
#define BSWAP(x) (GET8(x,0) + (GET8(x,1) << 8) + (GET8(x,2) << 16) + (GET8(x,3) << 24))
#endif
#endif

#ifndef ROTL32      /* make sure we have a rotation op to work with */
#define ROTL32(x,n) ((u32b)(((x) << (n)) ^ ((x) >> (32-(n)))))
#endif

typedef const u32b    * c32p; 
typedef       u32b    * u32p;

enum    /* Phelix algorithm internal constants */
    {
    OLD_Z_REG       =    4,                 /* which var used for "old" state   */
    ZERO_INIT_CNT   =    8,                 /* how many words of initial mixing */
    MAC_INIT_CNT    =    8,                 /* how many words of pre-MAC mixing */
    MAC_WORD_CNT    =  PHELIX_MAC_SIZE/32,  /* how many words of MAC output     */
    
    ROT_0a          =    9,     /* rotation constants for Phelix block function */
    ROT_1a          =   10,
    ROT_2a          =   17,
    ROT_3a          =   30,
    ROT_4a          =   13,

    ROT_0b          =   20,
    ROT_1b          =   11,
    ROT_2b          =    5,
    ROT_3b          =   15,
    ROT_4b          =   25
    };

#define MAC_MAGIC_XOR   0x912D94F1          /* magic constant for MAC */
#define AAD_MAGIC_XOR   0xAADAADAA          /* magic constant for AAD */

/* 1st half of a half block */
#define Phelix_H0(Z,xx)                                             \
            Z[0] +=(Z[3]^(xx));         Z[3] = ROTL32(Z[3],ROT_3b); \
            Z[1] += Z[4];               Z[4] = ROTL32(Z[4],ROT_4b); \
            Z[2] ^= Z[0];               Z[0] = ROTL32(Z[0],ROT_0a); \
            Z[3] ^= Z[1];               Z[1] = ROTL32(Z[1],ROT_1a); \
            Z[4] += Z[2];               Z[2] = ROTL32(Z[2],ROT_2a); 
                        
/* 2nd half of a half block */
#define Phelix_H1(Z,xx)                                             \
            Z[0] ^=(Z[3]+(xx));         Z[3] = ROTL32(Z[3],ROT_3a); \
            Z[1] ^= Z[4];               Z[4] = ROTL32(Z[4],ROT_4a); \
            Z[2] += Z[0];               Z[0] = ROTL32(Z[0],ROT_0b); \
            Z[3] += Z[1];               Z[1] = ROTL32(Z[1],ROT_1b); \
            Z[4] ^= Z[2];               Z[2] = ROTL32(Z[2],ROT_2b);

int  _debugPhelix_ = 0;             /* global: enable debug i/o */

/*  some useful macros for detailed debug output */
#ifdef ALLOW_DEBUG_IO
#include <stdio.h>
void DebugByteOut(const void *p,u32b cnt)
    { u32b j; for (j=0;j<cnt;j++) printf("%s%02X",(j%16)?" ":"\n  ",((U08P)p)[j]); }

void DebugWordOut(const void *p,u32b cnt,const char *suffix)
    { u32b j; for (j=0;j<cnt;j++) printf("%08X%s",((u32b *)p)[j],(j==cnt-1)?suffix:" "); }

#define DebugStr(s)                 if (_debugPhelix_) printf(s)
#define DebugStrN(s,N)              if (_debugPhelix_) printf(s,N)
#define DebugWords(fmtStr,v,wCnt,p)       if (_debugPhelix_)            \
                {  printf(fmtStr,v,wCnt);   DebugWordOut(p,wCnt," "); }
#define DebugBytes(fmtStr,v,bitCnt,p) if (_debugPhelix_)                \
                {  printf(fmtStr,v,bitCnt); DebugByteOut(p,bitCnt/8); }
#define DebugState(i,c,Z,s,v0,v1)   if (_debugPhelix_)                  \
                {  printf("Z.%02d%s=",i-7,c); DebugWordOut(Z,5,".");    \
                   printf(s,v0,v1); printf("\n"); }
#else   /* do not generate any debug i/o code at all (for speed) */
#define DebugStr(s) 
#define DebugStrN(s,N)
#define DebugState(i,c,Z,s,v0,v1)
#define DebugWords(fmtStr,v,wCnt,p)
#define DebugBytes(fmtStr,v,bitCnt,p)
#endif

const   u32b MASK_TAB[4]    =   {0,0xFF,0xFFFF,0xFFFFFF};

/* use this function to avoid unidentified MSVC42 warning in PhelixInit :-( */
void Assert(int x)
    {
    if (!x)
        {
        assert(x);
        }
    }

const char *PhelixCompiler_Name(void)
	{
#if   defined(_MSC_VER) 
	static char name[] = "MSVC v??.??";
	name[ 6]='0'+(_MSC_VER/1000)%10;
	name[ 7]='0'+(_MSC_VER/100 )%10;
	name[ 9]='0'+(_MSC_VER/10  )%10;
	name[10]='0'+(_MSC_VER     )%10;
#elif defined(__BORLANDC__)
	static char name[] = "BorlandC v?.??";
	name[10]='0'+((__BORLANDC__)/256)%10;
	name[12]='0'+((__BORLANDC__)%256)/16;
	name[13]='0'+((__BORLANDC__)%256)%16;
#elif defined(__MINGW_H) || (defined(__GNUC__) && (defined(__i386__) || defined(__386)))
	static char  name[] = "GCC v?.?";
	name[ 5]='0'+__GNUC__;
	name[ 7]='0'+__GNUC_MINOR__%10;
#else
	const char *name = "(unknown)";
#endif
	return name;
	}

/* one-time initialization: nothing to do for Phelix */
void PhelixInit(void)
    {
    u32b i = 0x00AA55FF;        /* check compile-time settings */
    if (((u08b *)&i)[0])        /* make sure that endianness is properly defined! */
        Assert(BSWAP(i) == i);
    else
        Assert(BSWAP(i) == 0xFF55AA00);
    i = 0;
    Assert(BSWAP(i) == 0);      /* sanity check on BSWAP */
    }

/* Phelix key schedule setup */
void PhelixSetupKey(PhelixContext *ctx,const U08P keyPtr,u32b keySize,u32b ivSize,u32b macSize)
    {
    u32b    i,j,k,Z[5],X[8];

    DebugStrN("SetupKey:  keySize = %d bits. ",keySize);
    DebugBytes("MAC tag = %d bits.\n  Raw Key = ",macSize,keySize,keyPtr);

    assert(PHELIX_NONCE_SIZE==ivSize);  /* Phelix only supports "full" nonces       */
    assert( 0  == (keySize%8));         /* Phelix only supports byte-sized keys     */
    assert(256 >=  keySize);            /* Phelix only supports keys <= 256 bits    */

    ctx->ks.keySize = keySize;          /* save key, mac, and nonce sizes           */
    ctx->ks.macSize = macSize;
    /* pre-compute X_1_bump "constant" to save clock cycles during SetupNonce       */
    ctx->ks.X_1_bump= keySize/2 + 256*(macSize % PHELIX_MAC_SIZE);

    for (i=0;i<(keySize+31)/32;i++)     /* copy key to X[], in correct endianness   */
        X[i] = BSWAP(((c32p)keyPtr)[i]);
    for (   ;i<8;i++)                   /* handle zero padding at the word level    */
        X[i] = 0;           
    if (keySize % 32)                   /* handle zero padding of the bit  level    */
        X[keySize/32] &= (1 << (keySize%32)) - 1;

    DebugWords("\nKeyMixing:\nX.%d  =",8,8,X);
    /* Now process the padded "raw" key, using a Feistel network */
    for (i=0;i<8;i++)
        {
        k = 4*(i&1);
        for (j=0;j<4;j++)
            Z[j] = X[k+j];
        Z[4] = (keySize/8) + 64;
        Phelix_H0(Z,0);
        Phelix_H1(Z,0);
        Phelix_H0(Z,0);
        Phelix_H1(Z,0);
        k = (k+4) % 8;
        for (j=0;j<4;j++)
            X[k+j] ^= Z[j];
        DebugWords("\nX.%d  =",7-i,8,X);
        }
    for (i=0;i<8;i++)
        ctx->ks.X_0[i] = X[i];

    }

/* Phelix per-nonce setup */
void PhelixSetupNonce(PhelixContext *ctx,const U08P noncePtr)
    {
    u32b i,j,n;
    c32p nonce = (c32p) noncePtr;
    PhelixContext pc;

    pc = *ctx;              /* make a local copy, for speed */

    /* initialize subkeys and Z values */
    for (i=0;i<4;i++)
        {
        n = BSWAP(nonce[i]);
        pc.ks.X_1[i  ] = pc.ks.X_0[i+4] +    n ;
        pc.ks.X_1[i+4] = pc.ks.X_0[i  ] + (i-n);
        pc.cs.Z  [i  ] = pc.ks.X_0[i+3] ^ BSWAP(nonce[i]) ;
        }
    pc.ks.X_1[1]   += pc.ks.X_1_bump;       /* X' adjustment for i==1 mod 4 */
    pc.ks.X_1[5]   += pc.ks.X_1_bump;
    pc.cs.Z[4]      = pc.ks.X_0[7];
    pc.cs.aadLen[0] = 0;
    pc.cs.aadLen[1] = 0;
    pc.cs.msgLen    = 0;

    /* show the completed key schedule and init state (if debugging) */
    DebugStrN("SetupNonce: keySize  = %d bits.  ",pc.ks.keySize);
    DebugBytes("MAC tag = %d bits.\n  Nonce=",pc.ks.macSize,PHELIX_NONCE_SIZE,noncePtr);
    DebugStr  ("\nWorking key schedule:");  
    DebugWords("\nX_i_0=",0,8,pc.ks.X_0); 
    DebugWords("\nX_i_1=",0,8,pc.ks.X_1);
    DebugStr  ("\n\n");
    DebugState(-1," ",pc.cs.Z,"\n",0,0);

    for (i=0;i<8;i++)
        {   /* customized version of loop for zero initialization */
        j = i & 7;

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],pc.ks.X_0[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);
        DebugState(i,"b",pc.cs.Z,"                          plainText= %08X.",0,0);

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"c",pc.cs.Z,"               %08s      X_i_1 = %08X.","",pc.ks.X_1[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s","",0);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    pc.cs.aadXor = AAD_MAGIC_XOR; /* perform the AAD xor */
    pc.cs.Z[1] ^= pc.cs.aadXor;
    pc.cs.i = i;
    *ctx    = pc;       /* copy back the fully initialized context */
    }

void PhelixEncryptBytes(PhelixContext *ctx,const U08P pt,U08P ct,u32b msgLen)
    {
    u32b i,j,w[2],plainText,cipherText,keyStream;
    c32p srcPtr = (c32p) pt;
    u32p dstPtr = (u32p) ct;
    u32p p      = NULL;
    PhelixContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStrN("EncryptBytes: %d bytes\n",msgLen);

    assert(0 == (pc.cs.msgLen % 4));    /* can only make ONE sub-word call! */
    pc.cs.msgLen += msgLen;
    i = pc.cs.i;
    pc.cs.Z[1] ^= pc.cs.aadXor;         /* do the AAD xor, if needed */
    pc.cs.aadXor= 0;                    /* next time, the xor will be a nop */

    for (;msgLen;i++,srcPtr++,dstPtr++)
        {
        if (msgLen >= 4)
            msgLen -= 4;
        else    /* remaining partial word */
            {
            p       = dstPtr;       /* where to put final partial word */
            w[0]    = srcPtr[0] & BSWAP(MASK_TAB[msgLen%4]);
            srcPtr  = w;
            dstPtr  = w;
            msgLen  = 0;
            }

        j = i & 7;

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],pc.ks.X_0[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);
        keyStream = pc.cs.Z[4] + pc.cs.oldZ[i&3];

        plainText   = BSWAP(*srcPtr);
        cipherText  = keyStream ^ plainText;
        *dstPtr     = BSWAP(cipherText);

        DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

        Phelix_H0(pc.cs.Z,plainText);
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    if (p)                              /* output partial word? */
        p[0] ^= (p[0] ^ w[0]) & BSWAP(MASK_TAB[pc.cs.msgLen%4]);

    pc.cs.i = i;
    ctx->cs = pc.cs;                    /* copy back the modified state */
    }

void PhelixDecryptBytes(PhelixContext *ctx,const U08P ct,U08P pt,u32b msgLen)
    {
    u32b i,j,w[2],plainText,keyStream;
    c32p srcPtr = (c32p) ct;
    u32p dstPtr = (u32p) pt;
    u32p p      = NULL;
    PhelixContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStrN("DecryptBytes: %d bytes\n",msgLen);

    assert(0 == (pc.cs.msgLen % 4));    /* can only make ONE sub-word call! */
    pc.cs.msgLen += msgLen;
    i = pc.cs.i;
    pc.cs.Z[1] ^= pc.cs.aadXor;         /* do the AAD xor, if needed */
    pc.cs.aadXor= 0;                    /* next time, the xor will be a nop */

    for (;msgLen;i++,srcPtr++,dstPtr++)
        {
        if (msgLen >= 4)
            msgLen -= 4;
        else    /* remaining partial word */
            {
            p       = dstPtr;           /* where to put final partial word */
            w[0]    = srcPtr[0] & BSWAP(MASK_TAB[msgLen%4]);
            srcPtr  = w;
            dstPtr  = w;
            msgLen  = 0;
            }

        j = i & 7;

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],pc.ks.X_0[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);
        keyStream = pc.cs.Z[4] + pc.cs.oldZ[i&3];

        plainText   = BSWAP(*srcPtr) ^ keyStream;
        if (srcPtr == w)                /* extend ciphertext to full 32 bits */
            plainText &= MASK_TAB[pc.cs.msgLen % 4];
        *dstPtr     = BSWAP(plainText);
        
        DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

        Phelix_H0(pc.cs.Z,plainText);
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    if (p)                              /* output partial word */
        p[0] ^= (p[0] ^ w[0]) & BSWAP(MASK_TAB[pc.cs.msgLen%4]);

    pc.cs.i = i;
    ctx->cs = pc.cs;                    /* copy back the modified state */
    }

void PhelixProcessAAD(PhelixContext *ctx,const U08P aad,u32b aadLen)
    {
    u32b i,j,w,plainText;
    c32p srcPtr = (c32p) aad;
    PhelixContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStrN("ProcessAAD: %d bytes\n",aadLen);

    assert(0 == (pc.cs.aadLen[0] % 4)); /* can only make ONE sub-word call! */
    pc.cs.aadLen[0] += aadLen;          /* do a 64-bit add into pc.cs.addLen[] */
    if (pc.cs.aadLen[0] < aadLen)
        pc.cs.aadLen[1]++;

    i = pc.cs.i;                        /* keep it short for cosmetics */
    for (;aadLen;i++,srcPtr++)
        {
        if (aadLen >= 4)        
            aadLen -= 4;
        else                            /* handle any final "odd" aad bytes */
            {
            w       = srcPtr[0] & BSWAP(MASK_TAB[aadLen % 4]);
            srcPtr  = &w;               /* use local copy of masked word */
            aadLen  = 0;                /* we're definitely done now */
            }
        
        j = i & 7;

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],pc.ks.X_0[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);

        plainText   = BSWAP(*srcPtr);

        DebugState(i,"b",pc.cs.Z,"  %22s  plainText= %08X.","",plainText);

        Phelix_H0(pc.cs.Z,plainText);
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"\n",0,0);

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    pc.cs.i = i;
    ctx->cs = pc.cs;                    /* copy back the modified state */
    }
            
#define PM32(dst,src,n) ((u32p)(dst))[n] = ((u32p)(src))[n]
#define PutMac(d,s,bits)    /* d = dst, s = src */  \
        switch (bits) {                 \
            case 128: PM32(d,s,3);      \
            case  96: PM32(d,s,2);      \
                      PM32(d,s,1);      \
                      PM32(d,s,0);      \
                      break;            \
            default:  memcpy(d,s,((bits)+7)/8); }   /* optimize "common" cases */

void PhelixFinalize(PhelixContext *ctx,U08P mac)
    {
    u32b i,j,k,plainText,cipherText,keyStream,tmp[MAC_INIT_CNT+MAC_WORD_CNT];
    PhelixContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStr("FinalizeMAC:\n");

    i           = pc.cs.i;
    plainText   = pc.cs.msgLen % 4;
    pc.cs.Z[0] ^= MAC_MAGIC_XOR;
    pc.cs.Z[4] ^= pc.cs.aadLen[0];
    pc.cs.Z[2] ^= pc.cs.aadLen[1];
    pc.cs.Z[1] ^= pc.cs.aadXor;         /* do this in case msgLen == 0 */

    for (k=0;k<MAC_INIT_CNT+MAC_WORD_CNT;k++,i++)
        {
        j = i & 7;

        Phelix_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],pc.ks.X_0[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);
        keyStream = pc.cs.Z[4] + pc.cs.oldZ[i&3];

        cipherText  = keyStream ^ plainText;
        tmp[k]      = BSWAP(cipherText);

        DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

        Phelix_H0(pc.cs.Z,plainText);
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    
    PutMac(mac,&tmp[MAC_INIT_CNT],pc.ks.macSize);

    /* no need to copy back any modified context -- we're done with this one! */
    }

u32b PhelixIncremental_CodeSize(void)
    {
#if defined(_MSC_VER) | defined(__STDC__)
    return 0; /* MSVC42 doesn't work here for some reason */
#else
    return ((u08b *) PhelixIncremental_CodeSize) - ((u08b *) PhelixInit);
#endif
    }

/* Phelix encryption/decryption in C (not real fast, but useful for checking vs. ASM) */
void PhelixProcessPacket(int action,PhelixPacketParms)
    {
    enum
        {
        PHASE_AAD      =    0,      /* "phases" of the encryption/decryption operation */
        PHASE_DATA,
        PHASE_ODD_BYTES,
        PHASE_MAC_GEN,
        PHASE_DONE
        };
    int     doEncrypt = (action == 0);
    u32b    i,j,k,phase,wCnt,keyStream,plainText,cipherText,encBlk,aadLeft;
    u32b    tmp[MAC_INIT_CNT + MAC_WORD_CNT + 4];
    c32p    srcPtr,aadPtr;           /* const pointers to u32b[]          */
    u32p    dstPtr;                  /*       pointer  to u32b[]          */
    PhelixContext pc = * ctx;        /* local copy of context (for speed) */

    DebugStrN("\n**********\nPhelix_C_%s:  ",(doEncrypt)?"Encrypt":"Decrypt");
    DebugStrN("msgLen = %d. ",msgLen);
    DebugStrN("aadLen = %d.\n",aadLen);
    PhelixSetupNonce(&pc,nonce);    /* do the initial nonce mixing */
    i = pc.cs.i;                    /* use "local" copy of i here   */
    aadLeft = aadLen;               /* number of bytes of aad remaining */
    aadPtr  = (c32p) aad;
    phase   = (aadLeft) ? PHASE_AAD : PHASE_DATA;
    srcPtr  = NULL;                 /* avoid compiler warnings :( */
    encBlk  = 0;

    for (; phase < PHASE_DONE ; phase++)
        {
        switch (phase)
            {
            case PHASE_AAD:
                if (aadLeft >= 4)           /* handle full words first */
                    {
                    srcPtr  = aadPtr;
                    dstPtr  = tmp;          /* discard the ciphertext */
                    wCnt    = ((aadLeft > sizeof(tmp)) ? sizeof(tmp) : aadLeft) /4;
                    aadLeft-= wCnt*4;       /* adjust count   */
                    aadPtr += wCnt;
                    if (aadLeft)            /* will there be anything left after this? */
                        phase--;            /* if so, we need to come back here for more */
                    DebugStrN("**********PHASE_AAD (%d bytes):\n",wCnt*4);
                    }
                else                        /* handle final odd bytes */
                    {
                    DebugStrN("**********PHASE_AAD (%d bytes + padding):\n",aadLeft);
                    tmp[0]  = aadPtr[0] & BSWAP(MASK_TAB[aadLeft % 4]);
                    srcPtr  = dstPtr = tmp; /* encrypt in place in temp buffer */
                    wCnt    = 1;            /* just one word */
                    aadLeft = 0;            /* done with AAD after this */
                    }
                encBlk  = 1;
                break;
            case PHASE_DATA:
                DebugStr("**********PHASE_DATA:\n");
                pc.cs.Z[1] ^= pc.cs.aadXor;
                srcPtr  = (c32p) src;
                dstPtr  = (u32p) dst;
                wCnt    = msgLen/4;         /* number of full words of source */
                encBlk  = doEncrypt;
                break;
            case PHASE_ODD_BYTES:
                if ((msgLen % 4) == 0)      /* are there any bytes left mod 4? */
                    continue;               /* if not, just start next phase */
                DebugStr("**********PHASE_ODD_BYTES:\n");
                tmp[0]  = srcPtr[0] & BSWAP(MASK_TAB[msgLen % 4]);
                srcPtr  = dstPtr = tmp;     /* encrypt in place in temp buffer */
                wCnt    = 1;
                encBlk  = doEncrypt;
                break;
            case PHASE_MAC_GEN:
                DebugStr("**********PHASE_MAC_GEN:\n");
                k = msgLen % 4;
                if (k)
                    {   /* first, do we need "cleanup" for odd byte results? */
                    dstPtr  = (u32p) dst;
                    tmp[0] &= BSWAP(MASK_TAB[k]);
                    dstPtr[msgLen/4] ^= (tmp[0] ^ dstPtr[msgLen/4]) & BSWAP(MASK_TAB[k]);
                    }
                pc.cs.Z[0] ^= MAC_MAGIC_XOR;/* magic XOR constant for MAC computation */
                pc.cs.Z[4] ^= aadLen;       /* only 32-bit aadLen used here */
                wCnt = MAC_INIT_CNT + MAC_WORD_CNT;
                k    = BSWAP(k);
                for (j=0;j<wCnt;j++)        /* use plaintext = (msgLen mod 4) */
                    tmp[j] = k;
                srcPtr  = dstPtr = tmp;
                encBlk = 1;                 /* use encrypt block function */
                break;
            default:
                assert(0);                  /* should never come here */
                wCnt    = 0;                /* (avoid uninitialized var compiler warnings */
                srcPtr  = NULL;
                dstPtr  = NULL;
            }
        /* here to do some encryption (wCnt words, using srcPtr, dstPtr) */
        for (;wCnt;wCnt--,srcPtr++,dstPtr++,i++)
            {
            j = i & 7;

            Phelix_H0(pc.cs.Z,0);
            DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                       pc.cs.oldZ[i&3],pc.ks.X_0[j]);

            Phelix_H1(pc.cs.Z,pc.ks.X_0[j]);
            keyStream = pc.cs.Z[4] + pc.cs.oldZ[i&3];
            if (encBlk)
                {   /* encryption block (used in some phases of decryption operation) */
                plainText   = BSWAP(*srcPtr);
                cipherText  = keyStream ^ plainText;
                *dstPtr     = BSWAP(cipherText);
                }
            else
                {   /* decryption block (only used during decryption operations) */
                plainText   = BSWAP(*srcPtr) ^ keyStream;
                if (phase == PHASE_ODD_BYTES)   /* extend ciphertext to full 32 bits */
                    plainText &= MASK_TAB[msgLen % 4];
                *dstPtr     = BSWAP(plainText);
                }
            DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

            Phelix_H0(pc.cs.Z,plainText);
            DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

            Phelix_H1(pc.cs.Z,pc.ks.X_1[j]+i);
            DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
            DebugStr("\n");

            pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
            }
        }
    /* done with encrypting/decrypting. Ouput/check MAC at tmp[MAC_INIT_CNT] */
    PutMac(mac,&(tmp[MAC_INIT_CNT]),pc.ks.macSize);
    }

void PhelixEncryptPacket(PhelixPacketParms)
	{
	PhelixProcessPacket(0,ctx,nonce,aad,aadLen,src,dst,msgLen,mac);
	}

void PhelixDecryptPacket(PhelixPacketParms)
	{
	PhelixProcessPacket(1,ctx,nonce,aad,aadLen,src,dst,msgLen,mac);
	}

u32b PhelixProcessPacket_CodeSize(void)
    {
#if defined(_MSC_VER) | defined(__STDC__)
    return 0; /* MSVC42 doesn't work here for some reason */
#else
    return ((u08b *) PhelixProcessPacket_CodeSize) - ((u08b *) PhelixProcessPacket);
#endif
    }
