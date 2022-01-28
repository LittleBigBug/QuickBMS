/*
    Copyright 2009-2014 Luigi Auriemma

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

// QuickBMS cool hexdump HTML output



enum {
    HEXHTML_HTML,
    HEXHTML_CONSOLE,
    //
    HEXHTML_NONE
};
int             hexhtml_idx     = 0,    // CMD.var[0], don't worry it's only a temporary global variable
                hexhtml_idx_max = 0,
                hexhtml_skip    = 1,
                hexhtml_output  = HEXHTML_HTML,
                hexhtml_noascii_details = 0; // save space for the browser
u8              *hexhtml_name   = NULL; // temporary variable

/* types
-1 = cool, small and safe way
0  = tables
1  = a title
2  = a span
*/



#define hexhtml_span        "span"  // IE wants span or nothing
#define hexthml_skip_char   "."     // "\xa0"

#define HEXHTML_ASSIGN \
        hexhtml_t   **hexhtml = NULL; \
        int     *hexhtml_size = NULL; \
        if(fdnum < 0) { \
            hexhtml      = &g_memory_file[-fdnum].hexhtml; \
            hexhtml_size = &g_memory_file[-fdnum].hexhtml_size; \
        } else { \
            CHECK_FILENUMX \
            hexhtml      = &g_filenumber[fdnum].hexhtml; \
            hexhtml_size = &g_filenumber[fdnum].hexhtml_size; \
        }
        //don't check hexhtml and hexhtml_size



u_int myftell(int fdnum);



int hexhtml_init(int fdnum, int filesize) {
    HEXHTML_ASSIGN

    hexhtml_skip = 0;
    if(hexhtml_output == HEXHTML_HTML) {
        if(filesize > ((1024 * 1024) / 2)) {
            fprintf(stderr, "\n"
                "Alert: the input file is too big for the HTML output.\n"
                "       I will skip the visualization of unhandled bytes\n");
            /*
            fprintf(stderr, "\n"
                "Error: the input file is too big for the HTML output.\n"
                "       it takes tons of resources of the browser so you can use this feature\n"
                "       only with small files.\n");
            myexit(QUICKBMS_ERROR_MEMORY);
            */
            hexhtml_skip = 1;
        }
    }

    if(hexhtml && *hexhtml) {
        real_free(*hexhtml);
        *hexhtml = NULL;
    }
    *hexhtml_size = filesize;
    *hexhtml = real_calloc(*hexhtml_size, sizeof(hexhtml_t));
    if(!*hexhtml) STD_ERR(QUICKBMS_ERROR_MEMORY);
    hexhtml_name = NULL;

    return 0;
}



int hexhtml_add(int fdnum, u8 *data, int size) {
    static int  db_names    = 0;
    static u8   **db_name   = NULL;
    int     i,
            j,
            j_hexhtml_name = -1,
            len,
            offset;
    u8      *p,
            *s;

    HEXHTML_ASSIGN
    if(!hexhtml || !*hexhtml) return -1;

    if(size < 0) return -1;
    if(!size) return 0;

    offset = myftell(fdnum);
    if(offset > *hexhtml_size) return -1;
    offset -= size;
    if(offset < 0) return -1;

    for(i = 0; i < size; i++) {
        (*hexhtml)[offset + i].byte = data[i];
        (*hexhtml)[offset + i].idx  = 1 + hexhtml_idx;
        if(hexhtml_name) {

            // bits are not handled separately because this is a file reading operation
            // consider this solution a work-around

            p = (*hexhtml)[offset + i].name;
            if(p) {
                len = strlen(p);
                s = real_malloc(len + 1 + strlen(hexhtml_name) + 1);
                if(!s) STD_ERR(QUICKBMS_ERROR_MEMORY);
                memcpy(s, p, len);
                s[len] = '|';
                strcpy(s + len + 1, hexhtml_name);
            } else {
                s = hexhtml_name;
            }

            if((s == hexhtml_name) && (j_hexhtml_name >= 0)) {
                j = j_hexhtml_name;
            } else {
                for(j = 0; j < db_names; j++) {
                    if(!stricmp(db_name[j], s)) break;
                }
            }
            if(j >= db_names) {
                j = db_names;
                db_names++;
                db_name = real_realloc(db_name, sizeof(u8 *) * db_names);
                if(!db_name) STD_ERR(QUICKBMS_ERROR_MEMORY);
                if(s == hexhtml_name) {
                    if(j_hexhtml_name < 0) j_hexhtml_name = j;
                    db_name[j] = real_strdup(s);
                } else {
                    db_name[j] = s; // already allocated
                }
            } else {
                if(s != hexhtml_name) real_free(s);
            }
            (*hexhtml)[offset + i].name = db_name[j];

        } else {
            (*hexhtml)[offset + i].name = NULL;
        }
        (*hexhtml)[offset + i].flags = i ? 0 : 1;
    }
    if(hexhtml_idx > hexhtml_idx_max) hexhtml_idx_max = hexhtml_idx;
    return 0;
}



u8 *hexhtml_color(int idx) {
    static const int minchr = 0xa0;
    static const int maxchr = 0xff;
    static u8   ret[32];
    i32     i;
    u8      rgb[3];

    if(idx <= 0) return("ffffff");

    u32     t;
    t = idx;
    for(i = 0; i < idx; i++) {
        t = (t * 214013) + 2531011;
    }

    rgb[0] = t >> 16;
    rgb[1] = t >> 8;
    rgb[2] = t;
    for(i = 0; i < 3; i++) {
        while((rgb[i] < minchr) || (rgb[i] > maxchr)) rgb[i] = (rgb[i] * 214013) + 2531011;
    }
    sprintf(ret, "%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
    return ret;
}



u8 *html_isprint(u8 byte) {
    static u8   ret[32];

         if(byte == '\"') { if(hexhtml_output == HEXHTML_HTML) return("&quot;"); }
    else if(byte == '&')  { if(hexhtml_output == HEXHTML_HTML) return("&amp;"); }
    else if(byte == '<')  { if(hexhtml_output == HEXHTML_HTML) return("&lt;"); }
    else if(byte == '>')  { if(hexhtml_output == HEXHTML_HTML) return("&gt;"); }
    else if(byte == 0x00) { if(hexhtml_output == HEXHTML_HTML) byte = 0xa0; else byte = ' '; }
    else if(byte == ' ')  { if(hexhtml_output == HEXHTML_HTML) byte = 0xa0; }
    else if(byte == '\t') { if(hexhtml_output == HEXHTML_HTML) byte = 0xa0; else byte = ' '; }
    else if(byte == '\v') { if(hexhtml_output == HEXHTML_HTML) byte = 0xa0; else byte = ' '; }
    else if(byte <= ' ')  byte = '.';
    else if(byte >= 0x7f) byte = '.';
    ret[0] = byte;
    ret[1] = 0;
    return ret;
}



int hexhtml_search(int fdnum, int offset, u8 *str) {
    int     i,
            o;

    HEXHTML_ASSIGN

    for(; offset < *hexhtml_size; offset++) {
        i = 0;
        for(o = offset; o < *hexhtml_size; o++) {
            if(!str[i]) return offset;
            if(tolower((*hexhtml)[o].byte) != tolower(str[i])) break;
            i++;
        }
    }
    return -1;
}



int hexhtml_build(int fdnum) {
    static u8   tmp_fname[PATHSZ + 1] = "";
    FILE    *fd = NULL;
    i32     i,
            j,
            len,
            table,
            old_idx,
            skip;
    u8      tmp[256],   // for hex viewer
            *name,
            *p;

    HEXHTML_ASSIGN
    if(!hexhtml || !*hexhtml) return -1;

    if(fdnum < 0) {
        name = "MEMORY_FILE";
    } else {
        name = g_filenumber[fdnum].filename;
        if(!name) {
            name = g_filenumber[fdnum].basename;
            if(!name) {
                name = g_filenumber[fdnum].fullname;
                if(!name) name = "FILE";
            }
        }
    }

    snprintf(tmp_fname, PATHSZ, "%.*s_%08x.htm", PATHSZ - 16, name, (i32)(time(NULL) + fdnum));
    p = clean_filename(tmp_fname, NULL);

    if(hexhtml_output == HEXHTML_CONSOLE) {
        fd = stdout;
    } else if(hexhtml_output == HEXHTML_HTML) {
        fprintf(stderr, "\n"
            "- hex HTML output:\n"
            "  file: %s\n"
            "  dump: %s\n"
            "\n"
            "- wait patiently...\n",
            name,
            p);

        if(check_overwrite(p, 0) < 0) return -1;
        fd = xfopen(p, "wb");
        if(!fd) STD_ERR(QUICKBMS_ERROR_FILE_WRITE);

        fprintf(fd,
            "<html>\n"
            "<title>QuickBMS: %s</title>\n"
            "<style type='text/css'>\n"
            "<!--\n"
            "*{margin:0;padding:0;color:#000000;font-size:14;font-family:courier,monospace;text-decoration:none}\n"
            "table{border-style:solid}\n"
            "table{table-layout:fixed}\n"
            "td{background-color:#f8f8f8;border-style:solid;border-width:0px 0px 1px 1px;border-color:#c0c0c0;padding:1px 0px 1px 0px}\n"
            "td.offset{background-color:#d4d0c8}\n"
            "td.hex   {background-color:#f8f8f8}\n"
            "td.ascii {background-color:#f4f0e8}\n",
            name);

        for(i = 0; i <= (hexhtml_idx_max + 1); i++) {
            for(j = 0;; j++) {
                switch(j) {
                    case 0:  p = "%s.x%d:hover";    break;
                    case 1:  p = "%s.x%d";          break;
                    //case 2:  p = "%s.x%d:link";     break;
                    default: p = NULL;              break;
                }
                if(!p) break;

                sprintf(tmp, p, g_enable_hexhtml ? "a" : "td", i);
                if(!i) {
                    tmp[0] = ' ';
                    tmp[1] = ' ';
                    tmp[2] = ' ';
                    sprintf(tmp + 3, "%s", g_enable_hexhtml ? "a" : "td");
                }

                fprintf(fd, tmp, NULL);
                fprintf(fd,
                    "{position:relative;z-index:%d;color:#%s;background-color:#%s;border-style:solid;border-width:0px 0px 0px 0px;border-color:#c0c0c0;padding:2px 0px 1px 0px}\n",
                    (!j) ? 200 : 0,
                    (!j) ? "ffffff" : "000000",
                    (!j) ? "0a246a" : (char *)hexhtml_color(i));

                fprintf(fd, tmp, NULL);
                if(j) {
                    fprintf(fd, " "hexhtml_span"{display:none}\n");
                } else {
                    fprintf(fd, " "hexhtml_span"{display:block;position:absolute;bottom:20;border:1px solid #80ff80;background-color:#008000;color:#ffffff;text-align:center}\n");
                }
            }
        }

        fprintf(fd,
            "-->\n"
            "</style>\n"
            "<body bgcolor=#808080>\n"
            "\n"
            "<table align=left border=3 cellspacing=0 cellpadding=0 rules=none>\n");
    }

    if(g_enable_hexhtml >= 0) goto hexhtml_other_types;


        // console

    if(hexhtml_output == HEXHTML_CONSOLE) {

        int     hexhtml_console_color[] = {
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_GREEN | BACKGROUND_BLUE,
                                                                                                        BACKGROUND_RED   | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_BLUE,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_GREEN,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_GREEN | BACKGROUND_BLUE,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_RED,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_RED   | BACKGROUND_BLUE,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_BLUE  | BACKGROUND_INTENSITY,
                                                                                                        BACKGROUND_GREEN | BACKGROUND_INTENSITY,
                                                                                                        BACKGROUND_GREEN | BACKGROUND_BLUE  | BACKGROUND_INTENSITY,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_RED   | BACKGROUND_INTENSITY,
                                                                                                        BACKGROUND_RED   | BACKGROUND_BLUE  | BACKGROUND_INTENSITY,
            FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY |              BACKGROUND_RED   | BACKGROUND_GREEN,
                                                                                                        BACKGROUND_RED   | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
            -1
        };
        int     hexhtml_console_colors = 0;

        CONSOLE_SCREEN_BUFFER_INFO  hexhtml_console_info;
        HANDLE          hexhtml_stdout,
                        hexhtml_stdin;

        hexhtml_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
        hexhtml_stdin  = GetStdHandle(STD_INPUT_HANDLE);
        for(i = 0; hexhtml_console_color[i] >= 0; i++);
        hexhtml_console_colors = i;
        GetConsoleScreenBufferInfo(hexhtml_stdout, &hexhtml_console_info);
        int     lines       = 0,
                max_lines   = 50,
                vis_i       = 0,
                old_i       = 0,
                color       = 0;

        max_lines = (hexhtml_console_info.srWindow.Bottom - hexhtml_console_info.srWindow.Top) - 1;
        i = 0;
        for(;;) {

            #ifdef WIN32
            if(i > *hexhtml_size) i = *hexhtml_size;
            if(i < 0)             i = 0;

            if(lines == 0) {
                old_i = vis_i;
                vis_i = i;
            }
            if(lines >= max_lines) {
                lines = 0;
                SetConsoleTextAttribute(hexhtml_stdout, FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                fprintf(fd, "\n");
                name = (*hexhtml)[vis_i].name;
                if(!name) name = get_varname((*hexhtml)[vis_i].idx - 1);
                if(name) fprintf(fd, "%.79s", name);
                fprintf(fd, "\n: ");

                INPUT_RECORD    ir;
                DWORD   unused;

                while(ReadConsoleInput(hexhtml_stdin, &ir, 1, &unused)) {
                    if((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown) {
                        if(ir.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) {
                            // Windows 98 bug
                            // after all I don't use the SHIFT key so no problem
                        } else {
                            break;
                        }
                    }
                }

                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_DELETE) {
                    for(i = vis_i - 1; i >= 0; i--) {
                        if((*hexhtml)[i].idx  != (*hexhtml)[vis_i].idx) break;
                        if((*hexhtml)[i].name != (*hexhtml)[vis_i].name) break;
                    }
                    j = i;
                    for(--i; i >= 0; i--) {
                        if((*hexhtml)[i].idx  != (*hexhtml)[j].idx) break;
                        if((*hexhtml)[i].name != (*hexhtml)[j].name) break;
                    }
                    i++;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_TAB) {
                    for(i = vis_i; i < *hexhtml_size; i++) {
                        if((*hexhtml)[i].idx  != (*hexhtml)[vis_i].idx) break;
                        if((*hexhtml)[i].name != (*hexhtml)[vis_i].name) break;
                    }
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_BACK) {
                    i = old_i;  // don't use old_i
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_UP) {
                    i = vis_i - 0x10;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
                    i = vis_i + 0x10;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_LEFT) {
                    i = vis_i - 1;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT) {
                    i = vis_i + 1;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_PRIOR) {
                    i = vis_i - (max_lines * 0x10);
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_NEXT) {
                    i = vis_i + (max_lines * 0x10);
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_SPACE) continue;
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
                    i = vis_i + 0x10;
                    continue;
                }
                if(!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) && (tolower(ir.Event.KeyEvent.uChar.AsciiChar) == 'q')) break;
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_HOME) {
                    i = 0;
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_END) {
                    i = *hexhtml_size - (max_lines * 0x10);
                    continue;
                }
                if(!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) && (ir.Event.KeyEvent.uChar.AsciiChar == 'g')) {
                    i = 0;
                    continue;
                }
                if(!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) && (ir.Event.KeyEvent.uChar.AsciiChar == 'G')) {
                    i = *hexhtml_size - (max_lines * 0x10);
                    continue;
                }
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) break;
                if(ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL) break;

                j = 0;
                do {
                    if((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown) {
                        if(ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) break;
                        if(!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY)) {
                            if(ir.Event.KeyEvent.uChar.AsciiChar) {
                                if(j < (sizeof(tmp) - 1)) tmp[j++] = ir.Event.KeyEvent.uChar.AsciiChar;
                                fputc(ir.Event.KeyEvent.uChar.AsciiChar, fd);
                            }
                        }
                    }
                } while(ReadConsoleInput(hexhtml_stdin, &ir, 1, &unused));
                tmp[j] = 0;

                if(tolower(tmp[0]) == 'q') break;
                if(tolower(tmp[0]) == 'e') break;
                switch(tmp[0]) {
                    case '/':
                        len = hexhtml_search(fdnum, vis_i, tmp + 1);
                        if(len >= 0) i = len;
                        break;
                    case '%':
                        len = readbase(tmp + 1, 16, NULL);
                        if(len < 0) len = 0;
                        if(len > 100) len = 100;
                        i = (*hexhtml_size / 100) * len;
                        break;
                    case '+':
                        len = readbase(tmp + 1, 16, NULL);
                        i = vis_i + len;
                        break;
                    case '-':
                        len = readbase(tmp + 1, 16, NULL);
                        i = vis_i - len;
                        break;
                    default:
                        i = readbase(tmp, 16, NULL);
                        break;
                }
                continue;
            }
            #else
            if(i > *hexhtml_size) break;
            #endif

            SetConsoleTextAttribute(hexhtml_stdout, FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            fprintf(fd, "\n%08X  ", i);

            for(table = 0;; table++) {
                for(j = 0; j < 0x10; j++) {
                    if(i >= *hexhtml_size) {
                        SetConsoleTextAttribute(hexhtml_stdout, 0);
                        if(!table) fprintf(fd, "   ");
                        else       fprintf(fd, " ");
                    } else {
                        //SetConsoleTextAttribute(hexhtml_stdout, hexhtml_console_color[(*hexhtml)[i].idx % hexhtml_console_colors]);

                        if(!(*hexhtml)[i].idx) {
                            SetConsoleTextAttribute(hexhtml_stdout, FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        } else {
                            //if((i > 0) && (*hexhtml)[i].idx != (*hexhtml)[i - 1].idx) 
                            color = (*hexhtml)[i].idx;
                            SetConsoleTextAttribute(hexhtml_stdout, hexhtml_console_color[color % hexhtml_console_colors]);
                        }

                        if(!table) {
                            fprintf(fd, "%02X", (*hexhtml)[i].byte);
                            if((j + 1) >= 0x10) SetConsoleTextAttribute(hexhtml_stdout, 0);
                            fprintf(fd, " ");
                        } else {
                            fprintf(fd, "%s", html_isprint((*hexhtml)[i].byte));
                        }
                    }
                    i++;
                }
                if(table) break;
                SetConsoleTextAttribute(hexhtml_stdout, 0);
                fprintf(fd, " ");
                i -= 0x10;
            }

            lines++;
        }
        SetConsoleTextAttribute(hexhtml_stdout, hexhtml_console_info.wAttributes);
        fprintf(fd, "\n");

        goto hexhtml_quit;
    }


        // all the others

    if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "<tr>\n");

    for(table = 0; table < 3; table++) {
        switch(table) {
            case 0:  name = "offset width=70";  break;
            case 1:  name = "hex";              break;
            case 2:  name = "ascii";            break;
            default: name = "";                 break;
        }
        if(hexhtml_output == HEXHTML_HTML) {
            fprintf(fd,
                "</td>\n"
                "<td class=%s>",
                name);
        }

        old_idx = -1;
        skip = 0;
        for(i = 0; i < *hexhtml_size; i++) {
            name = (*hexhtml)[i].name;
            if(!name) name = get_varname((*hexhtml)[i].idx - 1);

            if(hexhtml_skip) {
                if(!(*hexhtml)[i].idx) {
                    if(skip < 0) continue;
                    skip = 1;
                } else {
                    if(skip < 0) {
                        if(!table) {
                            len = sprintf(tmp, "...");
                            for(j = 8; j > len; j--) fprintf(fd, "\xa0");   // like a space/dot
                            fprintf(fd, "%s", tmp);
                            switch(hexhtml_output) {
                                case HEXHTML_HTML:      fprintf(fd, "<br>");    break;
                                case HEXHTML_CONSOLE:   fprintf(fd, "\n");      break;
                                default: break;
                            }
                            if(i & 0xf) {
                                len = sprintf(tmp, "%X", i - (i & 0xf));
                                for(j = 8; j > len; j--) fprintf(fd, "\xa0");   // like a space/dot
                                fprintf(fd, "%s", tmp);
                            }
                        } else {
                            for(j = 0; j < 16; j++) {
                                switch(table) {
                                    case 1:  fprintf(fd, hexthml_skip_char hexthml_skip_char " "); break;
                                    case 2:  fprintf(fd, hexthml_skip_char " ");  break;
                                    default: break;
                                }
                            }
                            fprintf(fd, "<br>");
                            for(j = 0; j < (i & 0xf); j++) {
                                /*
                                switch(table) {
                                    case 1:  fprintf(fd, hexthml_skip_char hexthml_skip_char " "); break;
                                    case 2:  fprintf(fd, hexthml_skip_char " ");  break;
                                    default: break;
                                }
                                */
                                switch(table) {
                                    case 1:  fprintf(fd, "%02X ", (*hexhtml)[i - (i & 0xf) + j].byte); break;
                                    case 2:  fprintf(fd, "%s ", html_isprint((*hexhtml)[i - (i & 0xf) + j].byte)); break;
                                    default: break;
                                }
                            }
                        }
                    }
                    skip = 0;
                }
            }

            if(!table) {
                if(!(i & 0xf)) {
                    len = sprintf(tmp, "%X", i);
                    for(j = 8; j > len; j--) fprintf(fd, "\xa0");   // like a space/dot
                    fprintf(fd, "%s", tmp);
                }
            } else {
                if(skip >= 0) {
                    if(i & 0xf) fprintf(fd, " ");
                }

                if((table == 2) && hexhtml_noascii_details) {
                    // do nothing
                } else {
                    if(((*hexhtml)[i].flags & 1) || ((*hexhtml)[i].idx != old_idx)) {
                        if(old_idx > 0) {
                            if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</a>");
                        }
                        if((*hexhtml)[i].flags & 1) {
                            if(hexhtml_output == HEXHTML_HTML) {
                                fprintf(fd,
                                    //"<a class=x%d href=:%X><"hexhtml_span">%s</"hexhtml_span">",    // popup mode (IE suffers!)
                                    "<a class=x%d href=:%X title=\'%s\'>",    // title mode
                                    (*hexhtml)[i].idx,
                                    i,
                                    name);
                            }
                        }
                        old_idx = (*hexhtml)[i].idx;
                    }
                }
            }

            //if(!skip) {
                switch(table) {
                    case 1:  fprintf(fd, "%02X", (*hexhtml)[i].byte); break;
                    case 2:  fprintf(fd, "%s", html_isprint((*hexhtml)[i].byte)); break;
                    default: break;
                }
            /*
            } else {
                switch(table) {
                    case 1:  fprintf(fd, hexthml_skip_char hexthml_skip_char); break;
                    case 2:  fprintf(fd, hexthml_skip_char);  break;
                    default: break;
                }
            }
            */

            if((i & 0xf) == 0xf) {
                switch(hexhtml_output) {
                    case HEXHTML_HTML:      fprintf(fd, "<br>");    break;
                    case HEXHTML_CONSOLE:   fprintf(fd, "\n");      break;
                    default: break;
                }
                if(skip) skip = -1;
            }
        }
    }

    if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</td>\n");
    goto hexhtml_quit;

hexhtml_other_types:

    /*
        the original cool methods have the big problem of being horribly slow
        and consume an incredible amount of resources... but they were really
        cool and nice to watch, oh well.
        WORK-IN-PROGRESS because I have modified the default values for type -1.
    */

    for(i = 0; i < *hexhtml_size; i += 0x10) {
        if(hexhtml_output == HEXHTML_HTML) {
            len = sprintf(tmp, "%X", i);
            fprintf(fd, "<tr><td class='offset'>");
            for(j = 8; j > len; j--) fprintf(fd, "&nbsp;");
            fprintf(fd, "%s</td></tr>\n", tmp);
        } else {
            fprintf(fd, "%08X", i);
        }
    }

    if(hexhtml_output == HEXHTML_HTML) {
        fprintf(fd,
            "</table>\n"
            "<table align=left border=3 cellspacing=0 cellpadding=0 rules=none>\n");
    }

    for(i = 0; i < *hexhtml_size; i++) {
        if(!(i & 0xf)) {
            if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "<tr>");
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "<td>");
            }
        }

        name = (*hexhtml)[i].name;
        if(!name) name = get_varname((*hexhtml)[i].idx - 1);

        if(!(*hexhtml)[i].idx) {
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) {
                    fprintf(fd,
                        "<a href=:%X>%02X</a>",
                        i,
                        (*hexhtml)[i].byte);
                } else {
                    fprintf(fd,
                        "%02X",
                        (*hexhtml)[i].byte);
                }
            } else {
                if(hexhtml_output == HEXHTML_HTML) {
                    fprintf(fd,
                        "<td>%02X</td>",
                        (*hexhtml)[i].byte);
                } else {
                    fprintf(fd,
                        "%02X",
                        (*hexhtml)[i].byte);
                }
            }
        } else {
            switch(g_enable_hexhtml) {
                case 2: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<a class=x%d href=:%X>%02X<"hexhtml_span">%s</"hexhtml_span"></a>",
                            (*hexhtml)[i].idx,
                            i,
                            (*hexhtml)[i].byte,
                            name);
                    } else {
                        fprintf(fd,
                            "%02X",
                            (*hexhtml)[i].byte);
                    }
                    break;
                }
                case 1: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<a class=x%d title=\'%s\' href=:%X>%02X</a>",
                            (*hexhtml)[i].idx,
                            name,
                            i,
                            (*hexhtml)[i].byte);
                    } else {
                        fprintf(fd,
                            "%02X",
                            (*hexhtml)[i].byte);
                    }
                    break;
                }
                default: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<td class=x%d>%02X<span>%s</span></td>",
                            (*hexhtml)[i].idx,
                            (*hexhtml)[i].byte,
                            name);
                    } else {
                        fprintf(fd,
                            "%02X",
                            (*hexhtml)[i].byte);
                    }
                    break;
                }
            }
        }
        if((i & 0xf) == 0xf) {
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</td>");
            }
            if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</tr>\n");
        }
    }

    if(hexhtml_output == HEXHTML_HTML) {
        fprintf(fd,
            "</table>\n"
            "<table align=left border=3 cellspacing=0 cellpadding=0 rules=none>\n");
    }

    for(i = 0; i < *hexhtml_size; i++) {
        if(!(i & 0xf)) {
            if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "<tr>");
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "<td>");
            }
        }

        name = (*hexhtml)[i].name;
        if(!name) name = get_varname((*hexhtml)[i].idx - 1);

        if(!(*hexhtml)[i].idx || hexhtml_noascii_details) {
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) {
                    fprintf(fd,
                        "<a href=:%X>%s</a>",
                        i,
                        html_isprint((*hexhtml)[i].byte));
                } else {
                    fprintf(fd,
                        "%s",
                        html_isprint((*hexhtml)[i].byte));
                }
            } else {
                if(hexhtml_output == HEXHTML_HTML) {
                    fprintf(fd,
                        "<td>%s</td>",
                        html_isprint((*hexhtml)[i].byte));
                } else {
                    fprintf(fd,
                        "%s",
                        html_isprint((*hexhtml)[i].byte));
                }
            }
        } else {
            switch(g_enable_hexhtml) {
                case 2: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<a class=x%d href=:%X>%s<"hexhtml_span">%s</"hexhtml_span"></a>",
                            (*hexhtml)[i].idx,
                            i,
                            html_isprint((*hexhtml)[i].byte),
                            name);
                    } else {
                        fprintf(fd,
                            "%s",
                            html_isprint((*hexhtml)[i].byte));
                    }
                    break;
                }
                case 1: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<a class=x%d title=\'%s\' href=:%X>%s</a>",
                            (*hexhtml)[i].idx,
                            name,
                            i,
                            html_isprint((*hexhtml)[i].byte));
                    } else {
                        fprintf(fd,
                            "%s",
                            html_isprint((*hexhtml)[i].byte));
                    }
                    break;
                }
                default: {
                    if(hexhtml_output == HEXHTML_HTML) {
                        fprintf(fd,
                            "<td class=x%d>%s<span>%s</span></td>",
                            (*hexhtml)[i].idx,
                            html_isprint((*hexhtml)[i].byte),
                            name);
                    } else {
                        fprintf(fd,
                            "%s",
                            html_isprint((*hexhtml)[i].byte));
                    }
                    break;
                }
            }
        }

        if((i & 0xf) == 0xf) {
            if(g_enable_hexhtml) {
                if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</td>");
            }
            if(hexhtml_output == HEXHTML_HTML) fprintf(fd, "</tr>\n");
        }
    }

hexhtml_quit:
    // check fd
    switch(hexhtml_output) {
        case HEXHTML_HTML:
            if(fd) {
                fprintf(fd,
                    "</table>\n"
                    "</body>\n"
                    "</html>\n");
            }
            break;
        case HEXHTML_CONSOLE:
            break;
        default:
            break;
    }

    FCLOSE(fd);
    fprintf(stderr, "- done\n");
    return 0;
}


