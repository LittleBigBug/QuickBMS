#ifdef WIN32
#include <tlhelp32.h>



static HWND window_hwnd = 0;



u8 *get_filename(u8 *);



BOOL CALLBACK window_class2hwnd(HWND h, LPARAM lParam) {
    u8      tmp[128];

    GetClassName(h, tmp, sizeof(tmp) - 1);
    if(!stricmp(tmp, (u8 *)lParam)) {
        window_hwnd = h;
        return(FALSE);
    }
    return(TRUE);
}



BOOL CALLBACK window_pid2hwnd(HWND h, LPARAM lParam) {
    DWORD   p;

    GetWindowThreadProcessId(h, &p);
    if(p == (DWORD)lParam) {
        window_hwnd = h;
        return(FALSE);
    }
    return(TRUE);
}



HWND window_exe2hwnd(u8 *name) {
    PROCESSENTRY32  Process;
    HANDLE  snapProcess;
    int     b;
    int     ext = 0,
            len;
    u8      *p,
            *l;

    p = strrchr(name, '.');
    if(p) ext = 1;
    len = strlen(name);

#define START(X,Y) \
            snap##X = CreateToolhelp32Snapshot(Y, Process.th32ProcessID); \
            X.dwSize = sizeof(X); \
            for(b = X##32First(snap##X, &X); b; b = X##32Next(snap##X, &X)) { \
                X.dwSize = sizeof(X);
#define END(X) \
            } \
            CloseHandle(snap##X);

    Process.th32ProcessID = 0;
    START(Process, TH32CS_SNAPPROCESS)

        p = get_filename(Process.szExeFile);

        if(ext) {
            l = p + strlen(p);
        } else {
            l = strrchr(p, '.');
            if(!l) l = p + strlen(p);
        }

        if(((l - p) == len) && !strnicmp(p, name, l - p)) {
            EnumWindows(window_pid2hwnd, (LPARAM)Process.th32ProcessID);
            break;
        }

    END(Process)

#undef START
#undef END
    return(window_hwnd);
}



HWND window_from_name(u8 *name) {
    if(!name || !name[0]) return(0);    // desktop

    // findwindow
    window_hwnd = FindWindow(NULL, name);

    // classname
    if(!window_hwnd) EnumWindows(window_class2hwnd, (LPARAM)name);

    // process name
    if(!window_hwnd) window_hwnd = window_exe2hwnd(name);

    // window number
    if(!window_hwnd && (strlen(name) == 8)) {
        window_hwnd = (HWND)(u32)readbase(name, 16, NULL); // default hex
    }

    return(window_hwnd);
}

#endif

