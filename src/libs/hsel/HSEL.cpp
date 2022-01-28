//#include "stdafx.h"
//#include "windef.h"
//#include <mmsystem.h>
#include "HSEL.h"
//#pragma comment(lib,"winmm.lib")
//#pragma auto_inline(on)

const int LIMIT_SWAP_BLOCK_COUNT	= 5;
const int HSEL_VERSION				= 3;

#define GetKey()			(		(rand()%34000+10256)	*	(rand()%30000+10256)		+5	)

#define GetMultiGab()		(	(	(rand()%15000+512)		*	(rand()%10000+256)		)	*2	+1	)
#define GetPlusGab()		(	(	(rand()%5000+512)		*	(rand()%1000+256)		)	*2	+1	)

#define GetNextKey(A,B,C)	(	( (A)*(B) )	+	(C)	)
#define HSELSWAP(A,B)		(	(A)^=(B)^=(A)^=(B)	)

#define SharpRandom()		srand( (unsigned int)/*timeGetTime*/GetTickCount() )

/*--------------------------------------------------------------------------------*/
////////////////		HSEL_STREAM Class Fuction Body				////////////////
/*--------------------------------------------------------------------------------*/
CHSEL_STREAM::CHSEL_STREAM(void)
{
	iVersion = HSEL_VERSION; 
	iHSELType  = 0; 
	SharpRandom();
}

CHSEL_STREAM::~CHSEL_STREAM(void)
{							

}

__int32 CHSEL_STREAM::Initial(HSEL_INITIAL hselinit)
{
	iHSELType =0;
	switch(hselinit.iDesCount	&0x000F)
	{
		case HSEL_DES_SINGLE:
			{
				lpDesEncryptType=(&CHSEL_STREAM::DESSingleEncode);
				lpDesDecryptType=(&CHSEL_STREAM::DESSingleDecode);
			}
			break;
		case HSEL_DES_TRIPLE:
			{
				lpDesEncryptType=(&CHSEL_STREAM::DESTripleEncode);
				lpDesDecryptType=(&CHSEL_STREAM::DESTripleDecode);
			}
			break;
		default:{	return 0;		}break;
	}
	
	if(hselinit.iEncryptType == HSEL_ENCRYPTTYPE_RAND )
	{
		switch(rand()%4)
		{
		case 0:
			hselinit.iEncryptType = HSEL_ENCRYPTTYPE_1;
			break;
		case 1:
			hselinit.iEncryptType = HSEL_ENCRYPTTYPE_2;
			break;
		case 2:
			hselinit.iEncryptType = HSEL_ENCRYPTTYPE_3;
			break;
		default:
			hselinit.iEncryptType = HSEL_ENCRYPTTYPE_4;
			break;
		}
	}

	switch(hselinit.iEncryptType	&0x00F0)
	{
		case HSEL_ENCRYPTTYPE_1:
			{	
				lpDesLeftEncrypt	= (&CHSEL_STREAM::DESLeftEncode_Type_1);
				lpDesRightEncrypt	= (&CHSEL_STREAM::DESRightEncode_Type_1);
				lpDesMiddleEncrypt	= (&CHSEL_STREAM::DESMiddleEncode_Type_1);
				
				lpDesLeftDecrypt	= (&CHSEL_STREAM::DESLeftDecode_Type_1);
				lpDesRightDecrypt	= (&CHSEL_STREAM::DESRightDecode_Type_1);
				lpDesMiddleDecrypt	= (&CHSEL_STREAM::DESMiddleDecode_Type_1);
			}break;
		case HSEL_ENCRYPTTYPE_2:
			{		
				lpDesLeftEncrypt	= (&CHSEL_STREAM::DESLeftEncode_Type_2);
				lpDesRightEncrypt	= (&CHSEL_STREAM::DESRightEncode_Type_2);
				lpDesMiddleEncrypt	= (&CHSEL_STREAM::DESMiddleEncode_Type_2);
				
				lpDesLeftDecrypt	= (&CHSEL_STREAM::DESLeftDecode_Type_2);
				lpDesRightDecrypt	= (&CHSEL_STREAM::DESRightDecode_Type_2);
				lpDesMiddleDecrypt	= (&CHSEL_STREAM::DESMiddleDecode_Type_2);
			}break;
		case HSEL_ENCRYPTTYPE_3:
			{		
				lpDesLeftEncrypt	= (&CHSEL_STREAM::DESLeftEncode_Type_3);
				lpDesRightEncrypt	= (&CHSEL_STREAM::DESRightEncode_Type_3);
				lpDesMiddleEncrypt	= (&CHSEL_STREAM::DESMiddleEncode_Type_3);
				
				lpDesLeftDecrypt	= (&CHSEL_STREAM::DESLeftDecode_Type_3);
				lpDesRightDecrypt	= (&CHSEL_STREAM::DESRightDecode_Type_3);
				lpDesMiddleDecrypt	= (&CHSEL_STREAM::DESMiddleDecode_Type_3);
			}break;
		case HSEL_ENCRYPTTYPE_4:
			{		
				lpDesLeftEncrypt	= (&CHSEL_STREAM::DESLeftEncode_Type_4);
				lpDesRightEncrypt	= (&CHSEL_STREAM::DESRightEncode_Type_4);
				lpDesMiddleEncrypt	= (&CHSEL_STREAM::DESMiddleEncode_Type_4);
				
				lpDesLeftDecrypt	= (&CHSEL_STREAM::DESLeftDecode_Type_4);
				lpDesRightDecrypt	= (&CHSEL_STREAM::DESRightDecode_Type_4);
				lpDesMiddleDecrypt	= (&CHSEL_STREAM::DESMiddleDecode_Type_4);
			}break;
		default:{	return 0;			}break;
	}

	switch(hselinit.iSwapFlag	&0x0F00)
	{
		case HSEL_SWAP_FLAG_ON:		
			{
				lpSwapEncrypt=(&CHSEL_STREAM::SwapEncrypt);
				lpSwapDecrypt=(&CHSEL_STREAM::SwapDecrypt);
			}
			break;
		case HSEL_SWAP_FLAG_OFF:	
			{
				lpSwapEncrypt=(&CHSEL_STREAM::NoSwapEncrypt);
				lpSwapDecrypt=(&CHSEL_STREAM::NoSwapDecrypt);
			}
			break;
		default:{	return 0;	}break;
	}

	Init = hselinit;

	switch(hselinit.iCustomize		&0xF000)
	{
		case HSEL_KEY_TYPE_CUSTOMIZE:	
			{
				SetKeyCustom(hselinit.Keys);
			}
			break;
		case HSEL_KEY_TYPE_DEFAULT:	
			{
				GenerateKeys(Init.Keys);
			}
			break;
		default:{	return 0;	}	break;
	}

	Init.iCustomize= HSEL_KEY_TYPE_CUSTOMIZE;

	iHSELType = (hselinit.iDesCount|hselinit.iEncryptType|hselinit.iSwapFlag|hselinit.iCustomize);
	return iHSELType;
}

void CHSEL_STREAM::SetKeyCustom(HselKey IntoKey)
{
	Init.Keys = IntoKey;
	iCRCValue = 0;
	return;
}

void CHSEL_STREAM::GenerateKeys(HselKey &IntoKey)
{
	IntoKey.iLeftKey		= GetKey();		
	IntoKey.iRightKey		= GetKey();	
	IntoKey.iMiddleKey		= GetKey();	
	IntoKey.iTotalKey		= GetKey();

	IntoKey.iLeftMultiGab	= GetMultiGab();
	IntoKey.iRightMultiGab	= GetMultiGab();
	IntoKey.iMiddleMultiGab	= GetMultiGab();
	IntoKey.iTotalMultiGab	= GetMultiGab();

	IntoKey.iLeftPlusGab	= GetPlusGab();
	IntoKey.iRightPlusGab	= GetPlusGab();
	IntoKey.iMiddlePlusGab	= GetPlusGab();
	IntoKey.iTotalPlusGab	= GetPlusGab();

	return;
}

void CHSEL_STREAM::SetNextKey(void)
{
	Init.Keys.iLeftKey	= GetNextKey
		(
		Init.Keys.iLeftKey,
		Init.Keys.iLeftMultiGab,
		Init.Keys.iLeftPlusGab
		);

	Init.Keys.iRightKey	= GetNextKey
		(
		Init.Keys.iRightKey,
		Init.Keys.iRightMultiGab,	
		Init.Keys.iRightPlusGab
		);

	Init.Keys.iMiddleKey	= GetNextKey
		(
		Init.Keys.iMiddleKey,
		Init.Keys.iMiddleMultiGab,
		Init.Keys.iMiddlePlusGab
		);

	Init.Keys.iTotalKey	= GetNextKey
		(
		Init.Keys.iTotalKey,
		Init.Keys.iTotalMultiGab,
		Init.Keys.iTotalPlusGab
		);
	return;
}

bool CHSEL_STREAM::ChackFaultStreamSize(const __int32 iStreamSize)
{
	if( 0 >= iStreamSize )	{return false;}
	//_ASSERTE(iStreamSize > 0);//
	return true;
}

bool CHSEL_STREAM::Encrypt(char *lpStream,const __int32 iStreamSize)
{	
	if(!ChackFaultStreamSize(iStreamSize)){return false;}//길이 체크

	(this->*lpSwapEncrypt)(lpStream,iStreamSize);//스와핑

	(this->*lpDesEncryptType)(lpStream,iStreamSize);//암호화

	SetNextKey();//다음키

	GetCRC(lpStream,iStreamSize);//CRC 값 받기

	return true;
}

bool CHSEL_STREAM::Decrypt(char *lpStream,const __int32 iStreamSize)
{
	if(!ChackFaultStreamSize(iStreamSize)){return false;}

	GetCRC(lpStream,iStreamSize);//CRC 값 받기

	(this->*lpDesDecryptType)(lpStream,iStreamSize);//복호화

	(this->*lpSwapDecrypt)(lpStream,iStreamSize);//역스왑

	SetNextKey();//다음키

	return true;
}

////////////////////////////////////////////////////////////////////////////
//				Single DES
void CHSEL_STREAM::DESSingleEncode(char *lpStream,const __int32 iStreamSize)
{
	(this->*lpDesLeftEncrypt)(lpStream,iStreamSize);
	return;
}

void CHSEL_STREAM::DESSingleDecode(char *lpStream,const __int32 iStreamSize)
{
	(this->*lpDesLeftDecrypt)(lpStream,iStreamSize);
	return;
}
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//				Triple DES
void CHSEL_STREAM::DESTripleEncode(char *lpStream,const __int32 iStreamSize)
{
	(this->*lpDesLeftEncrypt)(lpStream,iStreamSize);
	(this->*lpDesRightEncrypt)(lpStream,iStreamSize);
	(this->*lpDesMiddleEncrypt)(lpStream,iStreamSize);
	return;
}

void CHSEL_STREAM::DESTripleDecode(char *lpStream,const __int32 iStreamSize)
{
	(this->*lpDesMiddleDecrypt)(lpStream,iStreamSize);
	(this->*lpDesRightDecrypt)(lpStream,iStreamSize);
	(this->*lpDesLeftDecrypt)(lpStream,iStreamSize);
	return;
}
////////////////////////////////////////////////////////////////////////////

void CHSEL_STREAM::GetCRC(char *lpStream,const __int32 iStreamSize)
{
	iCRCValue	=	0;

	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4

	__int32 *lpBlock	= (__int32*)(lpStream);

	while(iBlockCount)
	{
		iCRCValue	^= (*(lpBlock++));
		iBlockCount--;
	}
	while(iRemainCount)
	{
		iCRCValue	^=	lpStream[iPos++];
		iRemainCount--;
	}
	return;
}

__int32		CHSEL_STREAM::GetCRCConvertInt(void)const
{
	return iCRCValue;
}

char	CHSEL_STREAM::GetCRCConvertChar(void)const
{
	char *cCRCKeyTemp = (char *)&iCRCValue;
	return (char)((*cCRCKeyTemp)^(*(cCRCKeyTemp+1))^(*(cCRCKeyTemp+2))^(*(cCRCKeyTemp+3)));
}

short	CHSEL_STREAM::GetCRCConvertShort(void)const
{
	short *sCRCKeyTemp = (short *)&iCRCValue;
	return (short)((*sCRCKeyTemp)	^ (*(sCRCKeyTemp+1)));
}

void CHSEL_STREAM::NoSwapEncrypt(char *lpStream, const __int32 iStreamSize)
{
	return;
}

void CHSEL_STREAM::NoSwapDecrypt(char *lpStream, const __int32 iStreamSize)
{
	return;
}

void CHSEL_STREAM::SwapEncrypt(char *lpStream,const __int32 iStreamSize)
{
	iBlockCount	= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;

	if(LIMIT_SWAP_BLOCK_COUNT > iBlockCount)	{return;}//블럭이 2개 이하라면 스왑 불가능.
	iBlockCount--;

	HselKey	*Keys = &Init.Keys;

	iLPos0 =	(Keys->iLeftKey&0x000F)%iBlockCount;
	iLPos1 =	((Keys->iLeftKey>>8)&0x000F)%iBlockCount;
	iLPos2 =	((Keys->iLeftKey>>16)&0x000F)%iBlockCount;
	iLPos3 =	((Keys->iLeftKey>>24)&0x000F)%iBlockCount;	

	iRPos0 =	(Keys->iRightKey&0x000F)%iBlockCount;
	iRPos1 =	((Keys->iRightKey>>8)&0x000F)%iBlockCount;
	iRPos2 =	((Keys->iRightKey>>16)&0x000F)%iBlockCount;
	iRPos3 =	((Keys->iRightKey>>24)&0x000F)%iBlockCount;	

	iMPos0 =	(Keys->iMiddleKey&0x000F)%iBlockCount;
	iMPos1 =	((Keys->iMiddleKey>>8)&0x000F)%iBlockCount;
	iMPos2 =	((Keys->iMiddleKey>>16)&0x000F)%iBlockCount;
	iMPos3 =	((Keys->iMiddleKey>>24)&0x000F)%iBlockCount;	

	__int32 *lpBlock = (__int32*)(lpStream);//스왑용 4바이트 단위

	if(iLPos0 != iRPos0){HSELSWAP(	(*(lpBlock+(iLPos0))),	(*(lpBlock+(iRPos0)))	);}
	if(iRPos0 != iMPos0){HSELSWAP(	(*(lpBlock+(iRPos0))),	(*(lpBlock+(iMPos0)))	);}

	if(iLPos1 != iRPos1){HSELSWAP(	(*(lpBlock+(iLPos1))),	(*(lpBlock+(iRPos1)))	);}
	if(iRPos1 != iMPos1){HSELSWAP(	(*(lpBlock+(iRPos1))),	(*(lpBlock+(iMPos1)))	);}

	if(iLPos2 != iRPos2){HSELSWAP(	(*(lpBlock+(iLPos2))),	(*(lpBlock+(iRPos2)))	);}
	if(iRPos2 != iMPos2){HSELSWAP(	(*(lpBlock+(iRPos2))),	(*(lpBlock+(iMPos2)))	);}

	if(iLPos3 != iRPos3){HSELSWAP(	(*(lpBlock+(iLPos3))),	(*(lpBlock+(iRPos3)))	);}
	if(iRPos3 != iMPos3){HSELSWAP(	(*(lpBlock+(iRPos3))),	(*(lpBlock+(iMPos3)))	);}
	return;
}

void CHSEL_STREAM::SwapDecrypt(char *lpStream,const __int32 iStreamSize)
{
	iBlockCount	= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;

	if(LIMIT_SWAP_BLOCK_COUNT > iBlockCount)	{return;}//블럭이 2개 이하라면 스왑 불가능.
	iBlockCount--;

	HselKey	*Keys = &Init.Keys;

	iLPos0 =	(Keys->iLeftKey&0x000F)%iBlockCount;
	iLPos1 =	((Keys->iLeftKey>>8)&0x000F)%iBlockCount;
	iLPos2 =	((Keys->iLeftKey>>16)&0x000F)%iBlockCount;
	iLPos3 =	((Keys->iLeftKey>>24)&0x000F)%iBlockCount;	

	iRPos0 =	(Keys->iRightKey&0x000F)%iBlockCount;
	iRPos1 =	((Keys->iRightKey>>8)&0x000F)%iBlockCount;
	iRPos2 =	((Keys->iRightKey>>16)&0x000F)%iBlockCount;
	iRPos3 =	((Keys->iRightKey>>24)&0x000F)%iBlockCount;	

	iMPos0 =	(Keys->iMiddleKey&0x000F)%iBlockCount;
	iMPos1 =	((Keys->iMiddleKey>>8)&0x000F)%iBlockCount;
	iMPos2 =	((Keys->iMiddleKey>>16)&0x000F)%iBlockCount;
	iMPos3 =	((Keys->iMiddleKey>>24)&0x000F)%iBlockCount;	

	__int32 *lpBlock = (__int32*)(lpStream);//스왑용 4바이트 단위

	if(iRPos3 != iMPos3){HSELSWAP(	(*(lpBlock+(iRPos3))),	(*(lpBlock+(iMPos3)))	);}
	if(iLPos3 != iRPos3){HSELSWAP(	(*(lpBlock+(iLPos3))),	(*(lpBlock+(iRPos3)))	);}

	if(iRPos2 != iMPos2){HSELSWAP(	(*(lpBlock+(iRPos2))),	(*(lpBlock+(iMPos2)))	);}
	if(iLPos2 != iRPos2){HSELSWAP(	(*(lpBlock+(iLPos2))),	(*(lpBlock+(iRPos2)))	);}

	if(iRPos1 != iMPos1){HSELSWAP(	(*(lpBlock+(iRPos1))),	(*(lpBlock+(iMPos1)))	);}
	if(iLPos1 != iRPos1){HSELSWAP(	(*(lpBlock+(iLPos1))),	(*(lpBlock+(iRPos1)))	);}

	if(iRPos0 != iMPos0){HSELSWAP(	(*(lpBlock+(iRPos0))),	(*(lpBlock+(iMPos0)))	);}
	if(iLPos0 != iRPos0){HSELSWAP(	(*(lpBlock+(iLPos0))),	(*(lpBlock+(iRPos0)))	);}
	return;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHSEL_STREAM::DESLeftEncode_Type_1(char *lpStream,const __int32 iStreamSize)
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar			= ((char *)&(Init.Keys.iLeftKey));
	
	while(iBlockCount)
	{
		(*lpBlock)		^=	(iTempLeftKey);
		(iTempLeftKey)	=	(*(lpBlock++));
		(iBlockCount--);
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESLeftDecode_Type_1(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2); //블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar		= ((char *)&Init.Keys.iLeftKey);

	lpBlock			+=	(iBlockCount-1);
	(iTempLeftKey)	=	(*(lpBlock-1));

	while(	1	<	iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*(lpBlock--))	^=	(iTempLeftKey);
        if((char *)lpBlock > lpStream)
		(iTempLeftKey)	=	(*(lpBlock-1))	;
		(iBlockCount--);
	}

	if(iBlockCount)
	{
		(*lpBlock)	^=	(Init.Keys.iLeftKey);
	}

	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESRightEncode_Type_1(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream+iPos+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	lpBlock = (__int32*)(lpStream+iStreamSize-4);

	while(iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*lpBlock)		^=	(iTempRightKey);
		(iTempRightKey)	=	(*(lpBlock--));
		iBlockCount--;
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	^=	(*(lpChar+iRemainCount));
	}
	return;
}

void CHSEL_STREAM::DESRightDecode_Type_1(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4

	__int32 *lpBlock	= (__int32*)(lpStream+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	iTempRightKey	= *(lpBlock+1);

	while(1 < iBlockCount)
	{
		(*(lpBlock++)) ^=	(iTempRightKey);
		iTempRightKey	=	(*(lpBlock+1));
		iBlockCount--;
	}

	if(iBlockCount)
	{
		*(lpBlock)		^=	(Init.Keys.iRightKey);
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	^=	(*(lpChar+iRemainCount));
	}

	return;
}

void CHSEL_STREAM::DESMiddleEncode_Type_1(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4

	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar	= ((char *)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) ^= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}

	return;
}

void CHSEL_STREAM::DESMiddleDecode_Type_1(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar			= ((char*)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) ^= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHSEL_STREAM::DESLeftEncode_Type_2(char *lpStream,const __int32 iStreamSize)
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar			= ((char *)&(Init.Keys.iLeftKey));
	
	while(iBlockCount)
	{
		(*lpBlock)		+=	(iTempLeftKey);
		(iTempLeftKey)	=	(*(lpBlock++));
		(iBlockCount--);
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	+=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESLeftDecode_Type_2(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2); //블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar		= ((char *)&Init.Keys.iLeftKey);

	lpBlock			+=	(iBlockCount-1);
	(iTempLeftKey)	=	(*(lpBlock-1));

	while(	1	<	iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*(lpBlock--))	-=	(iTempLeftKey);
        if((char *)lpBlock > lpStream)
		(iTempLeftKey)	=	(*(lpBlock-1))	;
		(iBlockCount--);
	}

	if(iBlockCount)
	{
		(*lpBlock)	-=	(Init.Keys.iLeftKey);
	}

	while(iRemainCount)
	{
		lpStream[iPos++]	-=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESRightEncode_Type_2(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream+iPos+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	lpBlock = (__int32*)(lpStream+iStreamSize-4);
	
	while(iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*lpBlock)		+=	(iTempRightKey);
		(iTempRightKey)	=	(*(lpBlock--));
		iBlockCount--;
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	+=	(*(lpChar+iRemainCount));
	}
	return;
}

void CHSEL_STREAM::DESRightDecode_Type_2(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	iTempRightKey	= *(lpBlock+1);
	
	while(1 < iBlockCount)
	{
		(*(lpBlock++))	-=	(iTempRightKey);
		iTempRightKey	=	(*(lpBlock+1));
		iBlockCount--;
	}
	
	if(iBlockCount)
	{
		*(lpBlock)		-=	(Init.Keys.iRightKey);
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	-=	(*(lpChar+iRemainCount));
	}
	
	return;
}

void CHSEL_STREAM::DESMiddleEncode_Type_2(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar	= ((char *)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) += iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	+=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}

	return;
}

void CHSEL_STREAM::DESMiddleDecode_Type_2(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar			= ((char*)&Init.Keys.iMiddleKey);
	
	while(iBlockCount)
	{
		(*(lpBlock++)) -= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	-=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHSEL_STREAM::DESLeftEncode_Type_3(char *lpStream,const __int32 iStreamSize)
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar			= ((char *)&(Init.Keys.iLeftKey));
	
	while(iBlockCount)
	{
		(*lpBlock)		-=	(iTempLeftKey);
		(iTempLeftKey)	=	(*(lpBlock++));
		(iBlockCount--);
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	-=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESLeftDecode_Type_3(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2); //블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);
	
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar		= ((char *)&Init.Keys.iLeftKey);

	lpBlock			+=	(iBlockCount-1);
	(iTempLeftKey)	=	(*(lpBlock-1));

	while(	1	<	iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*(lpBlock--))	+=	(iTempLeftKey);
        if((char *)lpBlock > lpStream)
		(iTempLeftKey)	=	(*(lpBlock-1));
		(iBlockCount--);
	}

	if(iBlockCount)
	{
		(*lpBlock)	+=	(Init.Keys.iLeftKey);
	}

	while(iRemainCount)
	{
		lpStream[iPos++]	+=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESRightEncode_Type_3(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream+iPos+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);
	
	lpBlock = (__int32*)(lpStream+iStreamSize-4);
	
	while(iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*lpBlock)		-=	(iTempRightKey);
		(iTempRightKey)	=	(*(lpBlock--));
		iBlockCount--;
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	-=	(*(lpChar+iRemainCount));
	}
	return;
}

void CHSEL_STREAM::DESRightDecode_Type_3(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	iTempRightKey	= *(lpBlock+1);
	
	while(1 < iBlockCount)
	{
		(*(lpBlock++))	+=	(iTempRightKey);
		iTempRightKey	=	(*(lpBlock+1));
		iBlockCount--;
	}
	
	if(iBlockCount)
	{
		*(lpBlock)		+=	(Init.Keys.iRightKey);
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	+=	(*(lpChar+iRemainCount));
	}
	
	return;
}

void CHSEL_STREAM::DESMiddleEncode_Type_3(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar	= ((char *)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) -= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	-=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}

	return;
}

void CHSEL_STREAM::DESMiddleDecode_Type_3(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4
	
	__int32 *lpBlock	= (__int32*)(lpStream);
	char *lpChar			= ((char*)&Init.Keys.iMiddleKey);
	
	while(iBlockCount)
	{
		(*(lpBlock++)) += iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	+=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHSEL_STREAM::DESLeftEncode_Type_4(char *lpStream,const __int32 iStreamSize)
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2);	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4
	
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar			= ((char *)&(Init.Keys.iLeftKey));
	
	while(iBlockCount)
	{
		(*lpBlock)		-=	(iTempLeftKey);
		(iTempLeftKey)	=	(*(lpBlock++));
		(iBlockCount--);
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESLeftDecode_Type_4(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempLeftKey	= Init.Keys.iLeftKey;//클래스 내의 LeftKey의 템프
	iBlockCount		= (iStreamSize>>2); //블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= (iStreamSize&3);//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);
	
	__int32 *lpBlock	= (__int32*)lpStream;//암호화 대상 블럭 지정용 포인터
	char *lpChar		= ((char *)&Init.Keys.iLeftKey);

	lpBlock			+=	(iBlockCount-1);
	(iTempLeftKey)	=	(*(lpBlock-1));

	while(	1	<	iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*(lpBlock--))	+=	(iTempLeftKey);
        if((char *)lpBlock > lpStream)
		(iTempLeftKey)	=	(*(lpBlock-1));
		(iBlockCount--);
	}

	if(iBlockCount)
	{
		(*lpBlock)	+=	(Init.Keys.iLeftKey);
	}

	while(iRemainCount)
	{
		lpStream[iPos++]	^=	(*(lpChar+iRemainCount));
		(iRemainCount--);
	}
	return;
}

void CHSEL_STREAM::DESRightEncode_Type_4(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	
	__int32 *lpBlock	= (__int32*)(lpStream+iStreamSize-4);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);
	
	while(iBlockCount)
	{
        if((char *)lpBlock < lpStream) break;
		(*lpBlock)		+=	(iTempRightKey);
		(iTempRightKey)	=	(*(lpBlock--));
		iBlockCount--;
	}

	while(iRemainCount)
	{
		lpStream[--iRemainCount]	^=	(*(lpChar+iRemainCount));
	}
	return;
}

void CHSEL_STREAM::DESRightDecode_Type_4(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempRightKey	= Init.Keys.iRightKey;//클래스 내의 RightKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4
	
	__int32 *lpBlock	= (__int32*)(lpStream+iRemainCount);
	char *lpChar	= ((char *)&Init.Keys.iRightKey);

	iTempRightKey	= *(lpBlock+1);
	
	while(1 < iBlockCount)
	{
		(*(lpBlock++))	-=	(iTempRightKey);
		iTempRightKey	=	(*(lpBlock+1));
		iBlockCount--;
	}
	
	if(iBlockCount)
	{
		*(lpBlock)		-=	(Init.Keys.iRightKey);
	}
	while(iRemainCount)
	{
		lpStream[--iRemainCount]	^=	(*(lpChar+iRemainCount));
	}
	
	return;
}

void CHSEL_STREAM::DESMiddleEncode_Type_4(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);			//바이트 단위 위치 iBlockCount*4

	__int32 *lpBlock = (__int32*)(lpStream);
	char *lpChar = ((char *)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) ^= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	+=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}

	return;
}

void CHSEL_STREAM::DESMiddleDecode_Type_4(char *lpStream,const __int32 iStreamSize)//길이에는 마이너스 값이 없다.
{
	iTempMiddleKey	= Init.Keys.iMiddleKey;//클래스 내의 MiddleKey의 템프
	iBlockCount		= iStreamSize>>2;	//블럭수 가지고 오기 == __int32 iBlockCount = iStreamSize/4;
	iRemainCount	= iStreamSize&3;	//짜투리 길이 받기==__int32 iRemainCount = iStreamSize%4; 
	iPos			= (iBlockCount<<2);	//바이트 단위 위치 iBlockCount*4

	__int32 *lpBlock = (__int32*)(lpStream);
	char *lpChar = ((char*)&Init.Keys.iMiddleKey);

	while(iBlockCount)
	{
		(*(lpBlock++)) ^= iTempMiddleKey;
		iBlockCount--;
	}
	while(iRemainCount)
	{
		lpStream[iPos++]	-=	(*(lpChar+iRemainCount));
		iRemainCount--;
	}
	return;
}