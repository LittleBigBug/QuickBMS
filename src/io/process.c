#ifdef WIN32
    #include <psapi.h>
    #include <tlhelp32.h>

            #ifndef TH32CS_SNAPMODULE32
            #define TH32CS_SNAPMODULE32 0
            #endif

    //typedef DWORD   pid_t;
#else
    #include <sys/ptrace.h>

    #ifdef __APPLE__
        #define PTRACE_TRACEME      PT_TRACE_ME
        #define PTRACE_PEEKTEXT     PT_READ_I
        #define PTRACE_PEEKDATA     PT_READ_D
        #define PTRACE_PEEKUSER     PT_READ_U
        #define PTRACE_POKETEXT     PT_WRITE_I
        #define PTRACE_POKEDATA     PT_WRITE_D
        #define PTRACE_POKEUSER     PT_WRITE_U
        #define PTRACE_CONT         PT_CONTINUE
        #define PTRACE_KILL         PT_KILL
        #define PTRACE_SINGLESTEP   PT_STEP
        #define PTRACE_GETREGS      PT_GETREGS
        #define PTRACE_SETREGS      PT_SETREGS
        #define PTRACE_GETFPREGS    PT_GETFPREGS
        #define PTRACE_SETFPREGS    PT_SETFPREGS
        #define PTRACE_ATTACH       PT_ATTACH
        #define PTRACE_DETACH       PT_DETACH
        #define PTRACE_GETFPXREGS   PT_GETFPXREGS
        #define PTRACE_SETFPXREGS   PT_SETFPXREGS
        #define PTRACE_SYSCALL      PT_SYSCALL
        #define PTRACE_SETOPTIONS   PT_SETOPTIONS
        #define PTRACE_GETEVENTMSG  PT_GETEVENTMS
        #define PTRACE_GETSIGINFO   PT_GETSIGINFO
        #define PTRACE_SETSIGINFO   PT_SETSIGINFO
    #endif

    //typedef uint32_t    DWORD;
    typedef uint32_t    HANDLE;
    typedef int32_t     BOOL;
#endif



typedef struct {
    void    *addr;
    u8      byte;
} process_int3_t;



typedef struct {
    HANDLE      hp;
    u8          *name;
    u8          *module;
    thread_id   tid;
    int         debug;
    DWORD       pid;
    u8          *base;
    u8          *pos;
    int         size;
    void        *prev;
    void        *next;
} process_file_t;



static int              g_page_size     = 0;
static int              process_int3s   = 0;
static process_int3_t   *process_int3   = NULL; // currently it's just an array and not a linked list, maybe in future... experimental!
static process_file_t   *process_file   = NULL;



u8 *get_filename(u8 *);



int get_page_size(void) {
    int     ret;

    #ifdef WIN32
    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    ret = sSysInfo.dwPageSize;
    #else
    ret = getpagesize();
    #endif
    if(!ret) ret = 4096;
    return ret;
}



#ifdef WIN32
#define ISHANDLEOK(X)   (X && (X != INVALID_HANDLE_VALUE))
SIZE_T readwrite_mem(HANDLE hp, u8 *addr, void *buff, SIZE_T size, int w) {
    static u8   *tmp_buff   = NULL;
    MEMORY_BASIC_INFORMATION    mbi;
    DWORD   attr,
            old_attr;
    SIZE_T  t,
            i,
            len;
    BOOL    rw;

    if(!ISHANDLEOK(hp)) return 0;

    if(!g_page_size) g_page_size = get_page_size();
    if(buff) {
        if(!w) memset(buff, 0, size);
    } else {
        if(!tmp_buff) {
            tmp_buff = malloc(g_page_size);
            if(!tmp_buff) return 0;
        }
        memset(tmp_buff, 0, g_page_size);
        buff = tmp_buff;
    }

    // quickbms
    if(w) {
        if((size == 1) && (((u8 *)buff)[0] == 0xcc)) {
            int     i;
            u8      c;
            for(i = 0; i < process_int3s; i++) {
                if(addr == process_int3[i].addr) break;
            }
            if(i >= process_int3s) {
                if(readwrite_mem(hp, addr, &c, 1, 0) == 1) {
                    process_int3 = realloc(process_int3, (process_int3s + 1) * sizeof(process_int3_t));
                    if(!process_int3) STD_ERR(QUICKBMS_ERROR_MEMORY);
                    process_int3[process_int3s].addr = addr;
                    process_int3[process_int3s].byte = c;
                    process_int3s++;
                }
            }
        }
    }
    // end quickbms

    // the process should be paused before performing this operation

    for(i = 0; i < size; i += len) {
        int align = (SIZE_T)(addr + i) & (g_page_size - 1);
        len = g_page_size - align;
        if((i + len) > size) len = size - i;

        // try to read/write before checking and setting the attributes
        if(w) rw = WriteProcessMemory(hp, addr + i, buff + i, len, &t);
        else  rw = ReadProcessMemory (hp, addr + i, buff + i, len, &t);

        if(!rw) {
            attr = PAGE_READWRITE;
            if(VirtualQueryEx(hp, addr + i - align, &mbi, g_page_size)) {
                attr = mbi.Protect;
                switch(attr & 0xff) {
                    case PAGE_EXECUTE: attr = (attr & ~0xff) | (w ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ); break;
                    case PAGE_EXECUTE_READ: if(w) attr = (attr & ~0xff) | PAGE_EXECUTE_READWRITE; break;
                    case PAGE_EXECUTE_READWRITE: break;
                    case PAGE_EXECUTE_WRITECOPY: attr = (attr & ~0xff) | PAGE_EXECUTE_READWRITE; break;
                    case PAGE_NOACCESS: attr = (attr & ~0xff) | (w ? PAGE_READWRITE : PAGE_READONLY); break;
                    case PAGE_READONLY: if(w) attr = (attr & ~0xff) | PAGE_READWRITE; break;
                    case PAGE_READWRITE: break;
                    case PAGE_WRITECOPY: attr = (attr & ~0xff) | PAGE_READWRITE; break;
                    default: break;
                }
            }

            if(VirtualProtectEx(hp, addr + i - align, g_page_size, attr, &old_attr)) {

                if(w) WriteProcessMemory(hp, addr + i, buff + i, len, &t);
                else  ReadProcessMemory (hp, addr + i, buff + i, len, &t);

                VirtualProtectEx(hp, addr + i - align, g_page_size, old_attr, &attr);
                if(w) {
                    switch(old_attr & 0xff) {
                        case PAGE_EXECUTE:
                        case PAGE_EXECUTE_READ:
                        case PAGE_EXECUTE_READWRITE:
                        case PAGE_EXECUTE_WRITECOPY:
                            FlushInstructionCache(hp, addr + i, len);
                            break;
                        default: break;
                    }
                }
            }
        }
    }
    return(i);
}
#endif



u8 *process_list(u8 *myname, u8 *mymodule, DWORD *mypid, DWORD *size) {
#ifdef WIN32
    PROCESSENTRY32  Process;
    MODULEENTRY32   Module;
    HANDLE          snapProcess,
                    snapModule;
    DWORD           retpid = 0;
    int             len;
    BOOL            b;
    u8              tmpbuff[60],
                    *process_name,
                    *module_name,
                    *module_print,
                    *tmp;

    if(mypid) retpid = *mypid;
    if(!myname && !retpid) {
        fprintf(stderr,
            "  pid/addr/size       process/module name\n"
            "  ---------------------------------------\n");
    }

#define PROCESS_START(X,Y,PID) \
            snap##X = CreateToolhelp32Snapshot(Y, PID); \
            X.dwSize = sizeof(X); \
            for(b = X##32First(snap##X, &X); b; b = X##32Next(snap##X, &X)) { \
                X.dwSize = sizeof(X);
#define PROCESS_END(X) \
            } \
            CloseHandle(snap##X);

    Process.th32ProcessID = 0;
    PROCESS_START(Process, TH32CS_SNAPPROCESS, Process.th32ProcessID)
        process_name = Process.szExeFile;

        if(!myname && !retpid) {
            fprintf(stderr, "  %-10lu ******** %s\n",
                Process.th32ProcessID,
                process_name);
        }
        if(myname && stristr(process_name, myname)) {
            retpid = Process.th32ProcessID;
        }

        // important note, you can access only
        // the 32bit modules if my code is compiled at 32bit
        // while if it's compiled at 64bit you can access both 32 and 64

        PROCESS_START(Module, TH32CS_SNAPMODULE, Process.th32ProcessID)
            module_name = Module.szExePath; // szModule?

            len = strlen(module_name);
            if(len >= 60) {
                tmp = get_filename(module_name);
                len -= (tmp - module_name);
                snprintf(tmpbuff, sizeof(tmpbuff),
                    "%.*s...%s",
                    54 - len,
                    module_name,
                    tmp);
                module_print = tmpbuff;
            } else {
                module_print = module_name;
            }

            if(!myname && !retpid) {
                fprintf(stderr, "    %p %08lx %s\n",
                    Module.modBaseAddr,
                    Module.modBaseSize,
                    module_print);
            }
            /*
            if(!retpid) { // in origin myname was valid for both processes and modules, too confusing
                if(myname && stristr(module_name, myname)) {
                    retpid = Process.th32ProcessID;
                }
            }
            */
            if(retpid && mypid && (Process.th32ProcessID == retpid)) {
                if(mymodule && mymodule[0]) {
                    if(stristr(module_name, mymodule)) goto done;
                } else {
                    goto done;
                }
            }

        PROCESS_END(Module)

    PROCESS_END(Process)

//#undef PROCESS_START
//#undef PROCESS_END

            // access 64bit processes without module information
            // you can't really access these processes from a 32bit
            // one but let give a chance
            if(retpid && mypid) {
                Module.modBaseAddr = (void *)0x00400000;    // 0x100000000LL without ASLR
                Module.modBaseSize = ((u_int)-1) >> 1;
                module_print = "64bit stub";
                goto done;
            }

    return NULL;

done:
    fprintf(stderr, "- %p %08lx %s\n",
        Module.modBaseAddr,
        Module.modBaseSize,
        module_print);
    *mypid = retpid;
    if(size) *size = Module.modBaseSize;
    return(Module.modBaseAddr);

#else

    //system("ps -eo pid,cmd");
    fprintf(stderr, "\n"
        "- use ps to know the pids of your processes, like:\n"
        "  ps -eo pid,cmd\n");

    return NULL;
#endif
}



int process_close(process_file_t *procfile) {
    if(procfile->hp) {
#ifdef WIN32
        CloseHandle(procfile->hp);
#else
        ptrace(PTRACE_DETACH, procfile->pid, NULL, NULL);
#endif
        procfile->hp = 0;
    }
    return 0;
}



// taken directly from udpsz
// note that this is only a temporary
// solution

#ifdef WIN32
static BOOL WINAPI (*_DebugSetProcessKillOnExit)(BOOL) = NULL;
static BOOL WINAPI (*_DebugActiveProcessStop)(DWORD) = NULL;
static HANDLE WINAPI (*_OpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId) = NULL;
int debug_missing(void) {
    static HMODULE kernel32 = NULL;

    if(!kernel32) kernel32 = GetModuleHandle("kernel32.dll");   // LoadLibrary may be dangerous
    if(kernel32) {
        if(!_DebugSetProcessKillOnExit)
            _DebugSetProcessKillOnExit = (void *)GetProcAddress(kernel32, "DebugSetProcessKillOnExit");
        if(!_DebugActiveProcessStop)
            _DebugActiveProcessStop = (void *)GetProcAddress(kernel32, "DebugActiveProcessStop");
        if(!_OpenThread)
            _OpenThread = (void *)GetProcAddress(kernel32, "OpenThread");
        return 0;
    }
    return -1;
}



u8 *show_exception(DWORD status) {
    static u8   tmp[32];
    u8          *ret = "";

    switch(status) {
        case STATUS_ACCESS_VIOLATION:           ret = "ACCESS_VIOLATION"; break;
        case STATUS_IN_PAGE_ERROR:              ret = "IN_PAGE_ERROR"; break;
        case STATUS_INVALID_HANDLE:             ret = "INVALID_HANDLE"; break;
        case STATUS_NO_MEMORY:                  ret = "NO_MEMORY"; break;
        case STATUS_ILLEGAL_INSTRUCTION:        ret = "ILLEGAL_INSTRUCTION"; break;
        case STATUS_NONCONTINUABLE_EXCEPTION:   ret = "NONCONTINUABLE_EXCEPTION"; break;
        case STATUS_INVALID_DISPOSITION:        ret = "INVALID_DISPOSITION"; break;
        case STATUS_ARRAY_BOUNDS_EXCEEDED:      ret = "ARRAY_BOUNDS_EXCEEDED"; break;
        case STATUS_FLOAT_DENORMAL_OPERAND:     ret = "FLOAT_DENORMAL_OPERAND"; break;
        case STATUS_FLOAT_DIVIDE_BY_ZERO:       ret = "FLOAT_DIVIDE_BY_ZERO"; break;
        case STATUS_FLOAT_INEXACT_RESULT:       ret = "FLOAT_INEXACT_RESULT"; break;
        case STATUS_FLOAT_INVALID_OPERATION:    ret = "FLOAT_INVALID_OPERATION"; break;
        case STATUS_FLOAT_OVERFLOW:             ret = "FLOAT_OVERFLOW"; break;
        case STATUS_FLOAT_STACK_CHECK:          ret = "FLOAT_STACK_CHECK"; break;
        case STATUS_FLOAT_UNDERFLOW:            ret = "FLOAT_UNDERFLOW"; break;
        case STATUS_INTEGER_DIVIDE_BY_ZERO:     ret = "INTEGER_DIVIDE_BY_ZERO"; break;
        case STATUS_INTEGER_OVERFLOW:           ret = "INTEGER_OVERFLOW"; break;
        case STATUS_PRIVILEGED_INSTRUCTION:     ret = "PRIVILEGED_INSTRUCTION"; break;
        case STATUS_STACK_OVERFLOW:             ret = "STACK_OVERFLOW"; break;
        case STATUS_CONTROL_C_EXIT:             ret = "CONTROL_C_EXIT"; break;
        //case STATUS_DLL_INIT_FAILED:            ret = "DLL_INIT_FAILED"; break;
        //case STATUS_DLL_INIT_FAILED_LOGOFF:     ret = "DLL_INIT_FAILED_LOGOFF"; break;
        default: {
            sprintf(tmp, "%08x", (int)status);
            ret = tmp;
            break;
        }
    }
    return ret;
}



QUICKBMS_int add_var(QUICKBMS_int idx, u8 *name, u8 *val, QUICKBMS_int val32, QUICKBMS_int valsz);



int quickbms_debug_context(DEBUG_EVENT *dbg) {
#if defined(i386)
    CONTEXT ctx;
    HANDLE  ht,
            hp;
    int     i;

    if(!_OpenThread) return -1;
    ht = _OpenThread(THREAD_ALL_ACCESS, FALSE, dbg->dwThreadId);
    if(!ht) return -1;

    memset(&ctx, 0, sizeof(CONTEXT));
    ctx.ContextFlags = CONTEXT_FULL;
    if(GetThreadContext(ht, &ctx)) {
        if(dbg->u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
            ctx.Eip--;  // show EIP of the breakpoint
        }

        add_var(0, "Eax", NULL, ctx.Eax, sizeof(int));
        add_var(0, "Ecx", NULL, ctx.Ecx, sizeof(int));
        add_var(0, "Edx", NULL, ctx.Edx, sizeof(int));
        add_var(0, "Ebx", NULL, ctx.Ebx, sizeof(int));
        add_var(0, "Esp", NULL, ctx.Esp, sizeof(int));
        add_var(0, "Ebp", NULL, ctx.Ebp, sizeof(int));
        add_var(0, "Esi", NULL, ctx.Esi, sizeof(int));
        add_var(0, "Edi", NULL, ctx.Edi, sizeof(int));
        add_var(0, "Eip", NULL, ctx.Eip, sizeof(int));
        add_var(0, "EFlags", NULL, ctx.EFlags, sizeof(int));
        add_var(0, "ExtendedRegisters", ctx.ExtendedRegisters, 0, MAXIMUM_SUPPORTED_EXTENSION);

        // automatically restore breakpoint
        if(dbg->u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
            for(i = 0; i < process_int3s; i++) {
                if((void *)ctx.Eip == process_int3[i].addr) break;
            }
            if(i < process_int3s) {
                ctx.EFlags |= 0x0100;
                SetThreadContext(ht, &ctx);
                hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dbg->dwProcessId);
                if(hp) {
                    readwrite_mem(hp, (void *)ctx.Eip, &(process_int3[i].byte), 1, 1);
                    FlushInstructionCache(hp, (void *)ctx.Eip, 1);
                    CloseHandle(hp);
                }
            }
        }
    }
    CloseHandle(ht);
#endif
    return 0;
}



u8 *debug_get_string(HANDLE hp, void *addr, int unicode, int write4) {
    static u8   tmp[2048 + 1] = "";
    void    *ptr    = NULL;
    int     i,
            c,
            len;
    u8      *p,
            *s;

    if(!addr) return("");
    if(write4) {
        if(readwrite_mem(hp, addr, &ptr, sizeof(void *), 0) != sizeof(void *)) return("");
        s = (u8 *)ptr;
    } else {
        s = addr;
    }
    tmp[0] = 0;
    len = 0;
    if(s) {
        p = tmp;
        for(i = 0; i < (sizeof(tmp) - 1); i++) {
            if(readwrite_mem(hp, s + i, &c, 1, 0) != 1) break;

            // this is not a real unicode, just a lame utf16 to ASCII conversion
            if(!(unicode && (i & 1))) {
                *p++ = c;
                if(!c) break;
            }
        }
        len = p - tmp;
        for(i = 0; i < len; i++) {
            if((tmp[i] >= '\t') && (tmp[i] <= 'z')) continue;
            break;
        }
        len = i;
    }
    if(len < 4) {
        if(!write4) return(debug_get_string(hp, addr, unicode, 1));
        tmp[0] = 0;
    }
    return(tmp);
}



quick_thread(process_debugger, process_file_t *proc) {
    DEBUG_EVENT *dbg = NULL;
    DWORD   pid;
    int     i,
            dbg_do,
            first_bp    = 1;
    u8      ans[16],
            *p;

    debug_missing();    // for OpenThread

    if(!proc) return 0;
    pid = proc->pid;
    if(!pid) return 0;

    for(i = 5; i >= 0; i--) {
        if(DebugActiveProcess(pid)) break;
        Sleep(ONESEC);
    }
    if(i < 0) goto quit; //winerr(0, NULL);
    printf("- debugger attached to pid %d\n", (int)pid);

    dbg = calloc(1, sizeof(DEBUG_EVENT));
    if(!dbg) STD_ERR(QUICKBMS_ERROR_MEMORY);

    while(proc->debug) {
        if(!WaitForDebugEvent(dbg, 500)) continue;
        dbg_do = DBG_CONTINUE;

        if(dbg->dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "EXCEPTION_DEBUG_EVENT", 0, -1);

            add_var(0, "ExceptionCode", NULL, (int)dbg->u.Exception.ExceptionRecord.ExceptionCode, sizeof(int));

            if(
                ((dbg->u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) && !first_bp) ||
                ((dbg->u.Exception.ExceptionRecord.ExceptionCode & 0xff000000) == 0xc0000000)
            ) {

                // dbg->u.Exception.dwFirstChance not handled to catch also the non critical errors
                //if(!dbg->u.Exception.dwFirstChance) {

                    add_var(0, "ExceptionAddress", NULL, (int)dbg->u.Exception.ExceptionRecord.ExceptionAddress, sizeof(int));

                    printf("\n\nDEBUG exception: %08x %s:",
                        (int)dbg->u.Exception.ExceptionRecord.ExceptionAddress,
                        show_exception(dbg->u.Exception.ExceptionRecord.ExceptionCode));
                    for(i = 0; i < dbg->u.Exception.ExceptionRecord.NumberParameters; i++) {
                        printf(" %08x", (int)dbg->u.Exception.ExceptionRecord.ExceptionInformation[i]);
                    }
                    printf("\n");

                    // set variables and reset INT3 if breakpoint
                    quickbms_debug_context(dbg);

                    Sleep(200); // useless, it's only to avoid output artefacts in quickbms
                    fprintf(stderr,
                        "\n"
                        "- Press ENTER when you want to try to continue the execution of the\n"
                        "  debugged program (maybe the script is doing something else in the\n"
                        "  background. In the meantime QuickBMS will continue to work separately\n"
                        "\n");
                    fgets(ans, sizeof(ans), stdin);
                    // wait user input
                    fprintf(stderr, "\n- DEBUG continue\n");

                //}
            }

            if(dbg->u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT) {
                if(first_bp) first_bp = 0;
            } else if(dbg->u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP) {
                // DBG_CONTINUE
            } else {
                dbg_do = DBG_EXCEPTION_NOT_HANDLED;
            }

        // Thread create
        } else if(dbg->dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "CREATE_THREAD_DEBUG_EVENT", 0, -1);
            if(g_verbose) printf("DEBUG thread        %08x %p %p\n",
                (int)dbg->u.CreateThread.hThread,
                dbg->u.CreateThread.lpThreadLocalBase,
                dbg->u.CreateThread.lpStartAddress);

        // Process create
		} else if(dbg->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "CREATE_PROCESS_DEBUG_EVENT", 0, -1);
            p = debug_get_string(dbg->u.CreateProcessInfo.hProcess, dbg->u.CreateProcessInfo.lpImageName, dbg->u.CreateProcessInfo.fUnicode, 1);
            if(g_verbose) printf("DEBUG process       %08x %08x %08x %p %08x %08x %p %p %s\n",
                (int)dbg->u.CreateProcessInfo.hFile,
                (int)dbg->u.CreateProcessInfo.hProcess,
                (int)dbg->u.CreateProcessInfo.hThread,
                dbg->u.CreateProcessInfo.lpBaseOfImage,
                (int)dbg->u.CreateProcessInfo.dwDebugInfoFileOffset,
                (int)dbg->u.CreateProcessInfo.nDebugInfoSize,
                dbg->u.CreateProcessInfo.lpThreadLocalBase,
                dbg->u.CreateProcessInfo.lpStartAddress,
                p);

        // Thread exit
		} else if(dbg->dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "EXIT_THREAD_DEBUG_EVENT", 0, -1);
            if(g_verbose) printf("DEBUG thread exit   %08x\n",
                (int)dbg->u.ExitThread.dwExitCode);

        // Process exit
        } else if(dbg->dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "EXIT_PROCESS_DEBUG_EVENT", 0, -1);
            if(g_verbose) printf("DEBUG exit          %08x\n",
                (int)dbg->u.ExitProcess.dwExitCode);

            break;

		} else if(dbg->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "LOAD_DLL_DEBUG_EVENT", 0, -1);
            p = debug_get_string(proc->hp, dbg->u.LoadDll.lpImageName, dbg->u.LoadDll.fUnicode, 1);
            if(g_verbose) printf("DEBUG dll load      %08x %p %s\n",
                (int)dbg->u.LoadDll.hFile,
                dbg->u.LoadDll.lpBaseOfDll,
                p);

		} else if(dbg->dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
            add_var(0, "dwDebugEventCode", "UNLOAD_DLL_DEBUG_EVENT", 0, -1);
            if(g_verbose) printf("DEBUG dll unload    %p\n",
                dbg->u.UnloadDll.lpBaseOfDll);

		} else if(dbg->dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
            add_var(0, "dwDebugEventCode", "OUTPUT_DEBUG_STRING_EVENT", 0, -1);
            p = debug_get_string(proc->hp, dbg->u.DebugString.lpDebugStringData, dbg->u.DebugString.fUnicode, 0);
            if(g_verbose) printf("DEBUG debug string  %s\n",
                p);

        } else if(dbg->dwDebugEventCode == RIP_EVENT) {
            add_var(0, "dwDebugEventCode", "RIP_EVENT", 0, -1);
            if(g_verbose) printf("DEBUG rip           %08x %08x\n",
                (int)dbg->u.RipInfo.dwError,
                (int)dbg->u.RipInfo.dwType);

        } else {
            add_var(0, "dwDebugEventCode", NULL, (int)dbg->dwDebugEventCode, sizeof(int));
        }

        if(!ContinueDebugEvent(dbg->dwProcessId, dbg->dwThreadId, dbg_do)) break;
    }
quit:
    debug_missing();
    if(_DebugActiveProcessStop) _DebugActiveProcessStop(pid);
    printf("- debugger detached\n");
    FREE(dbg);
    proc->debug = 0;
    return 0;
}
#else
quick_thread(process_debugger, int pid) {
    return 0;
}
#endif



int process_common(process_file_t *procfile) {
    #ifdef WIN32

    DWORD   size;
    u8      *baddr;

    if(procfile->hp) return 0;    // already set

    if(!procfile->name && !procfile->name[0]) return -1;

    if(procfile->name) {
        procfile->pid = myatoi(procfile->name);
        // procfile->pid is automatically 0 if invalid
    }

    baddr = process_list(procfile->pid ? NULL : procfile->name, procfile->module, &procfile->pid, &size);
    if(!baddr) {
        if(procfile->pid) fprintf(stderr, "\n       %d\n", (int)procfile->pid);
        else              fprintf(stderr, "\n       %s\n", procfile->name);
        fprintf(stderr, "\n"
            "Error: process name/PID not found or it's a 64bit process\n"
            "       By default QuickBMS is a 32bit program so can't list 64bit modules\n"
            "       using the SNAPMODULE features of ToolHelp, sorry\n"
        );
        myexit(QUICKBMS_ERROR_EXTRA);
    }

    fprintf(stderr,
        "- pid %u\n"
        "- base address %p\n",
        (u32)procfile->pid, baddr);

    procfile->hp = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        procfile->pid);
    if(!procfile->hp) winerr(0, NULL);

    procfile->size    = size;

    if(procfile->debug) {
        procfile->tid = quick_threadx(process_debugger, (void *)procfile);
    }

    #else

    void    *baddr;

    procfile->pid = atoi(procfile->name);
    baddr = (void *)0x8048000;  // sorry, not supported at the moment

    fprintf(stderr,
        "- pid %u\n"
        "- try using base address %p\n",
        procfile->pid, baddr);

    if(ptrace(PTRACE_ATTACH, procfile->pid, NULL, NULL) < 0) STD_ERR(QUICKBMS_ERROR_EXTRA);

    procfile->hp = 0;
    procfile->size    = 0;

    #endif

    procfile->base    = baddr;
    procfile->pos     = procfile->base /*0*/;

    return 0;
}



process_file_t *process_open(u8 *fname) {
    static  int init_process = 0;
    process_file_t  *procfile  = NULL,
                    *procfile_tmp;
    u8      name[256]   = "",
            module[255] = "",
            proto[16]   = "",
            *p;

    if(!strstr(fname, "://")) return NULL;

    procfile_tmp = calloc(1, sizeof(process_file_t));
    if(!procfile_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);

    sscanf(fname,
        "%10[^:]://%255[^,:/]:%255[^,/]",
        proto,
        name,
        module);

    if(
        stricmp(proto, "process") &&
        stricmp(proto, "proc") &&
        stricmp(proto, "mem") &&
        stricmp(proto, "process")
    ) {
        FREE(procfile_tmp);
        return NULL;
    }
    if(!name[0]) {
        FREE(procfile_tmp);
        return NULL;
    }

    if(!enable_process) {
        fprintf(stderr,
            "\n"
            "Error: the script uses processes, if you are SURE about the genuinity of\n"
            "       this script\n"
            "\n"
            "         you MUST use the -p or -process option at command-line.\n"
            "\n"
            "       note that the usage of the processs allows QuickBMS to read and modiy\n"
            "       the memory of the other programs so you MUST really sure about the\n"
            "       script you are using and what you are doing.\n"
            "       this is NOT a feature for extracting files!\n");
        myexit(QUICKBMS_ERROR_EXTRA);
    }
    if(!init_process) {
        debug_privileges();
        init_process = 1;
    }

    procfile_tmp->name = mystrdup_simple(name);
    procfile_tmp->module = mystrdup_simple(module);

    p = strchr(fname, '/');
    if(p) {
        if(stristr(p, "debug")) procfile_tmp->debug = 1;
    }

    for(procfile = process_file; procfile; procfile = procfile->next) {
        if(
            !stricmp(procfile->name, procfile_tmp->name) &&
            (procfile->pid == procfile_tmp->pid)
        ) {
            FREE(procfile_tmp->name);
            FREE(procfile_tmp->module);
            FREE(procfile_tmp);
            procfile_tmp = NULL;
            break;
        }
    }
    if(!procfile) {
        if(!process_file) {
            process_file = procfile_tmp;
            procfile = process_file;
        } else {
            // get the last element
            for(procfile = process_file;; procfile = procfile->next) {
                if(procfile->next) continue;
                procfile->next = procfile_tmp;
                procfile_tmp->prev = procfile;
                procfile = procfile_tmp;
                break;
            }
        }
    }

    process_common(procfile);
    return(procfile);
}



int process_read(process_file_t *procfile, u8 *data, int size) {
    DWORD   len;

    process_common(procfile);
    len = size;

    #ifdef WIN32

    len = readwrite_mem(
        procfile->hp,
        (void *)(/*procfile->base +*/ procfile->pos),
        data,
        size,
        0);
    if(len < 0) return -1; //winerr(0, NULL);

    //CloseHandle(process);

    #else

    u32     tmp;
    int     errno;

    errno = 0;
    for(len = 0; len < size; len += 4) {
        tmp = ptrace(PTRACE_PEEKDATA, procfile->pid, (void *)(/*(u8 *)procfile->base +*/ procfile->pos + len), NULL);
        if(errno && (errno != EIO)) return -1; //STD_ERR(QUICKBMS_ERROR_EXTRA);
        memcpy(data + len, &tmp, 4);
    }

    //if(ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) STD_ERR(QUICKBMS_ERROR_EXTRA);

    #endif

    if(len > 0) procfile->pos += len;
    return len;
}



int process_write(process_file_t *procfile, u8 *data, int size) {
    DWORD   len;

    process_common(procfile);
    len = size;

    #ifdef WIN32

    len = readwrite_mem(
        procfile->hp,
        (void *)(/*procfile->base +*/ procfile->pos),
        data,
        size,
        1);
    if(len < 0) return -1; //winerr(0, NULL);

    #else

    u32     tmp;
    int     errno;

    errno = 0;
    for(len = 0; len < size; len += 4) {
        memcpy(&tmp, data + len, 4);
        tmp = ptrace(PTRACE_POKEDATA, procfile->pid, (void *)(/*(u8 *)procfile->base +*/ procfile->pos + len), tmp);
        if(errno && (errno != EIO)) return -1; //STD_ERR(QUICKBMS_ERROR_EXTRA);
    }

    #endif

    if(len > 0) procfile->pos += len;
    return len;
}



#ifdef WIN32
typedef struct {
    void    *addr;
    int     size;
    u8      *szExePath;
    u8      *szModule;
} module_t;
module_t *scan_modules(u8 *myname, DWORD mypid, u8 **ret_base, int *ret_size) {
    int         modules = 0;
    module_t    *module = NULL;
    int     n,
            pid     = 0;

    if(ret_base) *ret_base = NULL;
    if(ret_size) *ret_size = 0;
    if(myname) {
        if(sscanf(myname, "%u%n", &pid, &n) == 1) {
            if((strlen(myname) == n) && pid) {
                // do nothing
            } else {
                pid = 0;
            }
        }
    }

    if(mypid) {
        pid = mypid;
        myname = NULL;
    }

    PROCESSENTRY32  Process;
    MODULEENTRY32   Module;
    HANDLE          snapProcess,
                    snapModule;
    DWORD           retpid = 0;
    BOOL            b;

    PROCESS_START(Process, TH32CS_SNAPPROCESS, 0)

        if(
            (pid    && (pid == Process.th32ProcessID)) ||
            (myname && stristr(Process.szExeFile, myname))
        ) {
            retpid = Process.th32ProcessID;

            PROCESS_START(Module, TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Process.th32ProcessID)

            if(Process.th32ProcessID == retpid) {

                module = realloc(module, sizeof(module_t) * (modules + 1 + 1));
                if(!module) STD_ERR(QUICKBMS_ERROR_EXTRA);
                module[modules].addr = (void *)Module.modBaseAddr;
                module[modules].size = Module.modBaseSize;
                module[modules].szModule = mystrdup_simple(Module.szModule);
                module[modules].szExePath = mystrdup_simple(Module.szExePath);
                modules++;
                module[modules].addr = NULL;    // delimiter

                if(ret_base && !*ret_base) {
                    *ret_base = Module.modBaseAddr;
                    if(ret_size) *ret_size = Module.modBaseSize;
                }

                // do NOT break
                //break;
            }

            PROCESS_END(Module)

            break;
        }

    PROCESS_END(Process)

    //return(retpid);
    return module;
}
#endif



int scandir_processes(void) {
#ifdef WIN32
    PROCESSENTRY32  Process;
    HANDLE          snapProcess;
    BOOL            b;

    PROCESS_START(Process, TH32CS_SNAPPROCESS, 0)

        add_files(Process.szExeFile, Process.th32ProcessID, NULL);

    PROCESS_END(Process)
    return 0;
#else
    return -1;
#endif
}



int scandir_modules(DWORD pid) {
#ifdef WIN32
    PROCESSENTRY32  Process;
    MODULEENTRY32   Module;
    HANDLE          snapProcess,
                    snapModule;
    BOOL            b;

    PROCESS_START(Process, TH32CS_SNAPPROCESS, 0)

        if(pid == Process.th32ProcessID) {

            PROCESS_START(Module, TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Process.th32ProcessID)

                // Module.szModule ???
                char    tmp[64];
                sprintf(tmp, "0x%p", Module.modBaseAddr);
                add_files(tmp, Module.modBaseSize, NULL);

            PROCESS_END(Module)

            break;
        }

    PROCESS_END(Process)
    return 0;
#else
    return -1;
#endif
}



int scandir_memory(HANDLE hp) {
#ifdef WIN32

    static BOOL WINAPI (*_QueryWorkingSet)(HANDLE hProcess, PVOID  pv, DWORD  cb) = NULL;
    static HMODULE psapi = NULL;
    if(!psapi) psapi = GetModuleHandle("psapi.dll");   // LoadLibrary may be dangerous
    if(!psapi) psapi = LoadLibrary("psapi.dll");
    if(psapi) {
        if(!_QueryWorkingSet) _QueryWorkingSet = (void *)GetProcAddress(psapi, "QueryWorkingSet");
    }
    if(!_QueryWorkingSet) return -1;

    typedef struct heap_region_t {
        void    *address;
        u32     size;
        UT_hash_handle hh;
        struct heap_region_t *next;
        struct heap_region_t *prev;
    } heap_region_t;
    heap_region_t   *g_heap_region = NULL,  // no static because it's used only once
                    *heap_region,
                    *heap_region_next;

    PSAPI_WORKING_SET_INFORMATION   pv1,
                                    *pv = NULL;
    u64     t;
    u8      *addr;
    int     i,
            pvsz;

    if(!g_page_size) g_page_size = get_page_size();

    if(!ISHANDLEOK(hp)) return -1;

    memset(&pv1, 0, sizeof(pv1));
    _QueryWorkingSet(hp, &pv1, sizeof(pv1));
    if(!pv1.NumberOfEntries) return -1;

    pvsz = sizeof(PSAPI_WORKING_SET_INFORMATION)
         + sizeof(PSAPI_WORKING_SET_BLOCK) * pv1.NumberOfEntries;
    pv = calloc(pvsz, 1);
    if(!pv) return -1;
    if(!_QueryWorkingSet(hp, pv, pvsz)) return -1;

    for(i = 0; i < pv->NumberOfEntries; i++) {
        if(!pv->WorkingSetInfo[i].VirtualPage) continue;    // zero?

        t = pv->WorkingSetInfo[i].VirtualPage;
        addr = (u8 *)(t << (u64)12);

        MEMORY_BASIC_INFORMATION    mbi;
        memset(&mbi, 0, sizeof(mbi));
        if(VirtualQueryEx(hp, (void *)addr, &mbi, g_page_size)) {
            if(mbi.State & MEM_COMMIT) {
                addr = mbi.AllocationBase;
                HASH_FIND_PTR(g_heap_region, &addr, heap_region);
                if(!heap_region) {
                    heap_region = real_calloc(1, sizeof(heap_region_t));
                    if(!heap_region) STD_ERR(QUICKBMS_ERROR_MEMORY);
                    heap_region->address = addr;
                    heap_region->size    = mbi.RegionSize;
                    HASH_ADD_PTR(g_heap_region, address, heap_region);

                    char    tmp[64];
                    sprintf(tmp, "0x%p", heap_region->address);
                    add_files(tmp, heap_region->size, NULL);
                }
            }
        }
    }

    free(pv);

    HASH_ITER(hh, g_heap_region, heap_region, heap_region_next) {
        HASH_DEL(g_heap_region, heap_region);
        real_free(heap_region);
    }

    return(0);
#else
    return -1;
#endif
}



// exists a better and faster version of this function but, currently, I opted
// for the "classical" solution to stay compatible with all the systems
int scandir_heap(DWORD pid) {
#ifdef WIN32
    HEAPENTRY32 he;
    HEAPLIST32  hl;
    HANDLE      hHeapSnap;

    hHeapSnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, pid);
    if(hHeapSnap == INVALID_HANDLE_VALUE) return(-1);

    hl.dwSize = sizeof(HEAPLIST32);
    if(Heap32ListFirst(hHeapSnap, &hl)) {
        do {
            he.dwSize = sizeof(HEAPENTRY32);
            if(Heap32First(&he, pid, hl.th32HeapID)) {
                do {

                    char    tmp[64];
                    //sprintf(tmp, "0x%"PRIx, (int)he.dwAddress);
                    sprintf(tmp, "0x%08x", (i32)he.dwAddress);
                    if(g_verbose) printf("- heap %s %d\n", tmp, (int)he.dwBlockSize);
                    add_files(tmp, he.dwBlockSize, NULL);

                    he.dwSize = sizeof(HEAPENTRY32);
                } while(Heap32Next(&he));
            }
            hl.dwSize = sizeof(HEAPLIST32);
        } while(Heap32ListNext(hHeapSnap, &hl));
    }
   
    CloseHandle(hHeapSnap);
    return 0;
#else
    return -1;
#endif
}

