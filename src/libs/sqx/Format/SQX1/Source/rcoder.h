/*********************************************************
RCODER.H

based on the carryless rangecoder by Mikael Lundqvist

*********************************************************/

#if !defined(__RCODER_H)   //- include this file only once per compilation
#define __RCODER_H

#if !defined(__CX_TYPE_H)   
#include "cx_type.h"
#endif

//- static or adaptive model?
#define  STATIC    0U
#define  ADAPT     1U

//- error codes 
#define RC_OK        0U
#define RC_ERROR     1U
#define RC_IO_ERROR  2U


#define  TOP       ((UWORD32)1 << 24)
#define  BOT       ((UWORD32)1 << 16)

typedef void (*RC_POST_INIT_PROC)(void *rcm);
typedef void (*RC_UPDATE_PROC)(void *rcm);


typedef struct 
{
	UWORD32  low;
	UWORD32  range;
	UWORD32  passed; 
	UWORD32  error;
	UWORD32  code;
}RANGE_CODER;

typedef struct 
{
	UWORD32				*freq;
	UWORD32				totFreq;
	UWORD32				incr;
	UWORD32				maxFreq;
	UWORD32				adapt;
	UWORD32				halfFreq;
	UWORD32				firstFreq; 
	UWORD32				lastFreq;
	UWORD32				lastSym;
	UWORD32				lastCumFreq;
	UWORD32				nsym;
	UWORD32				nsym2;
	UWORD32				nsym3; 
	UWORD32				nsym4; 
	UWORD32				nsym23;
}RC_MODEL;

static void RC_ModelFree(RC_MODEL *rcm);
static UWORD32 RC_ModelCreate(RC_MODEL *rcm,UWORD32 nsym,UWORD32 *ifreq,UWORD32 incr,UWORD32 maxFreq,UWORD32 adapt,RC_POST_INIT_PROC RC_PostInitProc,RC_UPDATE_PROC RC_UpdateProc);
static UWORD32 RC_ModelInit(RC_MODEL *rcm,UWORD32 nsym,UWORD32 *ifreq,UWORD32 incr,UWORD32 maxFreq,UWORD32 adapt);
static void RC_StartDecode(RANGE_CODER *rd);
static INT32 RC_DecodeSymbol(RANGE_CODER *rd,RC_MODEL *rdm);

#endif //- #if !defined(__RCODER_H)   

