/*
    Copyright 2009-2022 Luigi Auriemma

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

// QuickBMS internal file operations (fdnum)



int myfseek(int fdnum, u_int offset, int type);
int myfr(int fdnum, u8 *data, int size, int quit_if_diff);
int myfw(int fdnum, u8 *data, int size);



int myfread(FILE *fd, u8 *data, int size, int quit_if_diff) {
    int     len;

    if(size < 0) {
        size = BUFFSZ;
        quit_if_diff = 0;
    }
    if(data) {
        len = fread(data, 1, size, fd);
    } else {
        len = size;
        if(fseek(fd, size, SEEK_CUR) < 0) len = -1;
    }
    if((len != size) && quit_if_diff) {
        if(g_continue_anyway) return -1;
        myexit(QUICKBMS_ERROR_FILE_READ);
    }
    return len;
}



u64 myfilesize(int fdnum) {
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        return(g_memory_file[myabs(fdnum)].size);
    }
    CHECK_FILENUM
    if(g_filenumber[fdnum].fd) {
        return filesize(g_filenumber[fdnum].fd);
    }
    // sockets and streams want the max signed value
    if(g_filenumber[fdnum].pd) return(((u_int)(-1)) >> 1);    // 0x7fffffff... return(((process_file_t *)g_filenumber[fdnum].pd)->size);
    if(g_filenumber[fdnum].vd) return(((video_file_t *)g_filenumber[fdnum].vd)->size);
    if(g_filenumber[fdnum].sd && (((socket_file_t *)g_filenumber[fdnum].sd)->datasz > 0)) return ((socket_file_t *)g_filenumber[fdnum].sd)->datasz;
    return(((u_int)(-1)) >> 1); // 0x7fffffff...
}



u_int memfile_resize(memory_file_t *memfile, u_int offset) {
    u_int   oldsize = memfile->size;
    memfile->size = offset;
    if(offset <= oldsize) {
        if(memfile->pos > memfile->size) memfile->pos = memfile->size;
    } else {
        myalloc(&memfile->data, memfile->size, &memfile->maxsize);
        memset(memfile->data + oldsize, 0, memfile->size - oldsize);
    }
    return memfile->size;
}



u_int myftruncate(int fdnum, int fsize) {
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        return memfile_resize(&g_memory_file[myabs(fdnum)], fsize);
    }
    CHECK_FILENUM
    if(g_filenumber[fdnum].fd) {
        FILE    *fd = g_filenumber[fdnum].fd;
        fflush(fd);
#ifdef WIN32
        HANDLE  fh;
        fh = (HANDLE)_get_osfhandle(fileno(fd));
        if(fh == INVALID_HANDLE_VALUE) return 0;
        u_int oldoff = ftell(fd);
        fseek(fd, fsize, SEEK_SET);
        int ret = SetEndOfFile(fh);
        fseek(fd, oldoff, SEEK_SET);
        if(ret < 0) return 0;
        return fsize;
#else
        return ftruncate(fileno(fd), fsize);
#endif
    }

    if(myfseek(fdnum, fsize, SEEK_SET) < 0) return myfilesize(fdnum);   // no support for enlarging "files"
    if(g_filenumber[fdnum].sd) return(((socket_file_t  *)g_filenumber[fdnum].sd)->datasz = fsize);
    if(g_filenumber[fdnum].pd) return((u_int)(((process_file_t *)g_filenumber[fdnum].pd)->size = fsize));
    //if(g_filenumber[fdnum].ad) return(((audio_file_t *)  g_filenumber[fdnum].ad)->size = fsize);
    if(g_filenumber[fdnum].vd) return(((video_file_t *)  g_filenumber[fdnum].vd)->size = fsize);
    if(g_filenumber[fdnum].md) return(((winmsg_file_t *) g_filenumber[fdnum].md)->size = fsize);
    return 0;
}



FILE *get_fdnum_file_descriptor(int fdnum) {
    if(fdnum >= 0) {
        CHECK_FILENUM
        if(g_filenumber[fdnum].fd) {
            return g_filenumber[fdnum].fd;
        }
    }
    return NULL;
}



u8 *QUICKBMS_CACHED_IO(int *ret_size) {
    #define QUICKBMS_CACHED_IO_SIZE     (512 * 1024)    // this is probably the best size for optimal speed
    static u8   *tmp = NULL;
    if(!tmp) {
        tmp = malloc(QUICKBMS_CACHED_IO_SIZE);
        if(!tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);
    }
    if(ret_size) *ret_size = QUICKBMS_CACHED_IO_SIZE;
    return tmp;
}



#define TMP_FILE_READ(FREAD, FD, DO) \
    int     t, \
            tmpsz; \
    u8      *tmp = QUICKBMS_CACHED_IO(&tmpsz); \
    \
    for(len = 0; len < size; len += t) { \
        t = tmpsz; \
        if((size - len) < t) t = size - len; \
        t = FREAD(FD, tmp, t, TRUE); \
        if(t <= 0) { \
            DO; \
        }



int make_file_space(FILE *fd, int space) {
    u_int   offset;
    int     size;

    // there are only two solutions:
    // 1) read and write one byte per time
    // 2) read whole data
    // caching works but it may happen (don't know when) that our amount is smaller than expected and making a mess
    offset = ftell(fd);
    size = filesize(fd) - offset;
    if(size <= 0) return 0;

    static int  tmpsz   = 0;
    static u8   *tmp    = NULL;

    myalloc(&tmp, size, &tmpsz);
    myfread(fd, tmp, size, TRUE);
    fseek(fd, offset + space, SEEK_SET);
    if(fwrite(tmp, 1, size, fd) != size) return -1;
    //FREE(tmp)
    fseek(fd, offset, SEEK_SET);
    return 0;
}



int file_compare(FILE *fd, u8 *data, int size) {
    int     len;
    TMP_FILE_READ(myfread, fd, return -1)
        if(memcmp(tmp, data + len, t)) {
            return -1;
        }
    }
    return 0;
}



int file_duplicate(FILE *fd, FILE *fdo, int size) {
    int     len;
    TMP_FILE_READ(myfread, fd, return -1)
        if(fwrite(tmp, 1, t, fdo) != t) {
            return -1;
        }
    }
    return 0;
}



// just experimental, written on the fly
void file_display(int mode, u_int fsize, u_int offset, int size, double entropy) {
    #define file_display_XY 64
    static double   table  [file_display_XY][file_display_XY];
    static int      table_n[file_display_XY][file_display_XY];
    int     i,
            j;

    if(mode < 0) {
        for(i = 0; i < file_display_XY; i++) {
            for(j = 0; j < file_display_XY; j++) {
                table[i][j]   = 0.0;
                table_n[i][j] = 0;
            }
        }

    } else if(mode > 0) {
        fputc('\n', stdout);
        for(i = 0; i < file_display_XY; i++) {
            fputc(' ', stdout);
            fputc(' ', stdout);
            for(j = 0; j < file_display_XY; j++) {
                double  f = table[i][j];
                if(table_n[i][j]) f /= (double)table_n[i][j];
                // this is just experimental, probably wrong
                if(f <= 0.0) {
                    fputc('.', stdout);
                } else if(f <= 4.0) {
                    fputc('X', stdout); // 'X' or 'O'
                } else {
                    fputc('#', stdout); // '#' or '@'
                }
            }
            fputc('\n', stdout);
        }
        fputc('\n', stdout);

    } else {
        if(!fsize) return;
        if(size <= 0) return;
        if(offset > fsize) return;
        // lame work-around for files smaller than 64*64
        if(fsize <    (u_int)(file_display_XY * file_display_XY)) {
            offset *= (u_int)(file_display_XY * file_display_XY);
            size   *= (u_int)(file_display_XY * file_display_XY);
            fsize  *= (u_int)(file_display_XY * file_display_XY);
        }

        int chunk = fsize / (u_int)(file_display_XY * file_display_XY);
        if(chunk <= 0) chunk = 1;
        i32 idx_offset = offset / chunk;
        i32 idx_size   = size   / chunk;
        for(i = 0; i < file_display_XY; i++) {
            for(j = 0; j < file_display_XY; j++) {
                if(idx_offset <= 0) {
                    if(entropy == 0.0) entropy = 0.0001;    // just to give something to visualize
                    table[i][j] += entropy;
                    table_n[i][j]++;
                    if(idx_size <= 0) return;
                    idx_size--;
                }
                idx_offset--;
            }
        }
    }
}



int parsing_debug_sort(parsing_debug_t *a, parsing_debug_t *b) {
    return (a->offset > b->offset);
}



u8 *myitoa_decimal_names(u_int num) {
    static char ret[NUMBERSZ];  // used only once in a printf
    sprintf(ret,       g_decimal_names    ? "%"PRIu : "%"PRIx, num);
    return ret;
}



u8 *myitoa_decimal_notation(u_int num) {
    static int  flip = 0;       // in case we use it twice in a printf
    static char ret[2][NUMBERSZ];
    flip = (flip + 1) & 1;
    sprintf(ret[flip], g_decimal_notation ? "%"PRIu : "%"PRIx, num);
    return ret[flip];
}



int fcoverage(int fdnum) {
    memory_file_t   *memfile;
    filenumber_t    *filez  = NULL;
    u_int   coverage    = 0,
            fsize       = 0,
            offset      = 0;
    int     perc        = 0;

    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        coverage = memfile->coverage;
        fsize    = myfilesize(fdnum);
        offset   = myftell(fdnum);
    } else {
        CHECK_FILENUMX //do NOT use CHECK_FILENUM because the file can be unexistent too!
        filez = &g_filenumber[fdnum];
        coverage = filez->coverage;
        if(filez->fd) fsize = myfilesize(fdnum);
        if(filez->fd) offset = myftell(fdnum);
    }

    if(fsize) { // avoids division by zero
        perc = (u64)((u64)coverage * (u64)100) / (u64)fsize;
        u8 perc_fix = ' ';
        if((perc < 0) || (perc > 100)) {
            perc     = 100;
            perc_fix = '!';
        }
        fprintf(stderr,
            "  coverage file %-3d %3d%%%c  %-10s %-10s . offset %"PRIx"\n",
            (i32)fdnum,
            (i32)perc, perc_fix,
            myitoa_decimal_notation(coverage),
            myitoa_decimal_notation(fsize),
            offset);

        if(g_parsing_debug && fdnum_is_valid && filez && filez->fd) {   // filez already assigned
            parsing_debug_t *parsing_debug, *parsing_debug_tmp1, *parsing_debug_tmp2;
            parsing_debug = real_calloc(1, sizeof(parsing_debug_t));
            parsing_debug->offset       = fsize;
            parsing_debug->end_offset   = fsize;
            parsing_debug->entropy      = 0.0;
            CDL_APPEND(filez->parsing_debug, parsing_debug);    // EOF
            CDL_SORT(filez->parsing_debug, parsing_debug_sort);
            i32     chunks  = 0;
            CDL_COUNT(filez->parsing_debug, parsing_debug, chunks);
            if(chunks > 1) {
                double  entropy = 0.0;
                u_int   last_offset = 0,
                        next_offset;
                u8      fname[64];
                strcpy(fname, "QUICKBMS_DEBUG_FILE");
                if(fdnum > 0) sprintf(fname + strlen(fname), "%d", (i32)fdnum);
                FILE    *fd = xfopen(fname, "wb");
                if(fd) {
                    file_display(-1, fsize, 0, 0, 0.0);
                    CDL_FOREACH(filez->parsing_debug, parsing_debug) {
                        file_display(0, fsize, parsing_debug->offset, parsing_debug->end_offset - parsing_debug->offset, parsing_debug->entropy);
                        entropy += parsing_debug->entropy;
                        fseek(filez->fd, last_offset, SEEK_SET);
                        file_duplicate(filez->fd, fd, parsing_debug->offset - last_offset);
                        last_offset = parsing_debug->end_offset;
                        if(parsing_debug->next) {
                            if((parsing_debug->next)->offset < last_offset) last_offset = (parsing_debug->next)->offset;
                        }
                    }
                    CDL_FOREACH_SAFE(filez->parsing_debug, parsing_debug, parsing_debug_tmp1, parsing_debug_tmp2) {
                        CDL_DELETE(filez->parsing_debug, parsing_debug);
                        real_free(parsing_debug);
                    }
                    fprintf(stderr, "              > %s parsed entropy %g\n", fname, entropy / (double)chunks);
                    fprintf(stderr, "                %-10s bytes dumped from %d chunks\n", myitoa_decimal_notation((int)ftell(fd)/*cast from 64bit*/), chunks);
                    file_display(1, fsize, 0, 0, 0.0);
                    FCLOSE(fd);
                    fseek(filez->fd, offset, SEEK_SET); // restore
                }
            }
        }
    }
    return perc;
}



int myfreset(int fdnum) {
    memory_file_t   *memfile;
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        memfile_resize(memfile, 0);
    }
    myfseek(fdnum, 0, SEEK_SET);
    return 0;
}



int myfclose(int fdnum) {
    memory_file_t   *memfile;
    filenumber_t    *filez;

    fcoverage(fdnum);

    if(g_enable_hexhtml) hexhtml_build(fdnum);

    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        // do NOT free memfile->data, it can be reused
        memfile->pos  = 0;
        memfile->size = 0;
        if(memfile->hexhtml) {
            FREE(memfile->hexhtml)
            memfile->hexhtml_size = 0;
        }
    } else {
        CHECK_FILENUMX //do NOT use CHECK_FILENUM because the file can be unexistent too!
        filez = &g_filenumber[fdnum];

        if((g_reimport < 0) && filez->fd) {
            // it should work with archives like those of UE4
            if(filez->tail_toc_offset && (filez->tail_toc_size > 0)) {
                static u8   *tmp    = NULL;
                u_int       oldoff;
                tmp = realloc(tmp, filez->tail_toc_size);
                if(!tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);
                oldoff = myftell(fdnum);
                myfseek(fdnum, filez->tail_toc_offset, SEEK_SET);
                myfr(fdnum, tmp, filez->tail_toc_size, TRUE);
                myfseek(fdnum, 0, SEEK_END);
                myfw(fdnum, tmp, filez->tail_toc_size);
                myfseek(fdnum, oldoff, SEEK_SET);
            }
        }

             if(filez->fd) { FCLOSE(filez->fd);         filez->fd = NULL; }
        else if(filez->sd) { socket_close(filez->sd);   filez->sd = NULL; }
        else if(filez->pd) { process_close(filez->pd);  filez->pd = NULL; }
        else if(filez->ad) { audio_close(filez->ad);    filez->ad = NULL; }
        else if(filez->vd) { video_close(filez->vd);    filez->vd = NULL; }
        else if(filez->md) { winmsg_close(filez->md);   filez->md = NULL; }
        if(filez->hexhtml) {
            FREE(filez->hexhtml)
            filez->hexhtml_size = 0;
        }
    }
    return 0;
}



// it's necessary to use a copy of fname and a way to free it automatically
u8 *dumpa_backup_fname(u8 *fname) {
    static u8   *buff = NULL;
    return re_strdup(&buff, fname, NULL);
}



i32 mydown_get_host(u8 *url, u8 **hostx, u16 *portx, u8 **urix, u8 **userx, u8 **passx, i32 verbose);



int fdnum_uses_filexor(int fdnum, filexor_t *fx) {
    if((fx->size > 0) && ((fx->fd < -MAX_FILES) || (fx->fd == fdnum))) {
        return 1;
    }
    return 0;
}



int fdnum_open(u8 *fname, int fdnum, int error) {
    static u8   filedir[PATHSZ + 1];
    socket_file_t   *sockfile;
    process_file_t  *procfile;
    audio_file_t    *audiofile;
    video_file_t    *videofile;
    winmsg_file_t   *winmsgfile;
    filenumber_t    *filez;
    u64     filesize;
    u8      tmp[32],
            *p,
            *fullname   = fname;

    if(!fname) return 0;
    if((fdnum < 0) || is_MEMORY_FILE(fname)) {
        fprintf(stderr, "\n"
            "Error: the filenumber field is minor than 0, if you want to use MEMORY_FILE\n"
            "       you don't need to \"reopen\" it in this way, just specify MEMORY_FILE\n"
            "       as filenumber in the various commands like:\n"
            "         get VAR long MEMORY_FILE\n");
        myexit(QUICKBMS_ERROR_BMS);
    } else if(fdnum >= MAX_FILES) {
        fprintf(stderr, "\nError: the BMS script uses more files than how much supported by this tool\n");
        myexit(QUICKBMS_ERROR_BMS);
    }
    filez = &g_filenumber[fdnum];

    if(!fname[0]) { // flushing only
        if(filez->fd) fflush(filez->fd);  // flushing is a bad idea, anyway I allow to force it
        return 0;
    }

    myfclose(fdnum);

    // do NOT use memset to clear the structure
    filez->bitchr = 0;
    filez->bitpos = 0;
    filez->bitoff = 0;
    filez->coverage = 0;

    xgetcwd(filedir, PATHSZ);
    if(strchr(fname, ':') || (fname[0] == '/')) {
        fprintf(stderr, "- open input file %s\n", fname);
    } else {
        fprintf(stderr, "- open input file %s%c%s\n", filedir, PATHSLASH, fname);
    }

    // alternative input/output
    if(strstr(fname, "://")) {
        sockfile = socket_open(fname);
        if(sockfile) {
            filez->sd       = sockfile;
            re_strdup(&filez->fullname, fname, NULL);
            if((sockfile->proto == IPPROTO_HTTP) || (sockfile->proto == IPPROTO_HTTPS)) {
                u8      *host   = NULL;
                u16     port    = 0;
                u8      *uri    = NULL;
                mydown_get_host(fname, &host, &port, &uri, NULL, NULL, 0);

                fullname = uri;
                p = mystrchrs(fullname, "?&");
                if(p) *p = 0;
                goto set_filez;

                // ???
                if(host) real_free(host);
                if(uri)  real_free(uri);
            } else {
                sprintf(tmp, "%u", sockfile->port);
                filez->filename = realloc(filez->filename, strlen(sockfile->host) + 1 + strlen(tmp) + 1);
                p = get_filename(sockfile->host);
                if(sockfile->port && !strchr(fname, '/')) {
                    sprintf(filez->filename, "%s:%s", p, tmp);
                } else {
                    sprintf(filez->filename, "%s",    p);
                }
                p = mystrchrs(filez->filename, "?&");
                if(p) *p = 0;
                re_strdup(&filez->basename, sockfile->host, NULL);
                re_strdup(&filez->fileext,  tmp, NULL); // the port
            }
            return 0;
        }

        procfile = process_open(fname);
        if(procfile) {
            sprintf(tmp, "%u", (i32)procfile->pid);
            re_strdup(&filez->fullname, fname, NULL);
            filez->filename = realloc(filez->filename, strlen(procfile->name) + 1 + strlen(tmp) + 1);
            sprintf(filez->filename, "%s:%s", procfile->name, tmp);
            re_strdup(&filez->basename, procfile->name, NULL);
            re_strdup(&filez->fileext,  tmp, NULL);
            filez->pd       = procfile;
            return 0;
        }

        audiofile = audio_open(fname);
        if(audiofile) {
            re_strdup(&filez->fullname, fname, NULL);
            re_strdup(&filez->filename, audiofile->name, NULL);
            re_strdup(&filez->basename, audiofile->name, NULL);
            re_strdup(&filez->fileext,  "", NULL);
            filez->ad       = audiofile;
            return 0;
        }

        videofile = video_open(fname);
        if(videofile) {
            re_strdup(&filez->fullname, fname, NULL);
            re_strdup(&filez->filename, videofile->name, NULL);
            re_strdup(&filez->basename, videofile->name, NULL);
            re_strdup(&filez->fileext,  "", NULL);
            filez->vd       = videofile;
            return 0;
        }

        winmsgfile = winmsg_open(fname);
        if(winmsgfile) {
            re_strdup(&filez->fullname, fname, NULL);
            re_strdup(&filez->filename, winmsgfile->name, NULL);
            re_strdup(&filez->basename, winmsgfile->name, NULL);
            re_strdup(&filez->fileext,  "", NULL);
            filez->md       = winmsgfile;
            return 0;
        }
    }

    if(g_write_mode) {
        g_force_readwrite_mode = 1;
        filez->fd = xfopen(fname, "r+b");    // do NOT modify, it must be both read/write
        g_force_readwrite_mode = 0; // automatically resetted by xfopen but it's ok
        if(!filez->fd) {
            if(g_reimport) {
                if(error) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                return -1;
            } else {
                if(!error) return -1;
                fprintf(stderr, "\n"
                    "- the file %s doesn't exist.\n"
                    "  Do you want to create it from scratch (y/N)?\n"
                    "  ", fname);
                if(get_yesno(NULL) == 'y') {
                    filez->fd = xfopen(fname, "w+b"); // do NOT create new files! Use log for that
                }
                if(!filez->fd) {
                    if(error) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                    return -1;
                }
            }
        }
        //setbuf(filez->fd, NULL);    // seems to cause only problems... mah
    } else {
        if(!strcmp(fname, "-")) {
            filez->fd = stdin;  // blah
        } else {
            filez->fd = xfopen(fname, "rb");
            if(!filez->fd) {
                if(error) STD_ERR(QUICKBMS_ERROR_FILE_READ);
                return -1;
            }
            filez->temporary_file = 0;
            if(!stricmp(filez->fullname, TEMPORARY_FILE)) {
                filez->temporary_file = 1;
            }
        }
    }

    fseek(filez->fd, 0, SEEK_END);
    filesize = ftell(filez->fd);
    fseek(filez->fd, 0, SEEK_SET);
#ifndef QUICKBMS64
    if(filesize > (u64)0xffffffffLL) {
        fprintf(stderr, "\n"
            "- the file is bigger than 4 gigabytes so it's NOT supported by quickbms.exe,\n"
            "  I suggest you to answer 'n' to the following question and using\n"
            "  quickbms_4gb_files.exe that doesn't have such limitation.\n"
            "  are you sure you want to continue anyway (y/N)?\n"
            "  ");
        if(get_yesno(NULL) != 'y') {
            if(g_continue_anyway) return -1;
            myexit(QUICKBMS_ERROR_USER);
        }
    } else if(filesize > (u64)0x7fffffffLL) {
        fprintf(stderr,
            "- the file is bigger than 2 gigabytes, it should work correctly but contact me\n"
            "  or the author of the script in case of problems or invalid extracted files.\n"
            "  in case of any problem try to use quickbms_4gb_files.exe first\n");
    }
#endif

    if(g_enable_hexhtml) {
        hexhtml_init(fdnum, filesize);
    }

    // filesize
    //filez->filesize = filesize;

    if(filez->temporary_file) {
        if(filez->filename) {
            goto skip_set_filez;    // keep the old references
        }
    }

    // fullname
    re_strdup(&filez->fullname, get_fullpath_from_name(fname, 0), NULL);    // allocate
    fullname = filez->fullname;

set_filez:
    // filename
    p = get_filename(fullname);
    re_strdup(&filez->filename, p, NULL);

    // basename
    re_strdup(&filez->basename, filez->filename, NULL); // allocate
    p = strrchr(filez->basename, '.');
    if(p) *p = 0;

    // extension
    p = get_extension(filez->filename);
    re_strdup(&filez->fileext, p, NULL);

    // filepath
    re_strdup(&filez->filepath, fullname, NULL);        // allocate
    p = mystrrchrs(filez->filepath, PATH_DELIMITERS);
    if(!p) p = filez->filepath;
    *p = 0;

    // fullbasename
    re_strdup(&filez->fullbasename, fullname, NULL);    // allocate
    p = mystrrchrs(filez->fullbasename, PATH_DELIMITERS);
    if(!p) p = filez->fullbasename;
    p = strrchr(p, '.');
    if(p) *p = 0;

skip_set_filez:
    // basically they are used for every operation but it's better if they get reinitialized when fdnum 0 is open,
    // the alternative is calling myfseek(fdnum, 0, SEEK_SET) which is probably very time consuming,
    // another possible alternative is: post_fseek_actions(fdnum, QUICKBMS_MIN_INT)
    if(fdnum_uses_filexor(fdnum, &g_filexor))   *g_filexor.pos   = 0;
    if(fdnum_uses_filexor(fdnum, &g_filerot))   *g_filerot.pos   = 0;
    if(fdnum_uses_filexor(fdnum, &g_filecrypt)) *g_filecrypt.pos = 0;
    if(!fdnum) {
        if(g_mex_default) add_var(BytesRead_idx, NULL, NULL, 0, sizeof(int));
    }

    if(g_mex_default && !fdnum) g_mex_default_init(1);
    return 0;
}



u_int myftell(int fdnum) {
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        return(g_memory_file[myabs(fdnum)].pos);
    }
    CHECK_FILENUM
    if(g_filenumber[fdnum].fd) return(ftell(g_filenumber[fdnum].fd));
    if(g_filenumber[fdnum].sd) return(((socket_file_t  *)g_filenumber[fdnum].sd)->pos);
    if(g_filenumber[fdnum].pd) return((u_int)(((process_file_t *)g_filenumber[fdnum].pd)->pos));
    if(g_filenumber[fdnum].ad) return(((audio_file_t *)  g_filenumber[fdnum].ad)->pos);
    if(g_filenumber[fdnum].vd) return(((video_file_t *)  g_filenumber[fdnum].vd)->pos);
    if(g_filenumber[fdnum].md) return(((winmsg_file_t *) g_filenumber[fdnum].md)->pos);
    fprintf(stderr, "\n"
        "Error: I forgot to implement the myftell operation for this file type\n"
        "       contact me!\n");
    myexit(QUICKBMS_ERROR_BMS);
    return 0;
}



void bytesread_eof(int fdnum, int len) {
    if(!g_mex_default) return; // just in case...
    int     oldoff  = 0;
    if(!fdnum) {
        oldoff = get_var32(BytesRead_idx);
        oldoff += len;
        if(oldoff < 0) oldoff = 0;
        add_var(BytesRead_idx, NULL, NULL, oldoff, sizeof(int));
        if(myftell(fdnum) >= myfilesize(fdnum)) {
        //if(myfeof(fdnum)) {   // feof doesn't work
            add_var(NotEOF_idx, NULL, NULL, 0, sizeof(int));
        }
    }
}



void post_fseek_actions(int fdnum, int diff_offset) {
#define post_fseek_actions_do(X)    { \
        (*X) += diff_offset; \
        /*if((*X) < 0) (*X) = 0;*/ \
    }

    if(fdnum_uses_filexor(fdnum, &g_filexor))   post_fseek_actions_do(g_filexor.pos)
    if(fdnum_uses_filexor(fdnum, &g_filerot))   post_fseek_actions_do(g_filerot.pos)
    if(fdnum_uses_filexor(fdnum, &g_filecrypt)) post_fseek_actions_do(g_filecrypt.pos)
    if(g_mex_default) bytesread_eof(fdnum, diff_offset);
}



int myfeof(int fdnum) {
    memory_file_t   *memfile    = NULL;
    int     ret = 0;

    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        if(memfile->pos >= memfile->size) {
            ret = 1;
        }
    } else {
        CHECK_FILENUM
        if(g_filenumber[fdnum].fd) ret = feof(g_filenumber[fdnum].fd);
        // ret is already 0 for the others
    }
    return ret;
}



u8 *myfdata(int fdnum) {
    memory_file_t   *memfile    = NULL;
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        return memfile->data;
    } else {
        CHECK_FILENUM
        return NULL;
    }
    return NULL;
}



void post_fread_actions(int fdnum, u8 *data, int size) {
    int     i;

    // fdnum is used only for bytesread_eof so ignore it
    //if(!data) not needed here
    if(fdnum_uses_filexor(fdnum, &g_filexor)) {
        for(i = 0; i < size; i++) {
            data[i] ^= g_filexor.key[(*g_filexor.pos) % g_filexor.size];
            (*g_filexor.pos)++;
        }
    }
    if(fdnum_uses_filexor(fdnum, &g_filerot)) {
        for(i = 0; i < size; i++) {
            data[i] += g_filerot.key[(*g_filerot.pos) % g_filerot.size];
            (*g_filerot.pos)++;
        }
    }
    if(fdnum_uses_filexor(fdnum, &g_filecrypt)) {
        perform_encryption_and_crchash(data, size);
    }
    if(g_mex_default) bytesread_eof(fdnum, size);
}



// necessary for nameless files that read some bytes messing the correct coverage
int myfr_remove_coverage(int fdnum, int len) {
    len = -len; // just for an easy copy&paste from myfr()

    memory_file_t   *memfile    = NULL;
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        memfile->coverage += len;
    } else {
        CHECK_FILENUMX
        g_filenumber[fdnum].coverage += len;
    }
    return 0;
}



int myfr(int fdnum, u8 *data, int size, int quit_if_diff) {
    memory_file_t   *memfile    = NULL;
    int     len     = 0;
            //quit_if_diff    = 1;

    // if(!data) not necessary
    if(size < 0) {
        size = BUFFSZ;
        quit_if_diff = 0;
    }
    if(fdnum < 0) {
        CHECK_MEMNUM(size)    // fake writer
        memfile = &g_memory_file[myabs(fdnum)];
        if(!memfile->data) {
            if(myabs(fdnum) == 1) {
                fprintf(stderr, "\nError: MEMORY_FILE has not been used/declared yet\n");
            } else {
                fprintf(stderr, "\nError: MEMORY_FILE%d has not been used/declared yet\n", (i32)myabs(fdnum));
            }
            myexit(QUICKBMS_ERROR_BMS);
        }
        len = size;
        if((memfile->pos + size) > memfile->size) {
            len = memfile->size - memfile->pos;
        }
        memcpy(data, memfile->data + memfile->pos, len);
        memfile->pos += len;
        memfile->coverage += len;
    } else {
        CHECK_FILENUM
        if(g_filenumber[fdnum].fd) {
            len = fread(data, 1, size, g_filenumber[fdnum].fd);
            if(g_write_mode) {
                /*
                  in "r+b" mode the offsets are not synchronized so happens horrible things like:
                  - read 7 bytes, write 7 bytes... from offset 0 instead of 7
                  - file of 12 bytes, read 7, read 4, write 7... fails because can't increase size
                  the following lame solution works perfectly and solves the problem
                */
                fseek(g_filenumber[fdnum].fd, ftell(g_filenumber[fdnum].fd), SEEK_SET);
            }
        }
        else if(g_filenumber[fdnum].sd) len = socket_read(    g_filenumber[fdnum].sd, data, size);
        else if(g_filenumber[fdnum].pd) len = process_read(   g_filenumber[fdnum].pd, data, size);
        else if(g_filenumber[fdnum].ad) len = audio_read(     g_filenumber[fdnum].ad, data, size);
        else if(g_filenumber[fdnum].vd) len = video_read(     g_filenumber[fdnum].vd, data, size);
        else if(g_filenumber[fdnum].md) len = winmsg_read(    g_filenumber[fdnum].md, data, size);
        else {
            fprintf(stderr, "\n"
                "Error: I forgot to implement the myfr operation for this file type\n"
                "       contact me!\n");
            myexit(QUICKBMS_ERROR_BMS);
        }
        if(len < 0) len = 0;    // some functions may return a -1 error
        if(g_enable_hexhtml) hexhtml_add(fdnum, data, len);
        g_filenumber[fdnum].coverage += len;
    }

    if(g_parsing_debug && fdnum_is_valid && g_filenumber[fdnum].fd) {
        parsing_debug_t *parsing_debug = real_calloc(1, sizeof(parsing_debug_t));
        parsing_debug->end_offset = myftell(fdnum);
        parsing_debug->offset     = parsing_debug->end_offset - len;
        parsing_debug->entropy    = calculate_entropy(data, len, NULL);
        CDL_APPEND(g_filenumber[fdnum].parsing_debug, parsing_debug);
    }

    if((len != size) && quit_if_diff) {
        u8  *fullname = "";
        if(fdnum_is_valid && g_filenumber[fdnum].fullname) fullname = g_filenumber[fdnum].fullname;
        fprintf(stderr, "\n"
            "Error: incomplete input file %d: %s\n"
            "       Can't read %s bytes from offset %"PRIx".\n"
            "       Anyway don't worry, it's possible that the BMS script has been written\n"
            "       to exit in this way if it's reached the end of the archive so check it\n"
            "       or contact its author or verify that all the files have been extracted.\n"
            "       Please check the following coverage information to know if it's ok.\n"
            "\n",
            (i32)fdnum,
            fullname,
            myitoa_decimal_notation(size - len),
            myftell(fdnum));

        fcoverage(fdnum);

        if(g_continue_anyway) return -1;
        myexit(QUICKBMS_ERROR_FILE_READ);
    }
    post_fread_actions(fdnum, data, len);
    return len;
}



// copied from dumpa_memory_file, experimental
int fdnum_append_mode(memory_file_t *memfile, FILE *fd, int size) {
    if(g_append_mode == APPEND_MODE_NONE) return -1;

    int     cmd = g_last_cmd;
    if(
        (CMD.type != CMD_Put)
     && (CMD.type != CMD_PutDString)
     && (CMD.type != CMD_PutCT)
     && (CMD.type != CMD_PutBits)   // ???
    ) return -1;

    if(memfile) {

               if(g_append_mode == APPEND_MODE_APPEND) {
            memfile->pos   = memfile->size;
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            memfile->size += size;
            myalloc(&memfile->data, memfile->size, &memfile->maxsize);

        } else if(g_append_mode == APPEND_MODE_OVERWRITE) {
            // allow goto to decide where placing the new content
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            if((memfile->pos + size) > memfile->size) {
                memfile->size = memfile->pos + size;
                myalloc(&memfile->data, memfile->size, &memfile->maxsize);
            }

        } else if(g_append_mode == APPEND_MODE_BEFORE) {
            memfile->pos   = 0;
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            memfile->size += size;
            myalloc(&memfile->data, memfile->size, &memfile->maxsize);
            mymemmove(memfile->data + size, memfile->data, memfile->size - size);

        } else if(g_append_mode == APPEND_MODE_INSERT) {
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            memfile->size += size;
            myalloc(&memfile->data, memfile->size, &memfile->maxsize);
            mymemmove(memfile->data + memfile->pos + size, memfile->data + memfile->pos, memfile->size - (memfile->pos + size));
        }

    } else if(fd) {

        u_int   fd_pos  = ftell(fd);
        u_int   fd_size = filesize(fd);

               if(g_append_mode == APPEND_MODE_APPEND) {
            fseek(fd, 0, SEEK_END);
            fd_pos   = fd_size;
            if((fd_size + size) < fd_size) ALLOC_ERR;
            fd_size += size;

        } else if(g_append_mode == APPEND_MODE_OVERWRITE) {
            // allow goto to decide where placing the new content
            if((fd_size + size) < fd_size) ALLOC_ERR;
            if((fd_pos + size) > fd_size) fd_size = fd_pos + size;

        } else if(g_append_mode == APPEND_MODE_BEFORE) {
            fseek(fd, 0, SEEK_SET);
            fd_pos   = 0;
            if((fd_size + size) < fd_size) ALLOC_ERR;
            make_file_space(fd, size);
            fd_size += size;

        } else if(g_append_mode == APPEND_MODE_INSERT) {
            if((fd_size + size) < fd_size) ALLOC_ERR;
            make_file_space(fd, size);
            fd_size += size;
        }

    }

    return 0;
}



int myfw(int fdnum, u8 *data, int size) {
    memory_file_t   *memfile    = NULL;
    int     len = 0,
            tmp;

    // if(!data) not necessary
    if(size < 0) {
        fprintf(stderr, "\n"
            "Error: problems with input file number %d, can't write negative size.\n"
            "\n", (i32)fdnum);
        if(g_continue_anyway) return -1;
        myexit(QUICKBMS_ERROR_FILE_WRITE);
    }
    post_fread_actions(-1, data, size);
    if(fdnum < 0) {
        CHECK_MEMNUM(size)    // fake writer
        memfile = &g_memory_file[myabs(fdnum)];
        if(!memfile->data) {
            if(myabs(fdnum) == 1) {
                fprintf(stderr, "\nError: MEMORY_FILE has not been used/declared yet\n");
            } else {
                fprintf(stderr, "\nError: MEMORY_FILE%d has not been used/declared yet\n", (i32)myabs(fdnum));
            }
            myexit(QUICKBMS_ERROR_BMS);
        }
        len = size;
        if(!fdnum_append_mode(memfile, NULL, size)) {
            // nothing to do, all done by the function
        } else {
            tmp = memfile->pos + len;
            if((tmp < memfile->pos) || (tmp < len)) ALLOC_ERR;
            if(tmp > memfile->size) {
                memfile->size = tmp;
                myalloc(&memfile->data, memfile->size, &memfile->maxsize);
            }
        }
        memcpy(memfile->data + memfile->pos, data, len);
        memfile->pos += len;
    } else {
        CHECK_FILENUM
        if(g_filenumber[fdnum].fd) {
            // seems impossible but if you use the following script it will give no error
            //   get DUMMY long     # if you remove this line it will work
            //   put 1234  long
            // so, also for better security, I have added the -w check directly here
            if(g_write_mode) {
                fdnum_append_mode(NULL, g_filenumber[fdnum].fd, size);
                len = fwrite(data, 1, size, g_filenumber[fdnum].fd);
                fflush(g_filenumber[fdnum].fd);
            }
        }
        else if(g_filenumber[fdnum].sd) len = socket_write(   g_filenumber[fdnum].sd, data, size);
        else if(g_filenumber[fdnum].pd) len = process_write(  g_filenumber[fdnum].pd, data, size);
        else if(g_filenumber[fdnum].ad) len = audio_write(    g_filenumber[fdnum].ad, data, size);
        else if(g_filenumber[fdnum].vd) len = video_write(    g_filenumber[fdnum].vd, data, size);
        else if(g_filenumber[fdnum].md) len = winmsg_write(   g_filenumber[fdnum].md, data, size);
        else {
            fprintf(stderr, "\n"
                "Error: I forgot to implement the myfw operation for this file type\n"
                "       contact me!\n");
            myexit(QUICKBMS_ERROR_BMS);
        }
    }
    if(len != size) {
        fprintf(stderr, "\n"
            "Error: problems with input file number %d, can't write %s bytes.\n"
            "%s"
            "\n", (i32)fdnum, myitoa_decimal_notation(size - len),
            g_write_mode ? "" : "\n       you MUST use the -w option for enabling the file writing mode\n");
        if(g_continue_anyway) return -1;
        myexit(QUICKBMS_ERROR_FILE_WRITE);
    }
    return len;
}



int myfgetc(int fdnum) {
    int     c;
    u8      buff[1];

    c = myfr(fdnum, buff, 1, TRUE);
    if(c <= 0) return -1;
    return(buff[0]);
}



int myfpeek(int fdnum) {    // exactly like myfgetc, without the error
    int     c;
    u8      buff[1];

    c = myfr(fdnum, buff, 1, FALSE);
    if(c <= 0) return -1;
    return(buff[0]);
}



int myfputc(int c, int fdnum) {
    int     ret;
    u8      buff[1];

    buff[0] = c;
    ret = myfw(fdnum, buff, 1);
    if(ret < 0) return ret;
    return(c);
}



int myfseek_stream(int fdnum, u_int offset, int type) {
    u_int   oldoff,
            oldsize;
    int     i;

    // only for non-memory_file and non-real files
    if(fdnum < 0) return -1;
    if(g_filenumber[fdnum].fd) return -1;

    oldoff = myftell(fdnum);
    oldsize = myfilesize(fdnum);

    switch(type) {
        case SEEK_SET:                      break;
        case SEEK_CUR: offset += oldoff;    break;
        case SEEK_END: offset += oldsize;   break;
        default: break;
    }

    if(offset > oldoff) {
        for(i = 0; i < (offset - oldoff); i++) {
            if(myfgetc(fdnum) < 0) return -1;
        }
    }
    if(offset > myfilesize(fdnum)) return -1;   // check with new size to avoid issues, anyway a negative offset is still possible

    if(g_filenumber[fdnum].sd) (((socket_file_t  *)g_filenumber[fdnum].sd)->pos) = offset;
    if(g_filenumber[fdnum].pd) (((process_file_t *)g_filenumber[fdnum].pd)->pos) = (void *)offset;
    if(g_filenumber[fdnum].ad) (((audio_file_t *)  g_filenumber[fdnum].ad)->pos) = offset;
    if(g_filenumber[fdnum].vd) (((video_file_t *)  g_filenumber[fdnum].vd)->pos) = offset;
    if(g_filenumber[fdnum].md) (((winmsg_file_t *) g_filenumber[fdnum].md)->pos) = offset;
    return 0;
}



int myfseek(int fdnum, u_int offset, int type) {
    memory_file_t   *memfile    = NULL;
    u_int   oldoff,
            oldsize;
    int     err = 0;

    if(type == SEEK_END) {
        if((int)offset > 0) offset = -offset;
    }

    oldoff  = myftell(fdnum);
    oldsize = myfilesize(fdnum);
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        memfile = &g_memory_file[myabs(fdnum)];
        switch(type) {
            case SEEK_SET: memfile->pos = offset;                   break;
            case SEEK_CUR: memfile->pos += offset;                  break;
            case SEEK_END: memfile->pos = memfile->size + offset;   break;
            default: break;
        }
        if(memfile->pos < 0) memfile->pos = 0;
        if(memfile->pos > memfile->size) {
            if(g_append_mode) {
                if(g_append_mode == APPEND_MODE_APPEND) {
                    memfile->pos = memfile->size;
                } else {
                    // allocate space
                    memfile_resize(memfile, memfile->pos);
                }
            } else {
                err = -1;
            }
        }
    } else {
        CHECK_FILENUM
        if(g_filenumber[fdnum].fd) {
            if(type == SEEK_SET) {
                err = fseek(g_filenumber[fdnum].fd, offset, type);
            } else {    // signed
                err = fseek(g_filenumber[fdnum].fd, (int)offset, type);
                if((type == SEEK_END) && offset) {  // because "goto 0 0 SEEK_END" is usually used for other reasons
                    g_filenumber[fdnum].tail_toc_offset = myftell(fdnum);
                    g_filenumber[fdnum].tail_toc_size   = oldsize - g_filenumber[fdnum].tail_toc_offset;
                }
            }
            // if(g_append_mode)
            // there is probably no problem in reserving space in a file when it's used "w+b"
        } else if(g_filenumber[fdnum].pd) {
            switch(type) {
                case SEEK_SET: ((process_file_t *)g_filenumber[fdnum].pd)->pos = (void *)offset; break;
                case SEEK_CUR: ((process_file_t *)g_filenumber[fdnum].pd)->pos += offset; break;
                case SEEK_END: ((process_file_t *)g_filenumber[fdnum].pd)->pos = ((process_file_t *)g_filenumber[fdnum].pd)->base + ((process_file_t *)g_filenumber[fdnum].pd)->size + offset; break;
                default: break;
            }
        } else {
            err = myfseek_stream(fdnum, offset, type);
        /*
        } else {
            fprintf(stderr, "\n"
                "Error: I forgot to implement the myfseek operation for this file type\n"
                "       contact me!\n");
            myexit(QUICKBMS_ERROR_BMS);
        */
        }
    }
    if(err) {
        fprintf(stderr, "\nError: [myfseek] offset 0x%"PRIx" in file %d can't be reached\n", offset, (i32)fdnum);
        if(g_continue_anyway) return -1;
        myexit(QUICKBMS_ERROR_FILE_READ);
    }
    post_fseek_actions(fdnum, myftell(fdnum) - oldoff);
    return 0;
}



u64 getxx(u8 *tmp, int bytes) {
    u64     num;
    int     i;

    if(!tmp) return 0;
    num = 0;
    for(i = 0; i < bytes; i++) {
        if(g_endian == MYLITTLE_ENDIAN) {
            if(i >= (int)sizeof(num)) continue;
            num |= ((u64)tmp[i] << (u64)(i << (u64)3));
        } else {
            if(i < (bytes - (int)sizeof(num))) continue;
            num |= ((u64)tmp[i] << (u64)((bytes - (u64)1 - i) << (u64)3));
        }
    }
    return(num);
}



int putxx(u8 *data, u64 num, int bytes) {
    int     i;

    if(!data) return 0;
    for(i = 0; i < bytes; i++) {
        if(g_endian == MYLITTLE_ENDIAN) {
            if(i < (int)sizeof(num))            data[i] = num >> (i << (u64)3);
            else                                data[i] = 0;
        } else {
            if(i >= (bytes - (int)sizeof(num))) data[i] = num >> ((bytes - (u64)1 - i) << (u64)3);
            else                                data[i] = 0;
        }
    }
    return(bytes);
}



int fputxx(int fdnum, int num, int bytes) {
    u8      tmp[bytes];

    // if(!fd) do nothing, modify mywr
    putxx(tmp, num, bytes);
    return(myfw(fdnum, tmp, bytes));
}



int fgetxx(int fdnum, int bytes, int *error) {
    int     tmp_error;
    int     ret;
    u8      tmp[bytes];

    if(!error) error = &tmp_error;
    *error = 0;

    // if(!fd) do nothing, modify myfr
    ret = myfr(fdnum, tmp, bytes, TRUE);
    if(ret < 0) {
        *error = 1;
        return -1;
    }
    ret = getxx(tmp, bytes);
    if(g_endian_killer) { // reverse endianess
        g_endian = (g_endian == MYLITTLE_ENDIAN) ? MYBIG_ENDIAN : MYLITTLE_ENDIAN;
        myfseek(fdnum, -bytes, SEEK_CUR);
        fputxx(fdnum, ret, bytes);
        g_endian = (g_endian == MYLITTLE_ENDIAN) ? MYBIG_ENDIAN : MYLITTLE_ENDIAN;
    }
    return ret;
}



// how the bits reading works:
// the idea is having something that doesn't occupy much space in the file arrays (6 bytes per file)
// and that is not touched by the other functions to avoid to loose performances for a rarely used
// function so I have used the following fields:
//  bitchr = the current byte read from the file
//  bitpos = the amount of bits of bitchr that have been consumed (3 bits)
//  bitoff = the current offset, it's necessary to know if in the meantime
//           the user has changed offset and so bitpos must be resetted

u64 fd_read_bits(u_int bits, u8 *bitchr, u8 *bitpos, int fd, u8 **data) {
    u64     ret = 0;
    int     i,
            t;
    u8      bc  = 0,
            bp  = 0;

    if(bitchr) bc = *bitchr;
    if(bitpos) bp = *bitpos;
    //if(bits > INTSZ) return 0; // it's already called only for max 32/64 bits
    (bp) &= 7; // just for security
    for(i = 0; i < bits; i++) {
        if(!bp) {
            t = data ? *(*data)++ : myfgetc(fd);
            bc = (t < 0) ? 0 : t;
        }
        if(g_endian == MYLITTLE_ENDIAN) { // uhmmm I don't think it's very fast... but works
            ret = (ret >> (u64)1) | (u64)((((u64)bc >> (u64)bp) & (u64)1) << (u64)(bits - 1));
        } else {
            ret = (ret << (u64)1) | (u64)((((u64)bc << (u64)bp) >> (u64)7) & (u64)1);
        }
        (bp)++;
        (bp) &= 7; // leave it here
    }
    if(bitchr) *bitchr = bc;
    if(bitpos) *bitpos = bp;
    return ret;
}



int fd_write_bits(u64 num, u_int bits, u8 *bitchr, u8 *bitpos, int fd, u8 **data) {
    int     i,
            t,
            rem = 0;
    u8      bc  = 0,
            bp  = 0,
            bit;

    if(bitchr) bc = *bitchr;
    if(bitpos) bp = *bitpos;
    //if(bits > INTSZ) return 0; // it's already called only for max 32/64 bits
    (bp) &= 7; // just for security
    for(i = 0; i < bits; i++) {
        if(!bp) {
            if(rem) {
                if(data) {
                    (*data)--;
                    *(*data)++ = bc;
                } else {
                    myfseek(fd, -1, SEEK_CUR);
                    myfputc(bc, fd);
                }
                rem = 0;
            }
            t = data ? *(*data) : myfpeek(fd); //myfgetc(fd);
            if(t < 0) {
                bc = 0;
                if(data) {
                    *(*data)++ = bc;
                } else {
                    myfputc(bc, fd);
                }
            } else {
                bc = t;
            }
        }
        if(g_endian == MYLITTLE_ENDIAN) { // uhmmm I don't think it's very fast... but works
            t = (u64)1 << (u64)bp;
            bit = (num >> (u64)i) & (u64)1;
        } else {
            t = (u64)1 << (u64)(7 - bp);
            bit = (num >> (u64)((bits - i) - 1)) & 1;
        }
        if(bit) {
            bc |= t;   // put 1
        } else {
            bc &= ~t;  // put 0
        }
        (bp)++;
        (bp) &= 7; // leave it here
        rem++;
    }
    if(rem) {
        if(data) {
            (*data)--;
            *(*data)++ = bc;
        } else {
            myfseek(fd, -1, SEEK_CUR);
            myfputc(bc, fd);
        }
    }
    if(bitchr) *bitchr = bc;
    if(bitpos) *bitpos = bp;
    return i;
}



int bits2str(u8 *out, int outsz, int bits, u8 *bitchr, u8 *pos, int fd) {
    int     max8    = 8;
    u8      *o;

    if(!out) return 0;
    //outsz -= (*pos >> 3); pos is 3 bit
    if(outsz <= 0) return 0;
    if(outsz < (bits >> (int)3)) {
        bits = outsz << (int)3;
    }
    for(o = out; bits > 0; bits -= max8) {
        if(bits < 8) max8 = bits;
        *o++ = fd_read_bits(max8, bitchr, pos, fd, NULL);
    }
    return o - out;
}



int str2bits(u8 *in, int insz, int bits, u8 *bitchr, u8 *pos, int fd) {
    int     max8    = 8;
    u8      *o;

    if(!in) return 0;
    //insz -= (*pos >> 3); pos is 3 bit
    if(insz <= 0) return 0;
    if(insz < (bits >> (int)3)) {
        bits = insz << (int)3;
    }
    for(o = in; bits > 0; bits -= max8) {
        if(bits < 8) max8 = bits;
        fd_write_bits(*o++, max8, bitchr, pos, fd, NULL);
    }
    return(o - in);
}



int my_fdbits(int fdnum, u8 *out_bitchr, u8 *out_bitpos, u_int *out_bitoff, u8 in_bitchr, u8 in_bitpos, u_int in_bitoff) {
    if(fdnum < 0) {
        CHECK_MEMNUM(0)
        fdnum = myabs(fdnum);   // remember that fdnum is now changed
        if(out_bitchr && out_bitpos && out_bitoff) {
            *out_bitchr = g_memory_file[fdnum].bitchr;
            *out_bitpos = g_memory_file[fdnum].bitpos;
            *out_bitoff = g_memory_file[fdnum].bitoff;
        } else {
            g_memory_file[fdnum].bitchr = in_bitchr;
            g_memory_file[fdnum].bitpos = in_bitpos;
            g_memory_file[fdnum].bitoff = in_bitoff;
        }
    } else {
        CHECK_FILENUM
        if(out_bitchr && out_bitpos && out_bitoff) {
            *out_bitchr = g_filenumber[fdnum].bitchr;
            *out_bitpos = g_filenumber[fdnum].bitpos;
            *out_bitoff = g_filenumber[fdnum].bitoff;
        } else {
            g_filenumber[fdnum].bitchr = in_bitchr;
            g_filenumber[fdnum].bitpos = in_bitpos;
            g_filenumber[fdnum].bitoff = in_bitoff;
        }
    }
    return 0;
}



int _FILEZ(int fdnum) {
    // do not add any "fdnum < -MAX_FILES" because this work-around is currently used in filexor
    if(fdnum >= MAX_FILES) fdnum = get_var32(fdnum - MAX_FILES);
    if(g_replace_fdnum0 && !fdnum) fdnum = g_replace_fdnum0;    // this happens at runtime
    return fdnum;
}



int myatoifile(u8 *str) {   // for quick usage
    int     fdnum   = 0;

    if(is_MEMORY_FILE(str)) {
        fdnum = get_memory_file(str);
    } else if(str && !strnicmp(str, "ARRAY", 5)) {
        fdnum = myatoi(str + 5);
    } else {
        if(!str || !str[0]) goto quit;  // default is file number 0
        if(!myisdechex_string(str)) {
            int idx = add_var(0, str, NULL, 0, -2);
            /*if(!g_variable[idx].constant)*/ return(MAX_FILES + idx);  // the syntax of idstring is terrible, this is a work-around that works 99% of times
        }
        fdnum = myatoi(str);
    }
    if((fdnum < -MAX_FILES) || (fdnum >= MAX_FILES)) {
        fprintf(stderr, "\nError: [myatoifile] invalid FILE number (%d)\n", (i32)fdnum);
        myexit(QUICKBMS_ERROR_BMS);
    }
quit:
    //if(g_replace_fdnum0 && !fdnum) fdnum = g_replace_fdnum0;    // this happens while parsing the bms script (duplicate of _FILEZ, remove)
    return(fdnum);
}



int zero_fdnum(int fdnum, u8 chr, int size) {
    int     t;
    u8      *tmp = QUICKBMS_CACHED_IO(&t);
    memset(tmp, chr, t);
    while(size > 0) {   // skip negative size
        if(size < t) t = size;
        if(myfw(fdnum, tmp, t) < 0) return -1;
        size -= t;
    }
    return 0;
}



int dumpa_memory_file(memory_file_t *memfile, u8 **ret_data, int size, int *ret_size) {
    u8      *data   = NULL;

    if(ret_data) data = *ret_data;
    MAX_ALLOC_CHECK(size);
    if(g_append_mode) {
               if(g_append_mode == APPEND_MODE_APPEND) {
            memfile->pos   = memfile->size;
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            memfile->size += size;

        } else if(g_append_mode == APPEND_MODE_OVERWRITE) {
            // allow goto to decide where placing the new content
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            if((memfile->pos + size) > memfile->size) memfile->size = memfile->pos + size;

        } else if(g_append_mode == APPEND_MODE_BEFORE) {
            memfile->pos   = 0;
            if((memfile->size + size) < memfile->size) ALLOC_ERR;
            memfile->size += size;
        }
    } else {
        memfile->pos   = 0;
        memfile->size  = size;
    }
    if((memfile->pos + size) < memfile->pos) ALLOC_ERR;
    memfile->bitchr = 0;    // reset the bit stuff
    memfile->bitpos = 0;
    memfile->bitoff = 0;

    // the following are the new instructions for using less memory
    if(ret_size && !memfile->data && data) {
        memfile->data = data;   // direct assignment
        if(ret_data) *ret_data = NULL;  // set to NULL, do NOT free!
        *ret_size = 0;
        goto quit;
    }

    myalloc(&memfile->data, memfile->size, &memfile->maxsize);
    if(g_append_mode == APPEND_MODE_BEFORE) {
        mymemmove(memfile->data + size, memfile->data, memfile->size - size);
    }
    if(memfile->data) {
        memcpy(memfile->data + memfile->pos, data, size);

        // update positions in append mode only, just like a normal file!
        // don't do it for non-append_mode or many scripts will no longer work!
        if(g_append_mode) memfile->pos += size;
    }
quit:
    return size;
}



u8 *rename_auto(int cnt, u8 *old_name) {
    static u8   new_name[PATHSZ + 1];
    int     i,
            len,
            extlen;
    u8      *p,
            *ext;

    // new_name and old_name can be the same because it's used in a cycle
    extlen = 0;
    ext = strrchr(old_name, '.');
    if(ext) {
        *ext++ = 0;
        extlen = strlen(ext);
    }
    len = strlen(old_name);
    if((len + 1 + 8 + 1 + extlen) >= PATHSZ) {
        sprintf(new_name, g_decimal_names ? "%u.dat" : "%08x.dat", (i32)cnt);
    } else {
        if(ext) {
            p = new_name + len + 1 + 8 + 1;
            for(i = 0;; i++) {
                p[i] = ext[i];
                if(!ext[i]) break;
            }
        }
        mystrcpy(new_name, old_name, PATHSZ);
        len = strlen(new_name);
        p = new_name + len;
        sprintf(p, g_decimal_names ? "_%u" : "_%08x", (i32)cnt);
        if(ext) p[1 + 8] = '.';
    }
    if(ext) ext[-1] = '.';
    return(new_name);
}



u8 *rename_invalid(u8 *old_name) {
    static int  new_namey = 0;
    static u8   new_namex[MULTISTATIC][PATHSZ + 1];
    FILE    *fd;
    int     i;
    u8      tmp[1 + 8 + 1],
            c,
            *p,
            *new_name;

    new_name = (u8 *)new_namex[new_namey++ % MULTISTATIC];
    new_name[0] = 0;

redo:
    if(!old_name) old_name = "noname";
    fgetz(new_name, PATHSZ, stdin,
        "\n"
        "- it's not possible to create that file due to its filename or related\n"
        "  incompatibilities (for example already exists a folder with that name), so\n"
        "  now you must choose a new filename for saving it.\n"
        "  if you press ENTER a new name will be generated automatically.\n"
        "  - old: %s\n"
        "  - new: ", old_name);
    if(!new_name[0]) {
        for(i = 0; i < (PATHSZ - sizeof(tmp)); i++) { // reserve space for integer and chars
            c = old_name[i];
            if(!c) break;
            if(myisalnum(c) || strchr(PATH_DELIMITERS ".", c)) new_name[i] = c;
            else new_name[i] = '_';
        }
        if(!i) {
            fprintf(stderr, "\nError: rename_invalid failed to automatically generate the new filename\n");
            goto redo;
        }
        new_name[i] = 0;
        p = strrchr(new_name, '.');
        if(!p) p = new_name + strlen(new_name);
        memmove(p + 1 + 8, p, strlen(p) + 1);
        for(i = 0;; i++) {
            sprintf(tmp, g_decimal_names ? "_%u" : "_%08x", (i32)i);
            memcpy(p, tmp, 1 + 8);
            fd = xfopen(new_name, "rb");
            if(!fd) break;
            FCLOSE(fd);
        }
    }
    return(new_name);
}



// log to file happens only here
int dumpa_direct_copy(int fdnum, FILE *fd, u8 *out, int size, int no_compare, u8 *fname) {
    static u8   *cname  = NULL;

    FILE    *fdc        = NULL;
    int     len         = -1,
            do_compare,
            cres        = 0;    // 0 means ok / same file

    do_compare = g_compare_folder && !no_compare;

    if(do_compare) {
        if(!fname) return -1;
        spr(&cname, "%s%c%s", g_compare_folder, PATHSLASH, fname);
        fdc = xfopen(cname, "rb");
        if(fdc) {
            // check file size to avoid reading the whole file
            if(filesize(fdc) != size) { // no need to use goto
                FCLOSE(fdc);
                cres = -1;
            }
        }
    }

    if(out) {
        // normal buffer copy
        if(do_compare) {
            if(fdc) {
                cres = file_compare(fdc, out, size);
                // if(cres < 0) break;
            }
        } else {
            if(fd) len = fwrite(out, 1, size, fd);
            else   len = size;  // necessary for quickzip and compatible with next "if" statement
        }
    } else {
        // direct copy
        TMP_FILE_READ(myfr, fdnum, break)
            if(do_compare) {
                if(fdc) {
                    cres = file_compare(fdc, tmp, t);
                    if(cres < 0) break;
                }
            } else {
                if(fd) t = fwrite(tmp, 1, t, fd);
            }
            if(t <= 0) break;
        }
    }

    if(do_compare) {
        if(!fdc || (cres < 0)) {
            len = dumpa_direct_copy(fdnum, fd, out, size, 1, fname);    // do not use return or fdc doesn't get closed
        }
        /*if(fdc)*/ FCLOSE(fdc);
    }
    return len;
}



#define dumpa_state_check_compression   ((zsize > 0) && (size > 0)) // used also in reimport mode with -.



static void dumpa_state(int *quickbms_compression, int *quickbms_encryption, int zsize, int size, int xsize) {
    // notes:
    // encryption uses only the output buffer: memory = file_size
    // compression uses both input and output: memory = file_size * 2 (at least)
    // otherwise no memory is used

    //if(quickbms_compression) {
        *quickbms_compression = 0;
        if(dumpa_state_check_compression) *quickbms_compression = 1;
    //}
    //if(quickbms_encryption) {
        *quickbms_encryption = 0;
        if(!perform_encryption_and_crchash(NULL, -1)) *quickbms_encryption = 1;
    //}
}



int CMD_Encryption_func(int cmd, int invert_mode);



#define ask_force_reimport(COMPRESSION, FORCE_CMD) \
                fprintf(stderr, "\n" \
                    "Error: file \"%s\"\n", \
                    fname); \
                if(g_list_only || g_void_dump) { \
                    /* skip boring message */ \
                } else { \
                    fprintf(stderr, \
                        "       the reimport option acts as a reinjector, therefore you cannot insert a\n" \
                        "       file if it's larger than the original for not overwriting the rest of\n" \
                        "       the archive which cannot be loaded correctly:\n"); \
                } \
                \
                if(COMPRESSION) { \
                    fprintf(stderr, "\n" \
                        "         new size: %"PRId" uncompressed\n" \
                        "         old size: %"PRId" uncompressed\n" \
                        "\n", \
                        zsize, \
                        old_size); \
                } else { \
                    fprintf(stderr, "\n" \
                        "         new size: %"PRId" (%"PRId" uncompressed)\n" \
                        "         old size: %"PRId" (%"PRId" uncompressed)\n" \
                        "\n", \
                        size, zsize, \
                        old_zsize, old_size); \
                } \
                \
                if(g_list_only || g_void_dump) goto skip_import; \
                \
                fprintf(stderr, \
                    "- do you want to skip this file, quit or retry? (y/N/r)\n" \
                    "  y: continue with the next file and skip the current file\n" \
                    "  N: (default) terminate QuickBMS, maybe try later with the -r -r mode\n" \
                    "  r: retry the reimporting so you can edit the file in the meantime\n" \
                    "  force: corrupt the archive by writing the bigger file (NEVER use this!!!)\n" \
                    "  "); \
                fgetz(ans, sizeof(ans), stdin, NULL); \
                if(get_yesno(ans) == 'y') { \
                    goto skip_import; \
                } else if(get_yesno(ans) == 'r') { \
                    goto redo_import; \
                } else { \
                    if(!strnicmp(ans, "force", 5)) { \
                        FORCE_CMD \
                    } else { \
                        fprintf(stderr, \
                            "       now it's suggested to restore the backup of the original archive\n" \
                            "       because the current one could have been corrupted due to the\n" \
                            "       incomplete operation\n"); \
                        if(g_continue_anyway) { ret_value = -1; goto quit; } \
                        myexit(QUICKBMS_ERROR_COMPRESSION); \
                    } \
                }


                
int dumpa_xsize(int size, int xsize) {
    if(xsize <= 0) {
        // do nothing
    } else if(xsize > size) {
        // if it's already the total size
        size = xsize;
    } else if(xsize < size) {
        // if you specify the alignment size
        if(xsize && (size % xsize)) { size += (xsize - (size % xsize)); }
    }
    return size;
}



u8 *myfrx(int fdnum, int type, int *ret_num, int *error);
int myfwx(int fdnum, int varn, int type);
int CMD_FileXOR_func(int cmd);



void dumpa_skip_reimported_files(void) {
    static int  reimport_skip_progress_idx = 0;
    static char reimport_skip_progress[] = "|/-\\|/-\\";
    g_reimported_files_404++;
    if(!reimport_skip_progress[reimport_skip_progress_idx]) reimport_skip_progress_idx = 0;
    fputc(reimport_skip_progress[reimport_skip_progress_idx], stderr);
    fputc('\r', stderr);
    reimport_skip_progress_idx++;
}



variable_reimport_t *dumpa_reimport2_valid(int idx) {
    variable_reimport_t *ret;
    if(idx < 0) return NULL;    // do NOT raise errors, a negative idx is used by imptype
    ret = &g_variable[idx].reimport;
    if(ret->type == BMS_TYPE_NONE) return NULL;
    return ret;
}



#define dumpa_reimport2_sequential  ((g_reimport2_offset >= 0) && (g_variable[g_reimport2_offset].reimport.type == BMS_TYPE_NONE))
int dumpa_reimport2(int idx, int value, u8 *value_str, u_int force_offset) {
    variable_reimport_t *reimport;
    u_int   oldoff;

    reimport = dumpa_reimport2_valid(idx);
    if(!reimport) {
        if(force_offset == (u_int)-1LL) {
            return -1;
        }

        for(idx = 0; idx < MAX_VARS; idx++) {
            reimport = dumpa_reimport2_valid(idx);

            // unfortunately the only way is checking every single variable
            // because there is MAX_VARS is the only limit available
            if(!reimport) continue;
            //if(!reimport) break;

            if(reimport->offset == force_offset) {
                break;
            }
        }
        if(idx >= MAX_VARS) {
            idx = -1;   // add a new variable
        }

        variable_reimport_t tmp_reimport;
        if(reimport) {
            memcpy(&tmp_reimport, reimport, sizeof(tmp_reimport));
        } else {
            memset(&tmp_reimport, 0, sizeof(tmp_reimport));
            tmp_reimport.type   = BMS_TYPE_LONG;    // default value
            tmp_reimport.offset = force_offset;
        }
        reimport = &tmp_reimport;
    }

    if(reimport->math_ops > 0) {
        int     new_value = value;
        int     math_history;
        for(math_history = reimport->math_ops - 1; math_history >= 0; math_history--) {
            new_value = math_operations(-1, new_value, reimport->math_op[math_history], get_var32(reimport->math_value[math_history]), 1);
            add_var(idx, NULL, NULL, new_value, sizeof(int));   // yes it's necessary to keep it here in every cycle!
        }
    } else {
        add_var(idx, NULL, NULL, value, sizeof(int));
    }

    if(reimport->type == BMS_TYPE_ASIZE) {
        // do nothing, it happens with -r -r -r -w
    } else {
        oldoff = myftell(reimport->fd);
        int g_filexor_cmd_bck = g_filexor.cmd;
        if(reimport->use_filexor != g_filexor.cmd) CMD_FileXOR_func(reimport->use_filexor); // set
        myfseek(reimport->fd, reimport->offset, SEEK_SET);
        if(value_str) {
            int     len = value;    // value is the length of value_str if the latter is set (save one argument)
            if(len < 0) len = reimport->size;
            if(len < 0) len = strlen(value_str);
            myfw(reimport->fd, value_str, len);
        } else {
            myfwx(reimport->fd, idx, reimport->type);
        }
        if(g_filexor_cmd_bck     != g_filexor.cmd) CMD_FileXOR_func(g_filexor_cmd_bck);     // reset
        myfseek(reimport->fd, oldoff, SEEK_SET);
    }

    if(reimport->math_ops > 0) {
        add_var(idx, NULL, NULL, value, sizeof(int));
    }

    //reimport->type = BMS_TYPE_NONE;  // better to set it to invalid? maybe it gets reused in a second Log
    return 0;
}



int dumpa_reimport_crc(int var) {
    if(var < 0) return -1;
    int idx = get_var_from_name("QUICKBMS_CRC", -1);
    if(idx >= 0) {
        dumpa_reimport2(var, get_var32(idx), NULL, -1);
    } else {
        idx = get_var_from_name("QUICKBMS_HASH", -1);
        if(idx >= 0) {
            dumpa_reimport2(var, -1 /*get_var_fullsz(idx)*/, get_var(idx), -1);
        }
    }
    return 0;
}



int g_ignore_comp_errors_reimport(int endif_cmd, int size) {
    int     cmd;
    if(endif_cmd < 0) {
        cmd = g_last_cmd;
    } else {
        cmd = endif_cmd;
    }

    int     is_else = 0;
    int     is_log  = 0;

    while(--cmd >= 0) {
        if((CMD.type == CMD_If) || (CMD.type == CMD_Elif)) {
            if(endif_cmd >= 0) {
                if(CMD.type == CMD_If) break;
                continue;
            }
            if(!is_log) continue;
            if(VARISNUM(0) && VARISNUM(2)) {
                int var1n = VAR32(0);
                int var1x = CMD.var[0];
                u8  *cond = STR(1);
                int var2n = VAR32(2);
                int var2x = CMD.var[2];

                if(
                    (strstr(cond, "<>") || strstr(cond, "!="))  // different
                 || strstr(cond, "<")   // lower
                 || strstr(cond, ">")   // higher
                 || strstr(cond, "=")   // equal (last)
                ) {
                    if(var_is_a_constant(var1x)) {
                        if(is_else) {
                            //ok
                        } else {
                            var1n = size;
                        }
                        dumpa_reimport2(var2x, var1n, NULL, -1);

                    } else if(var_is_a_constant(var2x)) {
                        if(is_else) {
                            //ok
                        } else {
                            var2n = size;
                        }
                        dumpa_reimport2(var1x, var2n, NULL, -1);

                    } else {
                        //if(is_else) { // same???
                        if((var1x != g_reimport2_zsize) && (var1x != g_reimport2_size) && (var1x != g_reimport2_xsize)) {
                            dumpa_reimport2(var1x, size, NULL, -1);
                        }
                        if((var2x != g_reimport2_zsize) && (var2x != g_reimport2_size) && (var2x != g_reimport2_xsize)) {
                            dumpa_reimport2(var2x, size, NULL, -1);
                        }
                    }
                }
            }
            break;
        } else if(CMD.type == CMD_Else) {
            is_else = 1;
        } else if(CMD.type == CMD_EndIf) {
            cmd = g_ignore_comp_errors_reimport(cmd, size);
        } else if(CMD.type == CMD_Log) {
            is_log = 1;
        }
    }
    return cmd;
}



void dumpa_sha1(int fdnum, u8 *data, int size, u8 *ret) {
    struct sha1_ctx hash_ctx;

    hs_cryptohash_sha1_init(&hash_ctx);
    if(data) {
        if(size > 0) hs_cryptohash_sha1_update(&hash_ctx, data, size);
    } else {
        // direct copy
        int     len;
        TMP_FILE_READ(myfr, fdnum, break)
            hs_cryptohash_sha1_update(&hash_ctx, tmp, t);
        }
    }
    hs_cryptohash_sha1_finalize(&hash_ctx, ret);
}



int fputss(int fdnum, u8 *data, int chr_eos, int unicode, int line, int maxsz);



int dumpa_shrink_enlarge(int fdnum, u_int oldoff, u_int offset, int old_size, int new_size) {

    if(!g_reimport_shrink_enlarge) return 0;    // just in case

#define dumpa_shrink_enlarge_copy \
            myfseek(fdnum, tmpoff,  SEEK_SET); \
            if(myfr(fdnum, tmp, t, TRUE) <= 0) break; \
            myfseek(fdnum, tmpoff + diff_size, SEEK_SET); \
            if(myfw(fdnum, tmp, t) <= 0) break;

    int     myoff,
            targetoff,
            tmpoff,
            fsize,
            diff_size;
    u8      b;

    int     t,
            len;

    int  tmpsz   = 0;
    u8   *tmp    = QUICKBMS_CACHED_IO(&tmpsz);

    diff_size = new_size - old_size;
    if(!diff_size) return 0;

    myoff = myftell(fdnum);
    fsize = myfilesize(fdnum);

    if(diff_size < 0) { // new_size is smaller
        tmpoff    = offset + old_size;
        targetoff = fsize;
        while(tmpoff != targetoff) {
            t = tmpsz;
            len = targetoff - tmpoff;
            if(len < t) t = len;
            dumpa_shrink_enlarge_copy
            tmpoff += t;
        }
        myftruncate(fdnum, fsize + diff_size);
    } else {
        //myftruncate(fdnum, fsize + diff_size);
        tmpoff    = fsize;
        targetoff = offset + old_size;
        while(tmpoff != targetoff) {
            t = tmpsz;
            len = tmpoff - targetoff;
            if(len < t) t = len;
            tmpoff -= t;
            dumpa_shrink_enlarge_copy
        }
    }

    int     ret = 0;
    if((oldoff > (offset + old_size)) || (oldoff > myoff)) {
        ret = diff_size;
    }

    if(myoff > (offset + old_size)) {
        myoff += diff_size;
    }
    myfseek(fdnum, myoff, SEEK_SET);

    int i;
    for(i = 0; i < MAX_VARS; i++) {
        if(!g_variable[i].name) break;
        if(g_variable[i].reimport.fd == fdnum) {
            if(g_variable[i].reimport.type == BMS_TYPE_ASIZE) {
                //add_var(i, NULL, NULL, get_var32(i) + diff_size, sizeof(int));
                int tmpn = 0, error = 0;
                myfrx(fdnum, BMS_TYPE_ASIZE, &tmpn, &error);
                if(!error) add_var(i, NULL, NULL, tmpn, sizeof(int));
            }
            // ready to add others
        }
    }

    return ret;
}



int dumpa_slog_write(FILE *fd, int fdmem, u8 *str, int len) {
    if(!str) str = "";
    if(len < 0) len = strlen(str);

    if(fd) {
        if(fwrite(     str, 1, len, fd) != len) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
    } else {
        if(myfw(fdmem, str,    len    ) != len) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
    }
    return 0;
}



int dumpa_slog(int fdnum, u8 *fname, u_int offset /*-1 is u_int*/, int size, int type, int id_var) {

/*
When "size" is negative (NULL-delimited strings) and it's an unicode string
we have no quick and easy way to know if the data we are going to write is
bigger than the original so the following code doesn't work correctly in
this specific situation.
*/
#define dumpa_slog_reimport2_offset \
    if(g_reimport < 0) { \
        if((len > size) && (offset != (u_int)-1LL)) { \
            tmpoff = myfilesize(fdnum); \
            dumpa_reimport2(g_reimport2_offset, tmpoff, NULL, -1); \
            myfseek(fdnum, tmpoff, SEEK_SET); \
            size = len; \
        } \
    }

    typedef struct {    // another solution is keeping the file open
        u32     name_crc;
        i32     lines;
        u_int   offset;
        i32     utf16;
    } slog_file_t;

    static int          slog_files  = 0;
    static slog_file_t  *slog_file  = NULL;
    static int  buffsz      = 0;
    static u8   *buff       = NULL;
    static int  slog_var    = 0;    // makes things faster

    FILE    *fd     = NULL;
    u32     name_crc;
    int     slog_idx,
            len,
            oldoff,
            tmpoff,
            error,
            datan,
            t,
            endian_bck,
            ret     = 0;
    u8      *data,
            *allocated_out  = NULL,
            *out,
            tmp[NUMBERSZ + 1],
            *p,
            *fname_alloc    = NULL;

    #define dumpa_slog_info(void) { \
        if(!g_quiet) { \
            printf("%c %d: %*s%s\n", g_reimport ? '>' : '<', slog_file[slog_idx].lines, (i32)MIN(70,len), out, (len <= 70) ? "" : "..."); \
        } \
    }

    if(!fname) return -1;

    if(fname) fname = dumpa_backup_fname(fname);

    oldoff = myftell(fdnum);    // avoids useless warnings

    if(!fname[0]) {
        if(fdnum < 0) {
            p = g_filenumber[0].filename;
        } else {
            CHECK_FILENUMX
            p = g_filenumber[fdnum].filename;
        }
        if(!p || !p[0]) p = "INPUT_FILE";
        spr(&fname_alloc, "%s.txt", p);
        fname = fname_alloc;
    }

    fname = clean_filename(fname, NULL);
    name_crc = mycrc(fname, -1);    // case sensitive

    if(!slog_var) {
        slog_var = add_var(0, "QUICKBMS_SLOG", "", 0, -2);
    }

    // no need to optimize because there is probably just one file
    for(slog_idx = 0; slog_idx < slog_files; slog_idx++) {
        if(slog_file[slog_idx].name_crc == name_crc) break;
    }
    if(slog_idx >= slog_files) {
        slog_file = realloc(slog_file, (slog_files + 1) * sizeof(slog_file_t));
        if(!slog_file) STD_ERR(QUICKBMS_ERROR_MEMORY);
        memset(&slog_file[slog_idx], 0, sizeof(slog_file_t));
        slog_file[slog_idx].name_crc = name_crc;
        slog_files++;
    }

    // (offset != -1) instead of (offset >= 0) allows to use almost 4Gb
    if(offset != (u_int)-1LL) {
        oldoff = myftell(fdnum);
        myfseek(fdnum, offset, SEEK_SET);
    }

    // fdmem must be negative if it's valid
    int     fdmem = MAX_FILES;
    if(is_MEMORY_FILE(fname)) {
        fdmem = get_memory_file(fname);
    }

    g_slog_id++;    // it starts from zero so it's now 1
    u8  *id         = NULL;
    if(id_var >= 0) {
        p = get_varname(id_var);
        if(!p) p = "";  // impossible, ever valid
        if(!p[0]) {
            // nothing to do, we use g_slog_id
        } else {
            id = get_var(id_var);
            if(!id) id = "";
        }
    }

    if(g_reimport) {

        if(fdnum < 0) {
            fprintf(stderr, "- MEMORY_FILEs cannot be used for strings editing, reimporting anyway\n");
        }

        if(fdmem >= 0) {
            fname = create_dir(fname, 0, 0, 0, 1);  // needed to avoid xfopen("/file.txt", "rb");
            fd = xfopen(g_force_output ? g_force_output : fname, "rb");
            if(!fd) {
                //STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                dumpa_skip_reimported_files();
                goto quit_ok;
            }
        } else {
            myfseek(fdmem, 0, SEEK_SET);
        }
        // g_force_output_pos is not used here because slog_file[slog_idx].offset already does that job

        if(!slog_file[slog_idx].offset) {
            // currently this is totally useless and unsupported, only UTF-8 works
            u8  bom[3] = "";
            if(fd) {
                t = fread(bom, 1, 3, fd);
            } else {
                t = myfr(fdmem, bom, 3, FALSE);
            }
            if(t == 3) {
                if(!memcmp(bom, "\xef\xbb\xbf", 3)) {       // UTF-8 BOM
                    slog_file[slog_idx].offset = 3;
                    slog_file[slog_idx].utf16 = 0;
                } else if(!memcmp(bom, "\xff\xfe", 2)) {    // UTF-16 BOM LE
                    slog_file[slog_idx].offset = 2;
                    slog_file[slog_idx].utf16 = -1;
                } else if(!memcmp(bom, "\xfe\xff", 2)) {    // UTF-16 BOM BE
                    slog_file[slog_idx].offset = 2;
                    slog_file[slog_idx].utf16 = 1;
                }
            }
            // no need to reset fseek
        }

        // endianess is automatically handled by incremental_fread
        if(fd) {
            if(fseek(fd, slog_file[slog_idx].offset, SEEK_SET) < 0) goto quit;
            out = incremental_fread(fd, &len, 1, NULL, 0, slog_file[slog_idx].utf16);
        } else {
            if(myfseek(fdmem, slog_file[slog_idx].offset, SEEK_SET) < 0) goto quit;
            datafile_t df;
            if(datafile_init(&df, myfdata(fdmem) + myftell(fdmem), myfilesize(fdmem) - myftell(fdmem)) < 0) {
                STD_ERR(QUICKBMS_ERROR_MEMORY);
            }
            out = incremental_fread(NULL, &len, 1, &df, 0, slog_file[slog_idx].utf16);
        }
        allocated_out = out;
        if(!out) goto quit;

        if(id_var >= 0) {
            p = strchr(out, '=');
            if(!p) goto quit_ok;
            *p++ = 0;
            if(id) {
                if(stricmp(out, id)) goto quit_ok;
            } else {
                if(myatoi(out) != g_slog_id) goto quit_ok;
            }
            out = p;
        }

        dumpa_slog_info()

        slog_file[slog_idx].lines++;
        slog_file[slog_idx].offset = fd ? ftell(fd) : myftell(fdmem);
        //FCLOSE(fd);

        len = cstring(out, out, len, NULL, NULL);

        // necessary and good for debugging so you can check it at any time
        add_var(slog_var, NULL, out, 0, len);

        variable_reimport_t *reimport;
        reimport = dumpa_reimport2_valid(g_reimport2_offset);

        int is_nul_delimited = 0;
        if(size < 0) {
            is_nul_delimited = 1;
            len++;  // this is correct since we need to consider the NUL delimiter
            if(type == BMS_TYPE_LINE) len++;    // \r + \n?

            // find the original size for filling the difference with zeroes
            tmpoff = myftell(fdnum);
            myfrx(fdnum, type, NULL, &error);
            if(error) goto quit;
            if(g_list_only || g_void_dump) goto quit_ok;
            size = myftell(fdnum) - tmpoff;
            myfseek(fdnum, tmpoff, SEEK_SET);

            if(g_reimport < 0) {
                t = myfwx((i32)-0x80000000, slog_var, type);
                if(!reimport && g_reimport_shrink_enlarge && (t != size)) {
                    oldoff += dumpa_shrink_enlarge(fdnum, oldoff, myftell(fdnum), size, t);
                    len = size = t;
                }
            }

            dumpa_slog_reimport2_offset

            // unicode is handled automatically by myfwx!
            g_encrypt_mode = !g_encrypt_mode;   // this job is already done by CMD_Encryption_func
            t = myfwx(fdnum, slog_var, type);
            g_encrypt_mode = !g_encrypt_mode;
            if(t < 0) goto quit;
            len = myftell(fdnum) - tmpoff;
        } else {
            if(type == BMS_TYPE_UNICODE) {
                out = set_utf8_to_unicode(out, len, &len);
            } else if(type == BMS_TYPE_UNICODE32) {
                out = set_utf8_to_unicode32(out, len, &len);
            }
            if(!out) goto quit;
            if(g_list_only || g_void_dump) goto quit_ok;

            if(g_reimport < 0) {
                t = myfw((i32)-0x80000000, out, len);
                if(!reimport && g_reimport_shrink_enlarge && (t != size)) {
                    oldoff += dumpa_shrink_enlarge(fdnum, oldoff, myftell(fdnum), size, t);
                    len = size = t;
                }
            }

            dumpa_slog_reimport2_offset

            g_encrypt_mode = !g_encrypt_mode;   // this job is already done by CMD_Encryption_func
            t = myfw(fdnum, out, len);
            g_encrypt_mode = !g_encrypt_mode;
            if(t < 0) goto quit;
            if(g_reimport < 0) {
                dumpa_reimport2(g_reimport2_size, len, NULL, -1);
            }
        }

        if(len > size) {
            if(dumpa_reimport2_sequential) {
                fprintf(stderr, "\n"
                    "Error: your string %d is longer than the original of %d bytes!!!\n"
                    "       It's not possible to use reimport2 with sequential data bigger than the\n"
                    "       original since the new content overwrites the next data.\n"
                    "       Please restore the backup because the archive has been already modified.\n"
                    "\n", (i32)slog_file[slog_idx].lines /*line is correct in this way too*/, (i32)(len - size));
                myexit(QUICKBMS_ERROR_FILE_WRITE);
            }
            //fprintf(stderr, "- your string %d is longer than the original of %d bytes! Reimporting done\n", (i32)slog_file[slog_idx].lines /*line is correct in this way too*/, (i32)(len - size));
        }

        // zeroing the space
        if(len < size) {
            // NUL delimited data isn't affected by the filler
            if(is_nul_delimited || (g_log_filler_char == 0)) {
                if(zero_fdnum(fdnum, 0, size - len) < 0) goto quit;
            } else {    // includes < 0 too
                char slog_filler_char[sizeof(int) + 1];
                u_int   z = (g_log_filler_char < 0) ? ' ' : g_log_filler_char;
                for(t = 0; z; t++) {    // sort of utf8 support
                    slog_filler_char[t] = z;
                    z >>= 8;
                }
                slog_filler_char[t] = 0;
                if(!t) t = 1;   // just in case...
                u_int   limit = myftell(fdnum) + (size - len);
                // is_nul_delimited is not used, it's here only for possible future usages
                if(is_nul_delimited) {
                         if(type == BMS_TYPE_UNICODE)   limit -= (t * sizeof(u16));
                    else if(type == BMS_TYPE_UNICODE32) limit -= (t * sizeof(u32));
                    else                                limit -= t;
                }
                int unicode_option = 0;
                if(type == BMS_TYPE_UNICODE)   unicode_option = 1;
                if(type == BMS_TYPE_UNICODE32) unicode_option = -1;
                while(myftell(fdnum) < limit) {
                    if(fputss(fdnum, slog_filler_char, 0, unicode_option, 0, t) < 0) goto quit;
                }
                if(is_nul_delimited) {
                    while(myftell(fdnum) < limit) { // lazy way
                        if(fputss(fdnum, "",           0, unicode_option, 0, t) < 0) goto quit;
                    }
                }
            }
        }

    } else {

        if(size < 0) {
            data = myfrx(fdnum, type, &datan, &error);
            if(error) goto quit;
            if(data) size = strlen(data);
        } else {
            MAX_ALLOC_CHECK(size);
            myalloc(&buff, size + 1, &buffsz);
            size = myfr(fdnum, buff, size, TRUE);
            if(size < 0) goto quit;
            buff[size] = 0;

            if(type == BMS_TYPE_UNICODE) {
                data = set_unicode_to_utf8(buff, size, &size);
            } else if(type == BMS_TYPE_UNICODE32) {
                data = set_unicode32_to_utf8(buff, size, &size);
            } else {
                data = buff;
            }
            if(!data) goto quit;
        }

        if(data) {
            while((size > 0) && !data[size - 1]) size--;    // remove possible useless 0x00 at the end, they will be recreated by the reimporter
            if(type == BMS_TYPE_BINARY) {
                out = string_to_C(data, size, &len, 1);
            } else {
                out = string_to_C(data, size, &len, 0);
            }
        } else {
            len = sprintf(tmp, "%"PRId"", datan);
            out = tmp;
        }
        if(!out) goto quit;

        if(!g_list_only && !g_void_dump) {
            if(fdmem >= 0) {
                fname = create_dir(fname, 1, 0, 0, 1);
            }
            if(!slog_file[slog_idx].lines) {    // first line, create the file
                if(fdmem >= 0) {
                    if(check_overwrite(fname, 0) < 0) {
                        fprintf(stderr, "- the file will not be exported (auto renaming not allowed in Slog)\n");
                        goto quit;
                    }
                    fd = xfopen(fname, "wb");
                    if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                } else {
                    dumpa_memory_file(&g_memory_file[-fdmem], NULL, 0, NULL);
                }
                // the UTF-8 BOM is not necessary
                if(g_force_utf16 < 0) {
                    if(fd) t = fwrite(     "\xff\xfe", 1, 2, fd);
                    else   t = myfw(fdmem, "\xff\xfe",    2);
                    if(t != 2) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);   // UTF-16 LE
                } else if(g_force_utf16 > 0) {
                    if(fd) t = fwrite(     "\xfe\xff", 1, 2, fd);
                    else   t = myfw(fdmem, "\xfe\xff",    2);
                    if(t != 2) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);   // UTF-16 BE
                } else {
                    /*
                    if(fd) t = fwrite(     "\xef\xbb\xbf", 1, 3, fd);
                    else   t = myfw(fdmem, "\xef\xbb\xbf",    3);
                    if(t != 3) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);   // UTF-8 BOM
                    */
                }
            } else {
                if(fdmem >= 0) {
                    fd = xfopen(fname, "ab");
                    if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                } else {
                    // reset if not open yet
                    if(!g_memory_file[-fdmem].data) {
                        dumpa_memory_file(&g_memory_file[-fdmem], NULL, 0, NULL);
                    }
                }
            }

            u8  *o16;   // static
            int o16_len;
            endian_bck = g_endian;
            if(g_force_utf16 < 0) {         // UTF-16 LE
                g_endian = MYLITTLE_ENDIAN;

                if(id_var >= 0) {
                    p = id ? id : myitoa(g_slog_id);
                    o16 = set_utf8_to_unicode(p, -1,    &o16_len);
                    dumpa_slog_write(fd, fdmem, o16,    o16_len);
                    dumpa_slog_write(fd, fdmem, "=\0",  2);
                }
                o16 = set_utf8_to_unicode(out, len,     &o16_len);
                dumpa_slog_write(fd, fdmem, o16,        o16_len);
                dumpa_slog_write(fd, fdmem, "\r\0\n\0", 4);

            } else if(g_force_utf16 > 0) {  // UTF-16 BE
                g_endian = MYBIG_ENDIAN;

                if(id_var >= 0) {
                    p = id ? id : myitoa(g_slog_id);
                    o16 = set_utf8_to_unicode(p, -1,    &o16_len);
                    dumpa_slog_write(fd, fdmem, o16,    o16_len);
                    dumpa_slog_write(fd, fdmem, "\0=",  2);
                }
                o16 = set_utf8_to_unicode(out, len,     &o16_len);
                dumpa_slog_write(fd, fdmem, o16,        o16_len);
                dumpa_slog_write(fd, fdmem, "\0\r\0\n", 4);

            } else {

                if(id_var >= 0) {
                    p = id ? id : myitoa(g_slog_id);
                    dumpa_slog_write(fd, fdmem, p,      -1);
                    dumpa_slog_write(fd, fdmem, "=",    1);
                }
                dumpa_slog_write(fd, fdmem, out,        len);
                dumpa_slog_write(fd, fdmem, "\r\n",     2);

            }
            g_endian = endian_bck;

            slog_file[slog_idx].offset = fd ? ftell(fd) : myftell(fdmem);
            //FCLOSE(fd);
        }
        slog_file[slog_idx].lines++;

        add_var(slog_var, NULL, out, 0, len);
        dumpa_slog_info()
    }

quit_ok:
    ret = 0;
quit:
    if(offset != (u_int)-1LL) {
        myfseek(fdnum, oldoff, SEEK_SET);
    }
    FREE(allocated_out)
    FREE(fname_alloc)
    FCLOSE(fd);
    return ret;
}



// currently the following is used only in append mode for performance reasons
typedef struct extracted_file_t {
    int     id;
    u8      *name;
    u8      *new_name;
    u64     offset;
    u64     zsize;
    u64     size;
    u8      compression;    // only on:off
    u8      encryption;     // only on:off
    UT_hash_handle hh;
    struct extracted_file_t *next;
    struct extracted_file_t *prev;
} extracted_file_t;
static extracted_file_t *g_extracted_file   = NULL;



int dumpa_reimport_compression(int type) {
    int     i;

    static int init = 0;
    if(!init) {
        init = 1;
        int     j, num;
        u8      *name;
        for(i = 0; (name = quickbms_comp_list[i].name); i++) {
            num = i;
            switch(num) {
                case COMP_NONE:                 num = COMP_COPY;                break;
                case COMP_COPY:                 num = COMP_COPY;                break;
                case COMP_COPY2:                num = COMP_COPY2;               break;
                case COMP_NOP:                  num = COMP_COPY;                break;
                case COMP_ZLIB_NOERROR:         num = COMP_ZLIB_COMPRESS;       break;
                case COMP_UNZIP_DYNAMIC:        num = COMP_ZLIB_COMPRESS;       break;  // ???
                case COMP_DEFLATE_NOERROR:      num = COMP_DEFLATE_COMPRESS;    break;
                case COMP_LZMA_DYNAMIC:         num = COMP_LZMA_COMPRESS;       break;  // ???
                case COMP_LZMA2_DYNAMIC:        num = COMP_LZMA2_COMPRESS;      break;  // ???
                case COMP_STALKER_LZA:          num = COMP_LZHUFXR_COMPRESS;    break;
                case COMP_PUYO_LZ01:            num = COMP_LZSS0_COMPRESS;      break;
                case COMP_RNCb:                                                         // ???
                case COMP_RNCb_RAW:                                                     // ???
                //case COMP_RNCc:                                                       // ???
                case COMP_RNCc_RAW:                                                     // ???
                case COMP_RNC_RAW:                                                      // ???
                case COMP_SCUMMVM6:
                case COMP_SCUMMVM7:             num = COMP_RNC_COMPRESS;        break;
                case COMP_DEFLATEX:             num = COMP_DEFLATE_COMPRESS;    break;
                case COMP_ZLIBX:                num = COMP_ZLIB_COMPRESS;       break;
                case COMP_SYNLZ1partial:        num = COMP_SYNLZ1_COMPRESS;     break;
                case COMP_SYNLZ1b:              num = COMP_SYNLZ1_COMPRESS;     break;
                case COMP_DARKSECTOR_NOCHUNKS:  num = COMP_LZFX_COMPRESS;       break;
                case COMP_FASTLZAH:             num = COMP_LZFX_COMPRESS;       break;
                case COMP_EVOLUTION:            num = COMP_LZFX_COMPRESS;       break;
                case COMP_UNKNOWN6:             num = COMP_LZFX_COMPRESS;       break;
                case COMP_ZOPFLI_ZLIB_COMPRESS:                                 break;  // it's a compression-only algorithm, doing COMP_COMPRESS -> COMP is never used
                case COMP_ZOPFLI_DEFLATE_COMPRESS:                              break;  // it's a compression-only algorithm
                case COMP_KZIP_ZLIB_COMPRESS:                                   break;  // it's a compression-only algorithm
                case COMP_KZIP_DEFLATE_COMPRESS:                                break;  // it's a compression-only algorithm
                case COMP_EXECUTE:                                              break;  // remains the same, I can do nothing for it
                case COMP_EXPLODE:              num = COMP_PKWARE_DCL_COMPRESS; break;
                case COMP_LZ77WII:              num = COMP_DS_LZX_COMPRESS;     break;
                case COMP_OodleNetwork1UDP_Decode: num = COMP_OodleNetwork1UDP_Encode; break;
                case COMP_LBALZSS1:             num = COMP_LBALZSS_COMPRESS;    break;
                case COMP_LBALZSS2:             num = COMP_LBALZSS_COMPRESS;    break;
                case COMP_LBALZSS1X:            num = COMP_LBALZSS_COMPRESS;    break;
                case COMP_LBALZSS2X:            num = COMP_LBALZSS_COMPRESS;    break;
                default:                        num = -1;                       break;
            }
            if(num >= 0) {
                quickbms_comp_list[i].reimport = num;
            } else {
                u8  *tmp = alloca(strlen(name) + 32 + 1);
                if(!tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);
                if(!strcmpx(name, "_COMPRESS")) {
                    sprintf(tmp, "%.*s", (i32)((u8*)strrchr(name, '_') - name), name);
                } else {
                    sprintf(tmp, "%s%s", name, "_COMPRESS");
                }
                for(j = 0; (name = quickbms_comp_list[j].name); j++) {
                    if(!stricmp(name, tmp)) {
                        quickbms_comp_list[i].reimport = j;
                        break;
                    }
                }
            }
        }
    }

    if((type < 0) || (type >= COMP_ERROR)) return -1;
    if(type >= COMP_NONE_COMPRESS) return type; // nothing to do, necessary for zlib/deflate super compression with zopfli and uberflate
    return quickbms_comp_list[type].reimport;
}



int dumpa_reimport_crchash   (u8 *data, int datalen) {
    int     ret;
    g_encrypt_mode = !g_encrypt_mode;   // this job is already done by CMD_Encryption_func
    CMD_Encryption_func(-1, 1);
    ret = perform_crchash   (data, datalen);
    g_encrypt_mode = !g_encrypt_mode;
    return ret;
}



int dumpa_reimport_encryption(u8 *data, int datalen) {
    int     ret;
    g_encrypt_mode = !g_encrypt_mode;   // this job is already done by CMD_Encryption_func
    CMD_Encryption_func(-1, 1);
    ret = perform_encryption(data, datalen);
    g_encrypt_mode = !g_encrypt_mode;
    return ret;
}



static extracted_file_t *append_mode_extracted_file2(u8 *FNAME, FILE *fd, int zsize, int size) {
    extracted_file_t *ef    = NULL;
    if(g_script_uses_append || (g_extracted_file_tree_view_mode >= 0)) {
        HASH_FIND_STR(g_extracted_file, FNAME, ef);
        if(!ef) {
            ef = real_calloc(1, sizeof(extracted_file_t));
            if(!ef) STD_ERR(QUICKBMS_ERROR_MEMORY);
            ef->id      = g_reimport ? g_reimported_files : g_extracted_files;
            ef->name    = real_malloc(strlen(FNAME) + 1);
            if(fd) ef->offset  = ftell(fd); // first only
            strcpy(ef->name, FNAME);
            HASH_ADD_STR(g_extracted_file, name, ef);
            CDL_APPEND(g_extracted_file, ef);
        }
        ef->zsize   += zsize;
        ef->size    += size;
    }
    return ef;
}



int dumpa(int fdnum, u8 *fname, u8 *varname, u_int offset, int size, int zsize, int xsize) {

#define append_mode_extracted_file_new_name(FNAME, NEW_FNAME) \
    if(g_script_uses_append || (g_extracted_file_tree_view_mode >= 0)) { \
        HASH_FIND_STR(g_extracted_file, FNAME, ef); \
        if(ef) { \
            ef->new_name = real_realloc(ef->new_name, strlen(NEW_FNAME) + 1); \
            strcpy(ef->new_name, NEW_FNAME); \
        } \
    }

// check if we are in append mode and the file has been already extracted
#define dumpa_name_overwrite_check \
    if(filetmp) { \
    } else { \
        xname = fname; \
        for(;;) { \
            if(g_append_mode != APPEND_MODE_NONE) { \
                HASH_FIND_STR(g_extracted_file, fname, ef); \
                if(ef) { \
                    fname = ef->name; \
                    if(ef->new_name) fname = ef->new_name; \
                    break; \
                } \
            } \
            if(mycrc(fname, -1) == last_name_crc) break; \
            t = check_overwrite(fname, 0); \
            if(!t) break; \
            if((t == -2) || g_force_rename) { \
                fname = rename_auto(++rename_cnt, xname); \
                append_mode_extracted_file_new_name(xname, fname) \
            } else { \
                goto quit; \
            } \
        } \
        last_name_crc = mycrc(fname, -1); \
    }

#define append_mode_extracted_file(FNAME, FD, X) \
    ef = append_mode_extracted_file2(FNAME, FD, zsize, size); \
    if(ef) { \
        ef->compression = quickbms_compression; /* mah, chunks are bad */ \
        ef->encryption  = quickbms_encryption;  /* mah, chunks are bad */ \
        X \
    }

    //static  u8  tmpname[PATHSZ + 32 + 1] = "";  // 32 includes the dynamic extension
    static u8   *tmp_fname = NULL;
    static  int insize  = 0,    // ONLY as total allocated input size
                outsize = 0;    // ONLY as total allocated output size
    static  u8  *in     = NULL,
                *out    = NULL;

    static u8   extraction_hash [SHA1_DIGEST_SIZE];
           u8   extraction_hash2[SHA1_DIGEST_SIZE];

    static int  do_memfile_reimport = 0;    // -1=no, 0=?, 1=ok

    socket_file_t   *sockfile   = NULL;
    process_file_t  *procfile   = NULL;
    audio_file_t    *audiofile  = NULL;
    video_file_t    *videofile  = NULL;
    winmsg_file_t   *winmsgfile = NULL;

    memory_file_t   *memfile    = NULL;
    extracted_file_t *ef    = NULL;
    FILE    *fd             = NULL;
    u32     last_name_crc   = mycrc(NULL, 0);
    u_int   oldoff          = 0;
    int     len             = 0,
            filetmp         = 0,
            nametmp         = 0,
            direct_copy     = 0,
            quickbms_compression = 0,
            quickbms_encryption  = 0,
            //old_xsize       = 0,
            old_zsize       = 0,
            old_size        = 0,
            backup_xsize    = 0,
            backup_zsize    = 0,
            backup_size     = 0,
            old_compression_type = 0,
            new_compression_type = 0,
            non_files       = 0,
            rename_cnt      = 0,
            ret_value       = 0,
            is_folder       = 0,
            wildcard_pos    = -1,
            idx             = -1,
            t,
            memory_workaround=0;
    u8      tmpbuff[64]     = "",
            ans[16],
            *ext            = NULL,
            *tmp_ext        = NULL,
            *xname,
            append_mode_chr_vis = ' ';

    u8  *original_fname = fname;    // used for log "" in append mode
    if(!original_fname) original_fname = "";
    if(fname) fname = dumpa_backup_fname(fname);

    if(!fname && (size < 0) && (zsize < 0)) {   // all must be invalid
        if(in == out) out = NULL;
        FREE(in)
        FREE(out)
        insize  = 0;
        outsize = 0;
        { ret_value = -1; goto quit; }
    }

    // the following is a set of filename cleaning instructions to avoid that files or data with special names are not saved
    if(fname) {
        sockfile = socket_open(fname);
        if(sockfile)    { non_files = 1; } else {
        procfile = process_open(fname);
        if(procfile)    { non_files = 1; } else {
        audiofile = audio_open(fname);
        if(audiofile)   { non_files = 1; } else {
        videofile = video_open(fname);
        if(videofile)   { non_files = 1; } else {
        winmsgfile = winmsg_open(fname);
        if(winmsgfile)  { non_files = 1; } else {
            fname = clean_filename(fname, &wildcard_pos);
        }}}}}
    }

    if(!varname) varname = "";

    if(!fname || !fname[0]) {
        if(g_input_total_files <= 1) {    // extension added by sign_ext
            spr(&tmp_fname, "%s.dat", myitoa_decimal_names(g_extracted_files));
        } else {
            // the following works but would be good to have something generic rather than based on TEMPORARY_FILE
            xname = g_filenumber[0].basename;
            //if(!stricmp(xname, TEMPORARY_FILE) && g_filenumber[0].prev_basename) xname = g_filenumber[0].prev_basename; // old references are saved
            spr(&tmp_fname, "%s%c%s.dat", xname, PATHSLASH, myitoa_decimal_names(g_extracted_files));
        }
        fname = tmp_fname;
        nametmp = 1;
    } else if(wildcard_pos >= 0) {
        fname[wildcard_pos] = 0;
        if((wildcard_pos > 0) && strchr(PATH_DELIMITERS, fname[wildcard_pos - 1])) {
            spr(&tmp_fname, "%s%s.dat", fname, myitoa_decimal_names(g_extracted_files));
        } else {
            spr(&tmp_fname, "%s.dat", fname);
        }

        // doesn't work, it's how clean_filename works
        // this code remains "as-is" in case of further development
        ext = strrchr(fname + wildcard_pos + 1, '.');
        if(ext) tmp_ext = ext + 1;

        fname = tmp_fname;
        nametmp = 1;
    }

    // check if it's a folder or just a nameless file or bugged script
    if(fname[0] && strchr(PATH_DELIMITERS, fname[strlen(fname) - 1])) {
        if(!size && !zsize) {
            is_folder = 1;
        } else {
            spr(&tmp_fname, "%s%s.dat", fname, myitoa_decimal_names(g_extracted_files));
            fname = tmp_fname;
            nametmp = 1;
        }
    }

    switch(g_append_mode) {
        case APPEND_MODE_APPEND:    append_mode_chr_vis = '+'; break;
        case APPEND_MODE_OVERWRITE: append_mode_chr_vis = '^'; break;
        case APPEND_MODE_BEFORE:    append_mode_chr_vis = '-'; break;
        default:                    append_mode_chr_vis = ' '; break;
    }

    // handling of the output filename
    if(non_files) {
        // output non files like sockets, processes and so on
        // mem_file will be added later to skip the name check
        // do nothing

    } else if(is_MEMORY_FILE(varname) && !stricmp(varname, fname)) {
        memfile = &g_memory_file[-get_memory_file(fname)];    // yes, remember that it must be negative of negative
        if(g_verbose > 0) printf("- create a memory file from offset %"PRIx" of %s bytes\n", offset, myitoa_decimal_notation(size));
        memfile->coverage = 0;  // reset it

    } else if(!stricmp(varname, TEMPORARY_FILE) && !stricmp(varname, fname)) {
        g_temporary_file_used = 1;    // global for final unlink
        filetmp = 1;
        if(g_verbose > 0) printf("- create a temporary file from offset %"PRIx" of %s bytes\n", offset, myitoa_decimal_notation(size));

    } else {
        pathslash_fix(fname, 1);    // necessary for quick_simple_tmpname_scanner. I'm not aware of any compatibility problem with older versions at the moment
        if(nametmp) {
            // handle them later
        } else {
            if(check_wildcards(fname, g_filter_files) < 0) goto quit;
        }
        if(!g_reimport) {
            if(!g_quiet) printf("%c %"PRIx" %-10s %s\n", append_mode_chr_vis, offset, myitoa_decimal_notation(size), fname);
        }
        if(g_listfd) {
              fprintf(g_listfd, "%c %"PRIx" %-10s %s\n", append_mode_chr_vis, offset, myitoa_decimal_notation(size), fname);
            fflush(g_listfd);
        }
    }

    if(g_enable_hexhtml) hexhtml_name = fname;

    // output non files like sockets, processes and so on
    if(memfile) {
        non_files = 1;

        // this is just a work-around
        if(g_reimport && (g_append_mode == APPEND_MODE_APPEND) && (g_memfile_reimport_name >= 0)) {
            if(do_memfile_reimport >= 0) {
                fname = get_var(g_memfile_reimport_name);
                if(!g_quiet) printf("- REIMPORT MEMORY_FILE WORK-AROUND: \"%s\"\n", fname);
                ext = strrchr(fname, '.');
                if((wildcard_pos >= 0) || (ext && !ext[1]) || !fname[0]) {
                    fprintf(stderr, "\n"
                        "- reimporting of files with wildcard or guessed names is not possible\n");
                        do_memfile_reimport = -1;
                        ret_value = -1; goto quit;
                }
                if(!do_memfile_reimport) {
                    fprintf(stderr, "\n"
                        "- Do you want to use the experimental reimporting of chunked MEMORY_FILE (y/N)?\n"
                        "  ");
                    if(get_yesno(NULL) != 'y') {
                        myexit(QUICKBMS_ERROR_USER);    // exit now

                        do_memfile_reimport = -1;
                        ret_value = -1; goto quit;
                    }
                    do_memfile_reimport = 1;
                }
                ret_value = dumpa(fdnum, fname, get_varname(g_memfile_reimport_name), offset, size, zsize, xsize);
                goto quit;
            }
        }
    }

    // list, folders, reimport and dump
    if(g_list_only && !memfile && !filetmp) { // only memfile and not non_files
        // do nothing
            // for -0 and -l
            if(g_extracted_file_tree_view_mode >= 0) {
                append_mode_extracted_file(fname, fd, {})
            }

    } else if(is_folder) {
        // do nothing

    } else if(g_reimport && !non_files) {
        if(g_extraction_hash) {
            // currently the code doesn't check the size first, it's more easy to just
            // calculate the hash even if the size is different... maybe in future
            g_extraction_hash = -1;
            g_reimport = !g_reimport;
            if(dumpa(fdnum, original_fname, varname, offset, size, zsize, xsize) < 0) { ret_value = -1; goto quit; }
            g_reimport = !g_reimport;
            g_extraction_hash = 1;
        }
        #define dumpa_show_reimport_info(X) printf("%c %"PRIx" %-10s %s\n", X, offset, myitoa_decimal_notation(size), fname)
        if(nametmp) {
            quick_simple_tmpname_scanner(fname, PATHSZ);
            if(check_wildcards(fname, g_filter_files) < 0) goto quit;
        }
        int skip_ask_force_reimport_1   = 0;    // lame
        if(g_reimport < 0) skip_ask_force_reimport_1 = 1;

        if(g_ignore_comp_errors && (g_reimport < 0)) {
            dumpa_state(&quickbms_compression, &quickbms_encryption, zsize, size, xsize);
            if(quickbms_compression) {
                size  = zsize;
                zsize = 0;
                if(xsize > 0) xsize = (size + 15) & ~15; // wrong but no other ways
            }
        }

        backup_xsize = xsize;
        backup_zsize = zsize;
        backup_size  = size;
redo_import:
        fname = create_dir(fname, 0, 0, 0, 1);  // needed to avoid xfopen("/file.txt", "rb");
        if(g_reimport_zero) fd = (void *)-0x80000000;    // bypasses "if"
        else                fd = xfopen(g_force_output ? g_force_output : fname, "rb");
        if(fd) {
            if(g_reimport_zero) {
                fd = NULL;
            } else {
                if(g_force_output) fseek(fd, g_force_output_pos, SEEK_SET);
                append_mode_extracted_file(fname, fd,
                    {
                        fseek(fd, ef->offset, SEEK_SET);
                    }
                )
            }

            if(fdnum < 0) {
                if(do_memfile_reimport > 0) goto quit;
                if(!g_continue_anyway) {
                    fprintf(stderr, "\n"
                        "Error: script invalid for reimporting, it uses MEMORY_FILEs\n"
                        "       You can use the -. option or quickbmsver \"-.\" if this script is meant\n"
                        "       to act as a header/data builder (for example if it adds a RIFF header\n"
                        "       to raw PCM data embedded in the file, and so on), the MEMORY_FILE data\n"
                        "       will be skipped automatically allowing to reimport the real file data\n"
                    );
                    myexit(QUICKBMS_ERROR_BMS);
                }
                oldoff = offset + (dumpa_state_check_compression ? zsize : size);
                fseek(fd, size, SEEK_CUR);
                if(g_script_uses_append) {
                    if(ef) ef->offset = ftell(fd);
                }
                goto skip_import;
            }
            oldoff = myftell(fdnum);
            myfseek(fdnum, offset, SEEK_SET);
            dumpa_state(&quickbms_compression, &quickbms_encryption, backup_zsize, backup_size, backup_xsize);

            //old_xsize = backup_xsize;
            old_zsize = backup_zsize;
            old_size  = backup_size;

            // never use size/zsize, only backup_size/zsize are correct

            if(g_reimport_zero) {
                size = 0;
                if(quickbms_compression) {
                } else {
                    old_zsize = old_size; 
                }
                if(zero_fdnum(fdnum, 0, old_zsize) < 0) goto quit;
                goto skip_import;
            }

            // experimental
            // nothing to do here, just ignore any error/warning
            // remember that often it's used "log NAME 0 0" to initialize the file so it's not possible
            // to use "if(g_append_mode != APPEND_MODE_NONE)"
            // unfortunately there are no alternatives at the moment
            if(g_script_uses_append && !offset && !backup_size) {   // log NAME 0 0
                // do nothing
            } else if(g_append_mode != APPEND_MODE_NONE) {  // we are in append mode!
                // do nothing
            } else if(g_force_output) {
                // do nothing, you can't change the size
            } else {
                t = ftell(fd);  // just in case, for the future...
                fseek(fd, 0, SEEK_END);
                size = ftell(fd);
                fseek(fd, t, SEEK_SET);
            }

            // recap: it's not perfect because encryption and crchash were together before
            // extraction: decryption + crchash     + decompression
            // reimport:   crchash    + compression + encryption

            zsize = size;
            myalloc(&out, size,  &outsize); // will be allocated by perform_compression
            if(quickbms_compression) {

                if(zsize > old_size) {
                    if(!skip_ask_force_reimport_1) {
                        ask_force_reimport(1,
                            old_size = zsize;
                        )
                        skip_ask_force_reimport_1 = 1;
                    }
                }

                myalloc(&in,  zsize, &insize);
                zsize = fread(in, 1, zsize, fd);
                if(g_extraction_hash) {
                    dumpa_sha1(0, in, zsize, extraction_hash2);
                    if(!memcmp(extraction_hash, extraction_hash2, SHA1_DIGEST_SIZE)) goto skip_import;
                }

                zsize = dumpa_reimport_crchash(in, zsize);  // imptype crc

                old_compression_type = g_compression_type;
                g_compression_type = dumpa_reimport_compression(g_compression_type);
                if(g_compression_type < 0) {
                    fprintf(stderr, "\n"
                        "Error: unsupported compression %d in reimport mode\n"
                        "       you can try the -e option with reimport2 for disabling compression\n"
                        "       (experimental, it changes the field that triggered the CLog command)\n",
                        (i32)old_compression_type);
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_COMPRESSION);
                }
                size = perform_compression(in, zsize, &out, size, &outsize, offset);
                new_compression_type = g_compression_type;
                g_compression_type = old_compression_type;
                if(size < 0) {
                    fprintf(stderr, "\n"
                        "Error: there is an error with the recompression\n"
                        "       the returned output size is negative (%"PRId")\n", size);
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_COMPRESSION);
                }
            } else {

                old_zsize = old_size;   // avoid boring "if" during the check of the size
                size = fread(out, 1, size, fd);
                if(g_extraction_hash) {
                    dumpa_sha1(0, out, size, extraction_hash2);
                    if(!memcmp(extraction_hash, extraction_hash2, SHA1_DIGEST_SIZE)) goto skip_import;
                }

                size = dumpa_reimport_crchash(out, size);   // imptype crc
            }

            if(g_script_uses_append) {
                if(ef) ef->offset = ftell(fd);
            }
            if(g_force_output) g_force_output_pos = ftell(fd);
            FCLOSE(fd);

            // mainly for block ciphers, but also for cleaning the data
            // size and old_zsize are correct, check the next comment
            int fd_size = size;
            if(size < old_zsize) {
                myalloc(&out, old_zsize,  &outsize);
                memset(out + size, 0, old_zsize - size);
                size = old_zsize;
            }
            size = dumpa_reimport_encryption(out, size);
            if(size == -1) {
                fprintf(stderr, "\nError: the encryption failed during reimport\n");
                if(g_continue_anyway) { ret_value = -1; goto quit; }
                myexit(QUICKBMS_ERROR_ENCRYPTION);
            }

            if(g_reimport < 0) {

                variable_reimport_t *reimport;
                reimport = dumpa_reimport2_valid(g_reimport2_offset);

                // reimport3 mode activated when offset is not available
                // not supporting the size of perform_encryption
                if(!reimport && g_reimport_shrink_enlarge && (fd_size != old_zsize)) {
                    oldoff += dumpa_shrink_enlarge(fdnum, oldoff, offset, old_zsize, fd_size);
                    size = old_zsize = fd_size;   // bypass next checks
                }

                if(size > old_zsize) {
                    if(dumpa_reimport2_sequential) {
                        dumpa_show_reimport_info('!');  // display the name of the file with the error
                        fprintf(stderr, "\n"
                            "Error: your file is bigger than the original of %d bytes, skipping file!\n"
                            "       It's not possible to use reimport2 with sequential data bigger than the\n"
                            "       original since the new content overwrites the next data.\n"
                            "\n", (i32)(size - old_zsize));
                        goto skip_import;
                        //if(g_continue_anyway) { ret_value = -1; goto quit; }
                        //myexit(QUICKBMS_ERROR_BMS);
                    }
                    if(fdnum_is_valid && g_filenumber[fdnum].invalid_reimport2) {
                        dumpa_show_reimport_info('!');  // display the name of the file with the error
                        fprintf(stderr, "\nError: archive not supported by reimport2 using bigger files, skip file!\n");
                        goto skip_import;
                        //if(g_continue_anyway) { ret_value = -1; goto quit; }
                        //myexit(QUICKBMS_ERROR_BMS);
                    }

                    // useful for better compression of the new archive? not really important at the moment
                    if(zero_fdnum(fdnum, (g_log_filler_char < 0) ? 0 : g_log_filler_char, old_zsize) < 0) goto quit;

                    myfseek(fdnum, 0, SEEK_END);    // append the file at the end of fdnum
                    offset = myftell(fdnum);

                    // necessary for some files that use alignment
                    if(reimport && (reimport->math_ops > 0)) {
                        u_int   tmp_offset = offset;
                        for(t = 0;; t++) {
                            offset = tmp_offset + t;
                            int     math_history;
                            for(math_history = reimport->math_ops - 1; math_history >= 0; math_history--) {
                                offset = math_operations(-1, offset, reimport->math_op[math_history],    get_var32(reimport->math_value[math_history]), 1); // for example: / 0x800
                            }
                            for(math_history = 0; math_history < reimport->math_ops; math_history++) {
                                offset = math_operations(-1, offset, reimport->math_opbck[math_history], get_var32(reimport->math_value[math_history]), 1); // for example: * 0x800
                            }
                            if(offset >= tmp_offset) break;
                        }
                        if(zero_fdnum(fdnum, 0, offset - tmp_offset) < 0) goto quit; // not necessary
                        myfseek(fdnum, offset, SEEK_SET);
                        offset = myftell(fdnum);
                    }

                    // if the file fits the available space we don't need to rewrite the offset field!
                    dumpa_reimport2(g_reimport2_offset, offset, NULL, -1);

                    old_zsize = size;   // bypass next check
                }
            }

            // yes, size and old_zsize because it's the opposite of the extraction
            if(size > old_zsize) {
                // first try zopfli which is a bit faster and grants better results in some situations
                if((new_compression_type == COMP_ZLIB_COMPRESS) || (new_compression_type == COMP_DEFLATE_COMPRESS)) {
                    if(new_compression_type == COMP_ZLIB_COMPRESS)          g_compression_type = COMP_ZOPFLI_ZLIB_COMPRESS;
                    else                                                    g_compression_type = COMP_ZOPFLI_DEFLATE_COMPRESS;
                    fprintf(stderr, "- compressed size too big, I try using the zopfli method (may be slow)\n"); // non-extreme version of zopfli
                    myfseek(fdnum, oldoff, SEEK_SET);
                    xsize = backup_xsize;
                    zsize = backup_zsize;
                    size  = backup_size;
                    goto redo_import;
                }
                // the last chance is uberflate
                if((new_compression_type == COMP_ZOPFLI_ZLIB_COMPRESS) || (new_compression_type == COMP_ZOPFLI_DEFLATE_COMPRESS)) {
                    if(new_compression_type == COMP_ZOPFLI_ZLIB_COMPRESS)   g_compression_type = COMP_KZIP_ZLIB_COMPRESS;
                    else                                                    g_compression_type = COMP_KZIP_DEFLATE_COMPRESS;
                    fprintf(stderr, "- compressed size too big, I try using the uberflate/kzip method (may be very slow!)\n");
                    myfseek(fdnum, oldoff, SEEK_SET);
                    xsize = backup_xsize;
                    zsize = backup_zsize;
                    size  = backup_size;
                    goto redo_import;
                }

                ask_force_reimport(0,
                    old_zsize = size;
                )
            }
            // separated to allow the "force" writing
            if(size <= old_zsize) {
                if(g_list_only || g_void_dump) {
                    // do nothing
                    len = size;
                } else {
                    len = myfw(fdnum, out, size);
                }
                if(len != size) {
                    fprintf(stderr, "\n"
                        "Error: impossible to write 0x%"PRIx" bytes (total 0x%"PRIx")\n"
                        "       Check your disk space or the script is wrong\n",
                        (len < 0) ? size : (size - len), size);
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_FILE_WRITE);
                }
                if(!g_quiet) dumpa_show_reimport_info('<');
                if(g_append_mode == APPEND_MODE_NONE) g_reimported_files++;
                g_reimported_logs++;

                /* not needed at the moment, maybe in future but keep in mind the notes in quickbms.txt!
                myfseek(fdnum, reimport_zsize_offset, SEEK_SET);
                fputxx(fdnum, size, 4);     // zsize->size must be swapped!
                myfseek(fdnum, reimport_size_offset, SEEK_SET);
                fputxx(fdnum, zsize, 4);    // zsize->size must be swapped!
                */
            }

            if(g_reimport < 0) {
                // yeah, zsize and size are now inverted
                dumpa_reimport2(g_reimport2_zsize, size,  NULL, -1);
                dumpa_reimport2(g_reimport2_size,  zsize, NULL, -1);

                dumpa_reimport2(g_reimport2_xsize, zsize, NULL, -1);    // ???

                if(g_ignore_comp_errors) {
                    g_ignore_comp_errors_reimport(-1, size);
                }
            }

            g_reimported_files_skip--;  // little trick to avoid using an "ok" variable
skip_import:
            FCLOSE(fd); // just in case it isn't closed (g_reimport_zero automatically sets it to NULL, so it's ok)
            g_reimported_files_skip++;
            myfseek(fdnum, oldoff, SEEK_SET);
        } else {
            dumpa_skip_reimported_files();
        }

    } else if(memfile && !size && !zsize && !fdnum) {
        // memory file initialization: log MEMORY_FILE 0 0
        dumpa_memory_file(memfile, &out, size, &outsize);
        goto quit;

    } else {
        if((!g_void_dump && !non_files) || (g_void_dump && filetmp)) {
            if(g_list_only || g_force_output || g_quickiso || g_quickzip || g_extraction_hash) {
            } else {
                // the following is not so good for fname ""
                // because will ask the confirmation twice in some occasions
                // Fixed: last_name_crc does the job
                fname = create_dir(fname, 1, 0, 0, 1);
                dumpa_name_overwrite_check
            }
        }

        oldoff = myftell(fdnum);
        myfseek(fdnum, offset, SEEK_SET);
        dumpa_state(&quickbms_compression, &quickbms_encryption, zsize, size, xsize);

        // direct_copy saves memory with normal files
        if(!non_files && !quickbms_encryption && !quickbms_compression && !g_quickzip && fdnum_is_valid) {
            #ifdef ENABLE_DIRECT_COPY
            direct_copy = 1;
            #endif

            // find a way to "guess" if input and output are the same file:
            // it must be simple, cross-platform and doesn't matter if it's not "perfect"
            // because the difference is just in copying the file chunk-by-chunk or all-in-one
            // without negative effects in case of false positives and errors (same input and
            // output is usually wrong but it's valid for compression and encryption that are
            // not involved here).
            // Note that Windows has various API to get the exact path of an HANDLE (take a look
            // at offbreak) but I prefer to use this work-around at the moment.
            u8 *name1 = get_filename(fname);
            u8 *name2 = g_filenumber[fdnum].filename;
            if(name1 && name2 && !stricmp(name1, name2)) {
                fd = xfopen(fname, "rb");
                if(fd) {
                    // very easy, on Windows we have st_ino, st_mode, st_size and all the *time
                    struct stat xstat1, xstat2;
                    memset(&xstat1, 0, sizeof(xstat1));
                    fstat(fileno(g_filenumber[fdnum].fd), &xstat1);
                    memset(&xstat2, 0, sizeof(xstat2));
                    fstat(fileno(fd), &xstat2);
                    if(!memcmp(&xstat1, &xstat2, sizeof(xstat1))) {
                        direct_copy = 0;
                    }
                    FCLOSE(fd);
                }
            }

            // input non-files
            // it's better to avoid the direct_cop optimization
            // with these alternative files
            if(
                g_filenumber[fdnum].sd || // sockets
                g_filenumber[fdnum].pd || 
                g_filenumber[fdnum].ad ||
                g_filenumber[fdnum].vd ||
                g_filenumber[fdnum].md
            ) {
                direct_copy = 0;
            }
        }

        if(direct_copy) {
            // I need correct statistics for -0
            if(!g_void_dump || (g_void_dump && filetmp)) {
                // nothing to do, it will be handled in the last "if" later
            } else {
                len = dumpa_direct_copy(fdnum, NULL, NULL, size, 0, NULL);
            }
        } else {
            //MAX_ALLOC_CHECK(size);
            //myalloc(&out, (xsize > size) ? xsize : size, &outsize); // + 1 is NOT necessary, do not use dumpa_xsize
            myalloc_ret(&out, size,  &outsize);     // this solution avoids problems with xsize zero and size negative
            if(!out) {
                FREE(in)
                insize  = 0;
                myalloc(&out, size,  &outsize);
            }
            if(xsize > size) {
                myalloc_ret(&out, xsize, &outsize);
                if(!out) {
                    FREE(in)
                    insize  = 0;
                    myalloc(&out, xsize, &outsize);
                }
            }
            if(quickbms_compression && (g_compression_type != COMP_COPY)) { // remember that the (size == zsize) check is NOT valid so can't be used in a "generic" way!
                //MAX_ALLOC_CHECK(xsize);
                //MAX_ALLOC_CHECK(zsize);
                len = dumpa_xsize(zsize, xsize);
                myalloc_ret(&in, len, &insize);   // + 1 is NOT necessary
                if(!in) {
                    if(len > outsize) {
                        FREE(out)
                        outsize = 0;
                        myalloc(&out, len, &outsize);
                    }
                    memory_workaround = 1;
                    fprintf(stderr, "Alert: memory alloc work-around, data may be corrupted and a crash may occur\n");
                    in = (out + outsize) - len;
                }
                len = myfr(fdnum, in, len, TRUE);
                if(len < 0) { ret_value = -1; goto quit; }
                len = perform_encryption_and_crchash(in, len);
                if(len == -1) {
                    fprintf(stderr, "\nError: the encryption failed\n");
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_ENCRYPTION);
                }
                // zsize value will not be touched or xsize is totally useless
                if(len < zsize) zsize = len;

                size = perform_compression(in, zsize, &out, size, &outsize, offset);

                if(g_comtype_scan && (size <= 0)) {  // both invalid and empty
                    myfseek(fdnum, oldoff, SEEK_SET);   // important, NEVER forget it!
                    goto quit;
                }
                if(size < 0) {
                    fprintf(stderr, "\n"
                        "Error: there is an error with the decompression\n"
                        "       the returned output size is negative (%"PRId")\n", size);
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_COMPRESSION);
                }
                // size/outsize limit check done directly in perform_compression
                // do NOT add checks which verify if the unpacked size is like the expected one, I prefer the compatibility
            } else {
                len = size;
                if(g_compression_type == COMP_COPY) {
                    if((zsize > 0) && (zsize < size)) len = zsize;  // read the smallest amount if compression is enabled
                }
                len = dumpa_xsize(len, xsize);  // fix for UE4 cb2.pak
                len = myfr(fdnum, out, len, TRUE);
                if(len < 0) { ret_value = -1; goto quit; }
                if(g_compression_type == COMP_COPY) {
                    t = dumpa_xsize(size, xsize);   // do it again with the new "size", out is already correctly allocated
                    if(t > len) memset(out + len, 0, t - len);
                    len = t;
                }
                len = perform_encryption_and_crchash(out, len);
                if(len == -1) {
                    fprintf(stderr, "\nError: the encryption failed\n");
                    if(g_continue_anyway) { ret_value = -1; goto quit; }
                    myexit(QUICKBMS_ERROR_ENCRYPTION);
                }
                // size value will not be touched or xsize is totally useless
                if(len < size) size = len;
            }
        }

        len = size;
        if(sockfile) {
            len = socket_write(sockfile, out, size);

        } else if(procfile) {
            len = process_write(procfile, out, size);

        } else if(audiofile) {
            len = audio_write(audiofile, out, size);

        } else if(videofile) {
            len = video_write(videofile, out, size);

        } else if(winmsgfile) {
            len = winmsg_write(winmsgfile, out, size);

        } else if(memfile) {
            len = dumpa_memory_file(memfile, &out, size, &outsize);

        } else if(!g_void_dump || (g_void_dump && filetmp)) {

            if(g_extraction_hash) {
                dumpa_sha1(fdnum, direct_copy ? NULL : out, size, extraction_hash);
                if(g_extraction_hash > 0) {
                    printf("HASH ");
                    show_hex(extraction_hash, SHA1_DIGEST_SIZE);
                    printf(" %s\n", fname);
                }
                goto skip_extract;
            }

            if(g_force_output) {
                if(!strcmp(g_force_output, "-")) {
                    #ifdef WIN32
                    #define STDOUT_FILENAME "CON"
                    #else
                    #define STDOUT_FILENAME "/dev/tty"
                    #endif

                    // just an experimental and maybe useless thing because doesn't seem to redirect output
                    //freopen(STDOUT_FILENAME, "wb", stdout); fd = stdout;
                    fd = fopen(STDOUT_FILENAME, "wb");
                    if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                    #ifdef O_BINARY
                    setmode(fileno(fd), O_BINARY);
                    #endif
                } else {
                    // "ab" is perfect but we can't get the current offset of the file
                    // which may be necessary for some experimental formats that may
                    // be added in the future... for example TAR needs it for alignment
                    fd = xfopen(g_force_output, "r+b");
                    if(fd) {
                        fseek(fd, 0, SEEK_END);
                    } else {
                        if(!fd) fd = xfopen(g_force_output, "wb");  // if it doens't exist
                        if(!fd) fd = xfopen(g_force_output, "ab");  // fail-safe
                        if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                    }

                    //fd = xfopen(g_force_output, "ab");  // better than "wb" ?
                    //if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                }
                //fname = g_force_output;

                // alternative output formats (experimental)
                if(g_append_mode == APPEND_MODE_NONE) {
                    if(!stricmp(get_extension(g_force_output), "tar")) {
                        //if(strlen(fname) > 100) ... boring, unsupported

                        // padding of previous files
                        // it should be put after the file but this is just a test using simple code
                        // in one location without header/footer sections (may be added in the future)
                        while((u64)ftell(fd) % (u64)512) fputc(0, fd);

                        static u8 tar_buff[512];
                        memset(tar_buff, 0, sizeof(tar_buff));
                        strncpy(tar_buff +   0, fname, 100);            // name[100]
                        sprintf(tar_buff + 100, "%o", 0755);            // mode[8]
                        sprintf(tar_buff + 108, "%o", 0);               // uid[8]
                        sprintf(tar_buff + 116, "%o", 0);               // gid[8]
                        sprintf(tar_buff + 124, "%o", (i32)size);       // size[12]
                        sprintf(tar_buff + 136, "%o", (i32)time(NULL)); // mtime[12]
                        sprintf(tar_buff + 148, "%8s", "");             // chksum[8]
                        sprintf(tar_buff + 156, "%c", '0');             // typeflag;
                        sprintf(tar_buff + 257, "ustar ");              // magic[6]
                        // ignore all the other fields, set them to zero with padding/memset

                        u16 tar_chksum = 0;
                        for(t = 0; t < sizeof(tar_buff); t++) tar_chksum += tar_buff[t];
                        sprintf(tar_buff + 148, "%o", tar_chksum);      // chksum[8]

                        if(fwrite(tar_buff, 1, sizeof(tar_buff), fd) != sizeof(tar_buff)) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                    }
                }

            } else {
                if(nametmp) {
                    // used for log "" in append mode
                    if(g_append_mode != APPEND_MODE_NONE) {
                        HASH_FIND_STR(g_extracted_file, original_fname, ef);
                        if(ef) {
                            fname = ef->name;
                            if(ef->new_name) fname = ef->new_name;
                            nametmp = 0;
                        }
                    }
                }
                if(nametmp) {    // the length of the extension is fixed in the database
                    idx = get_var_from_name("QUICKBMS_FILENAME", -1);
                    if(idx >= 0) {
                        xname = get_var(idx);
                        if(!xname || !xname[0]) {
                            idx = -1;
                        } else {
                            fname = xname;
                            fname = clean_filename(fname, NULL);
                            fname = create_dir(fname, 1, 0, 0, 1);
                            dumpa_name_overwrite_check
                        }
                    }

                    // file extension guesser
                    if(idx < 0) {
                        if(direct_copy) {       // unfortunately will not catch the tga files in this way, that's the only price
                            len = size;         // but note that not all the tga files use the TRUEVISION-XFILE ending!
                            if(len > sizeof(tmpbuff)) len = sizeof(tmpbuff);
                            len = myfr(fdnum, tmpbuff, len, TRUE);
                            if(len < 0) { ret_value = -1; goto quit; }
                            myfr_remove_coverage(fdnum, len);
                            myfseek(fdnum, offset, SEEK_SET);
                            ext = sign_ext(tmpbuff, len);
                        } else {
                            ext = sign_ext(out, size);
                        }
                        if(tmp_ext) ext = tmp_ext;
                        strcpy(strrchr(fname, '.') + 1, ext);
                        dumpa_name_overwrite_check
                        // check_overwrite is used before processing the file for performance reasons
                        // because would be useless to extract a 2gb file that is already extracted
                        // that's why this function is not called below but only here and in the main
                        // part of the function above
                    }

                    if(check_wildcards(fname, g_filter_files) < 0) goto quit;

                    /*
                    used for log "" in append mode but can't really work, for example:
                    FAIL:    log "" 0  0 ; append ; log ""  0 100 -> "00000000.dat" without guessed extension
                    OK:      log "" 0 50 ; append ; log "" 50  50 -> "00000000.png"
                    FAIL:                  append ; log ""  0 100 -> "00000000.png" + "00000000.dat"
                    there is no solution yet and I doubt there will be one in future.
                    */
                    if(g_append_mode == APPEND_MODE_NONE) {
                        append_mode_extracted_file(original_fname, fd, {})
                        append_mode_extracted_file_new_name(original_fname, fname)
                    }
                }
                for(;;) {
                    quickbms_archive_output_write(iso, NULL)
                    quickbms_archive_output_write(zip, out)

                         if(g_append_mode == APPEND_MODE_APPEND)    { fd = xfopen(fname, "r+b"); if(!fd) fd = xfopen(fname, "wb"); }    // append
                    else if(g_append_mode == APPEND_MODE_OVERWRITE) { fd = xfopen(fname, "r+b"); if(!fd) fd = xfopen(fname, "wb"); }    // overwrite
                    else if(g_append_mode == APPEND_MODE_BEFORE)    { fd = xfopen(fname, "r+b"); if(!fd) fd = xfopen(fname, "wb"); }    // before
                    else                                            {                                    fd = xfopen(fname, "wb"); }
                    //if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);
                    if(fd) break;
                    fname = rename_invalid(fname);
                }
                if(g_append_mode == APPEND_MODE_APPEND) {   // use "r+b" instead of "ab" to have information about the real offset
                    fseek(fd, 0, SEEK_END);
                }
            }

            if(g_append_mode == APPEND_MODE_BEFORE) {
                make_file_space(fd, size);
            }
            append_mode_extracted_file(fname, fd,
                {
                    //ef->offset = ftell(fd);   // automatically done by the macro, let's keep the first offset only
                }
            )

            len = dumpa_direct_copy(
                fdnum, fd,
                direct_copy ? NULL : out,
                size,
                0, fname);

            if(g_quickiso) {
                quickiso_padding(g_quickiso);
                fd = NULL;
            }
            if(g_quickzip) {
                fd = NULL;
            }
            FCLOSE(fd);

            quickbms_execute_pipe(g_quickbms_execute_file, NULL, 0, NULL, 0, fname);

        } else /* if(!non_files) not needed */ {

            // for -0 and -l
            if(g_extracted_file_tree_view_mode >= 0) {
                append_mode_extracted_file(fname, fd, {})
            }

        }

        if(len != size) {
            fprintf(stderr, "\n"
                "Error: impossible to write 0x%"PRIx" bytes (total 0x%"PRIx")\n"
                "       Check your disk space or the script is wrong\n",
                (len < 0) ? size : (size - len), size);
            if(g_continue_anyway) { ret_value = -1; goto quit; }
            myexit(QUICKBMS_ERROR_FILE_WRITE);
        }

skip_extract:
        myfseek(fdnum, oldoff, SEEK_SET);
    }
    if(!memfile) {
        if(g_append_mode == APPEND_MODE_NONE) g_extracted_files++;
        g_extracted_logs++;
        if(g_mex_default) {
            add_var(EXTRCNT_idx, NULL, NULL, g_extracted_files, sizeof(int));
            add_var(EXTRCNT_idx, NULL, NULL, g_extracted_logs,  sizeof(int));
        }
    }
quit:
    // the following is used only for -f #NUM, it's the list of parsed files
    if(!memfile) {
        if(g_append_mode == APPEND_MODE_NONE) g_extracted_files2++;
    }
    if(memory_workaround) {
        in      = NULL;
        insize  = 0;
    }
    return(ret_value);
}



u8 *fgetss(int fdnum, int chr, int unicode, int line) {  // reads a chr terminated string, at the moment unicode is referred to the 16bit unicode
    static int  buffsz  = 0;
    static u8   *buff   = NULL;
    int     i,
            len,
            c,
            unicnt  = 0,
            except  = 0,
            wcsz    = (unicode < 0) ? sizeof(u32) : sizeof(u16);
    u32     wc; // was wchar_t but we need a 32bit
    u8      tmp[32];

    if(chr < 0) {
        chr = -chr;
        except = 1;
    }
    // if(!fd) do nothing, modify myfgetc
    for(i = 0;;) {

        //c = myfgetc(fdnum);
        if(myfr(fdnum, tmp, 1, line ? FALSE : TRUE) <= 0) {
            c = -1;
        } else {
            c = tmp[0];
        }

        if(c < 0) {
            if(!i) return NULL;    // return a NULL if EOF... this is for compatibility with old versions of quickbms although it's not so right
            break;
        }

        // use c if len is 1 or tmp if it's longer
        len = 1;
        if(unicode) {

            // shared with CMD_Set_func
            if(!unicnt) wc = 0;
            if(g_endian == MYLITTLE_ENDIAN) {
                wc |= ((u32)c << (u32)(8 * unicnt));
            } else {
                wc |= ((u32)c << (u32)(8 * (wcsz - (unicnt + 1))));
            }
            unicnt++;
            if(unicnt < wcsz) continue;
            unicnt = 0;
            if(unicode > 0) len = utf16_to_utf8_chr(wc, tmp, sizeof(tmp), 0, g_codepage);
            else            len = utf32_to_utf8_chr(wc, tmp, sizeof(tmp), 0, g_codepage);

            if(len == 1) c = wc; //tmp[0];
            else         c = -1; // to bypass "except"
        }

        if(line && !i) {
            //if(!c || strchr(" \t\r\n", c)) continue;
            if(strchr(" \t", c)) continue;
        }
        if(except) {
            if(c != chr) break;
        } else {
            if(line && !c) break;   // don't add '\r', I want a fgets-like solution
            if(c == chr) break;
        }

        if((i + len) >= buffsz) {
            buffsz += len + STRINGSZ;
            buff = realloc(buff, buffsz + max_utf8_char_size);
            if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
        }
        if(len == 1) {  // use c
            buff[i] = c;
        } else {        // use tmp
            memcpy(buff + i, tmp, len);
        }
        i += len;
    }
    //if(c < 0) return NULL;
    if(!buff) buff = malloc(1); // remember, anything returned by this function MUST be allocated
    buff[i] = 0;
    if(except) {
        if(c < 0) {
        } else {
            if(unicode) myfseek(fdnum, -wcsz, SEEK_CUR);
            else        myfseek(fdnum, -1, SEEK_CUR);
        }
    }
    if(line) {
        // or just use: skip_end_string(buff);
        i = strlen(buff);   // quickbms_4gb_files compatibility, necessary or the check will fail!
        for(--i; i >= 0; i--) {  // buff has been nulled
            if(!strchr(" \t\r\n", buff[i])) break;
            buff[i] = 0;
        }
    }
    return buff;
}



int fputss(int fdnum, u8 *data, int chr_eos, int unicode, int line, int maxsz) {  // writes a chr_eos terminated string, currently unicode is referred to utf16
    int     i,
            c,
            t;
    wchar_t wc;

    if(!data) data = "";
    if(maxsz < 0) maxsz = strlen(data) + 1;
    // if(!fd) do nothing, modify myfputc
    for(i = 0;;) {
        if((maxsz >= 0) && (i >= maxsz)) break;

        if(unicode) {
            if(unicode > 0) {
                u16 wc16;
                t = utf8_to_utf16_chr(data + i, maxsz - i, &wc16, 0, g_codepage);
                wc = wc16;
            } else {
                u32 wc32;
                t = utf8_to_utf32_chr(data + i, maxsz - i, &wc32, 0, g_codepage);
                wc = wc32;
            }
            if(t <= 0) break;
            i += t;
            c = wc;
        } else {
            c = data[i++];
        }

        if(line) {
            if((c == 0x00) || (c == '\r') || (c == '\n')) {
                i--;
                break;
            }
        }
        if((chr_eos < 0) && (c == 0x00)) break;
             if(unicode > 0) c = fputxx(fdnum, c, sizeof(u16));
        else if(unicode < 0) c = fputxx(fdnum, c, sizeof(u32));
        else                 c = myfputc(c, fdnum);
        if(c < 0) return -1;
        if(c == chr_eos) break;
    }
    if(line) {
        if(myfputc('\r', fdnum) < 0) return -1;
        if(myfputc('\n', fdnum) < 0) return -1;
        i += 2;
    }
    return i;
}



#include "types.c"



u8 *fget_filename(int fdnum, int type) {
    u8      *ret    = NULL;

    if(fdnum < 0) {
        return "";  // correct
    }

    CHECK_FILENUM

    switch(type) {
        case BMS_TYPE_FILENAME:     ret = g_filenumber[fdnum].filename;     break;
        case BMS_TYPE_BASENAME:     ret = g_filenumber[fdnum].basename;     break;
        case BMS_TYPE_FILEPATH:     ret = g_filenumber[fdnum].filepath;     break;
        case BMS_TYPE_FULLNAME:     ret = g_filenumber[fdnum].fullname;     break;
        case BMS_TYPE_FULLBASENAME: ret = g_filenumber[fdnum].fullbasename; break;
        case BMS_TYPE_EXTENSION:    ret = g_filenumber[fdnum].fileext;      break;
        default:                    ret = NULL;                             break;
    }
    return ret;
}



int myfr_endian(int fdnum, u8 *tmp, int size) {
    int     c, t;
    memset(tmp, 0, size);
    for(c = 0; c < size; c++) {
        if(g_endian == MYLITTLE_ENDIAN) {
            t = myfr(fdnum, tmp + c, 1, TRUE);
        } else {
            t = myfr(fdnum, tmp + (size - 1) - c, 1, TRUE);
        }
        if(t < 0) break;
    }
    return 0;
}

int myfw_endian(int fdnum, u8 *tmp, int size) {
    int     c;
    for(c = 0; c < size; c++) {
        if(g_endian == MYLITTLE_ENDIAN) {
            myfw(fdnum, tmp + c, 1);
        } else {
            myfw(fdnum, tmp + (size - 1) - c, 1);
        }
    }
    return 0;
}



u8 *myfrx(int fdnum, int type, int *ret_num, int *error) {
    long double tmp_longdouble;
    double  tmp_double;
    float   tmp_float;
    u64     tmp64;
    int     retn    = 0,
            i,
            t,
            mask    = 0,
            tmp_error,
            tmp_ret_num;
    u8      tmp[64],
            c,
            *ret    = NULL;

    if(!ret_num) ret_num = &tmp_ret_num;
    if(!error) error = &tmp_error;
    *error = 0;
    switch(type) {
        case BMS_TYPE_LONGLONG:     retn = fgetxx(fdnum, 8, error);     break;
        case BMS_TYPE_LONG:         retn = fgetxx(fdnum, 4, error);     break;
        case BMS_TYPE_SHORT:        retn = fgetxx(fdnum, 2, error);     break;
        case BMS_TYPE_BYTE:         retn = fgetxx(fdnum, 1, error);     break;
        case BMS_TYPE_THREEBYTE:    retn = fgetxx(fdnum, 3, error);     break;
        case BMS_TYPE_ASIZE:        retn = myfilesize(fdnum);           break;
        case BMS_TYPE_STRING: {
            ret  = fgetss(fdnum, 0,    0, 0);
            if(!ret) *error = 1;    // this damn error stuff is needed for compatibility with the old quickbms
            break;                  // and located here doesn't affect the performances
        }
        case BMS_TYPE_LINE: {
            ret  = fgetss(fdnum, '\n', 0, 1);
            if(!ret) *error = 1;
            delimit(ret);
            break;
        }
        case BMS_TYPE_FILENAME: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_BASENAME: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_FILEPATH: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_FULLNAME: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_FULLBASENAME: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_EXTENSION: {
            ret  = fget_filename(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_CURRENT_FOLDER: {
            ret  = g_current_folder;
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_FILE_FOLDER: {
            ret  = g_file_folder;
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_INOUT_FOLDER: {
            ret  = g_output_folder;
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_BMS_FOLDER: {
            ret  = g_bms_folder;
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_EXE_FOLDER: {
            ret  = g_exe_folder;
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_UNICODE: {
            ret  = fgetss(fdnum, 0,    1, 0);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_UNICODE32: {
            ret  = fgetss(fdnum, 0,   -1, 0);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_FLOAT: {
            // use fgetxx instead of myfr for handling the endianess
            retn = fgetxx(fdnum, 4, error);
            if(*error) break;
            //tmp_float = *(float *)((void *)(&retn));
            tmp_float = 0;
            memcpy(&tmp_float, &retn, 4);
            retn = (int)tmp_float;
            break;
        }
        case BMS_TYPE_DOUBLE: {
            // use fgetxx instead of myfr for handling the endianess
            tmp64 = fgetxx(fdnum, 8, error);
            if(*error) break;
            //tmp_double = *(double *)((void *)(&tmp64));
            tmp_double = 0;
            memcpy(&tmp_double, &tmp64, 8);
            retn = (int)tmp_double;
            break;
        }
        case BMS_TYPE_LONGDOUBLE: {
            //myfr(fdnum, tmp, 12, TRUE); // I want to handle also the endianess
            memset(tmp, 0, sizeof(tmp));
            for(c = 0; c < sizeof(tmp_longdouble); c++) {   // sizeof(tmp_longdouble) can be 12 or 16 or even 8
                if(g_endian == MYLITTLE_ENDIAN) {
                    t = myfr(fdnum, tmp + c, 1, TRUE);
                } else {
                    t = myfr(fdnum, tmp + (sizeof(tmp_longdouble) - 1) - c, 1, TRUE);
                }
                if(t < 0) break;
            }
            //tmp_longdouble = *(long double *)tmp;
            tmp_longdouble = 0;
            memcpy(&tmp_longdouble, tmp, sizeof(tmp_longdouble));
            retn = (int)tmp_longdouble;
            break;
        }
        case BMS_TYPE_VARIABLE: {
            do {
                c = fgetxx(fdnum, 1, error);
                if(*error) break;
                retn = ((u_int)retn << (u_int)7) | (c & 0x7f);
            } while(c & 0x80);
            break;
        }
        case BMS_TYPE_VARIABLE2: {
            retn = unreal_index(fdnum);
            break;
        }
        case BMS_TYPE_VARIABLE3: {
            i = 0;
            do {
                c = fgetxx(fdnum, 1, error);
                if(*error) break;
                retn += ((u_int)(c & 0x7f) << (u_int)i);
                i += 7;
            } while(!(c & 0x80));
            break;
        }
        case BMS_TYPE_VARIABLE4: {
            i = 0;
            do {
                c = fgetxx(fdnum, 1, error);
                if(*error) break;
                retn |= ((u_int)(c & 0x7f) << (u_int)i);
                i += 7;
            } while(c & 0x80);
            break;
        }
        case BMS_TYPE_VARIABLE5: {
            c = fgetxx(fdnum, 1, error);
            if(*error) break;
            mask = 0x80;
            for(i = 0; mask; i++) {
                if((c & mask) == 0) {
                    retn += ((u64)(c & (mask - 1)) << (u64)(i * 8));
                    break;
                }
                retn |= ((u64)fgetxx(fdnum, 1, error) << (u64)(i * 8));
                if(*error) break;
                mask >>= 1;
            }
            break;
        }
        case BMS_TYPE_VARIABLE6: {
            // currently ValueMax is not supported in quickbms, this is a placeholder
            t = get_var_from_name("ValueMax", -1);
            t = (t < 0) ? (((u_int)(-1)) >> 1) : get_var32(t);
            i = 0;
            c = 0;
            for(mask=1; retn+mask<t && mask; mask*=2,i++) {
                if(!(i&7)) {
                    c = fgetxx(fdnum, 1, error);
                    if(*error) break;
                }
                if(c & (1<<(i&7)) ) retn |= mask;
            }
            break;
        }
        case BMS_TYPE_VARIABLE7: {
            i = 0;
            t = 1;
            while(t) {
                c = fgetxx(fdnum, 1, error);
                if(*error) break;
                t = c & 1;
                c = c >> 1;
                retn += (u_int)c << (u_int)(7 * i++);
            }
            break;
        }
        case BMS_TYPE_VARIANT: {
            retn = fgetxx(fdnum, 2, error);
            if(*error) break;
            memset(tmp, 0, sizeof(tmp));
            t = myfr(fdnum, tmp, 6, TRUE);
            //if(t < 0) ??? do nothing because it's not even considered
            switch(retn) {
                case 0:  type = BMS_TYPE_NONE;      break;
                case 1:  type = BMS_TYPE_NONE;      break;
                case 2:  type = BMS_TYPE_SHORT;     break;
                case 3:  type = BMS_TYPE_LONG;      break;
                case 4:  type = BMS_TYPE_FLOAT;     break;  // float
                case 5:  type = BMS_TYPE_DOUBLE;    break;  // double
                case 6:  type = BMS_TYPE_LONGLONG;  break;
                case 7:  type = BMS_TYPE_LONGLONG;  break;
                case 8:  type = BMS_TYPE_UNICODE;   break;
                case 9:  type = BMS_TYPE_LONG;      break;
                case 10: type = BMS_TYPE_LONG;      break;
                case 11: type = BMS_TYPE_SHORT;     break;
                case 12: type = BMS_TYPE_VARIANT;   break;
                case 17: type = BMS_TYPE_BYTE;      break;
                default: type = BMS_TYPE_LONG;      break;  // ???
            }
            return(myfrx(fdnum, type, ret_num, error));
            break;
        }
        case BMS_TYPE_NONE: retn = 0;   break;
        case BMS_TYPE_TIME: {
            ret = time_to_strtime(fdnum);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_TIME64: {
            ret = time64_to_strtime(fdnum);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_CLSID: {
            ret = bytes2clsid(fdnum);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_IPV4: {
            retn = fgetxx(fdnum, 4, error);
            if(*error) break;
            if(g_endian != MYLITTLE_ENDIAN) retn = swap32(retn);  // because ip2str works in big endian
            ret = ip2str(retn);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_IPV6: {
            ret = ipv6_to_string(fdnum);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_ASM16:
        case BMS_TYPE_ASM64:
        case BMS_TYPE_ASM_ARM:
        case BMS_TYPE_ASM_ARM_THUMB:
        case BMS_TYPE_ASM_ARM64:
        case BMS_TYPE_ASM_MIPS:
        case BMS_TYPE_ASM_MIPS64:
        case BMS_TYPE_ASM_PPC:
        case BMS_TYPE_ASM_PPC64:
        case BMS_TYPE_ASM_SPARC:
        case BMS_TYPE_ASM_SYSZ:
        case BMS_TYPE_ASM_XCORE:
        case BMS_TYPE_ASM: {
            ret = quickbms_disasm(fdnum, type);
            if(!ret) *error = 1;
            break;
        }
        case BMS_TYPE_SIGNED_BYTE:         retn = fgetxx(fdnum, 1, error); if(retn & 0x80)       retn |= -0x100LL;         break;
        case BMS_TYPE_SIGNED_SHORT:        retn = fgetxx(fdnum, 2, error); if(retn & 0x8000)     retn |= -0x10000LL;       break;
        case BMS_TYPE_SIGNED_THREEBYTE:    retn = fgetxx(fdnum, 3, error); if(retn & 0x800000)   retn |= -0x1000000LL;     break;
        case BMS_TYPE_SIGNED_LONG:         retn = fgetxx(fdnum, 4, error); if(retn & 0x80000000) retn |= -0x100000000LL;   break;
        case BMS_TYPE_PROMPT:
            fprintf(stderr, "\n- please insert the content for the variable:\n  ");
            ret = incremental_fread(stdin, NULL, 1, NULL, 0, 0);
            break;
        default: {
            fprintf(stderr, "\nError: invalid datatype %d\n", (i32)type);
            myexit(QUICKBMS_ERROR_BMS);
            break;
        }
    }
    *ret_num = retn;
    //if(!ISNUMTYPE(type) && !ret) *error = 1;  // bad, decrease a lot the performances
    return ret;
}



int myfwx(int fdnum, int varn, int type) {
    long double tmp_longdouble;
    double  tmp_double;
    float   tmp_float;
    u64     tmp64;
    u32     tmp32;
    int     retn    = 0;
    u8      tmp[64],
            c;

    switch(type) {
        case BMS_TYPE_LONGLONG:     retn = fputxx(fdnum, get_var32(varn), 8);   break;
        case BMS_TYPE_SIGNED_LONG:
        case BMS_TYPE_LONG:         retn = fputxx(fdnum, get_var32(varn), 4);   break;
        case BMS_TYPE_SIGNED_SHORT:
        case BMS_TYPE_SHORT:        retn = fputxx(fdnum, get_var32(varn), 2);   break;
        case BMS_TYPE_SIGNED_BYTE:
        case BMS_TYPE_BYTE:         retn = fputxx(fdnum, get_var32(varn), 1);   break;
        case BMS_TYPE_SIGNED_THREEBYTE:
        case BMS_TYPE_THREEBYTE:    retn = fputxx(fdnum, get_var32(varn), 3);   break;
        case BMS_TYPE_ASIZE:        retn = fputxx(fdnum, myfilesize(fdnum), 4); break;
        case BMS_TYPE_STRING: { // NULL delimited string
            retn = fputss(fdnum, get_var(varn), 0, 0, 0, -1 /*get_varsz(varn)*/);
            break;
        }
        case BMS_TYPE_LINE: {
            retn = fputss(fdnum, get_var(varn), -1, 0, 1, -1 /*get_varsz(varn)*/);
            break;
        }
        case BMS_TYPE_FILENAME: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_BASENAME: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_FILEPATH: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_FULLNAME: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_FULLBASENAME: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_EXTENSION: {
            retn = fputss(fdnum, fget_filename(fdnum, type), -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_CURRENT_FOLDER: {
            retn = fputss(fdnum, g_current_folder, -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_FILE_FOLDER: {
            retn = fputss(fdnum, g_file_folder, -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_INOUT_FOLDER: {
            retn = fputss(fdnum, g_output_folder, -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_BMS_FOLDER: {
            retn = fputss(fdnum, g_bms_folder, -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_EXE_FOLDER: {
            retn = fputss(fdnum, g_exe_folder, -1, 0, 0, -1);
            break;
        }
        case BMS_TYPE_UNICODE: {    // NULL delimited
            retn = fputss(fdnum, get_var(varn), 0, 1, 0, -1);
            break;
        }
        case BMS_TYPE_UNICODE32: {    // NULL delimited
            retn = fputss(fdnum, get_var(varn), 0, -1, 0, -1);
            break;
        }
        case BMS_TYPE_FLOAT: {
            if(g_variable[varn].isnum < 0) {
                tmp_float = (float)g_variable[varn].float64;
            } else {
                retn = get_var32(varn);
                tmp_float = (float)retn;
                //retn = *(int *)((void *)(&tmp_float));
            }
            retn = 0;
            memcpy(&retn, &tmp_float, 4);
            retn = fputxx(fdnum, retn, 4);
            break;
        }
        case BMS_TYPE_DOUBLE: {
            if(g_variable[varn].isnum < 0) {
                tmp_double = (double)g_variable[varn].float64;
            } else {
                retn = get_var32(varn);
                tmp_double = (double)retn;
            }
            //tmp64 = *(u64 *)((void *)(&tmp_double));
            tmp64 = 0;
            memcpy(&tmp64, &tmp_double, 8);
            // fputxx is 32bit on !QUICKBMS64
            if(g_endian == MYLITTLE_ENDIAN)  {
                retn = fputxx(fdnum, tmp64, 4);
                retn = fputxx(fdnum, tmp64 >> 32, 4);
            } else {
                retn = fputxx(fdnum, tmp64 >> 32, 4);
                retn = fputxx(fdnum, tmp64, 4);
            }
            break;
        }
        case BMS_TYPE_LONGDOUBLE: {
            if(g_variable[varn].isnum < 0) {
                tmp_longdouble = (long double)g_variable[varn].float64;
            } else {
                retn = get_var32(varn);
                tmp_longdouble = (long double)retn;
            }
            memcpy(tmp, (void *)&tmp_longdouble, sizeof(tmp_longdouble));
            for(c = 0; c < sizeof(tmp_longdouble); c++) {
                if(g_endian == MYLITTLE_ENDIAN) {
                    myfw(fdnum, tmp + c, 1);
                } else {
                    myfw(fdnum, tmp + (sizeof(tmp_longdouble) - 1) - c, 1);
                }
            }
            retn = 0;
            break;
        }
        case BMS_TYPE_VARIABLE:     retn = put_type_variable(fdnum, get_var32(varn));   break;
        case BMS_TYPE_VARIABLE2: {
            c = make_unreal_index(get_var32(varn), tmp);
            retn = myfw(fdnum, tmp, c);
            break;
        }
        case BMS_TYPE_VARIABLE3:    retn = put_type_variable3(fdnum, get_var32(varn));  break;
        case BMS_TYPE_VARIABLE4:    retn = put_type_variable4(fdnum, get_var32(varn));  break;
        case BMS_TYPE_VARIABLE5:    retn = put_type_variable5(fdnum, get_var32(varn));  break;
        case BMS_TYPE_VARIABLE6:    retn = put_type_variable6(fdnum, get_var32(varn));  break;
        case BMS_TYPE_VARIABLE7:    retn = put_type_variable7(fdnum, get_var32(varn));  break;
        //case BMS_TYPE_VARIANT:    // unsupported
        case BMS_TYPE_NONE: retn = 0;   break;
        case BMS_TYPE_TIME: {
            strtime_to_time(get_var(varn), &tmp32, NULL);
            retn = fputxx(fdnum, tmp32, 4);
            break;
        }
        case BMS_TYPE_TIME64: {
            strtime_to_time(get_var(varn), NULL, &tmp64);
            // fputxx is 32bit on !QUICKBMS64
            if(g_endian == MYLITTLE_ENDIAN)  {
                retn = fputxx(fdnum, tmp64, 4);
                retn = fputxx(fdnum, tmp64 >> 32, 4);
            } else {
                retn = fputxx(fdnum, tmp64 >> 32, 4);
                retn = fputxx(fdnum, tmp64, 4);
            }
            break;
        }
        case BMS_TYPE_CLSID: {
            retn = clsid2bytes(fdnum, get_var(varn));
            break;
        }
        case BMS_TYPE_IPV4: {
            retn = str2ip(get_var(varn));
            if(g_endian != MYLITTLE_ENDIAN) retn = swap32(retn);  // because str2ip works in big endian
            retn = fputxx(fdnum, retn, 4);
            break;
        }
        case BMS_TYPE_IPV6: {
            retn = string_to_ipv6(fdnum, get_var(varn));
            break;
        }
        case BMS_TYPE_ASM: {
            retn = quickbms_asm(fdnum, get_var(varn));
            break;
        }
        default: {
            fprintf(stderr, "\nError: invalid or unsupported datatype %d\n", (i32)type);
            myexit(QUICKBMS_ERROR_BMS);
            break;
        }
    }
    return(retn);
}


