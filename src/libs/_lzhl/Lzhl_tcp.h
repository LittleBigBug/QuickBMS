/*
 *  LZHL_TCP description v 1.0
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Warning: this version is only a demonstration for LZHL algorithm usage;
 *           non-blocking sockets and OOB aren't supported
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
SOCKET lzhl_socket( int af, int type, int protocol );
SOCKET lzhl_accept( SOCKET s, struct sockaddr* addr, int* addrlen );
int lzhl_send( SOCKET s, const char* buf, int len, int flags );
int lzhl_recv( SOCKET s, char* buf, int len, int flags );
int lzhl_closesocket( SOCKET sock );
#ifdef __cplusplus
}
#endif
