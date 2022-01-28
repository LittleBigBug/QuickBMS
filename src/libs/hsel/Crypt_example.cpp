// Crypt.cpp: implementation of the CCrypt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Crypt_example.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCrypt::CCrypt()
{
	m_bInited	= FALSE;
}

CCrypt::~CCrypt()
{
}

void CCrypt::Create()
{
	HselInit eninit;
	eninit.iEncryptType		=	HSEL_ENCRYPTTYPE_RAND;
	eninit.iDesCount		=	HSEL_DES_TRIPLE;
	eninit.iCustomize		=	HSEL_KEY_TYPE_DEFAULT;
	eninit.iSwapFlag		=	HSEL_SWAP_FLAG_ON;
	
	if( !m_hEnStream.Initial(eninit) )
	{
		ASSERTMSG(0, "CCrypt Create Initial() failed");
		return;
	}
	m_eninit = m_hEnStream.GetHSELCustomizeOption();	//Get HSEL Keys and Option

	if( !m_hDeStream.Initial(eninit) )
	{
		ASSERTMSG(0, "CCrypt Create Initial() failed");
		return;
	}
	m_deinit = m_hDeStream.GetHSELCustomizeOption();	//Get HSEL Keys and Option

}

void CCrypt::Init( HselInit	eninit, HselInit deinit )
{
	m_eninit = deinit;			// 서버의 de key는 클라이언트의 en key
	m_deinit = eninit;			// 서버의 en key는 클라이언트의 de key

	// 서버의 de -> en
	if( !m_hEnStream.Initial(deinit) )
	{
		ASSERTMSG(0, "CCrypt EnInit Initial() failed");
		return;
	}

	// 서버의 en -> de
	if( !m_hDeStream.Initial(eninit) )
	{
		ASSERTMSG(0, "CCrypt DeInit Initial() failed");
		return;
	}
	m_bInited = TRUE;
}

BOOL CCrypt::Encrypt( char * eBuf, int bufSize )
{
	if( !m_bInited )
		return TRUE;
	return m_hEnStream.Encrypt( eBuf, bufSize );
}

BOOL CCrypt::Decrypt( char * eBuf, int bufSize )
{
	if( !m_bInited )
		return TRUE;
	return m_hDeStream.Decrypt( eBuf, bufSize );
}	

char CCrypt::GetEnCRCConvertChar()
{
	if( !m_bInited )
		return 0;
	return m_hEnStream.GetCRCConvertChar();
}
char CCrypt::GetDeCRCConvertChar()
{
	if( !m_bInited )
		return 0;
	return m_hDeStream.GetCRCConvertChar();
}

