long long __divdi3(long long u, long long v);
long long __moddi3(long long u, long long v);
unsigned long long __udivdi3(unsigned long long u, unsigned long long v);
unsigned long long __umoddi3(unsigned long long u, unsigned long long v);
long long __ashrdi3(long long a, int b);
unsigned long long __lshrdi3(unsigned long long a, int b);
long long __ashldi3(long long a, int b);
float __floatundisf(unsigned long long a);
double __floatundidf(unsigned long long a);
long double __floatundixf(unsigned long long a);
unsigned long long __fixunssfdi (float a1);
long long __fixsfdi (float a1);
unsigned long long __fixunsdfdi (double a1);
long long __fixdfdi (double a1);
unsigned long long __fixunsxfdi (long double a1);
long long __fixxfdi (long double a1);



int TCC_libtcc_symbols_quickbms(TCCState *tccstate) {
    #define tcc_symbols_add_quickbms(X) tcc_add_symbol(tccstate, #X, QUICKBMS_##X)

    // tcc_define_symbol does NOT work... no idea why, anyway problem solved with tcc_set_options
    //tcc_define_symbol(tccstate, "FILE", "void");

    //stdio.h
    tcc_symbols_add_quickbms(fopen);
    tcc_symbols_add_quickbms(freopen);
    tcc_symbols_add_quickbms(fflush);
    tcc_symbols_add_quickbms(fclose);
    tcc_symbols_add_quickbms(remove);
    tcc_symbols_add_quickbms(rename);
    tcc_symbols_add_quickbms(tmpfile);
    tcc_symbols_add_quickbms(tmpnam);
    tcc_symbols_add_quickbms(tempnam);
    tcc_symbols_add_quickbms(rmtmp);
    tcc_symbols_add_quickbms(unlink);
    tcc_symbols_add_quickbms(setvbuf);
    tcc_symbols_add_quickbms(setbuf);
    /*
    tcc_symbols_add_quickbms(fprintf);
    tcc_symbols_add_quickbms(printf);
    tcc_symbols_add_quickbms(sprintf);
    tcc_symbols_add_quickbms(vfprintf);
    tcc_symbols_add_quickbms(vprintf);
    tcc_symbols_add_quickbms(vsprintf);
    tcc_symbols_add_quickbms(snprintf);
    tcc_symbols_add_quickbms(vsnprintf);
    tcc_symbols_add_quickbms(vscanf);
    tcc_symbols_add_quickbms(fscanf);
    tcc_symbols_add_quickbms(scanf);
    tcc_symbols_add_quickbms(sscanf);
    */
    tcc_symbols_add_quickbms(fgetc);
    tcc_symbols_add_quickbms(fgets);
    tcc_symbols_add_quickbms(fputc);
    tcc_symbols_add_quickbms(fputs);
    tcc_symbols_add_quickbms(gets);
    tcc_symbols_add_quickbms(puts);
    tcc_symbols_add_quickbms(ungetc);
    tcc_symbols_add_quickbms(getc);
    tcc_symbols_add_quickbms(putc);
    tcc_symbols_add_quickbms(getchar);
    tcc_symbols_add_quickbms(putchar);
    tcc_symbols_add_quickbms(fread);
    tcc_symbols_add_quickbms(fwrite);
    tcc_symbols_add_quickbms(fseek);
    tcc_symbols_add_quickbms(ftell);
    tcc_symbols_add_quickbms(rewind);
    tcc_symbols_add_quickbms(fgetpos);
    tcc_symbols_add_quickbms(fsetpos);
    tcc_symbols_add_quickbms(feof);
    tcc_symbols_add_quickbms(ferror);
    tcc_symbols_add_quickbms(clearerr);
    tcc_symbols_add_quickbms(perror);
    tcc_symbols_add_quickbms(fgetchar);
    tcc_symbols_add_quickbms(fputchar);
    tcc_symbols_add_quickbms(fdopen);
    tcc_symbols_add_quickbms(fileno);
    /*
    tcc_symbols_add_quickbms(fwprintf);
    tcc_symbols_add_quickbms(wprintf);
    tcc_symbols_add_quickbms(vfwprintf);
    tcc_symbols_add_quickbms(vwprintf);
    tcc_symbols_add_quickbms(fwscanf);
    tcc_symbols_add_quickbms(wscanf);
    tcc_symbols_add_quickbms(swscanf);
    */
    tcc_symbols_add_quickbms(fgetwc);
    tcc_symbols_add_quickbms(fputwc);
    tcc_symbols_add_quickbms(ungetwc);
    tcc_symbols_add_quickbms(swprintf);
    tcc_symbols_add_quickbms(vswprintf);
    tcc_symbols_add_quickbms(fgetws);
    tcc_symbols_add_quickbms(fputws);
    tcc_symbols_add_quickbms(getwc);
    tcc_symbols_add_quickbms(getwchar);
    tcc_symbols_add_quickbms(putwc);
    tcc_symbols_add_quickbms(putwchar);
    /*
    tcc_symbols_add_quickbms(snwprintf);
    tcc_symbols_add_quickbms(vsnwprintf);
    */
    tcc_symbols_add_quickbms(wpopen);
    tcc_symbols_add_quickbms(fgetwchar);
    tcc_symbols_add_quickbms(fputwchar);
    tcc_symbols_add_quickbms(getw);
    tcc_symbols_add_quickbms(putw);

    return 0;
}



int TCC_libtcc_symbols(TCCState *tccstate) {
    #define tcc_symbols_add(X)  tcc_add_symbol(tccstate, #X, X)

    // once "added" it's not possible to replace a symbol with a new one,
    // only the first is kept and any subsequent one is ignored

    // the following converts any file operation into a memory one,
    // after all it's not possible to pass a FILE from calldll
    // so this is a good way for not modifying any C source code
    TCC_libtcc_symbols_quickbms(tccstate);


    // in case we want to load includes and libraries
    tcc_add_include_path    (tccstate, g_exe_folder);
    tcc_add_sysinclude_path (tccstate, g_exe_folder);
    tcc_add_library_path    (tccstate, g_exe_folder);


    //stdio.h
    tcc_symbols_add(fopen);
    tcc_symbols_add(freopen);
    tcc_symbols_add(fflush);
    tcc_symbols_add(fclose);
    //tcc_symbols_add(remove);
    //tcc_symbols_add(rename);
    //tcc_symbols_add(tmpfile);
    //tcc_symbols_add(tempnam);
    //tcc_symbols_add(rmtmp);
    //tcc_symbols_add(unlink);
    tcc_symbols_add(fprintf);
    tcc_symbols_add(printf);
    tcc_symbols_add(sprintf);
    tcc_symbols_add(vfprintf);
    tcc_symbols_add(vprintf);
    tcc_symbols_add(vsprintf);
    tcc_symbols_add(fscanf);
    tcc_symbols_add(scanf);
    tcc_symbols_add(sscanf);
    tcc_symbols_add(fgetc);
    tcc_symbols_add(fgets);
    tcc_symbols_add(fputc);
    tcc_symbols_add(fputs);
    //tcc_symbols_add(gets);
    tcc_symbols_add(puts);
    tcc_symbols_add(ungetc);
    tcc_symbols_add(getc);
    tcc_symbols_add(putc);
    tcc_symbols_add(getchar);
    tcc_symbols_add(putchar);
    tcc_symbols_add(fread);
    tcc_symbols_add(fwrite);
    tcc_symbols_add(fseek);
    tcc_symbols_add(ftell);
    tcc_symbols_add(rewind);
    tcc_symbols_add(fgetpos);
    tcc_symbols_add(fsetpos);
    tcc_symbols_add(feof);
    tcc_symbols_add(ferror);

    //stdlib.h
    tcc_symbols_add(atoi);
    tcc_symbols_add(atof);
    tcc_add_symbol(tccstate, "malloc",  real_malloc);   //tcc_symbols_add(malloc);
    tcc_add_symbol(tccstate, "calloc",  real_calloc);   //tcc_symbols_add(calloc);
    tcc_add_symbol(tccstate, "realloc", real_realloc);  //tcc_symbols_add(realloc);
    tcc_add_symbol(tccstate, "free",    real_free);     //tcc_symbols_add(free);
    //tcc_symbols_add(alloca);
    //tcc_symbols_add(itoa);
    tcc_symbols_add(exit);
    tcc_symbols_add(bsearch);
    tcc_symbols_add(qsort);
    tcc_symbols_add(div);
    tcc_symbols_add(abs);

    //string.h
    tcc_symbols_add(memchr);
    tcc_symbols_add(memcmp);
    tcc_symbols_add(memcpy);
    tcc_symbols_add(memmove);
    tcc_symbols_add(memset);
    tcc_symbols_add(strcat);
    tcc_symbols_add(strchr);
    tcc_symbols_add(strcmp);
    tcc_symbols_add(strcoll);
    tcc_symbols_add(strcpy);
    tcc_symbols_add(strcspn);
    tcc_symbols_add(strerror);
    tcc_symbols_add(strlen);
    tcc_symbols_add(strncat);
    tcc_symbols_add(strncmp);
    tcc_symbols_add(strncpy);
    tcc_symbols_add(strpbrk);
    tcc_symbols_add(strrchr);
    tcc_symbols_add(strspn);
    tcc_symbols_add(strstr);
    tcc_symbols_add(stristr);
    tcc_symbols_add(strtok);
    tcc_symbols_add(strxfrm);
    tcc_symbols_add(memccpy);
    tcc_symbols_add(memicmp);
    tcc_add_symbol(tccstate, "strdup",  real_strdup);   //tcc_symbols_add(strdup);
    //tcc_symbols_add(strcmpi);
    tcc_symbols_add(stricmp);
    //tcc_symbols_add(stricoll);    // not available on newer gcc
    //tcc_symbols_add(strlwr);
    tcc_symbols_add(strnicmp);
    //tcc_symbols_add(strnset);
    //tcc_symbols_add(strrev);
    //tcc_symbols_add(strset);
    //tcc_symbols_add(strupr);

    //math.h
    tcc_symbols_add(sin);
    tcc_symbols_add(cos);
    tcc_symbols_add(tan);
    tcc_symbols_add(sinh);
    tcc_symbols_add(cosh);
    tcc_symbols_add(tanh);
    tcc_symbols_add(asin);
    tcc_symbols_add(acos);
    tcc_symbols_add(atan);
    tcc_symbols_add(atan2);
    tcc_symbols_add(exp);
    tcc_symbols_add(log);
    tcc_symbols_add(log10);
    tcc_symbols_add(pow);
    tcc_symbols_add(sqrt);
    tcc_symbols_add(ceil);
    tcc_symbols_add(floor);
    tcc_symbols_add(fabs);
    tcc_symbols_add(ldexp);
    tcc_symbols_add(frexp);
    tcc_symbols_add(modf);
    tcc_symbols_add(fmod);

    //ctype.h
    tcc_symbols_add(isalnum);
    tcc_symbols_add(isalpha);
    tcc_symbols_add(iscntrl);
    tcc_symbols_add(isdigit);
    tcc_symbols_add(isgraph);
    tcc_symbols_add(islower);
    //tcc_symbols_add(isleadbyte);
    tcc_symbols_add(isprint);
    tcc_symbols_add(ispunct);
    tcc_symbols_add(isspace);
    tcc_symbols_add(isupper);
    tcc_symbols_add(isxdigit);
    tcc_symbols_add(tolower);
    tcc_symbols_add(toupper);

    //time.h
    tcc_symbols_add(time);
    tcc_symbols_add(difftime);
    tcc_symbols_add(mktime);
    tcc_symbols_add(asctime);
    tcc_symbols_add(ctime);
    tcc_symbols_add(gmtime);
    tcc_symbols_add(localtime);

#ifdef WIN32
    //windows.h/winbase.h
    tcc_symbols_add(LoadLibraryA);
    tcc_symbols_add(LoadLibraryExA);
    tcc_symbols_add(GetProcAddress);
    tcc_symbols_add(FreeLibrary);
    tcc_symbols_add(FindResourceA);
    tcc_symbols_add(GetModuleHandleA);
    //tcc_symbols_add(GetModuleHandleExA);  // not available on Win98
#endif

    //quickbms
    tcc_symbols_add(strristr);
    tcc_symbols_add(swap16);
    tcc_symbols_add(swap32);
    tcc_symbols_add(spr2);
    tcc_symbols_add(rol);
    tcc_symbols_add(ror);
    tcc_symbols_add(mycrc);
    tcc_symbols_add(mytolower);
    tcc_symbols_add(mytoupper);

    // libtcc.a
    #if defined(i386) // got a warning on MacOS but still working
    tcc_symbols_add(__ashldi3);
    tcc_symbols_add(__ashrdi3);
    tcc_symbols_add(__divdi3);
    tcc_symbols_add(__fixdfdi);
    tcc_symbols_add(__fixsfdi);
    tcc_symbols_add(__fixunsdfdi);
    tcc_symbols_add(__fixunssfdi);
    tcc_symbols_add(__fixunsxfdi);
    tcc_symbols_add(__fixxfdi);
    tcc_symbols_add(__floatundidf);
    tcc_symbols_add(__floatundisf);
    tcc_symbols_add(__floatundixf);
    tcc_symbols_add(__lshrdi3);
    tcc_symbols_add(__moddi3);
    tcc_symbols_add(__udivdi3);
    //tcc_symbols_add(__udivmoddi4);
    tcc_symbols_add(__umoddi3);
    #endif

    /*
    tcc_symbols_add(__getmainargs);
    tcc_symbols_add(__set_app_type);
    tcc_symbols_add(__try__);
    tcc_symbols_add(_controlfp);
    tcc_symbols_add(_dowildcard);
    tcc_symbols_add(_imp____argc);
    tcc_symbols_add(_imp____argv);
    tcc_symbols_add(_imp___environ);
    tcc_symbols_add(_runmain);
    tcc_symbols_add(_start);
    tcc_symbols_add(__wgetmainargs);
    tcc_symbols_add(_imp____wargv);
    tcc_symbols_add(_imp___wenviron);
    tcc_symbols_add(_runwmain);
    tcc_symbols_add(_wstart);
    tcc_symbols_add(_GetCommandLineA@0);
    tcc_symbols_add(_GetModuleHandleA@4);
    tcc_symbols_add(_GetStartupInfoA@4);
    tcc_symbols_add(_runwinmain);
    tcc_symbols_add(_strdup);
    tcc_symbols_add(_WinMain@16);
    tcc_symbols_add(_winstart);
    tcc_symbols_add(strstr);
    tcc_symbols_add(_GetCommandLineW@0);
    tcc_symbols_add(_GetModuleHandleW@4);
    tcc_symbols_add(_GetStartupInfoW@4);
    tcc_symbols_add(_runwwinmain);
    tcc_symbols_add(_wcsdup);
    tcc_symbols_add(_wWinMain@16);
    tcc_symbols_add(_wwinstart);
    tcc_symbols_add(wcsstr);
    tcc_symbols_add(___try__);
    tcc_symbols_add(__chkstk);
    tcc_symbols_add(_except_handler3);
    tcc_symbols_add(_exception_code);
    tcc_symbols_add(_exception_info);
    tcc_symbols_add(_exit);
    tcc_symbols_add(_XcptFilter);
    tcc_symbols_add(seh_except);
    tcc_symbols_add(seh_filter);
    tcc_symbols_add(seh_handler);
    tcc_symbols_add(seh_scopetable);
    tcc_symbols_add(__bound_calloc);
    tcc_symbols_add(__bound_check);
    tcc_symbols_add(__bound_delete_region);
    tcc_symbols_add(__bound_empty_t2);
    tcc_symbols_add(__bound_error_msg);
    tcc_symbols_add(__bound_exit);
    tcc_symbols_add(__bound_find_region);
    tcc_symbols_add(__bound_free);
    tcc_symbols_add(__bound_init);
    tcc_symbols_add(__bound_invalid_t2);
    tcc_symbols_add(__bound_local_delete);
    tcc_symbols_add(__bound_local_new);
    tcc_symbols_add(__bound_main_arg);
    tcc_symbols_add(__bound_malloc);
    tcc_symbols_add(__bound_memalign);
    tcc_symbols_add(__bound_memcpy);
    tcc_symbols_add(__bound_memmove);
    tcc_symbols_add(__bound_memset);
    tcc_symbols_add(__bound_new_page);
    tcc_symbols_add(__bound_new_region);
    tcc_symbols_add(__bound_ptr_add);
    tcc_symbols_add(__bound_ptr_indir1);
    tcc_symbols_add(__bound_ptr_indir12);
    tcc_symbols_add(__bound_ptr_indir16);
    tcc_symbols_add(__bound_ptr_indir2);
    tcc_symbols_add(__bound_ptr_indir4);
    tcc_symbols_add(__bound_ptr_indir8);
    tcc_symbols_add(__bound_realloc);
    tcc_symbols_add(__bound_strcpy);
    tcc_symbols_add(__bound_strlen);
    tcc_symbols_add(__bound_t1);
    tcc_symbols_add(__bounds_start);
    tcc_symbols_add(_imp___iob);
    tcc_symbols_add(add_region);
    tcc_symbols_add(bound_alloc_error);
    tcc_symbols_add(bound_error);
    tcc_symbols_add(bound_free_entry);
    tcc_symbols_add(bound_new_entry);
    tcc_symbols_add(delete_region);
    tcc_symbols_add(fprintf);
    tcc_symbols_add(free);
    tcc_symbols_add(get_page);
    tcc_symbols_add(get_region_size);
    tcc_symbols_add(inited);
    tcc_symbols_add(install_malloc_hooks);
    tcc_symbols_add(libc_free);
    tcc_symbols_add(libc_malloc);
    //tcc_symbols_add(malloc);
    tcc_symbols_add(mark_invalid);
    tcc_symbols_add(memcpy);
    tcc_symbols_add(memmove);
    tcc_symbols_add(memset);
    tcc_symbols_add(restore_malloc_hooks);
    tcc_symbols_add(alloca);
    tcc_symbols_add(__bound_alloca);
    tcc_symbols_add(__bound_new_region);
    tcc_symbols_add(exit);
    */

    return 0;
}

