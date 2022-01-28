/***************************************************
ZKengine.c	
ver: 3.00 (25 Jun 2006)


ZK-Crypt 

Submited by: Carmi Gressel et al    (carmi@fortressgb.com)
		FortressGB					(avi@fortressgb.com)

Response to Ecrypt call for eSTREAM Profile II (HW)

Code IS NOT OPTIMIZED for speed
***************************************************/


#include "ZKengine.h"


/*********************************************************************
/* Construction
*********************************************************************/

void ZKengine(ECRYPT_ctx* ctx, BOOL fZeroKeys)
{
	
	
	/* Set up variables	*/
	if (fZeroKeys == TRUE)
	{
		ctx->upCphWrd_1 =0;
		ctx->upCphWrd_2 =0;
		ctx->upCphWrd_3 =0;
		ctx->upCphWrd_4 =0;
		ctx->upCphWrd_5 =0;

		ctx->upIV_1 = 0;
		ctx->upIV_2 = 0;
		ctx->upIV_3 = 0;
		ctx->upIV_4 = 0;
		ctx->upIV_5 = 0;

		ctx->keysize = 0;
	}


	/* State variables	*/
	ctx->sttSuper = 0;
	ctx->sttTopBank =0;
	ctx->sttMidBank =0;
	ctx->sttBotBank =0;
	ctx->sttFeedBack =0;
	ctx->sttST_FeedBack =0;	
	ctx->fbA =0;
	ctx->fbB =0;
	ctx->fbC =0;
	ctx->fbD =0;

	ctx->sttTopXorNStore = 0;
	ctx->sttIntrXorNStore = 0;
	ctx->sttBotXorNStore = 0;
	ctx->sttTopHash = 0;
	ctx->sttBotHash = 0;

	ctx->sttClockFeedBack = 0;
	ctx->sttClockFeedBack_next = 0;

	/* controll variables	*/
	ctx->ctrlSuperBank =0;
	ctx->ctrlTopBank =0;
	ctx->ctrlMidBank =0;
	ctx->ctrlBotBank =0;

	ctx->ctrlFeedBack =0;


	ctx->ctrlTopHashMatrix = 0;
	ctx->ctrlBotHashMatrix = 0;

	ctx->ctrlClocks = 0;

	ctx->topBankClock_nLFR = 0;	/* 3 bits */
	ctx->topBankClockCounter = 0;	/* 4 bits */
	ctx->topBankClock_MC = 0;		/* 1 bit */

	ctx->midBankClock_nLFR = 0;	/* 5 bits */
	ctx->midBankClockCounter = 0;	/* 4 bits */
	ctx->midBankClock_MC = 0;		/* 1 bit */

	ctx->botBankClock_nLFR = 0;	/* 6 bits */
	ctx->botBankClockCounter = 0;	/* 4 bits */
	ctx->botBankClock_MC = 0;		/* 1 bit */

	ctx->hashCounter = 0;			/* 2 bits */

	ctx->longPClock = 0;			/* 9 bits */
	ctx->shortPClock = 0;			/* 2 bits */


}


/*********************************************************************
/*		ECRYPT Functions
*********************************************************************/

void ECRYPT_init(void) 
{

/*********************
	
	  DEFAULT set up
	  ==============
	  2 out of 3 tiers always on
	  use Super tier
	  Hash Table not locked

	
**********************/

	initOptions = ZK_OPTI_SUPER;

}

void ECRYPT_init_X(ECRYPT_ctx* ctx, u32 options)
{
	/*
#define ZK_OPTI_DEFUALT			0x00
#define ZK_OPTI_SUPER			0x01
#define ZK_OPTI_LEGACY			0x02

options = 0		DEFAULT - use super tier
options = 1		Without Super tier
options = 2		use mobile set (Hash locked on D and superTier)
options = 3		
	*/
	switch (options)
	{
	case 0:
	default:
		initOptions = ZK_OPTI_SUPER;
		break;

	case 1:
		initOptions = 0;
		break;

	case 2:
		initOptions = ZK_OPTI_LEGACY | ZK_OPTI_SUPER;
		break;


	}
}

void ECRYPT_keysetup(ECRYPT_ctx* ctx, const u8* key,  u32 keysize, u32 ivsize)
{
	/* Clear all state buffers including keys and IV*/
	ZKengine(ctx, TRUE);

	ctx->keysize = keysize;
	ctx->ivsize = ivsize;

	memcpy(&ctx->upCphWrd_1, &key[0], 4);
	memcpy(&ctx->upCphWrd_2, &key[4], 4);
	memcpy(&ctx->upCphWrd_3, &key[8], 4);
	memcpy(&ctx->upCphWrd_4, &key[12], 4);

	/* LOAD 5th word	*/
	if (keysize == 160)
	{
		memcpy(&ctx->upCphWrd_5, &key[16], 4);
	}
}

void ECRYPT_ivsetup(ECRYPT_ctx* ctx, const u8* iv)
{
	/* clear all the state buffers, but not the keys */
	ZKengine(ctx, FALSE);

	memcpy(&ctx->upIV_1, &iv[0], 4);
	memcpy(&ctx->upIV_2, &iv[4], 4);
	memcpy(&ctx->upIV_3, &iv[8], 4);
	memcpy(&ctx->upIV_4, &iv[12], 4);
	memcpy(&ctx->upIV_5, &iv[16], 4);

	setupZKengine(ctx);
}


void ECRYPT_process_bytes(int action, /* 0 = encrypt; 1 = decrypt; */
	ECRYPT_ctx* ctx, const u8* input, u8* output, u32 msglen)
	/* Message length in bytes. */ 
{
	/* action - ignored there is no difference between encryption and decryption */

	u32 lclLen = 0;
	u8	lclDif = 0;
	u32 in = 0;
	u32 out = 0;

	if (msglen>4)
	{
		for (lclLen = 0; lclLen<(msglen-4); lclLen+=4)
		{
			memcpy(&in, &input[lclLen], 4);
			out = cycleZKengine(ctx, in);
			memcpy(&output[lclLen], &out, 4);
		}
	}


	/* PAD if last 4 bytes are not full pad with 0's */
	lclDif = msglen - lclLen;
	if (lclDif > 0 && lclDif <= 4)
	{
		memset(&in,0,4);
		memcpy(&in,&input[lclLen],(lclDif));
		out = cycleZKengine(ctx, in);
		memcpy(&output[lclLen], &out, 4);
	}
}


void ECRYPT_keystream_bytes(ECRYPT_ctx* ctx, u8* keystream, u32 length)
			                /* Length of keystream in bytes. */
{
	u32 lclLen = length -4;
	u32 i = 0;
	u32 out = 0;

	for (i = 0; i <= lclLen; i+=4)
	{
		out = cycleZKengine(ctx, 0x00000000);
		memcpy(&keystream[i], &out, 4);
	}
}





/******************************************************************

	ZK-Crypt base routines

******************************************************************/



DWORD setupZKengine(ECRYPT_ctx* ctx)
{
	DWORD res =0;
	int i;
	int keyBlockCount = 0;
	int ivBlockCount = 0;

	/* setup operation mode	*/
	ctx->ctrlFeedBack = CTRL_FEEDBACK_RESET;
	feedbackStorage(ctx, 0);
	ctx->ctrlTopHashMatrix = 0;
	ctx->ctrlBotHashMatrix = 0;

	ctx->ctrlSuperBank = ctx->ctrlSuperBank |(CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlTopBank = ctx->ctrlTopBank | (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlMidBank = ctx->ctrlMidBank | (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlBotBank = ctx->ctrlBotBank | (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlClocks = ctx->ctrlClocks | CTRL_CLOCKS_LOAD;

	/* turn off output of results but use MAC	*/
	ctx->ctrlFeedBack = CTRL_FEEDBACK_ENABLE | CTRL_FEEDBACK_MAC;

	/* insert key into all key registers	*/
	nLFSR_Bank(ctx);
	cycleClock(ctx);
	ctx->ctrlSuperBank = ctx->ctrlSuperBank ^ (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlTopBank = ctx->ctrlTopBank ^ (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlMidBank = ctx->ctrlMidBank ^ (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlBotBank = ctx->ctrlBotBank ^ (CTRL_NLFSR_LOAD | CTRL_NLFSR_CLOCK);
	ctx->ctrlClocks = ctx->ctrlClocks & (~CTRL_CLOCKS_LOAD);

	/* Key size greater than 128 */
	keyBlockCount = (ctx->keysize - 128) / 32;
	if (keyBlockCount == 1)
		cycleZKengine(ctx, ctx->upCphWrd_5);


	/* run engine in no-report mode 4 times	*/
	for (i = 0; i<4; i++)
	{
		cycleZKengine(ctx, 0x0000);
	}

	/* feed in the IV through the MAC process	*/
	cycleZKengine(ctx, ctx->upIV_1);
	cycleZKengine(ctx, ctx->upIV_2);
	cycleZKengine(ctx, ctx->upIV_3);
	cycleZKengine(ctx, ctx->upIV_4);
	ivBlockCount = (ctx->ivsize - 128) / 32;
	if (ivBlockCount == 1)
		cycleZKengine(ctx, ctx->upIV_5);

	

	/* run engine in no-report mode 16 cycles to ensure full difusion
		of all key bits	*/
	for (i = 0; i<16; i++)
	{
		cycleZKengine(ctx, 0x0000);
	}

/*	if ((initOptions & ZK_OPTI_LEGACY) > 0)
	{
		/*	Clear the feedback *
		ctx->ctrlFeedBack = CTRL_FEEDBACK_RESET;
		feedbackStorage(ctx, 0);
		/*	set for output *
		ctx->ctrlFeedBack = CTRL_FEEDBACK_OUTPUT;
	}
	else*/
	{
		/* turn output back on and change to cipher feedback mode	*/
		ctx->ctrlFeedBack = CTRL_FEEDBACK_ENABLE | CTRL_FEEDBACK_CIPH | CTRL_FEEDBACK_OUTPUT;
	}



	
	return res;
}

DWORD cycleZKengine(ECRYPT_ctx* ctx, DWORD in)
/*	00B	&	00S	&	00F	&	1S	*/
{
	DWORD res = 0;
	DWORD lclMask = 0;
	static DWORD maskInversFeedBackClockBits = ~(CTRL_CLOCKS_FB_JUGG | CTRL_CLOCKS_FB_4TH | CTRL_CLOCKS_FB_DAS);
 
	lclMask = nLFSR_Bank(ctx);
	ctx->sttTest = lclMask; /* DEBUG */

	ctx->fbA = lclMask = topXor_N_Store(ctx, lclMask); 
	ctx->fbB = lclMask = topHash(ctx, lclMask);
	ctx->fbC = lclMask = intrXor_N_Store(ctx, lclMask);
	ctx->fbD = lclMask = botHash(ctx, lclMask);
	lclMask = botXor_N_Store(ctx, lclMask);

	res = in ^ lclMask;
	feedbackStorage(ctx, res);
	

	cycleClock(ctx);

	/* In HW the clock cycle (Left Hand Side) happens in parrallel to the nLFSR Bank
		run and the Data Churn (the Right Hand Side). 
		So changes in the feedback due to the 'RHS' should be propogated
		only after the clock cycle was executed */

	ctx->sttClockFeedBack = ctx->sttClockFeedBack_next;
	ctx->sttClockFeedBack_next = 0;


	if ((ctx->ctrlFeedBack & CTRL_FEEDBACK_OUTPUT) == 0)
	{
		res = 0;
	}

	return res;
}



/********************************************************************************
*********************************************************************************


		PRIVATE


 ********************************************************************************
 ********************************************************************************/

/* nLFSR banks	*/

DWORD nLFSR_Bank(ECRYPT_ctx* ctx)
{
	DWORD res= 0;
	DWORD tmpSprBank, tmpTopBank, tmpMidBank, tmpBotBank;
	DWORD tmpMajBank, tmpRolBank;
	BYTE lclCtrlBank = 0;


/*	Super Bank 28L	&	29R	&	30	*/
	lclCtrlBank = ctx->ctrlSuperBank | CTRL_NLFSR_FB;
	if ((initOptions & ZK_OPTI_SUPER) > 0)
		tmpSprBank = runSuperBank(ctx, &(ctx->sttSuper), lclCtrlBank, DEF_nLFSR_S1, DEF_nLFSR_S1,
								7, ctx->upCphWrd_5,  CTRL_CLOCKS_FB_THSH08, CTRL_CLOCKS_FB_THSH18); 

/* 19C	& 24C	& 25S	*/
	lclCtrlBank = ctx->ctrlTopBank | CTRL_NLFSR_FB;
	tmpTopBank = runBank(ctx, &(ctx->sttTopBank), lclCtrlBank, DEF_nLFSR_13, DEF_nLFSR_19,
								1, ctx->upCphWrd_2, CTRL_CLOCKS_FB_Q12, CTRL_CLOCKS_FB_Q18);
	
/*	20C	& 23C	& 26S	*/
	lclCtrlBank = ctx->ctrlMidBank | CTRL_NLFSR_FB;
	tmpMidBank = runBank(ctx, &(ctx->sttMidBank), lclCtrlBank, DEF_nLFSR_18, DEF_nLFSR_14,
								3, ctx->upCphWrd_3, CTRL_CLOCKS_FB_Q17, CTRL_CLOCKS_FB_Q13); 
	
/*	21C	& 22C	& 27S	*/
	lclCtrlBank = ctx->ctrlBotBank | CTRL_NLFSR_FB;
	tmpBotBank = runBank(ctx, &(ctx->sttBotBank), lclCtrlBank, DEF_nLFSR_15, DEF_nLFSR_17,
								5, ctx->upCphWrd_4, CTRL_CLOCKS_FB_Q14, CTRL_CLOCKS_FB_Q16);


	tmpMajBank = maj (tmpTopBank, tmpMidBank, tmpBotBank);
	tmpRolBank = RORn(tmpMajBank, 5);
	res = tmpMajBank ^ tmpRolBank;

	/* If using SUPER tier */
	if ((initOptions & ZK_OPTI_SUPER) > 0)
		res = res ^ tmpSprBank;


	return res;
}

DWORD runBank(ECRYPT_ctx* ctx, DWORD* ptrState, BYTE control, int leftIndex, int rightIndex, 
						int rotateNum, DWORD cipher, DWORD maskLeftClockFB, DWORD maskRightClockFB)
{
	DWORD res = 0;
	DWORD image = 0;
	DWORD left, right;
	int	lenLeft = nLFSR_Len[leftIndex];
	int lenRight = nLFSR_Len[rightIndex];

	if (control & CTRL_NLFSR_LOAD)
	{	
		/* loads are reveresed LSB is loaded into MSB	*/
		DWORD tmpCipher = revereseBits(cipher, 32);
		/* load new cipher - all inverted apart from LSbit of each bank (left right)	*/
		/* invert two bits back @ 0 and @ LSB of right hand side 	*/
		DWORD mask = (1<<lenLeft) + 0x0001;
		*ptrState = (~tmpCipher) ^ mask;
	}
	else
	{
		if (control & CTRL_NLFSR_CLOCK)
		{
			/* xor with feedback	*/
			if (control & CTRL_NLFSR_FB)
			{
				*ptrState = *ptrState ^ ctx->sttFeedBack;
			}
			left = nLFSR_iterate(ptrState, &control, TRUE, leftIndex);
			right = nLFSR_iterate(ptrState, &control, FALSE, rightIndex);

			ctx->sttClockFeedBack_next &= ~maskLeftClockFB;
			ctx->sttClockFeedBack_next &= ~maskRightClockFB;
			if (GETBIT_COUNT (left, lenLeft) == 1)
				ctx->sttClockFeedBack_next |= maskLeftClockFB;
			if (GETBIT_COUNT (right, lenRight) == 1)
				ctx->sttClockFeedBack_next |= maskRightClockFB;

			*ptrState = res = left | (right <<lenLeft);
		}
		else
		{
			res = *ptrState;
		}
		
		if (control & CTRL_NLFSR_BROWN)
		{
			image = ROLn(res, rotateNum);
			res = res ^ (~image); 
		}
		else
			return res;
		

	}

	return res;
}


DWORD runSuperBank(ECRYPT_ctx* ctx, DWORD* ptrState, BYTE control, int leftIndex, int rightIndex,
				int rotateNum, DWORD cipher, DWORD maskLeftHashMask, DWORD maskRightHashMask)
{
	DWORD res = 0;
	DWORD image = 0;
	DWORD left, right;
	int	lenLeft = nLFSR_Len[leftIndex];
	int lenRight = nLFSR_Len[rightIndex];

	BOOL fSlip;
	int iLen;
	DWORD mask;
	DWORD lclState;
	BOOL fTopMost;
	BOOL fNfix;


	if (control & CTRL_NLFSR_LOAD)
	{	
		/* loads are reveresed LSB is loaded into MSB	*
		DWORD tmpCipher = revereseBits(cipher, 32);
		/* load new cipher - all inverted apart from LSbit of each bank (left right)	*
		/* invert two bits back @ 0 and @ LSB of right hand side 	*
		DWORD mask = (1<<lenLeft) + 0x0001;
		*ptrState = (~tmpCipher) ^ mask;
		*/
		*ptrState = 0;



	}
	else
	{
		if ((ctx->ctrlClocks & CTRL_PRND_CLOCK) > 0) 
		{
			fSlip = TRUE;
		}
		else
		{
			fSlip = FALSE;
		}

		/* xor with feedback	*/
		if (control & CTRL_NLFSR_FB)
		{
			*ptrState = *ptrState ^ ctx->sttST_FeedBack; /* ctx->sttFeedBack; */
		}

		/*left = nLFSR_iterate(ptrState, &control, TRUE, leftIndex);*/
		iLen = nLFSR_Len[leftIndex];
		mask = ((1<<iLen)-1);
		lclState = *ptrState & mask;
		fTopMost = (BYTE)((lclState >> (iLen-1)) & 0x01);
		fNfix = NFIX(lclState, iLen);

		lclState = (lclState << 1) & mask;
		if (((fTopMost /*^ fSlip */ ^ fNfix) & BIT_ONE) == TRUE) /* tap is on	*/
		{
			lclState = lclState ^ nLFSR_Mask[leftIndex]; /* tap sequence	*/
			lclState = lclState | 0x0001;			/* LSB should be 1	*/
		}
		left = lclState;


		/*right = nLFSR_iterate(ptrState, &control, FALSE, rightIndex);*/
		lclState = 0;
		iLen = nLFSR_Len[rightIndex];		/***************/
		mask = ((1<<iLen)-1);
		lclState = *ptrState >> iLen;		/***************/
		fTopMost = (BYTE)((lclState >> (iLen-1)) & 0x01);
		fNfix = NFIX(lclState, iLen);

		lclState = (lclState << 1) & mask;
		if (((fTopMost /*^ fSlip */ ^ fNfix) & BIT_ONE) == TRUE) /* tap is on	*/
		{
			lclState = lclState ^ nLFSR_Mask[rightIndex]; /* tap sequence	*/
			lclState = lclState | 0x0001;			/* LSB should be 1	*/
		}
		right = lclState;

		*ptrState = res = left | (right <<lenLeft);
		image = ROLn(res, rotateNum);
		res = res ^ (~image); 
	}

	return res;
}

DWORD nLFSR_iterate(DWORD* ptrState, BYTE* ptrCtrl, BOOL fLeft, int index)
{
	DWORD res =0;
	int iLen = nLFSR_Len[index];
	DWORD mask = ((1<<iLen)-1);
	DWORD lclState;


	res = *ptrState;
	if (fLeft)
		lclState = res & mask;
	else
		lclState = res >> iLen;

	
	if (*ptrCtrl & CTRL_NLFSR_CLOCK)
	{
		BOOL fTopMost = (BYTE)((lclState >> (iLen-1)) & 0x01);
		BOOL fSlip;
		BOOL fNfix = NFIX(lclState, iLen);
		BOOL fFirstBit;
		if (fLeft == TRUE)
			fSlip = ((*ptrCtrl & CTRL_NLFSR_SLIP_LEFT) > 0);
		else
			fSlip = ((*ptrCtrl & CTRL_NLFSR_SLIP_RIGHT) > 0);

		fFirstBit = fTopMost ^ fNfix ^ fSlip; 


		lclState = (lclState << 1) & mask;

		if (fFirstBit) /* tap is on	*/
		{
			lclState = lclState ^ nLFSR_Mask[index]; /* tap sequence	*/

			lclState = lclState | 0x0001;			/* LSB should be 1	*/
		}

		res = lclState;
	}
	else
	{
		res = lclState;
	}


	return res;
}


/*Xor_N_Store	*/


DWORD topXor_N_Store(ECRYPT_ctx* ctx, DWORD in)
/*	33	&	11P	&	11S2	*/
{
	DWORD res;

	/* xor with 13 right rotated feedback	*/
 	in = in ^ (RORn(ctx->sttFeedBack, 13));
	res = xor_N_Store(&(ctx->sttTopXorNStore), in); 
	return res;
}

DWORD intrXor_N_Store(ECRYPT_ctx* ctx, DWORD in)
/*	33	&	11P	&	11S2	*/
{
	DWORD res;

	/*xor with 7 left rotated feedback	*/
	in = in ^ (ROLn(ctx->sttFeedBack, 7));
	res = xor_N_Store(&(ctx->sttIntrXorNStore), in);
	return res;
}

DWORD botXor_N_Store(ECRYPT_ctx* ctx, DWORD in)
/*	33	&	11P	&	11S2	*/
{
	DWORD res;

	/* NO feedback XORing is done	*/
	res = xor_N_Store(&(ctx->sttBotXorNStore), in);
	return res;  
}

DWORD xor_N_Store(DWORD* prev, DWORD in)
/*	33	*/
{
	DWORD res;
	res = *prev ^ in;
	*prev = in;
	return res;
}



/* Hash Matrix and EVNN	*/

/*	16S	&	17S	*/

DWORD topHash(ECRYPT_ctx* ctx, DWORD in)
{
	DWORD res;
 
	if ((ctx->ctrlTopHashMatrix & CTRL_HASH_VECTOR_A) > 0)
	{
		in = hashMix(in, hashTableA,32);
	}
	else if ((ctx->ctrlTopHashMatrix & CTRL_HASH_VECTOR_B) > 0)
	{
		in = hashMix(in, hashTableB,32);
	}
	else if ((ctx->ctrlTopHashMatrix & CTRL_HASH_VECTOR_C) > 0)
	{
		in = hashMix(in, hashTableC,32);
	}
	else /*if ((ctrlTopHashMatrix & CTRL_HASH_VECTOR_D) > 0)	*/
	{
		in = in;
	}

	if (((in >> 8) & BIT_ONE) == BIT_ONE)
	{	
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_THSH08;
	}

	if (((in >> 15) & BIT_ONE) == BIT_ONE)
	{	
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_THSH15;
	}

	if (((in >> 18) & BIT_ONE) == BIT_ONE)
	{	
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_THSH18;
	}

	if (((in >> 31) & BIT_ONE) == BIT_ONE)
	{	
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_THSH31;
	}

	res = ctx->sttTopHash = runEVNN(ctx, in, 1);
	return res;
}

DWORD botHash(ECRYPT_ctx* ctx, DWORD in)
{
	DWORD res;

	if ((ctx->ctrlBotHashMatrix & CTRL_HASH_VECTOR_A) > 0)
	{
		in = hashMix(in, hashTableA,32);
	}
	else if ((ctx->ctrlBotHashMatrix & CTRL_HASH_VECTOR_B) > 0)
	{
		in = hashMix(in, hashTableB,32);
	}
	else if ((ctx->ctrlBotHashMatrix & CTRL_HASH_VECTOR_C) > 0)
	{
		in = hashMix(in, hashTableC,32);
	}
	else /*if ((ctrlBotHashMatrix & CTRL_HASH_VECTOR_D) > 0)	*/
	{
		in = in;
	}

	ctx->sttBotHash = in;

	res = ctx->sttBotHash = runEVNN(ctx, in, 2);
	return res;
}

DWORD hashMix(DWORD in, const BYTE *table, int maxLen)
{
	DWORD res =0;
	int i;

	for (i=(maxLen -1); i>=0; i--)
	{
		res <<=1;
		res |= ((in >> table[i]) & 0x0001);
	}

	return res;
}

DWORD runEVNN(ECRYPT_ctx* ctx, DWORD in, int pos)
/*	16S	&	17S	&	32P	*/
{
	DWORD res;
	DWORD rot30 = ROLn(in, 2);	/* X n-30	*/
	DWORD rot31 = ROLn(in, 1);	/* X n-31	*/
	DWORD rot01 = RORn(in, 1);	/* X n+1	*/
	/*DWORD rot00 = in			/* X n	*/

	DWORD topMask = 0;
	DWORD midMask = 0;
	DWORD botMask = 0;
	DWORD frthMask = 0;

	DWORD lclCTRL = 0;
	DWORD ttlMask = 0;

	switch (pos)
	{
	case 1:
	default:
		lclCTRL = ctx->ctrlTopHashMatrix;
		break;

	case 2:
		lclCTRL = ctx->ctrlBotHashMatrix;
		break;

	}

	if ((lclCTRL & CTRL_HASH_ODDN_TOP) > 0)
	{
		topMask = 0x11111111;
	}

	if ((lclCTRL & CTRL_HASH_ODDN_MID) > 0)
	{
		midMask = 0x22222222;
	}

	if ((lclCTRL & CTRL_HASH_ODDN_BOT) > 0)
	{
		botMask = 0x44444444;
	}

	if ((lclCTRL & CTRL_HASH_ODDN_4TH) > 0)
	{
		frthMask = 0x88888888;
	}

	ttlMask = topMask | midMask | botMask | frthMask;
	res = maj(ttlMask, rot30, rot31);
	res = res ^ in ^ rot01;

	return res;

}

/* Feedback storage	*/

DWORD feedbackStorage(ECRYPT_ctx* ctx, DWORD out_ciph)
/* 11P	11S2	34	*/
{
  	/* static DWORD lclMACstorage = 0; */
	DWORD lclDisplacedMAC = 0;

	/* calculate Super-Tier feed back*/
	/* xor of lower 2 FB; each nibble mixed; everything 8 Right rotated */
	ctx->sttST_FeedBack = RORn(hashMix((ctx->fbC ^ ctx->fbD), MACnibMix,32),8);
	

	if ((ctx->ctrlFeedBack & CTRL_FEEDBACK_RESET) > 0)
	{	/* reset the internal store	*/
		ctx->sttFeedBack = 0;
		ctx->sttST_FeedBack = 0;
		ctx->lclMACstorage = 0;
		lclDisplacedMAC =0;
		ctx->ctrlFeedBack &= ~CTRL_FEEDBACK_RESET;
		return 0;
	}

	if ((ctx->ctrlFeedBack &CTRL_FEEDBACK_ENABLE) == 0)
	{	/* feedback is disabled return value is 0x00000000*/
		ctx->sttFeedBack = 0x00000000;
		ctx->sttST_FeedBack = 0;
		return 0x00000000;		
	}

	if ((ctx->ctrlFeedBack & CTRL_FEEDBACK_MAC) > 0)
	{	/* MAC feedback takes the input xors it with existing value and return it	*/
		ctx->sttFeedBack = out_ciph ^ ctx->lclMACstorage;
		ctx->lclMACstorage = out_ciph;
		/* calculate Super-Tier feed back in MAC mode*/
		/* Nibble displacement and XOR with FB */
		ctx->sttST_FeedBack =  ctx->sttST_FeedBack ^ hashMix(out_ciph, MACnibMix,32);
		return ctx->sttFeedBack;
	}

	if ((ctx->ctrlFeedBack & CTRL_FEEDBACK_CIPH) > 0)
	{	/* cipher feedbak enabled	*/

		/* Very Sparse FB AVG just less than 2 bits
		return ctx->sttFeedBack = (ctx->fbA & ctx->fbB & (ctx->fbC & ctx->fbD)); */
		/* Sparse FB AVG 4 bits*/
		return ctx->sttFeedBack = (ctx->fbA & ctx->fbB & (ctx->fbC ^ ctx->fbD));
		/* Dense FB 
		return ctx->sttFeedBack = (ctx->fbA ^ ctx->fbB  ^ ctx->fbC ^ ctx->fbD);
		*/
	}

	return 0;
}


/* CLOCKS	*/

void cycleClock(ECRYPT_ctx* ctx)
/*	10P	&	10S1	&	10S3	&	*/
{
	BYTE topClockRes = 0;
	BYTE midClockRes = 0;
	BYTE botClockRes = 0;

	BOOL fMC1 = 0;
	BOOL fMC2 = 0;
	BOOL fMC3 = 0;

	BOOL fBit1 = 0;
	BOOL fBit2 = 0;
	BOOL fBit3 = 0;

	BOOL fNotPRandom =0;
	BOOL fBit7 = 0;
	BOOL fBit8 = 0;
	BOOL fBit9 = 0;
	BOOL fBit10 = 0;

	BOOL fLHSlip = 0;
	BOOL fRHSlip = 0;

	if ((ctx->ctrlClocks & CTRL_CLOCKS_LOAD) > 0)
	{
		/* loads are reveresed LSB is loaded into MSB	*/
		DWORD tmpCipher = revereseBits(ctx->upCphWrd_1, 32);

		/* Load keys into nLFSR clocks 	*/
		/* As bits were reveresed we are taking them from the 'other' end	*/
		ctx->topBankClock_nLFR = (BYTE) ((tmpCipher>>29)& 0x0007);		/* bits 0-2	*/
		ctx->topBankClockCounter = (BYTE) ((tmpCipher >> 25)&0x000F);	/* bits 3-6	*/
		ctx->topBankClock_MC = 0;

		ctx->midBankClock_nLFR = (BYTE) ((tmpCipher >> 20) & 0x001F);	/* bits 7-11	*/
		ctx->midBankClockCounter = (BYTE) ((tmpCipher >> 16)&0x000F);/* bits 12-15	*/
		ctx->midBankClock_MC = 0;

		ctx->botBankClock_nLFR = (BYTE) ((tmpCipher >> 10) & 0x003F);/* bits 16-21	*/
		ctx->botBankClockCounter = (BYTE) ((tmpCipher >> 6)&0x000F);/* bits 22-25	*/
		ctx->botBankClock_MC = 0;


		ctx->hashCounter = (BYTE) ((tmpCipher >> 4)&0x0003); /* bit 26;27	*/
		/* to set up initial state	*/
		setHashVector(ctx);

		/* Load keys into PRandom Clock	*/
		ctx->longPClock = 0x0105;	/* in bits 100000101 => bits 0;1;2;3;8 are initialised	*/
		ctx->longPClock |= (WORD) ((tmpCipher &0x000F) << 4);/* bits 28-31;	*/

		ctx->shortPClock = 0x03;		/* in bits 11	*/

		cyclePRandomClock(ctx, TRUE); 



	}
	else
	{
		BOOL fInvBrnMaj = FALSE;

		if ((ctx->ctrlClocks & CTRL_PRND_CLOCK) > 0) 
		{
			topClockRes = cycleTopBankClock(ctx);
			midClockRes = cycleMidBankClock(ctx);
			botClockRes = cycleBotBankClock(ctx);
		}

		cycleHashCounter(ctx);

		cyclePRandomClock(ctx, FALSE); 
		
		/* setting up of control variables	*/


		/* Set Hash Matrix layers	*/

		ctx->ctrlTopHashMatrix &= 0x0F;
		ctx->ctrlBotHashMatrix &= 0x0F;

		fMC1 = getBitAtLocation(topClockRes, OUTP_CLOCKS_CTRL) && BIT_ONE;
		fMC2 = getBitAtLocation(midClockRes, OUTP_CLOCKS_CTRL) && BIT_ONE;
		fMC3 = getBitAtLocation(botClockRes, OUTP_CLOCKS_CTRL) && BIT_ONE;

		/*bit=NOT(XOR(TOP BRN, MID BRN, BOT BRN, (P)Random Clock ))	*/
		fInvBrnMaj = ((BYTE) (~(getBitAtLocation(topClockRes, OUTP_CLOCKS_BRN) ^
						getBitAtLocation(midClockRes, OUTP_CLOCKS_BRN) ^
						getBitAtLocation(botClockRes, OUTP_CLOCKS_BRN) ^
						getBitAtLocation(ctx->ctrlClocks, CTRL_PRND_CLOCK)				
					))) && BIT_ONE;
		/* TOP = MC1 XOR bit	*/
		if ((fMC1 ^ fInvBrnMaj) != 0)
		{
			ctx->ctrlTopHashMatrix |= CTRL_HASH_ODDN_TOP;
			ctx->ctrlBotHashMatrix |= CTRL_HASH_ODDN_4TH;
		}

		
		/* MIDDLE = MC2 XOR bit	*/
		if ((fMC2 ^ fInvBrnMaj) != 0)
		{
			ctx->ctrlTopHashMatrix |= CTRL_HASH_ODDN_MID;
			ctx->ctrlBotHashMatrix |= CTRL_HASH_ODDN_TOP;
			ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_ODDN_MID;
		}

		/* BOTTOM = MC3 XOR bit	*/
		if ((fMC3 ^ fInvBrnMaj) != 0)
		{
			ctx->ctrlTopHashMatrix |= CTRL_HASH_ODDN_BOT;
			ctx->ctrlBotHashMatrix |= CTRL_HASH_ODDN_MID;
		}


		if ((ctx->sttClockFeedBack & CTRL_CLOCKS_FB_4TH) != 0)
		{
			ctx->ctrlTopHashMatrix |= CTRL_HASH_ODDN_4TH;
			ctx->ctrlBotHashMatrix |= CTRL_HASH_ODDN_BOT;
		}



		/* Set nLFSR bank clocking	*/
		
		
		/* Reading in the delayed buffer */

		/* Clock */
		if ((ctx->delayedBuffer & DELAY_TOP_CLOCKS) > 0)
			ctx->ctrlTopBank |= CTRL_NLFSR_CLOCK;
		else
			ctx->ctrlTopBank &= ~CTRL_NLFSR_CLOCK;

		if ((ctx->delayedBuffer & DELAY_MID_CLOCKS) > 0)
			ctx->ctrlMidBank |= CTRL_NLFSR_CLOCK;
		else
			ctx->ctrlMidBank &= ~CTRL_NLFSR_CLOCK;

		if ((ctx->delayedBuffer & DELAY_BOT_CLOCKS) > 0)
			ctx->ctrlBotBank |= CTRL_NLFSR_CLOCK;
		else
			ctx->ctrlBotBank &= ~CTRL_NLFSR_CLOCK;


		/* Brownian */
		if ((ctx->delayedBuffer & DELAY_TOP_BROWN) > 0)
			ctx->ctrlTopBank |= CTRL_NLFSR_BROWN;
		else
			ctx->ctrlTopBank &= ~CTRL_NLFSR_BROWN;

		if ((ctx->delayedBuffer & DELAY_MID_BROWN) > 0)
			ctx->ctrlMidBank |= CTRL_NLFSR_BROWN;
		else
			ctx->ctrlMidBank &= ~CTRL_NLFSR_BROWN;
		
		if ((ctx->delayedBuffer & DELAY_BOT_BROWN) > 0)
			ctx->ctrlBotBank |= CTRL_NLFSR_BROWN;
		else
			ctx->ctrlBotBank &= ~CTRL_NLFSR_BROWN;


		/* Slip */
		if ((ctx->delayedBuffer & DELAY_LH_SLIP) > 0)
		{
			ctx->ctrlTopBank |= CTRL_NLFSR_SLIP_LEFT;
			ctx->ctrlMidBank |= CTRL_NLFSR_SLIP_LEFT;
			ctx->ctrlBotBank |= CTRL_NLFSR_SLIP_LEFT;
		}
		else
		{
			ctx->ctrlTopBank &= ~CTRL_NLFSR_SLIP_LEFT;
			ctx->ctrlMidBank &= ~CTRL_NLFSR_SLIP_LEFT;
			ctx->ctrlBotBank &= ~CTRL_NLFSR_SLIP_LEFT;
		}
		
		if ((ctx->delayedBuffer & DELAY_RH_SLIP) > 0)
		{
			ctx->ctrlTopBank |= CTRL_NLFSR_SLIP_RIGHT;
			ctx->ctrlMidBank |= CTRL_NLFSR_SLIP_RIGHT;
			ctx->ctrlBotBank |= CTRL_NLFSR_SLIP_RIGHT;
		}
		else
		{
			ctx->ctrlTopBank &= ~CTRL_NLFSR_SLIP_RIGHT;
			ctx->ctrlMidBank &= ~CTRL_NLFSR_SLIP_RIGHT;
			ctx->ctrlBotBank &= ~CTRL_NLFSR_SLIP_RIGHT;
		}

		/* (P)Random Clock Slip */
		if ((ctx->delayedBuffer & DELAY_PR_SLIP) > 0)
			ctx->ctrlClocks |= CTRL_CLOCKS_SLIP;
		else
			ctx->ctrlClocks &= ~CTRL_CLOCKS_SLIP;

		/* END read of delayed clocks */



		fBit1 = FALSE;
		fBit2 = FALSE;
		fBit3 = FALSE;

		if ((fMC1 == FALSE) && (fMC2 == FALSE) && (fMC3 == FALSE))
		{
			fBit1 = TRUE;
		}
		else if ((fMC1 == TRUE) && (fMC2 == FALSE) && (fMC3 == FALSE))
		{
			fBit3 = TRUE;
		}
		else if ((fMC1 == FALSE) && (fMC2 == TRUE) && (fMC3 == FALSE))
		{
			fBit3 = TRUE;
		}
		else if ((fMC1 == TRUE) && (fMC2 == TRUE) && (fMC3 == FALSE))
		{
			fBit1 = TRUE;
		}
		else if ((fMC1 == FALSE) && (fMC2 == FALSE) && (fMC3 == TRUE))
		{
			fBit2 = TRUE;
		}
		else if ((fMC1 == TRUE) && (fMC2 == FALSE) && (fMC3 == TRUE))
		{
			fBit1 = TRUE;
		}
		else if ((fMC1 == FALSE) && (fMC2 == TRUE) && (fMC3 == TRUE))
		{
			fBit3 = TRUE;
		}
		else if ((fMC1 == TRUE) && (fMC2 == TRUE) && (fMC3 == TRUE))
		{
			fBit2 = TRUE;
		}

		/* if current compansator is on (equiv to PrandClock off) */
		if ((ctx->ctrlClocks & CTRL_PRND_CLOCK) > 0)
		{
			ctx->delayedBuffer |= DELAY_TOP_CLOCKS;
			ctx->delayedBuffer |= DELAY_MID_CLOCKS;
			ctx->delayedBuffer |= DELAY_BOT_CLOCKS;
		} 
		else
		{
			/* top clock= NOT(bit2 AND bit)	*/
			if ((fBit2 && fInvBrnMaj) == FALSE)
			{
				ctx->delayedBuffer |= DELAY_TOP_CLOCKS;
			}

			/* middle clock = NOT(bit3 AND bit)	*/
			if ((fBit3 && fInvBrnMaj) == FALSE)
			{
				ctx->delayedBuffer |= DELAY_MID_CLOCKS;
			}

			/* bottom clock = NOT(bit1 AND bit)	*/
			if ((fBit1 && fInvBrnMaj) == FALSE)
			{
				ctx->delayedBuffer |= DELAY_BOT_CLOCKS;
			}
		}


		/* Brownian movment	*/
	
		/* NOT (P)Random Clock */
		fNotPRandom = ( ~ getBitAtLocation(ctx->ctrlClocks, CTRL_PRND_CLOCK)) && BIT_ONE ;

		/* bit7=TOP BRN OR bit1	OR NOT (P)Random Clock */
		fBit7 = (getBitAtLocation(topClockRes, OUTP_CLOCKS_BRN) && BIT_ONE) | fBit1 | fNotPRandom;

		/* bit8=MID BRN OR bit2	OR NOT (P)Random Clock */
		fBit8 = (getBitAtLocation(midClockRes, OUTP_CLOCKS_BRN) && BIT_ONE) | fBit2 | fNotPRandom;

		/* bit9=BOT BRN OR bit3	OR NOT (P)Random Clock */
		fBit9 = (getBitAtLocation(botClockRes, OUTP_CLOCKS_BRN) && BIT_ONE) | fBit3 | fNotPRandom;

		/* bit10 =AND(bit7,bit8,bit9)	*/
		fBit10 = fBit7 && fBit8 && fBit9;

		/* TOP BROWN = bit7 XOR (bit10 AND bit3)	*/
		if ((fBit7 ^ (fBit10 && fBit3)) == TRUE)
		{
			ctx->delayedBuffer |= DELAY_TOP_BROWN;
		}

		/* MID BROWN = bit8 XOR (bit10 AND bit1)	*/
		if ((fBit8 ^ (fBit10 && fBit1)) == TRUE)
		{
			ctx->delayedBuffer |= DELAY_MID_BROWN;
		}

		/* BOT BROWN = bit9 XOR (bit10 AND bit2)	*/
		if ((fBit9 ^ (fBit10 && fBit2)) == TRUE)
		{
			ctx->delayedBuffer |= DELAY_BOT_BROWN;
		}


		/* L/H Slip	*/
		fLHSlip = FALSE;
		if (( getBitAtLocation(topClockRes, OUTP_CLOCKS_LFT_SLP) &&
			getBitAtLocation(midClockRes, OUTP_CLOCKS_LFT_SLP) &&
			getBitAtLocation(botClockRes, OUTP_CLOCKS_RGT_SLP)
			) == FALSE)

		{
			ctx->delayedBuffer |= DELAY_LH_SLIP;
			fLHSlip = TRUE;

		}

		/* R/H Slip	*/
		fRHSlip = FALSE;
		if (( getBitAtLocation(topClockRes, OUTP_CLOCKS_RGT_SLP) &&
			getBitAtLocation(midClockRes, OUTP_CLOCKS_RGT_SLP) &&
			getBitAtLocation(botClockRes, OUTP_CLOCKS_LFT_SLP)
			) == FALSE)

		{
			ctx->delayedBuffer |= DELAY_RH_SLIP;
			fRHSlip = TRUE;
		}

		/* (P)Random Clock Slip	*/

		if (((fInvBrnMaj && fLHSlip) || (!fInvBrnMaj && fRHSlip)) == TRUE)
		{
			ctx->delayedBuffer |= DELAY_PR_SLIP;
		}
		else
		{
			ctx->delayedBuffer &= ~DELAY_PR_SLIP;
		}
	}
}


BYTE cycleTopBankClock(ECRYPT_ctx* ctx)
/*	13 C	*/
{

	BYTE res = 0;
	BYTE tmpBit = 0;
	BYTE tmp_nLFR = ctx->topBankClock_nLFR;

	BOOL fSlip = FALSE;

	/* bit=oldClock-R1[2] 	*/
	/*			XOR oldClock-R1[0] 	*/
	/*			XOR NFIX(oldClock-R1) 	*/
	/*			XOR (R2,1[17] AND Counter1 [0])	*/

	tmpBit = GETBIT_THREE(ctx->topBankClock_nLFR)
			^ GETBIT_ONE(ctx->topBankClock_nLFR)
			^ NFIX(ctx->topBankClock_nLFR, 3)
			^ ((getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q17)) & (GETBIT_ONE(ctx->topBankClockCounter)));

	ctx->topBankClock_nLFR = ctx->topBankClock_nLFR << 1;
	ctx->topBankClock_nLFR = (ctx->topBankClock_nLFR | (tmpBit & BIT_ONE)) &0x07 ;

	ctx->topBankClockCounter++;
	/* When the counter reaches 15	*/
	if (ctx->topBankClockCounter == 15)
	{
		
		/*	Counter1 [3]=XOR(Clock-R1[0], Clock-R1[1], MC1 ^ Juggle Hash)	*/
		tmpBit = (BYTE) ( GETBIT_ONE(tmp_nLFR) ^ 
					GETBIT_TWO(tmp_nLFR) ^
					(ctx->topBankClock_MC ^ (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_JUGG)))); 
		ctx->topBankClockCounter = tmpBit;
		ctx->topBankClockCounter = ctx->topBankClockCounter <<1; /*	Counter1 [2]=0	*/

		/* Counter1 [1]=XOR(R2,1[17], Clock-R1[0], Clock-R1[1])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q17) ^ 
					GETBIT_ONE(tmp_nLFR) ^
					GETBIT_TWO(tmp_nLFR)); 
		ctx->topBankClockCounter = ctx->topBankClockCounter <<1;
		ctx->topBankClockCounter += tmpBit;

		/* Counter1 [0]=XOR(R2,1[17], R2,2[13], Clock-R1[0])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q17) ^
					getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q13) ^
					GETBIT_ONE(tmp_nLFR));
		ctx->topBankClockCounter = ctx->topBankClockCounter <<1;
		ctx->topBankClockCounter += tmpBit;

		if (tmpBit == 0x01)
			ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_QTA;

		/*Then the value of MC1 is flipped.	*/
		ctx->topBankClock_MC = (~(ctx->topBankClock_MC)) & 0x01;

		fSlip = TRUE;

	}

	/* TOP BRN=XOR(Clock-R1[0],Clock-R1[1],Clock-R1[2])	*/
	if ((GETBIT_ONE(tmp_nLFR) ^ GETBIT_TWO(tmp_nLFR) ^ GETBIT_THREE(tmp_nLFR)) > 0)
		res |= OUTP_CLOCKS_BRN;

	/* In the event of SLIP (the counter arrives to 15), the XOR of the das bit and 	*/
	/* Clock-R1[2] chooses whether a left slip is outputed (when the XOR is 1) or 	*/
	/* a right slip (when the XOR is 0).	*/
	if (fSlip)
	{
		if ((ctx->sttClockFeedBack & CTRL_CLOCKS_FB_DAS) > 0)
			tmpBit = 1 ^ GETBIT_THREE(tmp_nLFR);
		else
			tmpBit = 0 ^ GETBIT_THREE(tmp_nLFR);

		if (tmpBit == 0)
			res |= OUTP_CLOCKS_RGT_SLP;
		else
			res |= OUTP_CLOCKS_LFT_SLP;
	}

	/* The state of MC1 is also passed	*/
	if ((ctx->topBankClock_MC & 0x01) > 0)
		res |= OUTP_CLOCKS_CTRL;

	return res;
}

BYTE cycleMidBankClock(ECRYPT_ctx* ctx)
/* 14 C	*/
{
	BYTE res = 0;
	BYTE tmpBit = 0;
	BYTE tmp_nLFR = ctx->midBankClock_nLFR;

	BOOL fSlip = FALSE;

	/* bit=oldClock-R2[4] 	*/
	/*			XOR oldClock-R2[1] 	*/
	/*			XOR NFIX(oldClock-R2) 	*/
	/*			XOR (R1,1[12] AND Counter2 [0])	*/

	tmpBit = GETBIT_FIVE(ctx->midBankClock_nLFR)
			^ GETBIT_TWO(ctx->midBankClock_nLFR)
			^ NFIX(ctx->midBankClock_nLFR, 5)
			^ ((getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q12)) & (GETBIT_ONE(ctx->midBankClockCounter)));

	ctx->midBankClock_nLFR = ctx->midBankClock_nLFR << 1;
	ctx->midBankClock_nLFR = (ctx->midBankClock_nLFR | (tmpBit & BIT_ONE)) & 0x1F;

	ctx->midBankClockCounter++;
	/* When the counter reaches 15	*/
	if (ctx->midBankClockCounter == 15)
	{
		/*	Counter2 [3]=XOR(Clock-R2[2], Clock-R2[3], MC2 ^ Juggle Hash)	*/
		tmpBit = (BYTE) ( GETBIT_THREE(tmp_nLFR) ^ GETBIT_FOUR(tmp_nLFR) ^
					(ctx->midBankClock_MC ^ (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_JUGG)))); 
		ctx->midBankClockCounter = tmpBit;
		ctx->midBankClockCounter = ctx->midBankClockCounter <<1; /*	Counter2 [2]=0	*/

		/* Counter2 [1]=XOR(R1,1[12], Clock-R2[0], Clock-R2[1])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q12) ^
					GETBIT_ONE(tmp_nLFR) ^
					GETBIT_TWO(tmp_nLFR)); 
		ctx->midBankClockCounter = ctx->midBankClockCounter <<1;
		ctx->midBankClockCounter += tmpBit;

		/* 	Counter2 [0]=XOR(R1,1[12], R1,2[18], Clock-R2[0])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q12) ^
					getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q18) ^
					GETBIT_ONE(tmp_nLFR));
		ctx->midBankClockCounter = ctx->midBankClockCounter <<1;
		ctx->midBankClockCounter += tmpBit;

		/*Then the value of MC2 is flipped.	*/
		ctx->midBankClock_MC = (~(ctx->midBankClock_MC)) & 0x01;

		fSlip = TRUE;

	}

	/* MID BRN=XOR(Clock-R2[0],Clock-R2[1],Clock-R2[2])	*/
	if ((GETBIT_ONE(tmp_nLFR) ^ GETBIT_TWO(tmp_nLFR) ^ GETBIT_THREE(tmp_nLFR)) > 0)
		res |= OUTP_CLOCKS_BRN;


	/* In the event of SLIP (the counter arrives to 15), the XOR of the das bit and 	*/
	/* Clock-R2[4] chooses whether a left slip is outputed (when the XOR is 1) or 	*/
	/* a right slip (when the XOR is 0).	*/
	if (fSlip)
	{
		if ((ctx->sttClockFeedBack & CTRL_CLOCKS_FB_DAS) > 0)
			tmpBit = 1 ^ GETBIT_FIVE(tmp_nLFR);
		else
			tmpBit = 0 ^ GETBIT_FIVE(tmp_nLFR);

		if (tmpBit == 0)
			res |= OUTP_CLOCKS_RGT_SLP;
		else
			res |= OUTP_CLOCKS_LFT_SLP;
	}

	/* The state of MC2 is also passed	*/
	if ((ctx->midBankClock_MC & 0x01) > 0)
		res |= OUTP_CLOCKS_CTRL;

	return res;
}

BYTE cycleBotBankClock(ECRYPT_ctx* ctx)
/*	15 C	*/
{
	BYTE res = 0;
	BYTE tmpBit = 0;
	BYTE tmp_nLFR = ctx->botBankClock_nLFR;

	BOOL fSlip = FALSE;

	/* bit=oldClock-R3[5] 	*/
	/*			XOR oldClock-R3[0] 	*/
	/*			XOR NFIX(oldClock-R3) 	*/
	/*			XOR (R3,1[14] AND Counter3 [0])	*/

	tmpBit = GETBIT_SIX(ctx->botBankClock_nLFR)
			^ GETBIT_ONE(ctx->botBankClock_nLFR)
			^ NFIX(ctx->botBankClock_nLFR, 6)
			^ ((getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q14)) & (GETBIT_ONE(ctx->botBankClockCounter)));

	ctx->botBankClock_nLFR = ctx->botBankClock_nLFR << 1;
	ctx->botBankClock_nLFR = (ctx->botBankClock_nLFR | (tmpBit & BIT_ONE)) & 0x3F;

	ctx->botBankClockCounter++;
	/* When the counter reaches 15	*/
	if (ctx->botBankClockCounter == 15)
	{

		/*	Counter3 [3]=XOR(Clock-R3[3], Clock-R3[4], MC3  ^ Juggle Hash)	*/
		tmpBit = (BYTE) ( GETBIT_FOUR(tmp_nLFR) ^ GETBIT_FIVE(tmp_nLFR) ^
					(ctx->botBankClock_MC ^ (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_JUGG)))); 
		ctx->botBankClockCounter = tmpBit;
		ctx->botBankClockCounter = ctx->botBankClockCounter <<1; /*	Counter3 [2]=0	*/

		/* Counter3 [1]=XOR(R3,1[14], Clock-R3[0], Clock-R3[1])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q14) ^
					GETBIT_ONE(tmp_nLFR) ^ GETBIT_TWO(tmp_nLFR)); 
		ctx->botBankClockCounter = ctx->botBankClockCounter <<1;
		ctx->botBankClockCounter += tmpBit;

		/* 		Counter3 [0]=XOR(R3,1[14], R3,2[16], Clock-R3[0])	*/
		tmpBit = (BYTE) (getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q14) ^
					getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_Q16) ^
					GETBIT_ONE(tmp_nLFR));
		ctx->botBankClockCounter = ctx->botBankClockCounter <<1;
		ctx->botBankClockCounter += tmpBit;

		/*Then the value of MC3 is flipped.	*/
		ctx->botBankClock_MC = (~(ctx->botBankClock_MC)) & 0x01;

		fSlip = TRUE;

	}

	/* BOT BRN=XOR(Clock-R3[0],Clock-R3[1],Clock-R3[2])	*/
	if ((GETBIT_ONE(tmp_nLFR) ^ GETBIT_TWO(tmp_nLFR) ^ GETBIT_THREE(tmp_nLFR)) > 0)
		res |= OUTP_CLOCKS_BRN;


	/* In the event of SLIP (the counter arrives to 15), the XOR of the das bit and 	*/
	/* Clock-R3[5] chooses whether a left slip is outputed (when the XOR is 1) or 	*/
	/* a right slip (when the XOR is 0).	*/
	if (fSlip)
	{
		if ((ctx->sttClockFeedBack & CTRL_CLOCKS_FB_DAS) > 0)
			tmpBit = 1 ^ GETBIT_SIX(tmp_nLFR);
		else
			tmpBit = 0 ^ GETBIT_SIX(tmp_nLFR);

		if (tmpBit == 0)
			res |= OUTP_CLOCKS_RGT_SLP;
		else
			res |= OUTP_CLOCKS_LFT_SLP;
	}

	/* The state of MC3 is also passed	*/
	if ((ctx->botBankClock_MC & 0x01) > 0)
		res |= OUTP_CLOCKS_CTRL;

	return res;
}

void setHashVector(ECRYPT_ctx* ctx)
{
	ctx->ctrlTopHashMatrix &= 0xF0;
	ctx->ctrlBotHashMatrix &= 0xF0;

	/* mobile mode fixed on D */
	if ((initOptions & ZK_OPTI_LEGACY) > 0)
	{
		ctx->ctrlTopHashMatrix |= CTRL_HASH_VECTOR_D;
		ctx->ctrlBotHashMatrix |= CTRL_HASH_VECTOR_D;
		return;
	}

	switch (ctx->hashCounter)
	{
	case 0:
	default:	/* ERROR	*/
		ctx->ctrlTopHashMatrix |= CTRL_HASH_VECTOR_A;
		ctx->ctrlBotHashMatrix |= CTRL_HASH_VECTOR_B;
		break;

	case 1:
		ctx->ctrlTopHashMatrix |= CTRL_HASH_VECTOR_B;
		ctx->ctrlBotHashMatrix |= CTRL_HASH_VECTOR_C;
		break;

	case 2:
		ctx->ctrlTopHashMatrix |= CTRL_HASH_VECTOR_C;
		ctx->ctrlBotHashMatrix |= CTRL_HASH_VECTOR_D;
		break;

	case 3:
		ctx->ctrlTopHashMatrix |= CTRL_HASH_VECTOR_D;
		ctx->ctrlBotHashMatrix |= CTRL_HASH_VECTOR_A;
		break;
	}
}

void cycleHashCounter(ECRYPT_ctx* ctx)
/* 18 C	*/
{
	BYTE bit26 = GETBIT_ONE(ctx->hashCounter);
	BYTE bit27 = GETBIT_TWO(ctx->hashCounter);
	/* bit 26	*/
	/* ((~bit27) ^ (TpHash15 ^ TpHash31) )	*/
	ctx->hashCounter = (~bit27) 
		^ (getBitAtLocation(ctx->sttClockFeedBack, CTRL_CLOCKS_FB_THSH15) 
					^ getBitAtLocation(ctx->sttClockFeedBack, CTRL_CLOCKS_FB_THSH31 ));
	ctx->hashCounter &= BIT_ONE;
	/* bit 27	*/
	ctx->hashCounter |= (bit26 ^ getBitAtLocation(ctx->sttClockFeedBack, CTRL_CLOCKS_FB_JUGG)) <<1;
	ctx->hashCounter &= 0x03;
	setHashVector(ctx);
}

void cyclePRandomClock(ECRYPT_ctx* ctx, BOOL fClean)
/*	4P	&	4C1B	&	4C2B	*/
{
	BYTE tmpBit = 0, tmpA =0, tmpB = 0;
	static BYTE F3, F4, F5, F6, F7, F8;
	WORD tmpLongPClock = ctx->longPClock;
	BYTE tmpShortPClock = ctx->shortPClock;

	if (fClean == TRUE)
	{
		F3 = F4 = F5 = F6 = F7 = F8 = 0;
		return;
	}

	/* bit = ClockRegister1[8] 	*/
	/*			XOR NFIX (ClockRegister1)	*/
	/*			XOR RandomClockSLIP	*/

	tmpBit = GETBIT_NINE(ctx->longPClock)
			^ NFIX(ctx->longPClock, 9)
			^ (getBitAtLocation (ctx->ctrlClocks, CTRL_CLOCKS_SLIP));

	ctx->longPClock = ctx->longPClock << 1;
	ctx->longPClock = (ctx->longPClock | (tmpBit & BIT_ONE)) & 0x01FF;

	/* tap is on	*/
	if (tmpBit == BIT_ONE)
	{
		ctx->longPClock = ctx->longPClock ^ 0x01B4; /* tap mask bits 1,3,4,6,7	*/
	}

	/* Short clock */
	/* A8 */
	tmpBit = ~((GETBIT_ONE(ctx->longPClock)) |
				(GETBIT_TWO(ctx->longPClock)) |
				(GETBIT_THREE(ctx->longPClock))
				);
	tmpB = GETBIT_ONE(ctx->shortPClock);
	tmpA = ~(GETBIT_ONE(ctx->shortPClock) & GETBIT_TWO(ctx->shortPClock)) | tmpBit;
	ctx->shortPClock = (tmpA & (tmpB <<1))& 0x03;


/*	4C1B	*/
	tmpBit = getBitAtLocation (ctx->sttClockFeedBack, CTRL_CLOCKS_FB_QTA);

	/*	Next State (Juggle Hash) */
	F6 = (tmpBit ^ F5 ^ F3 ^ F6) & BIT_ONE;
	if ((F6 & BIT_ONE) != 0)
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_JUGG;

	/* Fourthmask	*/
	F7 = (tmpBit ^ F5 ^ F3 ^ F7) & BIT_ONE;
	if ((F7 & BIT_ONE) != 0)
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_4TH;

	/* das	*/
	F8 = (tmpBit ^ F3 ^ F4 ^ F8) & BIT_ONE;
	if ((F8 & BIT_ONE) != 0)
		ctx->sttClockFeedBack_next |= CTRL_CLOCKS_FB_DAS;

	F3 = (BYTE) (GETBIT_TWO(tmpLongPClock) ^ 
			GETBIT_FIVE(tmpLongPClock) ^ 
			GETBIT_EIGHT(tmpLongPClock)); 

	F4 = (BYTE) (GETBIT_THREE(tmpLongPClock) ^ 
			GETBIT_SIX(tmpLongPClock) ^ 
			GETBIT_NINE(tmpLongPClock)); 


	F5 = (BYTE) (GETBIT_ONE(tmpLongPClock) ^ 
			GETBIT_FOUR(tmpLongPClock) ^ 
			GETBIT_SEVEN(tmpLongPClock));


	if (((F4 | F5 | (ctx->shortPClock & BIT_TWO)) & BIT_ONE) != 0)
	{
		ctx->ctrlClocks |= BIT_ONE; /* CTRL_PRND_CLOCK	*/
	}
}


/* GENERAL 	*/

BYTE NFIX(DWORD in, int len)
/* if all tested bits (0 to len-1) are 0 the function returns 1	*/
{
	DWORD mask = ((1<<(len-1))-1);	/* LSB = 0	*/
	if ((in & mask) == 0)
		return BIT_ONE;

	return 0;
}


DWORD maj(DWORD ai, DWORD bi, DWORD ci)
{
		DWORD	a1 = ai & bi;
		DWORD	b1 = bi & ci;
		DWORD	c1 = ci & ai;

		DWORD	res = a1 | b1 | c1;

		return res;
}



DWORD ROLn(DWORD in, int n)
{
	DWORD data;
	data = in << n;
	data |= in >> (32-n);

	return data;

}

DWORD RORn(DWORD in, int n)
{
	DWORD data;
	data = in >> n;
	data |= in << (32-n);

	return data;
}

BYTE  getBitAtLocation(DWORD in , DWORD loc)
{
	DWORD tmpIn = in & loc;

	if (tmpIn == 0 )
		return 0;
	else 
		return 1;
}

DWORD  revereseBits(DWORD in, int len)
{
	DWORD res = 0;
	int i;

	for (i = 0; i < len; i++)
	{
		res = res <<1;
		res |= (in >> i) & 0x01;
	}
	return res;
}


