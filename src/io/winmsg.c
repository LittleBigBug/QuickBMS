#ifdef WIN32
#include <windows.h>



typedef struct {
    u32 /*UINT*/    msg;
    u32 /*WPARAM*/  wpar;
    u32 /*LPARAM*/  lpar;
} winmsg_msgparam_t;

typedef struct {
    HWND    hwnd;
    u8      *name;
    int     pos;
    int     size;
    void    *prev;
    void    *next;
} winmsg_file_t;



static  winmsg_file_t   *winmsg_file    = NULL;



int winmsg_close(winmsg_file_t *g) {
    // nothing to close
    g->hwnd = 0;
    return(0);
}



int winmsg_common(winmsg_file_t *g) {
    // it will be made everytime because it's ok

    g->hwnd = window_from_name(g->name);

    // if(!g->hwnd) desktop
    return(0);
}



winmsg_file_t *winmsg_open(u8 *fname) {
    static  int init_winmsg = 0;
    winmsg_file_t    *winmsgfile  = NULL,
                    *winmsgfile_tmp;
    u8      name[256] = "",
            proto[16]   = "";

    if(!strstr(fname, "://")) return(NULL);

    winmsgfile_tmp = calloc(1, sizeof(winmsg_file_t));
    if(!winmsgfile_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);

    sscanf(fname,
        "%10[^:]://%255[^,]",
        proto,
        name);

    if(stricmp(proto, "winmsg") && stricmp(proto, "message")) {
        FREE(winmsgfile_tmp);
        return(NULL);
    }
    if(!name[0]) {
        FREE(winmsgfile_tmp);
        return(NULL);
    }

    if(!enable_winmsg) {
        fprintf(stderr,
            "\n"
            "Error: the script uses the Windows messages, if you are SURE about the\n"
            "       genuinity of this script\n"
            "\n"
            "         you MUST use the -m or -winmsg option at command-line.\n"
            "\n"
            "       you MUST really sure about the script you are using and what you are\n"
            "       doing because this is NOT a feature for extracting files!\n");
        myexit(QUICKBMS_ERROR_EXTRA);
    }
    if(!init_winmsg) {
        init_winmsg = 1;
    }

    if(!stricmp(name, "null") || !stricmp(name, "none")) name[0] = 0;
    winmsgfile_tmp->name = mystrdup_simple(name);

    for(winmsgfile = winmsg_file; winmsgfile; winmsgfile = winmsgfile->next) {
        if(
            !stricmp(winmsgfile->name, winmsgfile_tmp->name)
        ) {
            FREE(winmsgfile_tmp->name);
            FREE(winmsgfile_tmp);
            winmsgfile_tmp = NULL;
            break;
        }
    }
    if(!winmsgfile) {
        if(!winmsg_file) {
            winmsg_file = winmsgfile_tmp;
            winmsgfile = winmsg_file;
        } else {
            // get the last element
            for(winmsgfile = winmsg_file;; winmsgfile = winmsgfile->next) {
                if(winmsgfile->next) continue;
                winmsgfile->next = winmsgfile_tmp;
                winmsgfile_tmp->prev = winmsgfile;
                winmsgfile = winmsgfile_tmp;
                break;
            }
        }
    }

    winmsg_common(winmsgfile);
    fprintf(stderr, "- open Windows message: %s (%08x)\n",
        winmsgfile->name,
        (i32)winmsgfile->hwnd);
    return(winmsgfile);
}



int winmsg_read(winmsg_file_t *g, u8 *data, int size) {
    winmsg_msgparam_t   *mp;
    MSG     msg;
    int     len;

    winmsg_common(g);

    // does NOT work!

    len = 0;
    while(len < size) {
        if(!PeekMessage(&msg, g->hwnd, 0, 0, PM_NOREMOVE)) break;
        mp = (void *)(data + len);

        if((len + sizeof(mp->msg)) > size) break;
        mp->msg  = msg.message;
        len += sizeof(mp->msg);

        if((len + sizeof(mp->wpar)) > size) break;
        mp->wpar = msg.wParam;
        len += sizeof(mp->wpar);

        if((len + sizeof(mp->lpar)) > size) break;
        mp->lpar = msg.lParam;
        len += sizeof(mp->lpar);
    }
    return(len);
}



int winmsg_write(winmsg_file_t *g, u8 *data, int size) {
    winmsg_common(g);

    // all this boring static stuff is needed
    // to allow an easy collecting of the 3
    // needed parameters
    static winmsg_msgparam_t   mp = {0,0,0};
    static u8  mpl = 0;
    int     len,
            t;

    for(len = 0; len < size; len += t) {
        t = sizeof(mp);
        if(mpl) t -= mpl;
        if((size - len) < t) t = size - len;
        memcpy((void *)&mp + mpl, data + len, t);
        mpl += t;
        if(mpl >= sizeof(mp)) {
            mpl = 0;
            if(!PostMessage(g->hwnd, mp.msg, mp.wpar, mp.lpar)) break;
        }
    }
    return(len);
}



#else

typedef struct {
    u8      *name;
    int     pos;
    int     size;
} winmsg_file_t;
static  winmsg_file_t   *winmsg_file    = NULL;
winmsg_file_t *winmsg_open(u8 *fname) { return(NULL); }
int winmsg_read(winmsg_file_t *winmsgfile, u8 *data, int size) { return(-1); }
int winmsg_write(winmsg_file_t *winmsgfile, u8 *data, int size) { return(-1); }
int winmsg_close(winmsg_file_t *winmsgfile) { return(-1); }

#endif
