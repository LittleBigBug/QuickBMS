/*********************************************************
RCODER.C

based on the carryless rangecoder by Mikael Lundqvist

*********************************************************/

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

#if !defined(__CX_ERR_H)   
#include "cx_err.h"
#endif

#if !defined(__RCODER_H)   
#include "rcoder.h"
#endif

#if !defined(__ARCIO_H)   
#include "arcio.h"
#endif

#if !defined(__SQX_VARS_H)
#include "sqx_vars.h"
#endif

#if defined(_MSC_VER)
	#pragma warning(disable : 4146)
#endif

static 
void RC_ModelFree(RC_MODEL *rcm) 
{
	if( rcm->freq != NULL )
		free(rcm->freq);
	memset(rcm,0,sizeof(RC_MODEL));
}

static 
UWORD32 RC_ModelCreate(RC_MODEL *rcm,UWORD32 nsym,UWORD32 *ifreq,UWORD32 incr,UWORD32 maxFreq,UWORD32 adapt,RC_POST_INIT_PROC RC_PostInitProc,RC_UPDATE_PROC RC_UpdateProc)
{
UWORD32 i;

	rcm->nsym = nsym;  //- # of symbols
	//- nsym2 - nsym23 are just constants designed to speed things up
	rcm->nsym2 = nsym/2;
	rcm->nsym3 = nsym/3;
	rcm->nsym4 = nsym*5/22;  //- For firstFreq 
	rcm->nsym23 = nsym*2/3;  //- For lastFreq 
	
	rcm->freq = (UWORD32 *)malloc(nsym * sizeof(UWORD32));
	
	if( rcm->freq == NULL )
		return RC_ERROR;

	rcm->totFreq = 0;
	if (ifreq != NULL)
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (rcm->freq[i] = ifreq[i]);
	else
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (rcm->freq[i] = 1);
	
	//- Sums to help DecodeSymbol make better decisions 
	for (rcm->firstFreq = i = 0; i < rcm->nsym4; i++)
		rcm->firstFreq += rcm->freq[i];
	for (rcm->halfFreq = rcm->firstFreq, i = rcm->nsym4; i < rcm->nsym2; i++)
		rcm->halfFreq += rcm->freq[i];
	for (rcm->lastFreq = rcm->halfFreq, i = rcm->nsym2; i < rcm->nsym23; i++)
		rcm->lastFreq += rcm->freq[i];
	
	rcm->incr = incr;
	rcm->maxFreq = maxFreq;
	rcm->adapt = adapt;
	rcm->lastCumFreq = 0;
	rcm->lastSym = 0;
	
	//- Total frequency MUST be kept below 1 << 16 (= 65536) 
	if (rcm->maxFreq > BOT || rcm->totFreq > BOT)
		return RC_ERROR;
	
	return RC_OK;
}

static 
UWORD32 RC_ModelInit(RC_MODEL *rcm,UWORD32 nsym,UWORD32 *ifreq,UWORD32 incr,UWORD32 maxFreq,UWORD32 adapt) 
{
UWORD32 i;

	rcm->nsym = nsym;  //- # of symbols 
	//- nsym2 - nsym23 are just constants designed to speed things up
	rcm->nsym2 = nsym/2;
	rcm->nsym3 = nsym/3;
	rcm->nsym4 = nsym*5/22;  //- For firstFreq 
	rcm->nsym23 = nsym*2/3;  //- For lastFreq 
	
	rcm->totFreq = 0;
	if (ifreq != NULL)
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (rcm->freq[i] = ifreq[i]);
	else
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (rcm->freq[i] = 1);
	
	//- Sums to help DecodeSymbol make better decisions
	for (rcm->firstFreq = i = 0; i < rcm->nsym4; i++)
		rcm->firstFreq += rcm->freq[i];
	for (rcm->halfFreq = rcm->firstFreq, i = rcm->nsym4; i < rcm->nsym2; i++)
		rcm->halfFreq += rcm->freq[i];
	for (rcm->lastFreq = rcm->halfFreq, i = rcm->nsym2; i < rcm->nsym23; i++)
		rcm->lastFreq += rcm->freq[i];
	
	rcm->incr = incr;
	rcm->maxFreq = maxFreq;
	rcm->adapt = adapt;
	rcm->lastCumFreq = 0;
	rcm->lastSym = 0;
	
	//- Total frequency MUST be kept below 1 << 16 (= 65536)
	if (rcm->maxFreq > BOT || rcm->totFreq > BOT)
		return RC_ERROR;
	
	return RC_OK;
}


static 
void RC_StartDecode(RANGE_CODER *rd) 
{
UWORD32	uCode;
	rd->passed = rd->low = rd->code = 0;
	rd->range = (UWORD32)(-1);
	rd->error = RC_OK;
	
	uCode = NEEDBITS(8);
	DUMPBITS(8);
	rd->code = rd->code<<8 | uCode;

	uCode = NEEDBITS(8);
	DUMPBITS(8);
	rd->code = rd->code<<8 | uCode;

	uCode = NEEDBITS(8);
	DUMPBITS(8);
	rd->code = rd->code<<8 | uCode;

	uCode = NEEDBITS(8);
	DUMPBITS(8);
	rd->code = rd->code<<8 | uCode;
}

static 
void RC_Decode(RANGE_CODER *rd,RC_MODEL *rdm,UWORD32 cumFreq,UWORD32 sym) 
{
UWORD32	uCode;
	
	rd->low  += cumFreq * rd->range;
	rd->range *= rdm->freq[sym];
	while ( (rd->low ^ (rd->low+rd->range)) < TOP || (rd->range < BOT && ((rd->range = (-rd->low) & (BOT-1)),1)) )
	{	
		uCode = NEEDBITS(8);
		DUMPBITS(8);
		rd->code = rd->code<<8 | uCode;
		rd->range <<= 8;
		rd->low <<= 8;
	}
}

static 
INT32 RC_DecodeSymbol(RANGE_CODER *rd,RC_MODEL *rdm)
{
UWORD32 i, sym, cumFreq, rfreq;
	
	rfreq = (rd->code - rd->low) / (rd->range /= rdm->totFreq); //- GetFreq
	if (rfreq >= rdm->totFreq) 
	{
		rd->error = RC_ERROR;
		ArcStruct.ArchiveStatus = SQX_EC_BAD_FILE_FORMAT;
		rd->range = (-1);
		return -1;
	}
	
	//- Find symbol choosing the shortest possible path 
	if (rdm->lastCumFreq > rfreq)
		if (rfreq > rdm->halfFreq || (rdm->lastSym <= rdm->nsym23 && rfreq >= rdm->firstFreq)) {
			for (sym = rdm->lastSym-1, cumFreq = rdm->lastCumFreq-rdm->freq[sym];;) {
				if (cumFreq <= rfreq) break;
				cumFreq -= rdm->freq[--sym];
			}
			rdm->lastCumFreq = cumFreq;
			rdm->lastSym = sym;
		}
		else { //- if rfreq < halfFreq && (lastSym > nsym23 || rfreq < firstFreq)
			for (sym = 0, cumFreq = rdm->freq[0];;) {
				if (cumFreq > rfreq) break;
				cumFreq += rdm->freq[++sym];
			}
			cumFreq -= rdm->freq[sym];
			if (rdm->adapt) rdm->lastCumFreq += rdm->incr;
		}
	else //- lastCumFreq <= rfreq
		if (rfreq < rdm->halfFreq || (rdm->lastSym >= rdm->nsym3 && rfreq <= rdm->lastFreq)) {
			for (sym = rdm->lastSym, cumFreq = rdm->lastCumFreq + rdm->freq[sym];;) {
				if (cumFreq > rfreq) break;
				cumFreq += rdm->freq[++sym];
			}
			cumFreq -= rdm->freq[sym];
			rdm->lastCumFreq = cumFreq;
			rdm->lastSym = sym;
		}
		else //- if rfreq > halfFreq && (rdm->lastSym < rdm->nsym3 || rfreq > lastFreq) 
			for (sym = rdm->nsym-1, cumFreq = rdm->totFreq-rdm->freq[sym];;) {
				if (cumFreq <= rfreq) break;
				cumFreq -= rdm->freq[--sym];
			}
	
	RC_Decode(rd, rdm, cumFreq, sym);
	
	//- Update statistics 
	if (rdm->adapt) {
		rdm->freq[sym] += rdm->incr;
		if (sym < rdm->nsym23) {
			rdm->lastFreq += rdm->incr;
			if (sym < rdm->nsym2) {
				rdm->halfFreq += rdm->incr;
				if (sym < rdm->nsym4)
					rdm->firstFreq += rdm->incr;
			}
		}
		rdm->totFreq += rdm->incr;
		if (rdm->totFreq > rdm->maxFreq) {
			for (i = rdm->totFreq = 0; i < rdm->nsym; i++)
				rdm->totFreq += (rdm->freq[i] -= (rdm->freq[i] >> 1));
			rdm->halfFreq /= 2;
			rdm->firstFreq /= 2;
			rdm->lastFreq /= 2;
			rdm->lastCumFreq = 0;
			rdm->lastSym = 0;
		}
	}
	
	return sym;
}

#if defined(_MSC_VER)
	#pragma warning(default : 4146)
#endif
