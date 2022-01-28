/*
    Copyright 2011-2012 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

#ifdef WIN32
    #define quick_thread(NAME, ARG) DWORD WINAPI NAME(ARG)
    #define thread_id   HANDLE
#else
    #include <pthread.h>
    #include <signal.h>
    #define quick_thread(NAME, ARG) void *NAME(ARG)
    #define thread_id   pthread_t
#endif

thread_id quick_threadx(void *func, void *data) {
    thread_id   tid;
#ifdef WIN32
    DWORD   tmp;

    tid = CreateThread(NULL, 0, func, data, 0, &tmp);
    if(!tid) return 0;
#else
    if(pthread_create(&tid, NULL, func, data)) return 0;
#endif
    return(tid);
}

void quick_threadz(thread_id tid) {
#ifdef WIN32
    DWORD   ret;

    for(;;) {
        if(!GetExitCodeThread(tid, &ret)) break;
        if(!ret) break;
        Sleep(100);
    }
#else
    pthread_join(tid, NULL);
#endif
}

void quick_threadk(thread_id tid) {
#ifdef WIN32
    TerminateThread(tid, 0);
#else
    pthread_kill(tid, SIGINT);
#endif
}
