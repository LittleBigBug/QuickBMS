#include <stdlib.h>
#include <string.h>
//#include <windows.h>

typedef unsigned char   BYTE;
typedef int             BOOL;
typedef int             HRESULT;
#define TRUE            1
#define FALSE           0
#define S_OK ((HRESULT)0x00000000L)
#define E_OUTOFMEMORY ((HRESULT)0x80000002L)
#define E_FAIL ((HRESULT)0x80000008L)
#define ZeroMemory(MEM,SIZE)      memset(MEM,0,SIZE)



/* Copyright (c) Microsoft Corporation. All rights reserved. */

// MRCI.cpp
//

//
// 6/00 updated to remove use of globals and to test for corrupt
// input buffers during decompression.  Portions marked $Review were reviewed by 
// developer

// $Review...
// Implementation notes from developer with transcriptions:
//
//   -- The MRCI2 alrorithm (implemented here) is based on LZ77.  It
//      compresses 3-byte (or greater) repeated sequences.
//
//   -- The compressed stream is conceptually a series of bits, organized
//      into tokens.  A token may be:
//        -- 0xxxxxxx (8 bits): decompresses into literal byte 0xxxxxxx
//           (in the range 0x00 to 0x7F);
//        -- 11xxxxxxx (9 bits): decompresses into literal byte 1xxxxxxx
//           (in the range 0x80 to 0xFF);
//        -- 100yyyyyy (9 bits) followed by a "byte count" (see below): 
//           decompresses to the previously-output substring located yyyyyy
//           bytes (yyyyyy is a 6-bit "small displacement") before the end of
//           the current output buffer;
//        -- 1010yyyyyyyy (12 bits) followed by a "byte count" (see below): 
//           decompresses to the previously-output substring located yyyyyyyy
//           bytes (yyyyyyyy is an 8-bit "medium displacement") before the end
//           of the current output buffer;
//        -- 1011yyyyyyyyyyyy (16 bits) followed by a "byte count" (see below): 
//           decompresses to the previously-output substring located
//           yyyyyyyyyyyy bytes (yyyyyyyyyyyy is a 12-bit "big displacement")
//           before the end of the current output buffer.
//        -- 1011111111111111 (16 bits) = MAXDISPBIG: this is a special token
//           which indicates the end of a sector (a 512-byte block).
//
//   -- The "byte count" portion of a token specifies the length of the
//      matched substring.  The byte count is represented by N 0 bits,
//      followed by a 1 bit, followed by N bits that (partially) indicate
//      the byte count.  Specifically:
//        -- 1: means byte count is 3 (since 3 is the minimum byte count)
//        -- 01z: means byte count is 4 + z (e.g. 010 = 4-byte match, 011 = 5)
//        -- 001zz: means byte count is 6 + zz (i.e. 6, 7, 8, or 9 byte match)
//        -- 0001zzz: means byte count is 10 + zzz (i.e. 10 to 17 byte match)
//        --   ...
//        -- 000000001zzzzzzzz: (i.e. 257 to 412 byte match)
//           $Review: is this the maximum?
//
//   -- Best possible theoretical compression: one literal (e.g. 0xxxxxxx)
//      followed by aa series of tokens of the form
//      11xxxxxxx00000000111111111 (26 bits), where xxxxxxx is anything;
//      this 26-bit token specifies a run of 513 bytes, yielding an effective
//      compression ratio of about 157.5 to 1
//
//      (Developer says: "I would expect a run length of 9 would be needed to get
//      a MAXMATCH of 512.  8 bits would only get you to 257 or so.")
//
//   -- If the byte count is greater than the displacement (e.g. byte count
//      is 7, displacement is 2 pointing to substring "AB") then the matched
//      substring is repeated as many times as necessary to complete
//      (resulting in "ABABABA" in the previous example).
//

//#include "StdAfx.h"
#include "GrowBuf.h"

#define ASSERT(f)          ((void)0)

// optimize for speed
#pragma optimize("t", on)


//////////////////////////////////////////////////////////////////////////////
// Macros for internal use
//

#define BITMASK(x)      ((1 << x) - 1)  /* returns lower 'x' bits set */

#define LOGDISPSMALL    (6)             /* Number of bits in small disp */
#define LOGDISPMED      (8)             /* Number of bits in medium disp */
#define LOGDISPBIG      (12)            /* Number of bits in big displacement */

#define MAXDISPSMALL    ((1 << LOGDISPSMALL) - 1)
                                        /* Maximum small displacement */
#define MAXDISPMED      ((1 << LOGDISPMED) + MAXDISPSMALL)
                                        /* Maximum medium displacement */
#define MAXDISPBIG      ((1 << LOGDISPBIG) + MAXDISPMED)
                                        /* Maximum big displacement */

#define MINDISPSMALL    (0)             /* Minimum small displacement */
#define MINDISPMED      (MAXDISPSMALL + 1)
                                        /* Minimum medium displacement */
#define MINDISPBIG      (MAXDISPMED + 1)/* Minimum big displacement */

#define MAXOUTPERTOKEN  (513 + 3/*?*/)  /* max. uncomp. bytes per comp. token */



//////////////////////////////////////////////////////////////////////////////
// CMRCI Declaration
//
// Maintains internal compression/decompression state.
//

class CMRCI
{
// state
protected:
    unsigned m_abits;                  /* Array of bits */
    unsigned m_cbitsleft;              /* Number of bits in m_abits */
    unsigned m_cCompressed;            /* # bytes remaining @ m_pCompressed* */
    BOOL     m_fCorrupted;             /* Compressed input file corrupted? */

// state used only during compression
protected:
    unsigned char *m_pCompressedRW;    /* read-write pointer into cmp. data */

// state used only during decompression
protected:
    const unsigned char *m_pCompressed; /* read-only ptr into compressed data */

// construction
public:
    CMRCI() { ZeroMemory(this, sizeof(*this)); }

// public functions
public:
    HRESULT Mrci2Decompress(const unsigned char *pchin,unsigned cchin,
        TGrowBuf<BYTE> *pbufOut,unsigned cbOut);

// private helper functions
protected:
    unsigned getbit();
    unsigned getbits(unsigned cbits);
    void expandstring(unsigned char **ppchout,unsigned disp, unsigned cb);
};


//////////////////////////////////////////////////////////////////////////////
// CMRCI Implementation
//


/*
 *  (decompress) Get a single bit from the compressed input stream.
 */

unsigned CMRCI::getbit()
{
    unsigned bit;                       /* Bit */

    if (m_cbitsleft)                      /* If bits available */
    {
        m_cbitsleft--;                    /* Decrement bit count */

        bit = m_abits & 1;                /* Get a bit */

        m_abits >>= 1;                    /* Remove it */
    }
    else                                /* no bits available */
    {
        if (m_cCompressed == 0)
        {
            /* corrupted file */
            m_fCorrupted = TRUE;
            return 0;
        }

        m_cCompressed--;

        m_cbitsleft = 7;                  /* Reset count */

        m_abits = *m_pCompressed++;         /* Get a byte */

        bit = m_abits & 1;                /* Get a bit */

        m_abits >>= 1;                    /* Remove it */
    }

    return(bit);                        /* Return the bit */
}


/*
 *  (decompress) Get multiple bits from the compressed input stream.
 */

unsigned CMRCI::getbits(unsigned cbits)
{
    unsigned bits;                      /* Bits to return */
    unsigned cbitsdone;                 /* number of bits added so far */
    unsigned cbitsneeded;               /* number of bits still needed */

    if (cbits <= m_cbitsleft)             /* If we have enough bits */
    {
        bits = m_abits;                   /* Get the bits */
        m_cbitsleft -= cbits;             /* Decrement bit count */
        m_abits >>= cbits;                /* Remove used bits */
    }
    else                                /* If we'll need to read more bits */
    {
        bits = 0;                       /* No bits set yet */
        cbitsdone = 0;                  /* no bits added yet */
        cbitsneeded = cbits;            /* bits needed */

        do
        {
            if (m_cbitsleft == 0)         /* If no bits ready */
            {
                if (m_cCompressed == 0)
                {
                    /* corrupted file */
                    m_fCorrupted = TRUE;
                    return 0;
                }

                m_cCompressed--;

                m_cbitsleft = 8;          /* Reset count */

                m_abits = *m_pCompressed++;  /* Get 8 new bits */
            }

            bits |= (m_abits << cbitsdone);  /* copy bits for output */

            if (m_cbitsleft >= cbitsneeded)  /* if enough now */
            {
                m_cbitsleft -= cbitsneeded;  /* reduce bits remaining available */
                m_abits >>= cbitsneeded;  /* discard used bits */
                break;                  /* got them */
            }
            else                        /* if not enough yet */
            {
                cbitsneeded -= m_cbitsleft;  /* reduce bits still needed */
                cbitsdone += m_cbitsleft;  /* increase shift for future bits */
                m_cbitsleft = 0;          /* reduce bits remaining available */
            }
        } while (cbitsneeded);          /* go back if more bits needed */
    }

    return(bits & BITMASK(cbits));      /* Return the bits */
}


/*
 *  (decompress) Expand a match.
 *
 *  Note: source overwrite is required (so we can't memcpy or memmove)
 */

void CMRCI::expandstring(unsigned char **ppchout,unsigned disp,
        unsigned cb)
{
    unsigned char *source;
    unsigned char *target;

    ASSERT(cb != 0);

    target = *ppchout;                  /* where the bytes go */
    source = target - disp;             /* where the bytes come from */

    *ppchout += cb;                     /* Update the output pointer */

    while (cb--)
    {
        *target++ = *source++;
    }
}


/*
 *  (MRCI2) Decompress
 */

HRESULT CMRCI::Mrci2Decompress(const unsigned char *pchin,unsigned cchin,
    TGrowBuf<BYTE> *pbufOut,unsigned cbOut)
{
    unsigned length;                    /* Length of match */
    unsigned disp;                      /* Displacement */
    unsigned char *pchout;          /* Output buffer pointer */
    unsigned char *pchoutLimit;     /* Max. that pchout be written to */


    m_abits = 0;                          /* Bit buffer is empty */
    m_cbitsleft = 0;                      /* No bits read yet */
    m_pCompressed = pchin;                /* setup source pointer */
    m_cCompressed = cchin;                /* setup source counter */

	/* no input bytes --> no output bytes */
	if (cchin == 0)
		return S_OK;

    /* allocate output buffer; allocate MAXOUTPERTOKEN more than we think
       we need, so that we don't have to test for an out-of-buffer condition
       on every function call */
    unsigned cbAlloc = cbOut + MAXOUTPERTOKEN;
    if (!pbufOut->Need(cbAlloc))
        return E_OUTOFMEMORY;

    /* keep track of initial number of bytes in pbufOut (doesn't include
       the cbAlloc bytes allocated above */
    BYTE *pbInit = pbufOut->End();

    pchout = pbufOut->End();            /* Point at output buffer */
    pchoutLimit = pchout + cbAlloc;     /* first byte past end of buffer */

    /* loop once per compressed token -- each iteration of this loop
       will write at most MAXOUTPERTOKEN bytes to the output buffer */
    for (;;)
    {
        /* Set end of output buffer */
        if (m_fCorrupted || (pchout >= pchoutLimit))
            return E_FAIL;              /* Corrupted input file */
        pbufOut->SetEnd(pchout);

        if (getbit() == 0)              /* literal 00..7F */
        {
            *pchout++ = (unsigned char) getbits(7);

            continue;                   /* Next token */
        }

        if (getbit() == 1)              /* literal 80..FF */
        {
            *pchout++ = (unsigned char)(getbits(7) | 0x80);

            continue;                   /* Next token */
        }

        if (getbit() == 0)
        {
            disp = getbits(6) + MINDISPSMALL;
        }
        else
        {
            if (getbit() == 0)
            {
                disp = getbits(8) + MINDISPMED;
            }
            else
            {
                disp = getbits(12) + MINDISPBIG;
            }
        }

        if (disp == MAXDISPBIG)
        {
            if (m_cCompressed == 0)
            {
                break;                  // end of input data
            }
            else
            {
                continue;               // end of sector
            }
        }

        length = 0;                     /* Initialize */

        while ((getbit() == 0) && !m_fCorrupted)
        {
            length++;                   /* Count the leading zeroes */
        }

        // some corrupt input files could easily have arbitrarily large values of length, and in fact length
        // should not be more than 8, since 8 yields a maximum byte count of 512
        if (length > 8)
            return E_FAIL;              /* Corrupted input data */

        if (length)
        {
            length = getbits(length) + (1 << length) + 1;
        }
        else
        {
            length = 2;
        }

        /* check if displacement places source string before buffer start */
        if (int(disp) > (pchout - pbInit))
            return E_FAIL;              /* Corrupted input file */

        expandstring(&pchout,disp,length + 1);  /* Copy the match */
    }

    /* Set end of output buffer */
    if (m_fCorrupted || (pchout > pchoutLimit))
        return E_FAIL;              /* Corrupted input file */
    pbufOut->SetEnd(pchout);

    /* check that the uncompressed size matches expected size <cbOut> */
    if (pbufOut->End() - pbInit != int(cbOut))
        return E_FAIL;              /* Corrupted input file */

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Public Decompression Functions
//

// hr = MRCIDecompress(pb, cb, pbufOut, cbOut)
//
// Decompress buffer <pb> (containing <cb> bytes compressed using
// MRCICompress()).  Write the result to the end of buffer <pbufOut>.
//
// <cbOut> specifies the expected uncompressed buffer size.  (As an
// optimization, It's possible that <pbufOut> will be allocated up to
// MAXOUTPERTOKEN bytes larger, but on exit pbufOut->End() will point
// just past the last output byte.)  MRCIDecompress() uses <cbOut> to
// prevent corrupt or malicious data from generating arbitrarily large
// amounts of data in <pbufOut> (e.g. 1 byte of compressed data could
// expand to about 157.5 bytes of uncompressed data in the worst case)
//
// If the input data is corrupt, E_FAIL is returned.
//
HRESULT MRCIDecompress(const void *pb, unsigned cb, TGrowBuf<BYTE> *pbufOut,
    unsigned cbOut)
{
    CMRCI mrci;
    return mrci.Mrci2Decompress((BYTE *) pb, cb, pbufOut, cbOut);
}

// hr = MRCIDecompressWrapper(pb, cb, pOut, cbOut)
//
//
// A wrapper around the above function which takes pointers to the input and
// output buffers and constructs the necessary data structures to perform
// the decompression

// If the input data is corrupt, E_FAIL is returned.
//
extern "C"
HRESULT MRCIDecompressWrapper(const void *pb, int cb, const void *pOut,
    int cbOut)
{
	TGrowBuf<BYTE> pbufOut(cbOut);
    HRESULT retVal = MRCIDecompress(pb, (unsigned) cb, &pbufOut, (unsigned) cbOut);

	//Copy values in pbufOut into the output pointer

	BYTE* srcOut = pbufOut.Buf();
	BYTE* tgtOut = (BYTE*) pOut;
	for(unsigned i=0; i < cbOut; i++)
		*tgtOut++ = *srcOut++;

 	return retVal;

}

