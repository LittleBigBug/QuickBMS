/*
 *  LZHL_TCP implementation v 1.0
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Warning: this version is only a demonstration for LZHL algorithm usage;
 *           non-blocking sockets and OOB aren't supported
 *
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <assert.h>
#include <map>
using namespace std;
#include "lzhl.h"
#include "lzhl_tcp.h"

typedef unsigned char BYTE;

struct LZHL_SOCKET
{
public:
	LZHL_CHANDLE ch;
	LZHL_DHANDLE dh;

	BYTE* dBuf;
	int   dSz;
	int   dDisp;

public:
	LZHL_SOCKET()
	{
		ch = LZHL_CHANDLE_NULL;
		dh = LZHL_DHANDLE_NULL;

		dBuf = 0;
	}
};

typedef map< SOCKET, LZHL_SOCKET > GlobalMapType;
static GlobalMapType globalMap;

SOCKET lzhl_socket( int af, int type, int protocol )
{
	SOCKET sock = socket( af, type, protocol );
	if( sock >= 0 )
		globalMap.insert( GlobalMapType::value_type( sock, LZHL_SOCKET() ) );
	return sock;
}

SOCKET lzhl_accept( SOCKET s, struct sockaddr* addr, int* addrlen )
{
	SOCKET sock = accept( s, addr, addrlen );
	if( sock >= 0 )
		globalMap.insert( GlobalMapType::value_type( sock, LZHL_SOCKET() ) );
	return sock;
}

static void _putInt( BYTE*& p, unsigned int val )
{
	for(;;)
		if( val <= 127 )
		{
			*p++ = (BYTE)val;
			break;
		}
		else
		{
			*p++ = (BYTE)( 0x80 | ( val & 0x7F ) );
			val >>= 7;
		}
}

static int _getInt( BYTE*& p, int sz, unsigned int* val )
//returns: 0 - Ok, 1 - insufficient data, 2 - fatal error
{
	unsigned int bits = 0;
	int nBits = 0;
	for(;;)
	{
		if( sz == 0 )
			return 1;
		BYTE c = *p++;
		--sz;
		bits |= ( ( c & 0x7F ) << nBits );
		nBits += 7;
		if( nBits > 35 )
			return 2;

		if( ( c & 0x80 ) == 0 )
		{
			*val = bits;
			return 0;
		}
	}
}

int lzhl_send( SOCKET sock, const char* data, int dataSz, int flags )
{
	GlobalMapType::iterator iter = globalMap.find( sock );
	if( iter != globalMap.end() )
	{
		LZHL_CHANDLE& ch = (*iter).second.ch;
		if( ch == LZHL_CHANDLE_NULL )
			ch = LZHLCreateCompressor();

		size_t maxSz = 10 + LZHLCompressorCalcMaxBuf( dataSz );
		BYTE* buf = new BYTE[ maxSz ];
		size_t compSz = LZHLCompress( ch, buf + 10, data, dataSz );

		int dSz;
		{
		BYTE tmp[ 5 ];
		BYTE* p = tmp;
		_putInt( p, compSz );
		int szSz = p - tmp;
		dSz = szSz;
		assert( dSz <= 10 );
		memcpy( buf + 10 - dSz, tmp, szSz );
		
		p = tmp;
		_putInt( p, dataSz );
		szSz = p - tmp;
		dSz += szSz;
		assert( dSz <= 10 );
		memcpy( buf + 10 - dSz, tmp, szSz );
		}

		{
		BYTE* p = buf + 10 - dSz;
		int sz = dSz + compSz;
		while( sz )
		{
			int bytes = send( sock, (char*)( buf + 10 - dSz ), dSz + compSz, flags );
			if( bytes < 0 )
				return bytes;

			assert( bytes <= sz );
			sz -= bytes;
			p += bytes;
		}
		}
		delete [] buf;
		return dataSz;
	}
	else
		return send( sock, data, dataSz, flags );
}

int lzhl_recv( SOCKET sock, char* buf, int bufSz, int flags )
{
	GlobalMapType::iterator iter = globalMap.find( sock );
	if( iter != globalMap.end() )
	{
		LZHL_SOCKET& ls = (*iter).second;
		LZHL_DHANDLE& dh = ls.dh;
		if( dh == LZHL_DHANDLE_NULL )
			dh = LZHLCreateDecompressor();

		if( ls.dBuf == 0 )
		{
			unsigned int dataSz, compSz;

			BYTE tmp[ 10 ];
			int bytesRead = 0;
			int hdrSz;
			for(;;)
			{
				int bytes = recv( sock, (char*)( tmp + bytesRead ), 1, flags );
				if( bytes <= 0 )
					return bytes;
				bytesRead += bytes;

				BYTE* p = tmp;
				int err = _getInt( p, bytesRead, &dataSz );
				if( err == 1 )
					continue;//forever
				else if( err > 0 )
					return -1;

				err = _getInt( p, bytesRead - ( p - tmp ), &compSz );
				if( err == 1 )
					continue;//forever
				else if( err > 0 )
					return -1;

				hdrSz = p - tmp;
				break;//forever
			}

			BYTE* compBuf = new BYTE[ compSz ];
			bytesRead -= hdrSz;
			memcpy( compBuf, tmp + hdrSz, bytesRead );
			for(;;)
			{
				int bytes = recv( sock, (char*)( compBuf + bytesRead ), compSz - bytesRead, flags );
				if( bytes <= 0 )
				{
					delete [] compBuf;
					return bytes;
				}

				bytesRead += bytes;
				if( bytesRead == compSz )
					break;//forever
			}

			ls.dSz = dataSz;
			ls.dDisp = 0;
			ls.dBuf = new BYTE[ dataSz ];
			int Ok = LZHLDecompress( dh, ls.dBuf, &dataSz, compBuf, &compSz );
			delete [] compBuf;
			if( !Ok )
			{
				delete [] ls.dBuf;
				ls.dBuf = 0;
				return -1;
			}
		}//if( ls.dBuf == 0 )

		assert( ls.dBuf );
		int sz = min( bufSz, ls.dSz - ls.dDisp );
		memcpy( buf, ls.dBuf + ls.dDisp, sz );
		ls.dDisp += sz;
		if( ls.dDisp == ls.dSz )
		{
			delete [] ls.dBuf;
			ls.dBuf = 0;
		}
		return sz;
	}
	else
		return recv( sock, buf, bufSz, flags );
}

int lzhl_closesocket( SOCKET sock )
{
	GlobalMapType::iterator iter = globalMap.find( sock );
	if( iter != globalMap.end() )
	{
		LZHL_CHANDLE ch = (*iter).second.ch;
		if( ch != LZHL_CHANDLE_NULL )
			LZHLDestroyCompressor( ch );

		LZHL_DHANDLE dh = (*iter).second.dh;
		if( dh != LZHL_DHANDLE_NULL )
			LZHLDestroyDecompressor( dh );

		delete [] (*iter).second.dBuf;
	}
	return closesocket( sock );
}
