/***************************************************
ZKengine.h	
ver: 3.00 (25 Jun 2006)


ZK-Crypt 

Submited by: Carmi Gressel et al    (carmi@fortressgb.com)
		FortressGB					(avi@fortressgb.com)

Response to Ecrypt call for eSTREAM Profile II (HW)

Code IS NOT OPTIMIZED for speed
***************************************************/

#ifndef ZKENGINE_H
#define ZKENGINE_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ecrypt-sync.h"
#include "ZKdef.h"



static const int nLFSR_Len[8] ={13,19,18,14,15,17, 16, 16}; /* register length */

#define DEF_nLFSR_13	0
#define DEF_nLFSR_19	1
#define DEF_nLFSR_18	2
#define DEF_nLFSR_14	3
#define DEF_nLFSR_15	4
#define DEF_nLFSR_17	5
#define DEF_nLFSR_S1	6
#define DEF_nLFSR_S2	7


/*
Register	Length	Feedback Taps			Slip (Left/Right)	Clock bit
R1,1		13		2,3,5,8,9				Left			Top clock
R1,2		19		1,4,6,7,8,9,11,14,16	Right			Top clock
R2,1 		18		2,4,6,7,10,11,12,13,15  Left			Middle clock
R2,2		14		1,4,5,8,10				Right			Middle clock
R3,1 		15		0,1,5,6,10				Left			Bottom clock
R3,2		17		1,4,7,9,10,12,13		Right			Bottom clock
SP 1		16		2,5,6,10,11,12			Prand			ALWAYS
SP 2		16		5,8,11					Prand			ALWAYS

*/

/* feedcbak masks TAPS */
static const DWORD nLFSR_Mask[8]={		
	(1<<0)+(1<<3)+(1<<4)+(1<<6)+(1<<9)+(1<<10),
	(1<<0)+(1<<2)+(1<<5)+(1<<7)+(1<<8)+(1<<9)+(1<<10)+(1<<12)+(1<<15)+(1<<17),
	(1<<0)+(1<<3)+(1<<5)+(1<<7)+(1<<8)+(1<<11)+(1<<12)+(1<<13)+(1<<14)+(1<<16),
	(1<<0)+(1<<2)+(1<<5)+(1<<6)+(1<<9)+(1<<11),
	(1<<0)+(1<<1)+(1<<2)+(1<<6)+(1<<7)+(1<<11),
	(1<<0)+(1<<2)+(1<<5)+(1<<8)+(1<<10)+(1<<11)+(1<<13)+(1<<14),
	(1<<0)+(1<<2)+(1<<5)+(1<<6)+(1<<10)+(1<<11)+(1<<12),
	(1<<0)+(1<<5)+(1<<8)+(1<<11)
			};

/* Splash tables */
static const BYTE hashTableA[32] = {9, 18, 5,11,22,12,30,19, 7,15,31,25,28,24, 6, 3,17,13,27,23, 1, 2,26,21, 4,20, 8,16, 0,14,10,29};
static const BYTE hashTableB[32] = {30,15, 6,12,25,18,16, 9,19, 7, 3,31, 0,29,27,21,14,28,24,17,23, 5,10, 2,11,22,13,26,20, 8, 4, 1};
static const BYTE hashTableC[32] = {19, 7,14,29, 3,27, 0,13,25,16,15,30,20, 1,26,31, 8, 6, 2, 4, 9,18,12,10,21,11,22, 5,24,23,28,17};


/*	PUBLIC	
****************/
	void ZKengine(ECRYPT_ctx* ctx, BOOL fZeroKeys);
	DWORD setupZKengine(ECRYPT_ctx* ctx);
	DWORD cycleZKengine(ECRYPT_ctx* ctx, DWORD in);

/*	PRIVATE 
***************/

 /* ZK-crypt INIT options */
	DWORD initOptions;

	/* nLFSR Bank	*/
	DWORD nLFSR_Bank(ECRYPT_ctx* ctx);
	DWORD runBank(ECRYPT_ctx* ctx, DWORD* ptrState, BYTE control, 
			int leftIndex, int rightIndex, int rotateNum, DWORD cipher, 
			DWORD maskLeftClockFB, DWORD maskRightClockFB);
	DWORD runSuperBank(ECRYPT_ctx* ctx, DWORD* ptrState, BYTE control,
			int leftIndex, int rightIndex, int rotateNum, DWORD cipher,
			DWORD maskLeftHashMask, DWORD maskRightHashMask);
	DWORD nLFSR_iterate(DWORD* ptrState, BYTE* ptrCtrl, BOOL fLeft, int index);
	


	/* Xor_N_Store	*/
	DWORD xor_N_Store(DWORD* prev, DWORD in);

	DWORD topXor_N_Store(ECRYPT_ctx* ctx, DWORD in);
	DWORD intrXor_N_Store(ECRYPT_ctx* ctx, DWORD in);
	DWORD botXor_N_Store(ECRYPT_ctx* ctx, DWORD in);


	/* Hash Matrix and EVNN filter	*/
	DWORD topHash(ECRYPT_ctx* ctx,DWORD in);
	DWORD botHash(ECRYPT_ctx* ctx,DWORD in);

	DWORD hashMix(DWORD in, const BYTE *table, int maxLen);
	DWORD runEVNN(ECRYPT_ctx* ctx, DWORD in, int pos);

	/* Feedback manipulator	*/

	DWORD feedbackStorage(ECRYPT_ctx* ctx, DWORD in);


	/* clocks	*/

	void cycleClock(ECRYPT_ctx* ctx);

	BYTE cycleTopBankClock(ECRYPT_ctx* ctx);
	BYTE cycleMidBankClock(ECRYPT_ctx* ctx);
	BYTE cycleBotBankClock(ECRYPT_ctx* ctx);

	void cycleHashCounter(ECRYPT_ctx* ctx);
	void setHashVector(ECRYPT_ctx* ctx);

	void cyclePRandomClock(ECRYPT_ctx* ctx, BOOL fClean);


	/* UTIL	*/
	BYTE  NFIX(DWORD in, int len);
	DWORD maj(DWORD ai, DWORD bi, DWORD ci);
	DWORD ROLn(DWORD in, int n);
	DWORD RORn(DWORD in, int n);
	BYTE  getBitAtLocation(DWORD in , DWORD loc);
	DWORD revereseBits(DWORD in, int len);


#endif /* #ifndef ZKENGINE_H */
