#ifdef WIN32



typedef struct {
    HDC     screen;
    HDC     window;
    HWND    hwnd;
    HWND    myhwnd;
    HBITMAP bmp;
    u8      *name;
    i32     x;      // desired width
    i32     y;      // desired height
    i32     b;      // desired bits
    i32     width;
    i32     height;
    i32     bpp;
    int     pos;
    int     size;
    void    *prev;
    void    *next;
} video_file_t;



static  video_file_t   *video_file    = NULL;



int video_close(video_file_t *g) {
    if(g->bmp)    { DeleteObject(g->bmp);   g->bmp    = NULL; }
    if(g->window) { DeleteDC(g->window);    g->window = NULL; }
    if(g->screen) {
        ReleaseDC(g->hwnd, g->screen);
        DeleteDC(g->screen);
        g->screen = NULL;
    }
    if(g->myhwnd) {
        //DestroyWindow(g->myhwnd); // destroyed in video_WndProc
        g->myhwnd = 0;
    }
    return(0);
}



BOOL CALLBACK video_EnumWindowsProc(HWND h, LPARAM lParam) {
    u8      tmp[128];

    GetClassName(h, tmp, sizeof(tmp) - 1);
    if(!stricmp(tmp, ((video_file_t *)lParam)->name)) {
        ((video_file_t *)lParam)->hwnd = h;
        return(FALSE);
    }
    return(TRUE);
}



LRESULT CALLBACK video_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;

    switch(message) {
        case WM_PAINT: {
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SETFOCUS: {
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return(0);
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return(0);
            break;
        }
        default: break;
    }
    return(DefWindowProc(hwnd, message, wParam, lParam));
}



int video_init(video_file_t *g) {
    RECT    dim;
    int     screen_width,
            screen_height;

    // I reopen the device each time because the windows
    // of the normal programs could have changed size

    video_close(g); // in case it's not freed yet

    g->width  = g->x;
    g->height = g->y;
    g->bpp    = g->b;

    g->hwnd = window_from_name(g->name);
    if(g->hwnd || (g->width <= 0) || (g->height <= 0) || (g->bpp <= 0)) {
        g->screen = GetDC(g->hwnd);
        if(!g->screen) return(-1);
        screen_width  = GetDeviceCaps(g->screen, HORZRES);
        screen_height = GetDeviceCaps(g->screen, VERTRES);
        g->bpp        = GetDeviceCaps(g->screen, BITSPIXEL);
        if(g->hwnd) {
            GetClientRect(g->hwnd, &dim);
            g->width  = dim.right  - dim.left;
            g->height = dim.bottom - dim.top;
            if((g->width  + dim.left) > screen_width)  g->width  = screen_width  - dim.left;
            if((g->height + dim.top)  > screen_height) g->height = screen_height - dim.top;
        } else {
            g->width  = screen_width;
            g->height = screen_height;
        }
        if((g->x > 0) && (g->x < g->width))  g->width  = g->x;
        if((g->y > 0) && (g->y < g->height)) g->height = g->y;
        if((g->b > 0) && (g->b < g->bpp))    g->bpp    = g->b;
        if((g->width < 0) || (g->height < 0) || (g->bpp % 8)) return(-1);
    }
    g->size = g->height * g->width * (g->bpp / 8);
    return(0);
}



video_file_t *video_open(u8 *fname) {
    static  int init_video = 0;
    video_file_t    *videofile  = NULL,
                    *videofile_tmp;
    u8      name[256] = "",
            proto[16] = "",
            c;

    if(!strstr(fname, "://")) return(NULL);

    videofile_tmp = calloc(1, sizeof(video_file_t));
    if(!videofile_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);

    sscanf(fname,
        "%10[^:]://%255[^,],%d%c%d%c%d",
        proto,
        name,
        &videofile_tmp->x,
        &c,
        &videofile_tmp->y,
        &c,
        &videofile_tmp->b);

    if(stricmp(proto, "video") && stricmp(proto, "graphic")) {
        FREE(videofile_tmp);
        return(NULL);
    }
    if(!name[0]) {
        FREE(videofile_tmp);
        return(NULL);
    }

    if(!enable_video) {
        fprintf(stderr,
            "\n"
            "Error: the script uses the video graphic device, if you are SURE about the\n"
            "       genuinity of this script\n"
            "\n"
            "         you MUST use the -g or -video option at command-line.\n"
            "\n"
            "       you MUST really sure about the script you are using and what you are\n"
            "       doing because this is NOT a feature for extracting files!\n");
        myexit(QUICKBMS_ERROR_EXTRA);
    }
    if(!init_video) {
        init_video = 1;
    }

    if(!stricmp(name, "null") || !stricmp(name, "none")) name[0] = 0;
    videofile_tmp->name = mystrdup_simple(name);

    for(videofile = video_file; videofile; videofile = videofile->next) {
        if(
            !stricmp(videofile->name, videofile_tmp->name)
        ) {
            FREE(videofile_tmp->name);
            FREE(videofile_tmp);
            videofile_tmp = NULL;
            break;
        }
    }
    if(!videofile) {
        if(!video_file) {
            video_file = videofile_tmp;
            videofile = video_file;
        } else {
            // get the last element
            for(videofile = video_file;; videofile = videofile->next) {
                if(videofile->next) continue;
                videofile->next = videofile_tmp;
                videofile_tmp->prev = videofile;
                videofile = videofile_tmp;
                break;
            }
        }
    }

    video_init(videofile);
    fprintf(stderr, "- open video graphic device: %dx%dx%d (%d bytes)\n",
        videofile->width,
        videofile->height,
        videofile->bpp,
        videofile->size);
    video_close(videofile);
    return(videofile);
}



int video_read(video_file_t *g, u8 *data, int size) {
    int     ret_size = size;

    video_init(g);

    g->screen = GetDC(g->hwnd);
    if(!g->screen) return(-1);
    g->window = CreateCompatibleDC(g->screen);
    if(!g->window) return(-1);
    g->bmp = CreateCompatibleBitmap(g->screen, g->width, g->height);
    if(!g->bmp) return(-1);
    if(!SelectObject(g->window, g->bmp)) return(-1);

    if(size > g->size) size = g->size;
    if(!BitBlt(g->window, 0, 0, g->width, g->height, g->screen, 0, 0, SRCCOPY)) return(-1);
    if(!GetBitmapBits(g->bmp, size, data)) return(-1);

    video_close(g);
    return(ret_size);
}



int video_write(video_file_t *g, u8 *data, int size) {
    static const u8 video_title[] = "QuickBMS video";
    HINSTANCE hInstance = NULL;
    WNDCLASS wndclass;
    MSG     msg;
    int     ret_size = size;

    video_init(g);

    if(!g->hwnd) {
        // a window is created if none is specified or available
        wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wndclass.lpfnWndProc   = video_WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon(NULL, NULL);
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = video_title;
        RegisterClass(&wndclass);

        g->hwnd = CreateWindow(
            video_title,
            video_title,
            WS_OVERLAPPEDWINDOW, 
            CW_USEDEFAULT, CW_USEDEFAULT,
            g->width, g->height, 
            NULL, NULL, hInstance, NULL);
        if(g->hwnd) {
            ShowWindow(g->hwnd, SW_SHOW);
            UpdateWindow(g->hwnd);
            g->myhwnd = g->hwnd;
        }
    }

    g->screen = GetDC(g->hwnd);
    if(!g->screen) return(-1);
    g->window = CreateCompatibleDC(g->screen);
    if(!g->window) return(-1);
    g->bmp = CreateCompatibleBitmap(g->screen, g->width, g->height);
    if(!g->bmp) return(-1);
    if(!SelectObject(g->window, g->bmp)) return(-1);

    if(size > g->size) size = g->size;
    if(!SetBitmapBits(g->bmp, size, data)) return(-1);
    if(!BitBlt(g->screen, 0, 0, g->width, g->height, g->window, 0, 0, SRCCOPY)) return(-1);

    if(g->myhwnd) {
        while(GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    video_close(g);
    return(ret_size);
}



#else

typedef struct {
    u8      *name;
    int     pos;
    int     size;
} video_file_t;
static  video_file_t   *video_file    = NULL;
video_file_t *video_open(u8 *fname) { return(NULL); }
int video_read(video_file_t *videofile, u8 *data, int size) { return(-1); }
int video_write(video_file_t *videofile, u8 *data, int size) { return(-1); }
int video_close(video_file_t *videofile) { return(-1); }

#endif
