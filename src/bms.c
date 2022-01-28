/*
    Copyright 2009-2021 Luigi Auriemma

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

// QuickBMS script reading and parsing operations



// the rule is simple: start_bms is executed for EACH recursive command like do, for, if
int start_bms(int startcmd, int nop, int this_is_a_cycle, int *invoked_if, int *invoked_break, int *invoked_continue, u8 **invoked_label) {

#define BMS_QUIT_ERROR      { quit = 1;  break; }
#define BMS_QUIT_NO_ERROR   { quit = -1; break; }

#define BMS_START_NEW(B,X,Y,Z) \
    cmd = B(X, Y, Z, invoked_if, invoked_break, invoked_continue, invoked_label); \
    if(cmd < 0) BMS_QUIT_ERROR \

#define BMS_START_RESET \
    if(startcmd < 0) { \
        cmd = 0;    /* needed because it's the beginning */ \
    } else { \
        cmd = startcmd; \
    }

#define BMS_END_OF_LOOP_RESET \
    BMS_START_RESET \
    cmd--;  /* due to "cmd++" */

#define BMS_END_OF_LOOP \
    if(*invoked_label) { \
        if(!invoked_label_loop) { \
            invoked_label_loop++; \
            BMS_END_OF_LOOP_RESET \
        } else { \
            BMS_QUIT_NO_ERROR \
        } \
    } else { \
        if((nop < 0) && this_is_a_cycle) { \
            nop = 0; \
            *invoked_continue = 0; \
        } else { \
            if(nop) BMS_QUIT_NO_ERROR \
        } \
    }

    int     this_if         = 0;
    int     this_break      = 0;
    int     this_continue   = 0;
    u8      *this_label     = NULL;

    int     cmd;
    int     quit            = 0;
    u8      *error          = NULL;

    i32     invoked_label_loop  = 0;

    if(!invoked_if)       invoked_if        = &this_if;
    if(!invoked_break)    invoked_break     = &this_break;
    if(!invoked_continue) invoked_continue  = &this_continue;
    if(!invoked_label)    invoked_label     = &this_label;

    if(this_is_a_cycle) {
        this_break       = *invoked_break;      // inherit
        this_continue    = *invoked_continue;   // inherit
        invoked_break    = &this_break;
        invoked_continue = &this_continue;
    }

    if(g_debug_output) {
        xdebug_print(0, this_is_a_cycle ? "[" : "{", -1, NULL, -1, 0, 0);
        g_debug_output->level++;
    }

    // do not use redo_time limiters, this feature requires endless loops

    BMS_START_RESET
    if(g_verbose > 0) printf("             .start_bms start: %d %d %d/%d\n", (i32)startcmd, (i32)nop, (i32)*invoked_break, (i32)*invoked_continue);
    for(;; cmd++) {

        if(CMD.type == CMD_NONE) {
            if(*invoked_label) {
                if(!invoked_label_loop) {
                    invoked_label_loop++;
                    BMS_START_RESET
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if(!nop)
        {
            if(*invoked_label)    nop = 1;  // break and continue get resetted at end of cycle
            if(*invoked_break)    nop = 1;
            if(*invoked_continue) nop = -1;
        }

        g_last_cmd = cmd;
        g_bms_line_number = CMD.bms_line_number;

        //if(g_verbose && CMD.debug_line) printf("\n%"PRIx" %s%s\n", myftell(0), CMD.debug_line, nop ? " (SKIP)" : "");
        if((g_verbose > 0) && CMD.debug_line && !nop) {
            printf("\n%"PRIx" %02x  %s\n",
                g_filenumber[0].fd ? myftell(0) : 0,
                CMD.type,
                CMD.debug_line);
        }

        if(g_enable_hexhtml) {
            hexhtml_idx  = CMD.var[0];
            hexhtml_name = NULL;
        }

        switch(CMD.type) {
            case CMD_For: {
                //if(nop) break;
                //if(g_verbose < 0) printf(".\n");  // useful
                BMS_START_NEW(start_bms, cmd + 1, nop, 1)
                break;
            }
            case CMD_Next: {
                BMS_END_OF_LOOP
                if(CMD_Next_func(cmd) < 0) BMS_QUIT_ERROR
                BMS_END_OF_LOOP_RESET
                break;
            }
            case CMD_Prev: {
                BMS_END_OF_LOOP
                if(CMD_Prev_func(cmd) < 0) BMS_QUIT_ERROR
                BMS_END_OF_LOOP_RESET
                break;
            }
            case CMD_ForTo: {
                if(nop) break;
                //if(g_verbose < 0) printf(".\n");  // useful
                if(check_condition(cmd, -1, NULL, -1) == FALSE) nop = 1;
                break;
            }
            case CMD_Get: {
                if(nop) break;
                if(CMD_Get_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_GetDString: {
                if(nop) break;
                if(CMD_GetDString_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_GoTo: {
                if(nop) break;
                if(CMD_GoTo_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_IDString: {
                if(nop) break;
                if(CMD_IDString_func(cmd) < 0) {
                    error = "the signature doesn't match";        
                    BMS_QUIT_ERROR
                }
                break;
            }
            case CMD_Log: {
                if(nop) break;
                if(CMD_Log_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_CLog: {
                if(nop) break;
                if(CMD_CLog_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Math: {
                if(nop) break;
                if(CMD_Math_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_XMath: {
                if(nop) break;
                if(CMD_XMath_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_NameCRC: {
                if(nop) break;
                if(CMD_NameCRC_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Codepage: {
                if(nop) break;
                if(CMD_Codepage_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_SLog: {
                if(nop) break;
                if(CMD_SLog_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_SavePos: {
                if(nop) break;
                if(CMD_SavePos_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Set: {
                if(nop) break;
                if(CMD_Set_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_String: {
                if(nop) break;
                if(CMD_String_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_If:
                invoked_if = &this_if;
                *invoked_if = 0;
            case CMD_Elif:
            case CMD_Else: {
                //if(nop) break;
                if(!*invoked_if && (check_condition(cmd, -1, NULL, -1) == TRUE)) {
                    BMS_START_NEW(start_bms, cmd + 1, nop, 0)
                    *invoked_if = 1;
                } else {
                    BMS_START_NEW(start_bms, cmd + 1, 1,   0)
                }
                break;
            }
            case CMD_If_Return: {
                //if(nop) break;
                BMS_QUIT_NO_ERROR
                break;
            }
            case CMD_EndIf: {
                *invoked_if = 0;
                //if(nop) break;
                BMS_QUIT_NO_ERROR
                break;
            }
            case CMD_GetCT: {
                if(nop) break;
                if(CMD_GetCT_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ComType: {
                if(nop) break;
                if(CMD_ComType_func(cmd, NULL, NULL, 0) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Open: {
                if(nop) break;
                if(CMD_Open_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ReverseShort: {
                if(nop) break;
                if(CMD_ReverseShort_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ReverseLong: {
                if(nop) break;
                if(CMD_ReverseLong_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ReverseLongLong: {
                if(nop) break;
                if(CMD_ReverseLongLong_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Endian: {
                if(nop) break;
                if(CMD_Endian_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_FileXOR: {
                if(nop) break;
                if(CMD_FileXOR_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_FileRot13: {
                if(nop) break;
                if(CMD_FileRot13_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_FileCrypt: {
                if(nop) break;
                if(CMD_FileCrypt_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Break: {
                if(nop) break;  // like cleanexit, don't touch
                *invoked_break = 1;
                *invoked_label = CMD_Label_func(cmd, NULL);
                break;
            }
            case CMD_Continue: {
                if(nop) break;  // like cleanexit, don't touch
                *invoked_continue = 1;
                *invoked_label = CMD_Label_func(cmd, NULL);
                break;
            }
            case CMD_GetVarChr: {
                if(nop) break;
                if(CMD_GetVarChr_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_PutVarChr: {
                if(nop) break;
                if(CMD_PutVarChr_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Append: {
                if(nop) break;
                if(CMD_Append_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Encryption: {
                if(nop) break;
                if(CMD_Encryption_func(cmd, 0) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_GetArray: {
                if(nop) break;
                if(CMD_GetArray_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_PutArray: {
                if(nop) break;
                if(CMD_PutArray_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_SortArray: {
                if(nop) break;
                if(CMD_SortArray_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_SearchArray: {
                if(nop) break;
                if(CMD_SearchArray_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_StartFunction: {
                //if(nop) break;
                BMS_START_NEW(CMD_Function_func, cmd, 1,   1)
                break;
            }
            case CMD_CallFunction: {
                if(nop) break;
                if(g_verbose < 0) printf(".\n");  // useful
                BMS_START_NEW(CMD_Function_func, cmd, nop, 1)
                break;
            }
            case CMD_EndFunction: {
                if(nop) BMS_QUIT_NO_ERROR
                BMS_QUIT_NO_ERROR
                break;
            }
            case CMD_Debug: {
                if(nop) break;
                //verbose = !verbose;
                if(CMD_Debug_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Padding: {
                if(nop) break;
                if(CMD_Padding_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ScanDir: {
                if(nop) break;
                if(CMD_ScanDir_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_CallDLL: {
                if(nop) break;
                if(CMD_CallDLL_func(cmd, NULL, 0, NULL, 0) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Put: {
                if(nop) break;
                if(CMD_Put_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_PutDString: {
                if(nop) break;
                if(CMD_PutDString_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_PutCT: {
                if(nop) break;
                if(CMD_PutCT_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Strlen: {
                if(nop) break;
                if(CMD_Strlen_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_Do: {
                //if(nop) break;
                if(g_verbose < 0) printf(".\n");  // useful
                BMS_START_NEW(start_bms, cmd + 1, nop, 1)
                break;
            }
            case CMD_While: {
                BMS_END_OF_LOOP
                if(check_condition(cmd, -1, NULL, -1) == FALSE) BMS_QUIT_NO_ERROR
                BMS_END_OF_LOOP_RESET
                break;
            }
            case CMD_Print: {
                if(nop) break;
                if(CMD_Print_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_FindLoc: {
                if(nop) break;
                if(CMD_FindLoc_func(cmd) < 0) {
                    error = "the searched string has not been found";
                    BMS_QUIT_ERROR
                }
                break;
            }
            case CMD_GetBits: {
                if(nop) break;
                if(CMD_GetBits_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_PutBits: {
                if(nop) break;
                if(CMD_PutBits_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_ImpType: {
                if(nop) break;
                if(CMD_ImpType_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_CleanExit: {
                if(nop) break;  // don't touch
                error = "invoked the termination of the extraction (CleanExit)";
                BMS_QUIT_ERROR
                break;
            }
            case CMD_Label: {
                //if(nop) break;
                if(*invoked_label && CMD_Label_func(cmd, *invoked_label)) {
                    *invoked_label      = NULL;
                    *invoked_break      = 0;
                    *invoked_continue   = 0;
                    invoked_label_loop  = 0;
                    nop = 0;
                }
                break;
            }
            case CMD_Reimport: {
                if(nop) break;
                if(CMD_Reimport_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            // internal
            case CMD_DirectoryExists: {
                if(nop) break;
                if(CMD_DirectoryExists_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_FEof: {
                if(nop) break;
                if(CMD_FEof_func(cmd) < 0) BMS_QUIT_ERROR
                break;
            }
            case CMD_NOP: {
                if(nop) break;
                // no operation, do nothing
                break;
            }
            default: {
                fprintf(stderr, "\nError: invalid command %d\n", (i32)CMD.type);
                myexit_cmd(cmd, QUICKBMS_ERROR_BMS);
                break;
            }
        }

        if(quit) break;   // we no longer use goto redo/quit/quit, do not move into the for()
    }

    if(!quit) {
        cmd = -1; //return -1; // CMD_NONE
    }
    if(quit >= 0) { // was quit_error:
        if(cmd >= 0) {
            //if(*invoked_label) goto redo;
            if(g_verbose > 0) fprintf(stderr, "\nError: %s\n", error ? error : (u8 *)"something wrong during the extraction");
            //myexit(QUICKBMS_ERROR_BMS);
            cmd = -1; //return -1;
        }
    }
    // was quit:
    if(cmd >= 0) {
        //if(*invoked_label) goto redo;
        if(g_verbose > 0) fprintf(stderr, "             .start_bms end: %d %d %d/%d (ret %d)\n", (i32)startcmd, (i32)nop, (i32)*invoked_break, (i32)*invoked_continue, (i32)cmd);
    }
    if(g_debug_output) {
        g_debug_output->level--;
        xdebug_print(0, this_is_a_cycle ? "]," : "},", -1, NULL, -1, 0, 0);
    }
    return(cmd);
}



void check_g_command(int cmd) {
    if(cmd >= MAX_CMDS) {
        fprintf(stderr, "\nError: the BMS script uses more commands than those supported by this tool (%d / %d)\n", (i32)cmd, (i32)MAX_CMDS);
        myexit(QUICKBMS_ERROR_BMS);
    }
}



int bms_line(FILE *fd, u8 **input_line, u8 **argument, int cmd, u8 *ret_is_const, int eol_mode) {
#define ARGS_DELIMITER  " \t" ",()"
    static  int wide_comment = 0;
    static  u8  crlf[2] = {0,0};    // used only for g_bms_line_number!
    static  int buffsz  = 0;
    static  u8  *buff   = NULL;
    static  u8  tmpchars[MAX_ARGS][NUMBERSZ + 1] = {""};
    static int  do_multiline = 0;
    static  u8  *multiline   = NULL;
    int     i,
            argi    = 0,
            j,
            c;
    u8      tmp[1 + 1],
            *line,
            *p,
            *s;

    if(!argument) {
        FREE(buff)
        buffsz = 0;
        memset(&crlf, 0, sizeof(crlf));
        return -1;
    }

    if(!g_bms_line_number) wide_comment = 0;
    if(!g_bms_line_number) do_multiline = 0;

redo:
    //if(!input_line || !*input_line) return 0; NEVER
    do {
        g_bms_line_number++;
        for(i = 0;;) {
            if(fd) {
                c = fgetc(fd);
            } else if(input_line) {
                c = **input_line;
                if(!c) c = -1;  // a buffer ends with 0
                else (*input_line)++;   // this is necessary or we can't find the end: "string\x00memory"
            } else {
                break;
            }
            if(!c) continue;    // unicode blah, !i is used to handle only the first bytes 
            if((g_bms_line_number <= 1) && !i && ((c == 0xef) || (c == 0xbb) || (c == 0xbf) || (c == 0xfe) || (c == 0xff))) continue;
            if(c == 0xc2) continue;
            if(c == 0xa0) c = ' ';
            if(c < 0) {
                if(!i) {    // end of file
                    g_bms_line_number = 0;
                    return -1;
                }
                break;
            }
            if((eol_mode == 1) && (c == ';')) {
                break;
            }
            if((c == '\n') || (c == '\r')) {
                if(!i) {    // used only for g_bms_line_number!
                    if(
                        (!crlf[0])
                     || ((crlf[0] == '\n') && (c == '\n'))
                     || ((crlf[0] == '\r') && (c == '\r'))
                    // || ((crlf[1] == '\r') && (crlf[0] == '\n') && (c == '\r'))
                     || ((crlf[1] == '\n') && (crlf[0] == '\r') && (c == '\n'))
                    ) {
                        g_bms_line_number++;
                    }
                    crlf[1] = crlf[0];
                    crlf[0] = c;
                    continue;
                }
                crlf[1] = 0;
                crlf[0] = c;
                break;
            }
            if(i >= buffsz) {
                //if((buffsz + STRINGSZ + 1) < buffsz) ALLOC_ERR;
                buffsz += STRINGSZ;
                buff = realloc(buff, buffsz + 1);
                if(!buff) STD_ERR(QUICKBMS_ERROR_MEMORY);
            }
            buff[i] = c;
            i++;
        }
        if(!buff) buff = malloc(1);
        buff[i] = 0;

        for(p = buff; *p && (*p != '\n') && (*p != '\r'); p++);
        *p = 0;

        if(do_multiline) {
            line = buff;
            goto goto_multiline;
        }

        if(fd) {
            for(p--; (p >= buff) && strchr(ARGS_DELIMITER ";", *p); p--);
            if(p >= buff) p[1] = 0;

            for(p = buff; *p && strchr(ARGS_DELIMITER "}", *p); p++);   // '}' is for C maniacs like me
            line = p;
            if((line[0] == '/') && (line[1] == '*')) {
                wide_comment = 1;
                break;
            }
            if((line[0] == '*') && (line[1] == '/')) {
                if(wide_comment) break;
            }
            if(!myisalnum(line[0])) line[0] = 0;  // so we avoid both invalid chars and comments like # ; // and so on
        } else {
            line = buff;
        }
    } while(!line[0]);

    if(cmd >= 0) {
        check_g_command(cmd);
        CMD.bms_line_number = g_bms_line_number;
        CMD.debug_line = realloc(CMD.debug_line, 32 + strlen(line) + 1);
        if(!CMD.debug_line) STD_ERR(QUICKBMS_ERROR_MEMORY);
        sprintf(CMD.debug_line, "%-3d %s", (i32)g_bms_line_number, line);
        if(g_verbose > 0) printf("READLINE %s\n", CMD.debug_line);
    }

    for(i = 0; i < MAX_ARGS; i++) { // reset all
        argument[i] = NULL;
        if(ret_is_const) ret_is_const[i] = 0;
    }

    if(wide_comment) {
        p = strstr(line, "*/");
        if(!p) {
            line[0] = 0;
        } else {
            mymemmove(line, p + 2, -1);
            wide_comment = 0;
        }
    }

    for(argi = 0;;) {
        if(argi >= MAX_ARGS) {
            fprintf(stderr, "\nError: the BMS script uses more arguments than those supported by this tool (%d / %d) at line %d\n", (i32)argi, (i32)MAX_ARGS, (i32)g_bms_line_number);
            myexit(QUICKBMS_ERROR_BMS);
        }
        for(p = line; *p && strchr(ARGS_DELIMITER, *p); p++);
        if(!*p) break;
        line = p;

        if((line[0] == '/') && (line[1] == '/')) break;
        if((line[0] == '/') && (line[1] == '*')) {
            wide_comment = 1;
            p = strstr(line + 2, "*/");
            if(!p) {
                break;
            } else {
                mymemmove(line, p + 2, -1);
                wide_comment = 0;
                continue;
            }
        }
        if(line[0] == '#') break;
        if(line[0] == ';') break;
        if(line[0] == '\'') {     // C char like 'A' or '\x41'
            line++;
            strcpy(tmpchars[argi], "0x");
            i = 0;
            for(p = line; *p; p += c) {
                c = 1;
                if((p[0] == '\\') && (p[1] == '\'')) {
                    c = 2;
                    continue;
                }
                if(*p == '\'') break;

                if(i < sizeof(int)) {
                    cstring(p, tmp, 1, &c, NULL);
                    sprintf(tmpchars[argi] + strlen(tmpchars[argi]), "%02x", tmp[0]);
                    i++;
                }
            }
            if(!i) strcpy(tmpchars[argi], "0");
            argument[argi] = tmpchars[argi];

        } else if(line[0] == '\"') {  // string
            line++;
            if(multiline) multiline[0] = 0;
goto_multiline:
            s = line;
            for(p = line;; p++) {
                c = *p; // needed only for this "if"
                if(!c || (c == '\"')) {
                    if(!c) do_multiline++;
                    if(do_multiline) {
                        *s = 0;
                        j = 0;
                        if(multiline) j = strlen(multiline);
                        multiline = realloc(multiline, j + 2 + strlen(line) + 1);
                        if(!multiline) STD_ERR(QUICKBMS_ERROR_MEMORY);
                        if(j) {
                            multiline[j++] = '\r';
                            multiline[j++] = '\n';
                        }
                        strcpy(multiline + j, line);
                    }
                    if(!c) goto redo;
                    break;
                }
                // THIS IS A WORK-AROUND because there is no clear definition of delimiters
                // examples: "c:\\\"x" "c:\\\"x" "c:\\" "c:\" and multi lines with \" at the end of the line
                if((p[0] == '\\') && (p[1] == '\"') /*&& p[2]*/) {  // for working with "c:\\"
                    if((!multiline && p[2]) || multiline) {
                        p++;
                        *s++ = *p;
                        continue;
                    }
                }
                *s++ = *p;
            }
            if(s != p) *s = 0;
            if(do_multiline) argument[argi] = multiline;
            else             argument[argi] = line;
            do_multiline = 0;
            if(ret_is_const) ret_is_const[argi] = 1;

        } else {
            for(p = line; *p; p++) {
                if(strchr(ARGS_DELIMITER, *p)) break;
            }
            argument[argi] = line;
        }
        //if(p == line) break;  // this must be ignored otherwise "" is not handled

        c = *p;
        *p = 0;
        argument[argi] = mystrdup_simple(argument[argi]);   // create a new copy, introduced with multiline
        argi++;

        if(!c) break;
        line = p + 1;
    }
    argument[argi] = NULL;
    return(argi);
}



void set_quickbms_arg(u8 *quickbms_arg) {
    int     i,
            arg,
            argc;
    u8      tmp[64],
            *argument[MAX_ARGS + 1] = { NULL };

    if(!quickbms_arg) return;
    if(g_verbose > 0) printf("- quickbms_arg:   %s\n", quickbms_arg);

    u8  *quickbms_arg_var = NULL;
    int idx = get_var_from_name("quickbms_arg", -1);
    if(idx >= 0) {  // variable
        u8 *p = get_var(idx);
        if(p) {
            quickbms_arg_var = realloc(quickbms_arg_var, strlen(p) + 1 + strlen(quickbms_arg) + 1);
            if(!quickbms_arg_var) STD_ERR(QUICKBMS_ERROR_MEMORY);
            sprintf(quickbms_arg_var, "%s %s", p, quickbms_arg);
        }
    }
    add_var(0, "quickbms_arg", quickbms_arg_var ? quickbms_arg_var : quickbms_arg, 0, -1);    // probably useless but consider it a fail-safe solution
    FREE(quickbms_arg_var)

    argc = bms_line(NULL, &quickbms_arg, argument, -1, NULL, 0);
    for(i = 0;; i++) {
        sprintf(tmp, "quickbms_arg%d", (i32)i + 1);
        if(get_var_from_name(tmp, -1) < 0) break;
    }
    arg = i;
    for(i = 0; i < argc; i++) {
        sprintf(tmp, "quickbms_arg%d", (i32)arg + (i32)i + 1);
        if(g_verbose > 0) printf("- %s:  %s\n", tmp, ARG[i]);
        add_var(0, tmp, ARG[i], 0, -1);
    }
}



// zlib + base64
u8 *type_decompress(u8 *str, int *ret_len) {
    int     len;
    if(ret_len) *ret_len = 0;
    if(str) {
        len = unbase64(str, -1, str, -1);   // use the same buffer
        if(len >= 0) {
            u8  *ret = NULL;
            len = unzip_dynamic(str, len, &ret, NULL, 0);
            if(len >= 0) {
                if(ret_len) *ret_len = len;
                return ret;
            }
        }
    }
    fprintf(stderr, "\nError: failed Set type decompression at line %d, recheck your script\n", (i32)g_bms_line_number);
    myexit(QUICKBMS_ERROR_BMS);
    return NULL;
}



void c_struct_clean(u8 *str) {
    u8      *s;

    if(!str) return;
    for(s = str + strlen(str) - 1; s >= str; s--) {
        if(!strchr(" \t;", *s)) break;  // example: int magic3; // asdf
        *s = 0;
    }
}



int c_structs(u8 *argument[MAX_ARGS + 1], int argc, int *ret_cmd) {

    typedef struct {
        u8  *old;
        u8  *new;
    } define_t;

    static u8       tmp[256] = "";
    static int      defines = 0;
    static define_t *define = NULL;

    int     i,
            cmd,
            type    = BMS_TYPE_NONE,
            array   = 0,
            filenum = 0;
    u8      *arrayp = NULL,
            *put    = NULL,
            *p;

    static int  is_enum = 0;
    static int  enum_value = 0;

    if(!argument && !argc && !ret_cmd) {
        is_enum = 0;
        enum_value = 0;
        return 0;
    }

    // lame solution necessary because argument[] is allocated while here we use constants
    u8 *bck_argument[MAX_ARGS + 1];
    for(i = 0; i < MAX_ARGS; i++) {
        bck_argument[i] = argument[i];
    }

    fprintf(stderr, "- c_structs (%d):", (i32)argc);
    for(i = 0; (i < MAX_ARGS) && argument[i]; i++) {
        c_struct_clean(argument[i]);
        fprintf(stderr, " \"%s\"", argument[i]);
    }
    fprintf(stderr, "\n");

    for(i = 0; i <= argc; i++) {
        if(is_MEMORY_FILE(ARG[i])) {
            filenum = myatoifile(ARG[i]);
            ARG[i] = "";
        }
    }

    // enum is complex to implement: enum_value, no handling of the final ';'
    if(!stricmp(ARG[0], "enum")) {
        is_enum = 1;
        enum_value = 0;
        ARG[0] = "NOP";
        goto quit;
    }
    if(is_enum) {
        if(strchr(ARG[0], '}')) {   // '}' is filtered so this instruction is useless
            is_enum = 0;
            enum_value = 0;
            ARG[0] = "NOP";
            goto quit;
        }
        if(argc == 0) {
            ARG[1] = ARG[0];
            ARG[0] = "math";
            ARG[2] = "=";
            sprintf(tmp, "%"PRIu, enum_value);
            ARG[3] = tmp;
            argc = 3;
            enum_value++;
            goto quit;
        }
        if(argc == 2) {
            if(!strcmp(ARG[1], "=")) {
                for(i = argc; i >= 0; i--) {
                    ARG[i + 1] = ARG[i];
                }
                argc++;
                ARG[0] = "math";
                enum_value = myatoi(ARG[3]);
                enum_value++;
                goto quit;
            }
        }
        is_enum = 0;
    }

    if(argc <= 0) {
        ARG[0] = "NOP";
        goto quit;
    }

    while(
      !stricmp(ARG[0], "unsigned") ||
      !stricmp(ARG[0], "signed") ||
      !stricmp(ARG[0], "const") ||
      !stricmp(ARG[0], "static") ||
      !stricmp(ARG[0], "local") ||
      !stricmp(ARG[0], "global") ||
      !stricmp(ARG[0], "volatile") ||
        // ???
      !stricmp(ARG[0], "hexadecimal") ||
      !stricmp(ARG[0], "decimal") ||
      !stricmp(ARG[0], "octal") ||
      !stricmp(ARG[0], "read-only")
    ) {
        for(i = 1; i <= argc; i++) {
            ARG[i - 1] = ARG[i];
        }
        argc--;
    }

    if(!stricmp(ARG[1], "int")) {
        for(i = 2; i <= argc; i++) {
            ARG[i - 1] = ARG[i];
        }
        argc--;
    }

    if(!stricmp(ARG[1], "*")) {
        for(i = 2; i <= argc; i++) {
            ARG[i - 1] = ARG[i];
        }
        argc--;
        array = -1;
    }

    if(!stricmp(ARG[0], "struct") || !stricmp(ARG[1], "struct")) {
        ARG[0] = "NOP";
        argc = 0;
        goto quit;
    }

    if(!stricmp(ARG[0], "#define") || !stricmp(ARG[0], "define")) {
        i = (defines + 1) * sizeof(define_t);
        if(i < sizeof(define_t)) ALLOC_ERR;
        define = realloc(define, i);
        if(!define) STD_ERR(QUICKBMS_ERROR_MEMORY);
        mystrdup(&define[defines].new, ARG[1]);
        mystrdup(&define[defines].old, ARG[2]);
        defines++;
        ARG[0] = "NOP";
        argc = 0;
        goto quit;
    }

    if(!stricmp(ARG[0], "typedef")) {
        i = (defines + 1) * sizeof(define_t);
        if(i < sizeof(define_t)) ALLOC_ERR;
        define = realloc(define, i);
        if(!define) STD_ERR(QUICKBMS_ERROR_MEMORY);
        mystrdup(&define[defines].old, ARG[1]);
        mystrdup(&define[defines].new, ARG[2]);
        defines++;
        ARG[0] = "NOP";
        argc = 0;
        goto quit;
    }

    for(i = 0; i < defines; i++) {
        if(!stricmp(ARG[0], define[i].new)) {
            ARG[0] = define[i].old;
            break;
        }
    }

    p = strchr(ARG[0], '*');
    if(p) {
        *p = 0;
        array = -1;
    }

    p = strchr(ARG[1], '*');
    if(!p) p = strchr(ARG[0], '*');
    if(p) {
        p++;
        for(i = 0;; i++) {
            ARG[1][i] = p[i];
            if(!p[i]) break;
        }
        array = -1;
    }

    p = strstr(ARG[1], "[]");
    if(!p && ARG[2]) p = strstr(ARG[2], "[]");
    if(p) {
        *p = 0;
        array = -1;
    }

    p = strchr(ARG[1], ':');
    if(p && myisdigit(p[1])) {
        *p++ = 0;
        array  = 1;
        arrayp = p; // bits
        ARG[0] = "bits";
    }

    p = strchr(ARG[1], '=');
    if(p && strcmp(ARG[1], "=") && !strchr(ARG[1], '(')) {
        *p++ = 0;
        while(*p && (*p <= ' ')) p++;
        put = p;
        /*
        for(i = 0;; i++) {
            ARG[1][i] = p[i];
            if(!p[i]) break;
        }
        */
    }

    while(ARG[0][0] == '_') {
        p = ARG[0] + 1;
        for(i = 0;; i++) {
            ARG[0][i] = p[i];
            if(!p[i]) break;
        }
    }

    if(!strncmp(ARG[0], "LP", 2)) {          // LPVOID
        p = ARG[0] + 2;
        for(i = 0;; i++) {
            ARG[0][i] = p[i];
            if(!p[i]) break;
        }
        array = -1;
    } else if(!strncmp(ARG[0], "P", 1)) {    // PCHAR, PLONG
        p = ARG[0] + 1;
        for(i = 0;; i++) {
            ARG[0][i] = p[i];
            if(!p[i]) break;
        }
        array = -1;
    }

    i = 0;
    p = NULL;
    if((argc >= 2) && (ARG[2][0] == '[')) {
        p = ARG[2];
        i = 1;
    }
    if(!p) p = strchr(ARG[0], '[');
    if(!p) p = strchr(ARG[1], '[');
    if(p) {
        *p++ = 0;
        array  = 1;
        arrayp = p;
        while(*p) {
            if(*p == ']') {
                *p = 0;
                break;
            }
            p++;
        }
    }
    if(i) {
        for(i = 3; i <= argc; i++) {
            ARG[i - 1] = ARG[i];
        }
        argc--;
    }

    if((argc >= 2) && !stricmp(ARG[2], "=")) {
        put = ARG[3];
        /*
        for(i = 3; i <= argc; i++) {
            ARG[i - 2] = ARG[i];
        }
        */
        argc -= 2;
    }

    if(
        !stricmp(ARG[0], "8") ||
        !stricmp(ARG[0], "8bit") ||
        !stricmp(ARG[0], "byte") ||
        !stricmp(ARG[0], "ubyte") ||
        !stricmp(ARG[0], "char") ||
        !stricmp(ARG[0], "cchar") ||
        !stricmp(ARG[0], "tchar") ||
        !stricmp(ARG[0], "uchar") ||
        !stricmp(ARG[0], "u_char") ||
        !stricmp(ARG[0], "uint8_t") ||
        !stricmp(ARG[0], "uint8") ||
        !stricmp(ARG[0], "int8_t") ||
        !stricmp(ARG[0], "int8") ||
        !stricmp(ARG[0], "u8") ||
        !stricmp(ARG[0], "i8") ||
        !stricmp(ARG[0], "si8") ||
        !stricmp(ARG[0], "ui8") ||
        !stricmp(ARG[0], "ch") ||
        !stricmp(ARG[0], "tch") ||
        !stricmp(ARG[0], "str") ||
        !stricmp(ARG[0], "sz") ||
        !stricmp(ARG[0], "ctstr") ||
        !stricmp(ARG[0], "tstr") ||
        !stricmp(ARG[0], "fchar") ||
        !stricmp(ARG[0], "boole8") ||
        !stricmp(ARG[0], "string") ||
        !stricmp(ARG[0], "zstring") ||
        !stricmp(ARG[0], "binary")
    ) {
        type = BMS_TYPE_BYTE;
    } else if(
        !stricmp(ARG[0], "16") ||
        !stricmp(ARG[0], "16bit") ||
        !stricmp(ARG[0], "word") ||
        !stricmp(ARG[0], "short") ||
        !stricmp(ARG[0], "ushort") ||
        !stricmp(ARG[0], "u_short") ||
        !stricmp(ARG[0], "uint16_t") ||
        !stricmp(ARG[0], "uint16") ||
        !stricmp(ARG[0], "int16_t") ||
        !stricmp(ARG[0], "int16") ||
        !stricmp(ARG[0], "u16") ||
        !stricmp(ARG[0], "i16") ||
        !stricmp(ARG[0], "si16") ||
        !stricmp(ARG[0], "ui16") ||
        !stricmp(ARG[0], "fixed8") ||   // 8.8 = 16
        !stricmp(ARG[0], "wchar") ||
        !stricmp(ARG[0], "wchar_t") ||
        !stricmp(ARG[0], "wch") ||
        !stricmp(ARG[0], "wstr") ||
        !stricmp(ARG[0], "fshort") ||
        !stricmp(ARG[0], "char16") ||
        !stricmp(ARG[0], "string16") ||
        !stricmp(ARG[0], "boole16") ||
        !stricmp(ARG[0], "zstring16")
    ) {
        type = BMS_TYPE_SHORT;
    } else if(
        !stricmp(ARG[0], "32") ||
        !stricmp(ARG[0], "32bit") ||
        !stricmp(ARG[0], "dword") ||
        !stricmp(ARG[0], "unsigned") ||
        !stricmp(ARG[0], "int") ||
        !stricmp(ARG[0], "uint") ||
        !stricmp(ARG[0], "u_int") ||
        !stricmp(ARG[0], "long") ||
        !stricmp(ARG[0], "ulong") ||
        !stricmp(ARG[0], "u_long") ||
        !stricmp(ARG[0], "uint32_t") ||
        !stricmp(ARG[0], "uint32") ||
        !stricmp(ARG[0], "int32_t") ||
        !stricmp(ARG[0], "int32") ||
        !stricmp(ARG[0], "u32") ||
        !stricmp(ARG[0], "i32") ||
        !stricmp(ARG[0], "si32") ||
        !stricmp(ARG[0], "ui32") ||
        !stricmp(ARG[0], "fixed") ||
        !stricmp(ARG[0], "float16") ||  // 16.16 = 32
        !stricmp(ARG[0], "bool") ||
        !stricmp(ARG[0], "boolean") ||
        !stricmp(ARG[0], "boole32") ||
        !stricmp(ARG[0], "void") ||
        !stricmp(ARG[0], "handle") ||
        !stricmp(ARG[0], "flong") ||
        !stricmp(ARG[0], "DOSDateTime") ||
        !stricmp(ARG[0], "UNIXDateTime") ||
        !stricmp(ARG[0], "time_t")
    ) {
        type = BMS_TYPE_LONG;
    } else if(
        !stricmp(ARG[0], "64") ||
        !stricmp(ARG[0], "64bit") ||
        !stricmp(ARG[0], "longlong") ||
        !stricmp(ARG[0], "ulonglong") ||
        !stricmp(ARG[0], "u_longlong") ||
        !stricmp(ARG[0], "uint64_t") ||
        !stricmp(ARG[0], "uint64") ||
        !stricmp(ARG[0], "int64_t") ||
        !stricmp(ARG[0], "int64") ||
        !stricmp(ARG[0], "u64") ||
        !stricmp(ARG[0], "i64") ||
        !stricmp(ARG[0], "si64") ||
        !stricmp(ARG[0], "ui64") ||
        !stricmp(ARG[0], "void64") ||
        !stricmp(ARG[0], "FileTime") ||
        !stricmp(ARG[0], "OLEDateTime") ||
        !stricmp(ARG[0], "SQLDateTime") ||
        !stricmp(ARG[0], "JavaDateTime")
    ) {
        type = BMS_TYPE_LONGLONG;
    } else if(
        !stricmp(ARG[0], "float")
    ) {
        type = BMS_TYPE_FLOAT;
    } else if(
        !stricmp(ARG[0], "double")
    ) {
        type = BMS_TYPE_DOUBLE;
    } else if(
        !stricmp(ARG[0], "encodedu32") ||
        !stricmp(ARG[0], "encoded")
    ) {
        type = BMS_TYPE_VARIABLE;
    } else if(
        !stricmp(ARG[0], "bits") ||
        !stricmp(ARG[0], "sb") ||
        !stricmp(ARG[0], "ub") ||
        !stricmp(ARG[0], "fb")
    ) {
        type = BMS_TYPE_BITS;
    } else {
        type = BMS_TYPE_LONG;

        if(argc == 2) {
            if(mystrchrs(ARG[1], "+-*/%<>^&|!=")) {
                for(i = argc; i >= 0; i--) {
                    ARG[i + 1] = ARG[i];
                }
                argc++;
                ARG[0] = "math";
                goto quit;
            }
        }

        argc = -1; // give an error
        goto quit2;
    }

    if(array < 0) {
        if(type != BMS_TYPE_BYTE) {
            cmd = *ret_cmd;
            CMD.type   = CMD_Do;
            (*ret_cmd)++;

            ARG[0] = put ? "put" : "get";
            // ARG[1] is ok
            ARG[2] = "string";
            cmd = *ret_cmd;
            CMD.type   = CMD_Get;
            CMD.var[0] = add_var(0, ARG[1], NULL, 0, -2);       // varname
            CMD.num[1] = type;                                  // type
            CMD.num[2] = filenum;                               // filenumber
            (*ret_cmd)++;

            cmd = *ret_cmd;
            CMD.type   = CMD_While;
            CMD.var[0] = add_var(0, ARG[1], NULL, 0, -2);       // Varname
            CMD.str[1] = "!=";                                  // Criterium
            CMD.var[2] = add_var_const(0, "0", NULL, 0, -2);    // VarName2
            (*ret_cmd)++;

            ARG[0] = "nop";
            argc = 1;
            goto quit2;
        }

        ARG[0] = put ? "put" : "get";
        // ARG[1] is ok
        ARG[2] = "string";
        argc = 2;
        if(filenum) ARG[++argc] = myitoa(filenum);
        goto quit;
    }

    if(arrayp) {
        for(p = arrayp; *p; p++) {
            if(*p == ']') break;
            if(*p == ',') break;
            if(*p == ')') break;
        }
        *p = 0;
    } else {
        arrayp = "0";
    }

    if(array > 0) {
        if(type == BMS_TYPE_BITS) {
            ARG[0] = put ? "putbits" : "getbits";
            // ARG[1] is ok
            sprintf(tmp, "%.*s", (i32)(sizeof(tmp) - 4), arrayp);
            ARG[2] = tmp;
            argc = 2;
            if(filenum) ARG[++argc] = myitoa(filenum);
            goto quit;
        }
        ARG[0] = put ? "putdstring" : "getdstring";
        // ARG[1] is ok
        switch(type) {
            case BMS_TYPE_BYTE:     sprintf(tmp, "%.*s",   (i32)(sizeof(tmp) - 4), arrayp); break;
            case BMS_TYPE_SHORT:    sprintf(tmp, "%.*s*2", (i32)(sizeof(tmp) - 4), arrayp); break;
            case BMS_TYPE_LONG:     sprintf(tmp, "%.*s*4", (i32)(sizeof(tmp) - 4), arrayp); break;
            case BMS_TYPE_LONGLONG: sprintf(tmp, "%.*s*8", (i32)(sizeof(tmp) - 4), arrayp); break;
            case BMS_TYPE_FLOAT:    sprintf(tmp, "%.*s*4", (i32)(sizeof(tmp) - 4), arrayp); break;
            case BMS_TYPE_DOUBLE:   sprintf(tmp, "%.*s*8", (i32)(sizeof(tmp) - 4), arrayp); break;
            default:                sprintf(tmp, "%.*s*4", (i32)(sizeof(tmp) - 4), arrayp); break;
        }
        ARG[2] = tmp;
        argc = 2;
        if(filenum) ARG[++argc] = myitoa(filenum);
        goto quit;
    }

        ARG[0] = put ? "put" : "get";
        // ARG[1] is ok
        switch(type) {
            case BMS_TYPE_BYTE:     sprintf(tmp, "byte");       break;
            case BMS_TYPE_SHORT:    sprintf(tmp, "short");      break;
            case BMS_TYPE_LONG:     sprintf(tmp, "long");       break;
            case BMS_TYPE_LONGLONG: sprintf(tmp, "longlong");   break;
            case BMS_TYPE_FLOAT:    sprintf(tmp, "float");      break;
            case BMS_TYPE_DOUBLE:   sprintf(tmp, "double");     break;
            case BMS_TYPE_VARIABLE: sprintf(tmp, "variable");   break;
            default:                sprintf(tmp, "long");       break;
        }
        ARG[2] = tmp;
        argc = 2;
        if(filenum) ARG[++argc] = myitoa(filenum);
        goto quit;

quit:
    if(argc < 0) goto quit2;

    ARG[argc + 1] = "";
    if(put) {
        cmd = *ret_cmd;
        CMD.type   = CMD_Set;
        CMD.var[0] = add_var(0, ARG[1], NULL, 0, -2);       // VarName
        CMD.num[1] = add_datatype(ARG[2]);                  // datatype
        CMD.var[2] = add_var(0, put, NULL, 0, -2);          // Var/Number
        (*ret_cmd)++;
    }
quit2:
    // argument[] must be allocated
    for(i = 0; i < MAX_ARGS; i++) {
        if(argument[i] != bck_argument[i]) {
            // first strdup and then free!
            if(argument[i]) argument[i] = mystrdup_simple(argument[i]);
            // the following is correct: j=i, xalloc and ARG[i+1]=ARG[i]
            int j;
            for(j = i; j < MAX_ARGS; j++) {
                if(argument[j] == bck_argument[i]) break;
            }
            if(j >= MAX_ARGS) {
                FREE(bck_argument[i]);
            }
        }
    }
    return(argc);
}



int quickbmsver(u8 *version) {
    int     n,
            len,
            ret,
            seq,
            plen;
    u8      *p,
            *l,
            *filter_in_files_tmp = NULL;

    if(!version) return 0;
    version = skip_delimit(version);
    ret = 0;
    seq = 24;
    plen = strlen(version); // just in case
    for(p = version; *p && (p < (version + plen)); p += len) {
        len = 1;
        if(*p == '.') {
            seq -= 8;
            //if(seq < 0) break;
        } else if(strchr(" \t\r\n,;|(){}[]", *p)) {
            // do nothing
        } else if(strchr("-/", *p)) {
            if((p[1] == '6') && (p[2] == '4')) {    // "64"
                #ifdef QUICKBMS64
                #else
                    fprintf(stderr,
                        "\n\nError: the script requires quickbms_4gb_files.exe\n\n");
                    myexit(QUICKBMS_ERROR_BMS);
                #endif
                len += 2;
            } else if((p[1] == '3') && (p[2] == '2')) { // "32"
                #ifdef QUICKBMS64
                    fprintf(stderr,
                        "\n\nError: the script requires quickbms.exe\n\n");
                    myexit(QUICKBMS_ERROR_BMS);
                #endif
                len += 2;
            } else {
                switch(p[1]) {
                    case '9':
                        if(XDBG_ALLOC_ACTIVE) {
                            fprintf(stderr,
                                "\n"
                                "- the script requires the disabling of the secure allocation XDBG_ALLOC_ACTIVE\n"
                                "  this operation may have negative security effects so consider it only if the\n"
                                "  script comes from a trusted source.\n"
                                "  do you want to disable XDBG_ALLOC_ACTIVE? (y/N)\n"
                                "  ");
                            if(get_yesno(NULL) == 'y') xdbg_toggle();
                        }
                        break;
                    case 'I': g_insensitive             = 0;    break;  // when case sensitive makes the difference
                    case '.': g_continue_anyway         = 1;    break;  // useful in reimport mode with header builders
                    case 'N': g_decimal_names           = 1;    break;  // 00000000.dat -> 0.dat
                    case 'q': g_quiet                   = 1;    break;  // maybe useful
                    case 'T': g_keep_temporary_file     = 1;    break;  // in some rare scripts may be helpful to keep the temporary file
                    case 'd': g_quickbms_outname        = 1;    break;
                    case 'D': g_quickbms_outname        = -1;   break;
                    case 'e': g_ignore_comp_errors      = 1;    break;  // now useful in reimport mode
                    case 'J': g_force_cstring           = 1;    break;
                    case 'F':
                        p += 2; // -F
                        while(*p && (*p <= ' ')) p++;
                        append_list(&filter_in_files_tmp, p);
                        len = -1; /*lame break, this is experimental since , and ; are already handled as separators*/
                        break;
                    case 'x': g_decimal_notation        = 0;    break;
                    case 'j': g_force_utf16             = -1;   break;
                    case 'b':
                        for(l = p; *l && (*l <= ' '); l++);
                        g_log_filler_char         = ((l - p) == 1) ? p[0] : myatoi(p);
                        p = l;
                        break;
                    // no need of -P for codepage since there is a command for that

                    // experimental, it's not like the -c in command-line
                    case 'c': g_c_structs_allowed       = 1;    break;

                    default: len--; /* to fix the next len++ */ break;
                }
                if(len < 0) break;
                len++;
            }
        } else if(((*p >= 'a') && (*p <= 'z')) || ((*p >= 'A') && (*p <= 'Z'))) {
            if(seq >= 0) {
                ret += *p;
            }
        } else {
            n = readbase(p, 10, &len);
            if(len <= 0) break;
            if(seq >= 0) {
                ret += n << seq;
            }
        }
        if(len < 0) break;  // just in case...
    }
    if(filter_in_files_tmp) {
        build_filter(&g_filter_in_files, filter_in_files_tmp);
        FREE(filter_in_files_tmp)
    }
    return ret;
}



int set_uint_flex(int cmd, u8 *name, u8 *arg) {
    int     i,
            len;
    u8      max = 0,
            min = 0xff,
            mini[32],
            *tmp;

    tmp = numbers_to_bytes(arg, &len, 0, 0);
    for(i = 0; i < len; i++) {
        if(tmp[i] < min) min = tmp[i];
        if(tmp[i] > max) max = tmp[i];
    }
    max++;

    if(min) {
        sprintf(mini, "%u", min);
        CMD.type   = CMD_GetBits;
        CMD.var[0] = add_var(0, QUICKBMS_DUMMY, NULL, 0, -2); // varname
        CMD.var[1] = add_var(0, mini, NULL, 0, -2);         // bits
        CMD.num[2] = 0;                                     // filenumber
        cmd++;
        check_g_command(cmd);
    }

    sprintf(mini, "%u", max - min);
    CMD.type   = CMD_GetBits;
    CMD.var[0] = add_var(0, name, NULL, 0, -2);         // varname
    CMD.var[1] = add_var(0, mini, NULL, 0, -2);         // bits
    CMD.num[2] = 0;                                     // filenumber

    if(max < 32) {
        cmd++;
        check_g_command(cmd);
        sprintf(mini, "%u", 32 - max);
        CMD.type   = CMD_GetBits;
        CMD.var[0] = add_var(0, QUICKBMS_DUMMY, NULL, 0, -2); // varname
        CMD.var[1] = add_var(0, mini, NULL, 0, -2);         // bits
        CMD.num[2] = 0;                                     // filenumber
    }

    return(cmd);
}



int bms_get_endian(u8 *arg) {
    if(arg) {
        if(!stricmp(arg, "little") || !stricmp(arg, "intel") || !stricmp(arg, "4321") || strstr(arg, "44332211")) {
            return MYLITTLE_ENDIAN;
        }
        if(!stricmp(arg, "big") || !stricmp(arg, "network")  || !stricmp(arg, "1234") || strstr(arg, "11223344")) {
            return MYBIG_ENDIAN;
        }
    }
    return -1;
}



void set_cmd_args_ptr(int cmd, int idx, u8 *arg) {
    CMD.num[idx] = 0;                       // &var disabled
    if(!arg) return;
    if((arg[0] == '&') || (arg[0] == '*')) {
        CMD.num[idx] = 1;                   // &var enabled
        arg++;
    }
    if(is_MEMORY_FILE(arg)) {
        CMD.var[idx] = get_memory_file(arg);
    } else {
        CMD.var[idx] = add_var(0, arg, NULL, 0, -2);
    }
}



int set_cmd_args(int cmd, int argc, u8 *arg[MAX_ARGS + 1], int argx) {
    int     i;

    if(argc > argx) {
        CMD.num[0] = argc - argx;                           // number of arguments
        for(i = argx + 1; i <= argc; i++) {
            set_cmd_args_ptr(cmd, i - 1, arg[i]);
        }
    }
    return(cmd);
}



int set_cmd_string_op(u8 *str) {
    int     op;
    int     is_negative = 0;

    if(str[0] == '0') {
        is_negative = 1;
        str++;
    }

    op = str[0];

         if(!stricmp(str, "equal"))     op = '=';
    else if(!stricmp(str, "copy"))      op = '=';
    else if(!stricmp(str, "append"))    op = '+';
    else if(!stricmp(str, "add"))       op = '+';
    else if(!stricmp(str, "truncate"))  op = '-';
    else if(!stricmp(str, "remove"))    op = '-';
    else if(!stricmp(str, "xor"))       op = '^';
    else if(!stricmp(str, "shl"))       op = '<';
    else if(!stricmp(str, "shift_left")) op = '<';
    else if(!stricmp(str, "mod"))       op = '%';
    else if(!stricmp(str, "shr"))       op = '>';
    else if(!stricmp(str, "shift_right")) op = '>';
    else if(!stricmp(str, "hex"))       op = 'b';   // to hex
    else if(!stricmp(str, "byte2hex"))  op = 'b';
    else if(!stricmp(str, "byte2hex_string"))  op = 'B';
    else if(!stricmp(str, "byte"))      op = 'h';   // to byte
    else if(!stricmp(str, "hex2byte"))  op = 'h';
    else if(!stricmp(str, "encrypt"))   op = 'e';
    else if(!stricmp(str, "encryption"))op = 'e';
    else if(!stricmp(str, "encrypt_string"))   op = 'E';
    else if(!stricmp(str, "compress"))  op = 'c';
    else if(!stricmp(str, "compression")) op = 'c';
    else if(!stricmp(str, "comtype"))   op = 'c';
    else if(!stricmp(str, "compress_string"))  op = 'C';
    else if(!stricmp(str, "upper"))     op = 'u';
    else if(!stricmp(str, "toupper"))   op = 'u';
    else if(!stricmp(str, "lower"))     op = 'l';
    else if(!stricmp(str, "tolower"))   op = 'l';
    else if(!stricmp(str, "reverse"))   op = 'r';
    else if(!stricmp(str, "replace"))   op = 'R';
    else if(!stricmp(str, "cstring"))   op = 'x';
    else if(!stricmp(str, "filter"))    op = 'f';
    else if(!stricmp(str, "math"))      op = 'm';
    else if(!stricmp(str, "xmath"))     op = 'm';
    else if(!stricmp(str, "split"))     op = 'S';

    else if(!stricmp(str, "printf"))    op = 'p';
    else if(!stricmp(str, "sprintf"))   op = 'p';
    else if(!stricmp(str, "scanf"))     op = 's';
    else if(!stricmp(str, "sscanf"))    op = 's';

    else if(!stricmp(str, "strchr"))    op = '&';
    else if(!stricmp(str, "strstr"))    op = '&';
    else if(!stricmp(str, "strichr"))   op = '&';
    else if(!stricmp(str, "stristr"))   op = '&';
    else if(!stricmp(str, "strchrx"))   op = '|';
    else if(!stricmp(str, "strstrx"))   op = '|';
    else if(!stricmp(str, "strichrx"))  op = '|';
    else if(!stricmp(str, "stristrx"))  op = '|';
    else if(!stricmp(str, "strchr1"))   op = '|';
    else if(!stricmp(str, "strstr1"))   op = '|';
    else if(!stricmp(str, "strichr1"))  op = '|';
    else if(!stricmp(str, "stristr1"))  op = '|';

    else if(!stricmp(str, "strrchr"))   op = '$';
    else if(!stricmp(str, "strrstr"))   op = '$';
    else if(!stricmp(str, "strrichr"))  op = '$';
    else if(!stricmp(str, "strristr"))  op = '$';
    else if(!stricmp(str, "strrchrx"))  op = '!';
    else if(!stricmp(str, "strrstrx"))  op = '!';
    else if(!stricmp(str, "strrichrx")) op = '!';
    else if(!stricmp(str, "strristrx")) op = '!';
    else if(!stricmp(str, "strrchr1"))  op = '!';
    else if(!stricmp(str, "strrstr1"))  op = '!';
    else if(!stricmp(str, "strrichr1")) op = '!';
    else if(!stricmp(str, "strristr1")) op = '!';

    else if(!stricmp(str, "mult"))      op = '*';
    else if(!stricmp(str, "replicate")) op = '*';

    else if(!stricmp(str, "hex2uri"))   op = 'w';

    else if(!stricmp(str, "uri2hex"))   op = 'W';

    else if(!stricmp(str, "parser"))    op = 'X';
    else if(!stricmp(str, "xml"))       op = 'X';

    if(is_negative) op = -op;

    return(op);
}



#define CMD_FileXOR_bms \
    CMD.num[0] = 0; /* used to contain the size of str[0], improves the performances */ \
    if(myisdigit(ARG[1][0]) || (ARG[1][0] == '\\')) { \
        NUMS2BYTES(ARG[1], CMD.num[1], CMD.str[0], CMD.num[0], 0)  /* acts like a realloc */ \
    } else { \
        CMD.var[0] = parse_bms_add_var(1);              /* string */ \
    } \
    CMD.num[2] = 0;                                     /* reset pos */ \
    if(argc == 1) { \
        CMD.num[4] = -(MAX_FILES + 1);                  /* all the files, DO NOT use MAX_FILES+1 because _FILEZ() will corrupt it */ \
    } else { \
        CMD.var[3] = parse_bms_add_var(2);              /* first position offset (used only for Log and multiple bytes in rare occasions) */ \
        CMD.num[4] = myatoifile(ARG[3]);                /* filenumber (not implemented) */ \
    }



int parse_bms(FILE *fds, u8 *inputs, int cmd, int eol_mode) {
    FILE    *include_fd;
    int     i,
            t,
            argc,
            c_structs_do,
            is_append   = 0,    // only for reimport, not nested ("include")
            is_append_hashing_necessary = 0,
            include_workaround = 0;
    u8      //*debug_line = NULL,
            *argument[MAX_ARGS + 1] = { NULL },
            *tmp,
            is_const[MAX_ARGS + 1]; // currently this is not used, the idea is only to alert users if
                                    // they specify a constant string like "test" and a variable named TEST
                                    // ... maybe in future

    // the following "lame" solution is used to consider the "values" (between quotes) as constants,
    // it has been adopted to make no changes to the original code.
    // the trick works by assigning g_lame_add_var_const_workaround and passing 0 as first argument of add_var
#define parse_bms_add_var(X) \
    add_var( \
        (g_lame_add_var_const_workaround = is_const[X]) & 0, \
        ARG[X], NULL, 0, -2)

    c_structs(NULL, 0, NULL);

    for(;;) {       // do NOT use "continue;"!
        check_g_command(cmd);

        // never do memset because commands are initialized in a particular way (check bms_init)
        //memset(&g_command[cmd], 0, sizeof(command_t));

        argc = bms_line(fds, &inputs, argument, cmd, is_const, eol_mode); //&debug_line);
        if(argc < 0) break; // means "end of file"
        if(!argc) continue; // means "no command", here is possible to use "continue"

        argc--; // remove command argument
        // remember that myatoi is used only for the file number, all the rest must be add_var

        c_structs_do = 1;
redo:
               if(!stricmp(ARG[0], "QuickBMSver")   && (argc >= 1)) {
            CMD.type   = CMD_NOP;
            t = quickbmsver(ARG[1]);
            if(t && (t > g_quickbms_version)) {
                fprintf(stderr, "\n"
                    "Error: this script has been created for a newer version of QuickBMS:\n"
                    "         expected %08x - %s\n"
                    "         yours    %08x - %s\n"
                    "       you can download it from:\n"
                    "\n"
                    "         http://aluigi.org/quickbms\n"
                    "\n",
                    (i32)t, ARG[1],
                    (i32)g_quickbms_version, VER
                );
                myexit(QUICKBMS_ERROR_BMS);
            }

        } else if(!stricmp(ARG[0], "CLog")          && (argc >= 4)) {
            CMD.type   = CMD_CLog;
            CMD.var[0] = parse_bms_add_var(1);                  // name
            CMD.var[1] = parse_bms_add_var(2);                  // offset
            CMD.var[2] = parse_bms_add_var(3);                  // compressed size
            CMD.var[3] = -1;                                    // offsetoffset
            CMD.var[4] = -1;                                    // resourcesizeoffset
            CMD.var[6] = -1;                                    // uncompressedsizeoffset
            if(argc >= 8) { // old useless MexScript
                CMD.var[5] = parse_bms_add_var(6);              // uncompressedsize
                CMD.num[7] = myatoifile(ARG[8]);                // filenumber
                if(argc >= 9) CMD.var[8] = parse_bms_add_var(9);// xsize
            } else {
                CMD.var[5] = parse_bms_add_var(4);              // uncompressedsize
                CMD.num[7] = myatoifile(ARG[5]);                // filenumber
                if(argc >= 6) CMD.var[8] = parse_bms_add_var(6);// xsize
            }

        } else if(!stricmp(ARG[0], "SLog")          && (argc >= 3)) {
            CMD.type   = CMD_SLog;
            CMD.var[0] = parse_bms_add_var(1);                  // name
            CMD.var[1] = parse_bms_add_var(2);                  // offset
            CMD.var[2] = parse_bms_add_var(3);                  // size
            if(argc >= 4) {
                CMD.num[3] = add_datatype(ARG[4]);              // type
            } else {
                CMD.num[3] = add_datatype("string");
            }
            CMD.num[4] = myatoifile(ARG[5]);                    // filenumber
            if(argc >= 6) {
                CMD.var[5] = parse_bms_add_var(6);              // tag
            }

        } else if(
                 (!stricmp(ARG[0], "Do")            && (argc >= 0))
              || (!stricmp(ARG[0], "Loop")          && (argc >= 0))) {  // mex inifile (not BMS)
            CMD.type   = CMD_Do;

        } else if(!stricmp(ARG[0], "FindLoc")       && (argc >= 3)) {
            CMD.type   = CMD_FindLoc;
            CMD.var[0] = parse_bms_add_var(1);                  // var
            CMD.num[1] = add_datatype(ARG[2]);                  // datatype
            if(CMD.num[1] == BMS_TYPE_REGEX) {                  // regex is just the string as-is
                mystrdup(&CMD.str[2], ARG[3]);
                CMD.num[2] = strlen(CMD.str[2]);
            } else {
                CSTRING(2, ARG[3])                              // text/number
            }
            if(argc >= 4) {
                if(!ARG[4][0]) {    // a typical mistake that I do too!
                    CMD.num[3] = 0;
                    mystrdup(&CMD.str[4], ARG[4]);
                } else {
                    CMD.num[3] = myatoifile(ARG[4]);            // filenumber
                    mystrdup(&CMD.str[4], ARG[5]);              // optional/experimental: the value you want to return in case the string is not found
                }
                if(argc >= 6) {
                    CMD.var[5] = parse_bms_add_var(6);
                }
            } else {
                CMD.num[3] = 0;                                 // filenumber
                FREE(CMD.str[4])                                // optional/experimental: the value you want to return in case the string is not found
            }

        } else if(!stricmp(ARG[0], "FindFileID")    && (argc >= 2)) {   // mex inifile (not BMS)
            CMD.type   = CMD_FindLoc;
            CMD.var[0] = parse_bms_add_var(2);                  // var
            CMD.num[1] = add_datatype("String");                // datatype
            mystrdup(&CMD.str[2], ARG[1]);                      // text/number
            CMD.num[3] = myatoifile(ARG[3]);                    // filenumber

        } else if(!stricmp(ARG[0], "For")           && (argc >= 0)) {
            if(argc >= 3) {
                CMD.type   = CMD_Math;
                CMD.var[0] = parse_bms_add_var(1);              // VarName
                CMD.num[1] = ARG[2][0];                         // operation
                CMD.var[2] = parse_bms_add_var(3);              // Var/Number
                cmd++;
                check_g_command(cmd);
            }

            CMD.type   = CMD_For;   // yes, no arguments, this is the new way

            if(argc >= 5) {
                cmd++;
                check_g_command(cmd);
                CMD.type   = CMD_ForTo;
                CMD.var[0] = parse_bms_add_var(1);              // T
                                                                // = T_value (check later, it must be check_condition compatible)
                if(!stricmp(ARG[4], "To")) {                    // To
                    mystrdup(&CMD.str[1], "<=");
                } else {
                    mystrdup(&CMD.str[1], ARG[4]);
                }
                CMD.var[2] = parse_bms_add_var(5);              // To_value
                //CMD.var[3] = parse_bms_add_var(3);            // T_value (not used)
            }

        } else if(!stricmp(ARG[0], "Get")           && (argc >= 2)) {
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype(ARG[2]);                  // type
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber
            if(argc >= 4) {
                CMD.var[3] = parse_bms_add_var(4);
            }

        } else if(!stricmp(ARG[0], "GetBits")       && (argc >= 2)) {
            CMD.type   = CMD_GetBits;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.var[1] = parse_bms_add_var(2);                  // bits
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(!stricmp(ARG[0], "PutBits")       && (argc >= 2)) {
            CMD.type   = CMD_PutBits;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.var[1] = parse_bms_add_var(2);                  // bits
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(!stricmp(ARG[0], "Put")           && (argc >= 2)) {   // write mode
            CMD.type   = CMD_Put;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            //set_cmd_args_ptr(cmd, 0, ARG[1]);                   // varname ('&' is currently unused)
            CMD.num[1] = add_datatype(ARG[2]);                  // type
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(!stricmp(ARG[0], "GetLong")       && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype("Long");                  // type
            CMD.num[2] = myatoifile(ARG[2]);                    // filenumber

        } else if(!stricmp(ARG[0], "GetInt")        && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype("Int");                   // type
            CMD.num[2] = myatoifile(ARG[2]);                    // filenumber

        } else if(!stricmp(ARG[0], "GetByte")       && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype("Byte");                  // type
            CMD.num[2] = myatoifile(ARG[2]);                    // filenumber

        } else if(!stricmp(ARG[0], "GetString")     && (argc >= 2)) {   // mex inifile (not BMS)
            CMD.type   = CMD_GetDString;
            CMD.var[0] = parse_bms_add_var(2);                  // varname
            CMD.var[1] = parse_bms_add_var(1);                  // NumberOfCharacters
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(!stricmp(ARG[0], "GetNullString") && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype("String");                // type
            CMD.num[2] = myatoifile(ARG[2]);                    // filenumber

        } else if(
                  (!stricmp(ARG[0], "GetDString")    && (argc >= 2))
               || (!stricmp(ARG[0], "fread")         && (argc >= 2))) {
            CMD.type   = CMD_GetDString;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.var[1] = parse_bms_add_var(2);                  // NumberOfCharacters
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(
                  (!stricmp(ARG[0], "PutDString")    && (argc >= 2))
               || (!stricmp(ARG[0], "fwrite")        && (argc >= 2))) {   // write mode
            CMD.type   = CMD_PutDString;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.var[1] = parse_bms_add_var(2);                  // NumberOfCharacters
            CMD.num[2] = myatoifile(ARG[3]);                    // filenumber

        } else if(
                  (!stricmp(ARG[0], "GoTo")          && (argc >= 1))
               || (!stricmp(ARG[0], "fseek")         && (argc >= 1))) {
            CMD.type   = CMD_GoTo;
            CMD.var[0] = parse_bms_add_var(1);                  // pos
            CMD.num[1] = myatoifile(ARG[2]);                    // file
            CMD.num[2] = SEEK_SET;
            if(argc >= 3) {
                     if(stristr(ARG[3], "SET")) CMD.num[2] = SEEK_SET;
                else if(stristr(ARG[3], "CUR")) CMD.num[2] = SEEK_CUR;
                else if(stristr(ARG[3], "END")) CMD.num[2] = SEEK_END;
            }

        } else if(
                  (!stricmp(ARG[0], "IDString")     && (argc >= 1))
               || (!stricmp(ARG[0], "ID")           && (argc >= 1))     // mex inifile (not BMS)
               || (!stricmp(ARG[0], "memcmp")       && (argc >= 1))
               || (!stricmp(ARG[0], "strcmp")       && (argc >= 1))) {
            CMD.type   = CMD_IDString;
            if(argc == 1) {
                CMD.num[0] = 0;
                CSTRING(1, ARG[1])                              // string
            } else {
                CMD.num[0] = myatoifile(ARG[1]);                // filenumber
                /* //no longer works from quickbms 0.8.3 where filenumber can be a variable
                if(CMD.num[0] == MAX_FILES) {                   // simple work-around to avoid the different syntax of idstring
                    CSTRING(1, ARG[1])                          // string
                    CMD.num[0] = myatoifile(ARG[2]);
                } else*/ {
                    CSTRING(1, ARG[2])                          // string
                    // CMD.num[0] = myatoifile(ARG[1]); // already set
                }
            }

        } else if(!strnicmp(ARG[0], "ID=", 3)       && (argc >= 0)) {   // mex inifile (not BMS)
            CMD.type   = CMD_IDString;
            CMD.num[0] = 0;
            mystrdup(&CMD.str[1], ARG[0] + 3);                  // bytes

        } else if(!stricmp(ARG[0], "ImpType")       && (argc >= 1)) {
            CMD.type   = CMD_ImpType;
            mystrdup(&CMD.str[0], ARG[1]);                      // type

        } else if(!stricmp(ARG[0], "Log")           && (argc >= 3)) {
            CMD.type   = CMD_Log;
            CMD.var[0] = parse_bms_add_var(1);                  // name
            CMD.var[1] = parse_bms_add_var(2);                  // offset
            CMD.var[2] = parse_bms_add_var(3);                  // size
            CMD.var[3] = -1;                                    // offsetoffset
            CMD.var[4] = -1;                                    // resourcesizeoffset
            if(argc >= 6) { // old useless MexScript
                CMD.num[5] = myatoifile(ARG[6]);                // filenumber
                if(argc >= 7) CMD.var[6] = parse_bms_add_var(7);// xsize
            } else {
                CMD.num[5] = myatoifile(ARG[4]);                // filenumber
                if(argc >= 5) CMD.var[6] = parse_bms_add_var(5);// xsize
            }

            // work-around for reimporting of memory files
            if((CMD.num[5] < 0) && !stricmp(ARG[2], "0")) {
                if(
                    //g_script_uses_append && // ttgames.bms! where is used a function to handle chunks
                    !is_append
                ) {
                    // yes, only the latest in case of multiple "log NAME 0 SIZE MEMORY_FILE"... this is a work-around!
                    g_memfile_reimport_name = CMD.var[0];
                }
            }

        } else if(!stricmp(ARG[0], "ExtractFile")   && (argc >= 0)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Log;
            CMD.var[0] = add_var(0, "FILENAME", NULL, 0, -2);   //  name
            CMD.var[1] = add_var(0, "FILEOFF",  NULL, 0,  0);   // offset
            CMD.var[2] = add_var(0, "FILESIZE", NULL, 0, -2);   // size
            CMD.var[3] = -1;                                    // offsetoffset
            CMD.var[4] = -1;                                    // resourcesizeoffset
            CMD.num[5] = myatoifile(ARG[6]);                    // filenumber

        } else if(!stricmp(ARG[0], "Math")          && (argc >= 3)) {
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = set_math_operator(ARG[2], &CMD.num[2], NULL);  // op
            if(!stricmp(ARG[2], "long")) CMD.num[1] = '=';      // a stupid error that can happen
            CMD.var[2] = parse_bms_add_var(3);                  // var2

        } else if(!stricmp(ARG[0], "XMath")         && (argc >= 2)) {
            CMD.type   = CMD_XMath;
            CMD.var[0] = parse_bms_add_var(1);                  // output
            mystrdup(&CMD.str[1], ARG[2]);                      // input instructions, "1 + 2"

        } else if(!stricmp(ARG[0], "NameCRC")      && (argc >= 2)) {
            CMD.type   = CMD_NameCRC;
            CMD.var[0] = parse_bms_add_var(1);                  // output name
            CMD.var[1] = parse_bms_add_var(2);                  // input value
            if(argc >= 3) CMD.var[2] = parse_bms_add_var(3);    // names file
            if(argc >= 4) CMD.var[3] = parse_bms_add_var(4);    // type
            if(argc >= 5) CSTRING(4, ARG[5])                    // key
            if(argc >= 6) CSTRING(5, ARG[6])                    // ivec

        } else if(!stricmp(ARG[0], "Codepage")       && (argc >= 1)) {
            CMD.type   = CMD_Codepage;
            CMD.var[0] = parse_bms_add_var(1);                  // codepage

        } else if(!stricmp(ARG[0], "Add")           && (argc >= 3)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '+';                                   // op (skip specifier!)
            CMD.var[2] = parse_bms_add_var(3);                  // var2

        } else if(!stricmp(ARG[0], "Inc")           && (argc >= 1)) {
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '+';                                   // op (skip specifier!)
            CMD.var[2] = add_var(0, "1", NULL, 0, -2);          // var2

        } else if(!stricmp(ARG[0], "Subst")         && (argc >= 3)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '-';                                   // op (skip specifier!)
            CMD.var[2] = parse_bms_add_var(3);                  // var2

        } else if(!stricmp(ARG[0], "Dec")           && (argc >= 1)) {
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '-';                                   // op (skip specifier!)
            CMD.var[2] = add_var(0, "1", NULL, 0, -2);          // var2

        } else if(!stricmp(ARG[0], "Multiply")      && (argc >= 5)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Set;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName
            CMD.num[1] = add_datatype("String");                // datatype
            CMD.var[2] = parse_bms_add_var(3);                  // Var/Number
            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '*';                                   // op
            CMD.var[2] = parse_bms_add_var(5);                  // var2

        } else if(!stricmp(ARG[0], "Divide")        && (argc >= 5)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Set;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName
            CMD.num[1] = add_datatype("String");                // datatype
            CMD.var[2] = parse_bms_add_var(3);                  // Var/Number
            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '/';                                   // op
            CMD.var[2] = parse_bms_add_var(5);                  // var2

        } else if(!stricmp(ARG[0], "Up")            && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '+';                                   // op (skip specifier!)
            CMD.var[2] = add_var(0, "1", NULL, 0, -2);          // var2

        } else if(!stricmp(ARG[0], "Down")          && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Math;
            CMD.var[0] = parse_bms_add_var(1);                  // var1
            CMD.num[1] = '-';                                   // op (skip specifier!)
            CMD.var[2] = add_var(0, "1", NULL, 0, -2);          // var2

        } else if(!stricmp(ARG[0], "Next")          && (argc >= 0)) {
            CMD.type   = CMD_Next;
            if(!argc) {
                CMD.var[0] = -1;
            } else {
                CMD.var[0] = parse_bms_add_var(1);              // VarName
                if(argc >= 2) {
                    CMD.num[1] = set_math_operator(ARG[2], &CMD.num[2], NULL);
                    CMD.var[2] = parse_bms_add_var(3);
                }
            }

        } else if(!stricmp(ARG[0], "Prev")          && (argc >= 0)) {
            CMD.type   = CMD_Prev;
            if(!argc) {
                CMD.var[0] = -1;
            } else {
                CMD.var[0] = parse_bms_add_var(1);              // VarName
            }

        } else if(!stricmp(ARG[0], "Continue")      && (argc >= 0)) {
            CMD.type   = CMD_Continue;
            if(argc >= 1) {
                mystrdup(&CMD.str[0], ARG[1]);
            } else {
                FREE(CMD.str[0])
            }

        } else if(!stricmp(ARG[0], "Open")          && (argc >= 1)) {   // now 1 instead of 2 due to the new feature
            CMD.type   = CMD_Open;
            CMD.var[0] = parse_bms_add_var(1);                  // Folder/Specifier
            if(argc >= 2) {
                CMD.var[1] = parse_bms_add_var(2);              // Filename/Extension
                CMD.num[2] = myatoifile(ARG[3]);                // File (default is 0, the same file)
                CMD.var[3] = parse_bms_add_var(4);              // optional/experimental: this var will be 1 if exists otherwise 0
            }

        } else if(
                  (!stricmp(ARG[0], "SavePos")       && (argc >= 1))
               || (!stricmp(ARG[0], "ftell")         && (argc >= 1))) {
            CMD.type   = CMD_SavePos;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName
            CMD.num[1] = myatoifile(ARG[2]);                    // File

        } else if(!stricmp(ARG[0], "Set")           && (argc >= 2)) {
            CMD.type   = CMD_Set;
            //CMD.var[0] = parse_bms_add_var(1);                // VarName
            if(is_MEMORY_FILE(ARG[1])) {
                CMD.var[0] = get_memory_file(ARG[1]);
            } else {
                CMD.var[0] = parse_bms_add_var(1);
            }
            if(argc == 2) {
                CMD.num[1] = add_datatype("String");            // datatype
                //CMD.var[2] = parse_bms_add_var(2);            // Var/Number
                tmp = ARG[2];
                g_lame_add_var_const_workaround = is_const[2];
            } else {
                if(ARG[2][0] == '=') {
                    CMD.num[1] = add_datatype("String");
                } else if(!stricmp(ARG[2], "strlen")) { // I'm crazy
                    CMD.type   = CMD_Strlen;
                    CMD.var[0] = parse_bms_add_var(1);          // dest var
                    CMD.var[1] = parse_bms_add_var(3);          // string
                    CMD.num[2] = 0;
                } else {
                    CMD.num[1] = add_datatype(ARG[2]);          // datatype
                }
                //CMD.var[2] = parse_bms_add_var(3);            // Var/Number
                tmp = ARG[3];
                g_lame_add_var_const_workaround = is_const[3];
            }
            if(CMD.num[1] == BMS_TYPE_BINARY) {
                CSTRING(2, tmp)
            } else if(CMD.num[1] == BMS_TYPE_COMPRESSED) {
                CMD.num[1] = BMS_TYPE_BINARY;
                FREE(CMD.str[2])
                CMD.str[2] = type_decompress(tmp, &CMD.num[2]);
            } else if(CMD.num[1] == BMS_TYPE_TCC) {
                CMD.num[1] = BMS_TYPE_BINARY;
                FREE(CMD.str[2])

                external_executable_prompt(cmd, ARG[1], 0);
                if(TCC_libtcc_init() < 0) myexit(QUICKBMS_ERROR_BMS);
                TCCState *tccstate = tcc_compiler(tmp);
                CMD.num[2] = tcc_relocate(tccstate, NULL);
                if(CMD.num[2] < 0) myexit(QUICKBMS_ERROR_BMS);
                CMD.str[2] = malloc(CMD.num[2]);
                /*DO NOT UNCOMMENT OR WILL RETURN 0!!! CMD.num[2] =*/ tcc_relocate(tccstate, CMD.str[2]);
                tcc_delete(tccstate);
            } else {
                CMD.var[2] = add_var(0, tmp, NULL, 0, -2);
            }
            g_lame_add_var_const_workaround = 0;

        } else if(!stricmp(ARG[0], "SETFILECNT")    && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Set;
            CMD.var[0] = add_var(0, "FILECNT", NULL, 0, -2);    // VarName
            CMD.num[1] = add_datatype("String");                // datatype
            CMD.var[2] = parse_bms_add_var(1);                  // Var/Number

        } else if(!stricmp(ARG[0], "SETBYTESREAD")  && (argc >= 1)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Set;
            CMD.var[0] = add_var(0, "BYTESREAD", NULL, 0, -2);  // VarName
            CMD.num[1] = add_datatype("String");                // datatype
            CMD.var[2] = parse_bms_add_var(1);                  // Var/Number

        } else if(!stricmp(ARG[0], "While")         && (argc >= 3)) {
            CMD.type   = CMD_While;
            /*
            CMD.var[0] = parse_bms_add_var(1);                  // Varname
            mystrdup(&CMD.str[1], ARG[2]);                      // Criterium
            CMD.var[2] = parse_bms_add_var(3);                  // VarName2
            */
            // copy from CMD_If
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], ARG[2]);                      // Criterium
            CMD.var[2] = parse_bms_add_var(3);                  // VarName2
            for(i = 4; i <= argc; i += 4) {
                mystrdup(&CMD.str[(i-1)], ARG[i]);              // and/or
                CMD.var[(i-1)+1] = parse_bms_add_var(i+1);      // VarName1
                mystrdup(&CMD.str[(i-1)+2], ARG[i+2]);          // Criterium
                CMD.var[(i-1)+3] = parse_bms_add_var(i+3);      // VarName2
            }

        } else if(!stricmp(ARG[0], "EndLoop")       && (argc >= 2)) {   // mex inifile (not BMS)
            CMD.type   = CMD_While;
            CMD.var[0] = parse_bms_add_var(1);                  // Varname
            mystrdup(&CMD.str[1], "!=");                        // Criterium
            CMD.var[2] = parse_bms_add_var(2);                  // VarName2

        } else if(!stricmp(ARG[0], "String")        && (argc >= 3)) {
            CMD.type   = CMD_String;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            CMD.num[1] = set_cmd_string_op(ARG[2]); /* NO tolower! */ // op
            CMD.var[2] = parse_bms_add_var(3);                  // VarName2
            CMD.num[0] = argc - 3;
            for(i = 4; i <= argc; i++) {
                //if(is_MEMORY_FILE(ARG[i])) {
                    //CMD.var[i - 1] = get_memory_file(ARG[i]);
                //} else {
                    CMD.var[i - 1] = parse_bms_add_var(i);
                //}
            }

        } else if(!stricmp(ARG[0], "CleanExit")     && (argc >= 0)) {
            CMD.type   = CMD_CleanExit;

        } else if(!stricmp(ARG[0], "Exit")          && (argc >= 0)) {
            CMD.type   = CMD_CleanExit;

        } else if(!stricmp(ARG[0], "ExitIfNoFilesOpen") && (argc >= 0)) {
            CMD.type   = CMD_CleanExit;

        } else if(!stricmp(ARG[0], "Case")          && (argc >= 2)) {   // mex inifile (not BMS)
            CMD.type   = CMD_If;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], "=");                         // Criterium
            CMD.var[2] = parse_bms_add_var(2);                  // VarName2
            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_EndIf;

        } else if(!stricmp(ARG[0], "If")            && (argc >= 3)) {
            CMD.type   = CMD_If;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], ARG[2]);                      // Criterium
            CMD.var[2] = parse_bms_add_var(3);                  // VarName2
            for(i = 4; i <= argc; i += 4) {
                mystrdup(&CMD.str[(i-1)], ARG[i]);              // and/or
                CMD.var[(i-1)+1] = parse_bms_add_var(i+1);      // VarName1
                mystrdup(&CMD.str[(i-1)+2], ARG[i+2]);          // Criterium
                CMD.var[(i-1)+3] = parse_bms_add_var(i+3);      // VarName2
            }

        } else if((!stricmp(ARG[0], "Elif") || !stricmp(ARG[0], "ElseIf")) && (argc >= 3)) {   // copy as above!
            CMD.type   = CMD_If_Return;
            cmd++;
            check_g_command(cmd);

            CMD.type   = CMD_Elif;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], ARG[2]);                      // Criterium
            CMD.var[2] = parse_bms_add_var(3);                  // VarName2
            for(i = 4; i <= argc; i += 4) {
                mystrdup(&CMD.str[(i-1)], ARG[i]);              // and/or
                CMD.var[(i-1)+1] = parse_bms_add_var(i+1);      // VarName1
                mystrdup(&CMD.str[(i-1)+2], ARG[i+2]);          // Criterium
                CMD.var[(i-1)+3] = parse_bms_add_var(i+3);      // VarName2
            }

        } else if(!stricmp(ARG[0], "Else")          && (argc >= 0)) {
            CMD.type   = CMD_If_Return;
            cmd++;
            check_g_command(cmd);

            CMD.type   = CMD_Else;
            if((argc >= 4) && !stricmp(ARG[1], "If")) {         // copy as above!
                CMD.type   = CMD_Elif;
                CMD.var[0] = parse_bms_add_var(2);              // VarName1
                mystrdup(&CMD.str[1], ARG[3]);                  // Criterium
                CMD.var[2] = parse_bms_add_var(4);              // VarName2
                for(i = 5; i <= argc; i += 4) {
                    mystrdup(&CMD.str[(i-2)], ARG[i]);          // and/or
                    CMD.var[(i-2)+1] = parse_bms_add_var(i+1);  // VarName1
                    mystrdup(&CMD.str[(i-2)+2], ARG[i+2]);      // Criterium
                    CMD.var[(i-2)+3] = parse_bms_add_var(i+3);  // VarName2
                }
            }

        } else if(!stricmp(ARG[0], "IfEqual")       && (argc >= 2)) {
            CMD.type   = CMD_If;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], "==");                        // Criterium
            CMD.var[2] = parse_bms_add_var(2);                  // VarName2

        } else if(!stricmp(ARG[0], "IfGreater")     && (argc >= 2)) {
            CMD.type   = CMD_If;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], ">");                         // Criterium
            CMD.var[2] = parse_bms_add_var(2);                  // VarName2

        } else if(!stricmp(ARG[0], "IfLower")       && (argc >= 2)) {
            CMD.type   = CMD_If;
            CMD.var[0] = parse_bms_add_var(1);                  // VarName1
            mystrdup(&CMD.str[1], "<");                         // Criterium
            CMD.var[2] = parse_bms_add_var(2);                  // VarName2

        } else if(!stricmp(ARG[0], "EndIf")         && (argc >= 0)) {
            CMD.type   = CMD_EndIf;

        } else if(!stricmp(ARG[0], "GetCT")         && (argc >= 3)) {
            CMD.type   = CMD_GetCT;
            CMD.var[0] = parse_bms_add_var(1);                  // variable
            CMD.num[1] = add_datatype(ARG[2]);                  // datatype
            CMD.var[2] = parse_bms_add_var(3);                  // character
            CMD.num[3] = myatoifile(ARG[4]);                    // filenumber

        } else if(!stricmp(ARG[0], "PutCT")         && (argc >= 3)) {   // write mode
            CMD.type   = CMD_PutCT;
            CMD.var[0] = parse_bms_add_var(1);                  // variable
            CMD.num[1] = add_datatype(ARG[2]);                  // datatype
            CMD.var[2] = parse_bms_add_var(3);                  // character
            CMD.num[3] = myatoifile(ARG[4]);                    // filenumber

        } else if(!stricmp(ARG[0], "ComType")       && (argc >= 1)) {
            CMD.type   = CMD_ComType;
            mystrdup(&CMD.str[0], ARG[1]);                      // ComType
            if(argc <= 2) {
                CSTRING(1, ARG[2])                              // optional static binary dictionary
            } else {
                CMD.var[1] = parse_bms_add_var(2);              // optional variable dictionary
                CMD.var[2] = parse_bms_add_var(3);
            }

        } else if(
                 (!stricmp(ARG[0], "ReverseShort")  && (argc >= 1))
              || (!stricmp(ARG[0], "FlipShort")     && (argc >= 1))) {  // mex inifile (not BMS)
            CMD.type   = CMD_ReverseShort;
            CMD.var[0] = parse_bms_add_var(1);                  // variable
            CMD.num[1] = -1;
            if(argc >= 2) CMD.num[1] = bms_get_endian(ARG[2]);

        } else if(
                 (!stricmp(ARG[0], "ReverseLong")   && (argc >= 1))
              || (!stricmp(ARG[0], "FlipLong")      && (argc >= 1))) {  // mex inifile (not BMS)
            CMD.type   = CMD_ReverseLong;
            CMD.var[0] = parse_bms_add_var(1);                  // variable
            CMD.num[1] = -1;
            if(argc >= 2) CMD.num[1] = bms_get_endian(ARG[2]);

        } else if(
                 (!stricmp(ARG[0], "ReverseLongLong")   && (argc >= 1))
              || (!stricmp(ARG[0], "FlipLongLong")      && (argc >= 1))) {  // mex inifile (not BMS)
            CMD.type   = CMD_ReverseLongLong;
            CMD.var[0] = parse_bms_add_var(1);                  // variable
            CMD.num[1] = -1;
            if(argc >= 2) CMD.num[1] = bms_get_endian(ARG[2]);

        } else if(!stricmp(ARG[0], "PROMPTUSER")    && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "EVENTS")        && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "SEPPATH")       && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "NOFILENAMES")   && (argc >= 0)) {   // mex inifile (not BMS)
            CMD.type   = CMD_Set;
            CMD.var[0] = add_var(0, "FILENAME", NULL, 0, -2);   // VarName
            CMD.num[1] = add_datatype("String");                // datatype
            CMD.var[2] = add_var_const(0, "", NULL, 0, -2);     // Var/Number

        } else if(!stricmp(ARG[0], "WriteLong")     && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "StrCReplace")   && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "StrEResizeC")   && (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "SeperateHeader")&& (argc >= 0)) {   // mex inifile (not BMS)
            // do nothing, this command is useless
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "Endian")        && (argc >= 1)) {
            CMD.type   = CMD_Endian;
            t = bms_get_endian(ARG[1]);
            if(t >= 0) {
                CMD.num[0] = t;

            } else if(!stricmp(ARG[1], "guess32") || !stricmp(ARG[1], "guess")) {
                CMD.num[0] = -2;
                CMD.var[1] = parse_bms_add_var(2);
                
            } else if(!stricmp(ARG[1], "guess16")) {
                CMD.num[0] = -3;
                CMD.var[1] = parse_bms_add_var(2);
                
            } else if(!stricmp(ARG[1], "guess64")) {
                CMD.num[0] = -4;
                CMD.var[1] = parse_bms_add_var(2);
                
            } else if(!stricmp(ARG[1], "guess24")) {
                CMD.num[0] = -5;
                CMD.var[1] = parse_bms_add_var(2);
                
            } else if(!stricmp(ARG[1], "swap") || !stricmp(ARG[1], "invert") || !stricmp(ARG[1], "change")) {
                CMD.num[0] = -1;
                
            } else if(!stricmp(ARG[1], "save") || !stricmp(ARG[1], "store")) {
                CMD.num[0] = -6;
                CMD.var[1] = parse_bms_add_var(2);

            } else if(!stricmp(ARG[1], "set") || !stricmp(ARG[1], "restore")) {
                CMD.num[0] = -7;
                CMD.var[1] = parse_bms_add_var(2);

            } else {
                CMD.num[0] = -7;
                CMD.var[1] = parse_bms_add_var(1);

                //fprintf(stderr, "\nError: invalid endian value %s at line %d\n", ARG[1], (i32)g_bms_line_number);
                //myexit(QUICKBMS_ERROR_BMS);
            }

        } else if(!stricmp(ARG[0], "FileXOR")       && (argc >= 1)) {
            CMD.type   = CMD_FileXOR;
            CMD_FileXOR_bms

        } else if(!strnicmp(ARG[0], "FileRot", 7)   && (argc >= 1)) {
            CMD.type   = CMD_FileRot13;
            CMD_FileXOR_bms

        } else if(!stricmp(ARG[0], "FileCrypt")     && (argc >= 1)) {
            CMD.type   = CMD_FileCrypt;
            CMD_FileXOR_bms

        } else if(!stricmp(ARG[0], "Break")         && (argc >= 0)) {
            CMD.type   = CMD_Break;
            if(argc >= 1) {
                mystrdup(&CMD.str[0], ARG[1]);
            } else {
                FREE(CMD.str[0])
            }

        } else if(!stricmp(ARG[0], "Strlen")        && (argc >= 2)) {
            CMD.type   = CMD_Strlen;
            CMD.var[0] = parse_bms_add_var(1);                  // dest var
            CMD.var[1] = parse_bms_add_var(2);                  // string
            if(argc >= 3) {
                CMD.num[2] = myatoi(ARG[3]);
            } else {
                CMD.num[2] = 0;
            }

        } else if(!stricmp(ARG[0], "GetVarChr")     && (argc >= 3)) {
            CMD.type   = CMD_GetVarChr;
            CMD.var[0] = parse_bms_add_var(1);                  // dst byte
            if(is_MEMORY_FILE(ARG[2])) {
                CMD.var[1] = get_memory_file(ARG[2]);
            } else {
                CMD.var[1] = parse_bms_add_var(2);              // src var
            }
            CMD.var[2] = parse_bms_add_var(3);                  // offset
            if(argc == 3) {
                CMD.num[3] = add_datatype("byte");
            } else {
                CMD.num[3] = add_datatype(ARG[4]);
            }

        } else if(!stricmp(ARG[0], "PutVarChr")     && (argc >= 3)) {
            CMD.type   = CMD_PutVarChr;
            if(is_MEMORY_FILE(ARG[1])) {
                CMD.var[0] = get_memory_file(ARG[1]);
            } else {
                CMD.var[0] = parse_bms_add_var(1);              // dst var
            }
            CMD.var[1] = parse_bms_add_var(2);                  // offset
            //CMD.var[2] = parse_bms_add_var(3);                  // src byte
            set_cmd_args_ptr(cmd, 2, ARG[3]);
            if(argc == 3) {
                CMD.num[3] = add_datatype("byte");
            } else {
                CMD.num[3] = add_datatype(ARG[4]);
            }

        } else if(!stricmp(ARG[0], "Debug")         && (argc >= 0)) {
            CMD.type   = CMD_Debug;
            if(argc >= 1) CMD.var[0] = parse_bms_add_var(1);       // type of verbosity

        } else if(!stricmp(ARG[0], "Padding")       && (argc >= 1)) {
            CMD.type   = CMD_Padding;
            CMD.var[0] = parse_bms_add_var(1);                  // padding size
            CMD.num[1] = myatoifile(ARG[2]);                    // filenumber
            if(argc >= 3) CMD.var[2] = parse_bms_add_var(3);    // base offset

        } else if(!stricmp(ARG[0], "Append")        && (argc >= 0)) {
            CMD.type   = CMD_Append;
            if(argc >= 1) CMD.var[0] = parse_bms_add_var(1);    // direction
            g_script_uses_append = 1;
            is_append = !is_append; // just a work-around

        } else if(!stricmp(ARG[0], "Encryption")    && (argc >= 2)) {
            CMD.type   = CMD_Encryption;
            CMD.var[0] = parse_bms_add_var(1);                  // type
            if(!stricmp(ARG[2], "?")) {
                fprintf(stderr, "\n"
                    "Error: seems that the script you are using needs that you specify a fixed\n"
                    "       %s key at line %d for using it, so edit the script source code\n"
                    "       adding this needed value, examples:\n"
                    "         encryption %s \"mykey\"\n"
                    "         encryption %s \"\\x6d\\x79\\x6b\\x65\\x79\"\n"
                    "\n", ARG[1], (i32)g_bms_line_number, ARG[1], ARG[1]);
                myexit(QUICKBMS_ERROR_BMS);
            }
            CSTRING(1, ARG[2])                                  // key
            CSTRING(2, ARG[3])                                  // ivec
            if(argc >= 4) {
                CMD.var[3] = parse_bms_add_var(4);              // decrypt/encrypt
            }
            if(argc >= 5) {
                CMD.var[4] = parse_bms_add_var(5);              // keylen
            }

        } else if(!stricmp(ARG[0], "Print")         && (argc >= 1)) {
            CMD.type   = CMD_Print;
            CSTRING(0, ARG[1])                                  // message

        } else if(!stricmp(ARG[0], "MessageBox")    && (argc >= 1)) {
            CMD.type   = CMD_Print;
            CSTRING(0, ARG[1])                                  // message

        } else if(!stricmp(ARG[0], "GetArray")      && (argc >= 3)) {
            CMD.type   = CMD_GetArray;
            for(i = 1; i <= (argc - 2); i++) {
                CMD.var[i-1] = parse_bms_add_var(i);            // var
            }
            CMD.var[i-1] = parse_bms_add_var(i);                // array number
            i++;
            CMD.var[i-1] = parse_bms_add_var(i);                // number/string

        } else if(!stricmp(ARG[0], "PutArray")      && (argc >= 3)) {
            CMD.type   = CMD_PutArray;
            CMD.var[0] = parse_bms_add_var(1);                  // array number
            CMD.var[1] = parse_bms_add_var(2);                  // number/string
            for(i = 3; i <= argc; i++) {
                CMD.var[i-1] = parse_bms_add_var(i);            // var
            }

        } else if(!stricmp(ARG[0], "SortArray")      && (argc >= 1)) {
            CMD.type   = CMD_SortArray;
            CMD.var[0] = parse_bms_add_var(1);                  // array number
            if(argc >= 2) CMD.var[1] = parse_bms_add_var(2);    // all

        } else if(!stricmp(ARG[0], "SearchArray")    && (argc >= 3)) {
            CMD.type   = CMD_SearchArray;
            CMD.var[0] = parse_bms_add_var(1);                  // var
            CMD.var[1] = parse_bms_add_var(2);                  // array number
            CMD.var[2] = parse_bms_add_var(3);                  // number/string

        } else if(!stricmp(ARG[0], "StartFunction") && (argc >= 1)) {
            CMD.type   = CMD_StartFunction;
            mystrdup(&CMD.str[0], ARG[1]);

        } else if(!stricmp(ARG[0], "CallFunction")  && (argc >= 1)) {
            CMD.type   = CMD_CallFunction;
            mystrdup(&CMD.str[0], ARG[1]);
            CMD.num[1] = myatoi(ARG[2]);
            cmd = set_cmd_args(cmd, argc, ARG, 2);              // number of arguments

        } else if(!stricmp(ARG[0], "EndFunction")   && (argc >= 0)) {
            CMD.type   = CMD_EndFunction;
            //mystrdup(&CMD.str[0], ARG[1]);

        } else if(!stricmp(ARG[0], "ScanDir")       && (argc >= 3)) {
            CMD.type   = CMD_ScanDir;
            CMD.var[0] = parse_bms_add_var(1);                  // path to scan
            CMD.var[1] = parse_bms_add_var(2);                  // filename
            CMD.var[2] = parse_bms_add_var(3);                  // filesize
            if(ARG[4] && !g_filter_files) {
                tmp = mystrdup_simple(ARG[4]);
                build_filter(&g_filter_in_files, tmp);
                FREE(tmp)
            }

        } else if(!stricmp(ARG[0], "CallDLL")       && (argc >= 3)) {
            CMD.type   = CMD_CallDLL;
            mystrdup(&CMD.str[0], ARG[1]);                      // name of the dll
            mystrdup(&CMD.str[1], ARG[2]);                      // name of the function or relative offset
            mystrdup(&CMD.str[2], ARG[3]);                      // stdcall/cdecl
            //CMD.var[3] = parse_bms_add_var(4);                // return value
            set_cmd_args_ptr(cmd, 3, ARG[4]);                   // return value with optional &var
            cmd = set_cmd_args(cmd, argc, ARG, 4);              // number of arguments

        } else if(!stricmp(ARG[0], "include")       && (argc >= 1)) {
            tmp = quickbms_path_open(ARG[1]);
            if(tmp) {
                include_fd = xfopen(tmp, "rb");
                FREE(tmp)
            } else {
                include_fd = xfopen(ARG[1], "rb");
            }
            if(!include_fd) {
                fprintf(stderr, "- requested script \"%s\" not found at line %d\n", ARG[1], (i32)g_bms_line_number);
                myexit(QUICKBMS_ERROR_BMS);
            }
            cmd = parse_bms(include_fd, NULL, cmd, 0);
            cmd--;  // needed!
            FCLOSE(include_fd);
            include_workaround = 1;

        } else if(!stricmp(ARG[0], "Game") || !stricmp(ARG[0], "Archive")
               || !strnicmp(ARG[0], "Game ", 5)
               || !strnicmp(ARG[0], "Game:", 5)
               || !strnicmp(ARG[0], "Archive", 7)
               || !strnicmp(ARG[0], "Archive:", 8)
               || strstr(ARG[0], "-------")
               || strstr(ARG[0], "=-=-=-=")
               || stristr(ARG[0], "<bms")
               || stristr(ARG[0], "<bms>")
               || stristr(ARG[0], "</bms>")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "NOP")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "template")) {
            CMD.type   = CMD_Debug;
            CMD.num[0] = 1;

        } else if(!stricmp(ARG[0], "description")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "applies_to")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "fixed_start") && (argc >= 1)) {
            CMD.type   = CMD_GoTo;
            CMD.var[0] = parse_bms_add_var(1);                  // pos
            CMD.num[1] = 0;                                     // file
            CMD.num[2] = SEEK_SET;

        } else if(!stricmp(ARG[0], "sector-aligned")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "big-endian") || !stricmp(ARG[0], "BigEndian")) {
            CMD.type   = CMD_Endian;
            CMD.num[0] = MYBIG_ENDIAN;

        } else if(!stricmp(ARG[0], "little-endian") || !stricmp(ARG[0], "LittleEndian")) {
            CMD.type   = CMD_Endian;
            CMD.num[0] = MYLITTLE_ENDIAN;

        } else if(!stricmp(ARG[0], "requires") && argc >= 2) {
            CMD.type   = CMD_SavePos;
            CMD.var[0] = add_var(0, QUICKBMS_DUMMY, NULL, 0, -2);   // VarName
            CMD.num[1] = 0;                                     // File

            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_GoTo;
            CMD.var[0] = parse_bms_add_var(1);                  // pos
            CMD.num[1] = 0;                                     // file
            CMD.num[2] = SEEK_SET;

            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_IDString;
            CMD.num[0] = 0;
            NUMS2BYTES_HEX(ARG[2], CMD.num[1], CMD.str[1], CMD.num[2], is_const[2])  // string (acts like a realloc)

            cmd++;
            check_g_command(cmd);
            CMD.type   = CMD_GoTo;
            CMD.var[0] = add_var(0, QUICKBMS_DUMMY, NULL, 0, -2);   // pos
            CMD.num[1] = 0;                                     // file
            CMD.num[2] = SEEK_SET;

        } else if(!stricmp(ARG[0], "begin")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "end")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "hex") && argc >= 2) {
            CMD.type   = CMD_GetDString;
            CMD.var[0] = parse_bms_add_var(2);                  // varname
            CMD.var[1] = parse_bms_add_var(1);                  // NumberOfCharacters
            CMD.num[2] = 0;                                     // filenumber

        } else if(!stricmp(ARG[0], "move") && argc >= 1) {
            CMD.type   = CMD_GoTo;
            CMD.var[0] = parse_bms_add_var(1);                  // pos
            CMD.num[1] = 0;                                     // file
            CMD.num[2] = SEEK_CUR;

        } else if(!stricmp(ARG[0], "uint_flex") && (argc >= 2)) {
            cmd = set_uint_flex(cmd, ARG[2], ARG[1]);

        } else if(!stricmp(ARG[0], "section")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "endsection")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "numbering")) {
            CMD.type   = CMD_NOP;

        } else if(!stricmp(ARG[0], "zstring") && (argc >= 1)) {
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);                  // varname
            CMD.num[1] = add_datatype("string");                // type
            CMD.num[2] = 0;                                     // filenumber

        } else if(!stricmp(ARG[0], "Label") && (argc >= 1)) {
            CMD.type   = CMD_Label;
            mystrdup(&CMD.str[0], ARG[1]);

        } else if(!strnicmp(ARG[0], "ConvertBytesTo", 14) && (argc >= 2)) {
            CMD.type   = CMD_GetVarChr;
            CMD.var[0] = parse_bms_add_var(1);
            CMD.var[1] = add_var(0, "0", NULL, 0, -2);
            CMD.var[2] = parse_bms_add_var(2);
            CMD.num[3] = add_datatype(ARG[0] + 14);

        } else if(!stricmp(ARG[0], "ConvertDataToBytes") && (argc >= 2)) {
            CMD.type   = CMD_PutVarChr;
            CMD.var[0] = parse_bms_add_var(1);
            CMD.var[1] = add_var(0, "0", NULL, 0, -2);
            CMD.var[2] = parse_bms_add_var(2);
            CMD.num[3] = add_datatype("long");  // default

        } else if(!stricmp(ARG[0], "DirectoryExists") && (argc >= 2)) {
            CMD.type   = CMD_DirectoryExists;
            CMD.var[0] = parse_bms_add_var(1);  // output var
            CMD.var[1] = parse_bms_add_var(2);  // folder

        } else if(!stricmp(ARG[0], "FEof") && (argc >= 1)) {
            CMD.type   = CMD_FEof;
            CMD.var[0] = parse_bms_add_var(1);  // output var
            CMD.num[1] = myatoifile(ARG[2]);    // optional file number

        } else if(!stricmp(ARG[0], "FSkip") && (argc >= 1)) {
            CMD.type   = CMD_GoTo;
            CMD.var[0] = parse_bms_add_var(1);                  // pos
            CMD.num[1] = myatoifile(ARG[2]);                    // file
            CMD.num[2] = SEEK_CUR;

        } else if(!stricmp(ARG[0], "reimport") && (argc >= 0)) {
            CMD.type   = CMD_Reimport;
            if(argc >= 1) CMD.var[0] = parse_bms_add_var(1);    // type

        } else if(
            (   !stricmp(ARG[0], "ReadByte")   || !stricmp(ARG[0], "ReadDouble") || !stricmp(ARG[0], "ReadFloat")  ||
                !stricmp(ARG[0], "ReadHFloat") || !stricmp(ARG[0], "ReadInt")    || !stricmp(ARG[0], "ReadInt64")  ||
                !stricmp(ARG[0], "ReadQuad")   || !stricmp(ARG[0], "ReadShort")  || !stricmp(ARG[0], "ReadUByte")  ||
                !stricmp(ARG[0], "ReadUInt")   || !stricmp(ARG[0], "ReadUInt64") || !stricmp(ARG[0], "ReadUQuad")  ||
                !stricmp(ARG[0], "ReadUShort") || !stricmp(ARG[0], "ReadLine")   || !stricmp(ARG[0], "ReadString") ||
                !stricmp(ARG[0], "ReadDString") // added by me
            ) && (argc >= 1)) {
            if(argc >= 2) {
                CMD.type   = CMD_GoTo;
                CMD.var[0] = parse_bms_add_var(2);
                CMD.num[1] = myatoifile(ARG[3]);
                CMD.num[2] = SEEK_SET;
            }
            CMD.type   = CMD_Get;
            CMD.var[0] = parse_bms_add_var(1);
            CMD.num[1] = add_var(0, ARG[0] + 4, NULL, 0, -2);
            CMD.num[2] = myatoifile(ARG[3]);



        // last part, put your instructions above this line

        } else if((argc == 0) && (strchr(ARG[0], ':'))) {
            CMD.type   = CMD_Label;
            mystrdup(&CMD.str[0], ARG[0]);
            strchr(CMD.str[0], ':')[0] = 0;

        } else {

            if(!g_c_structs_allowed) {
                fprintf(stderr, "\nAlert: invalid command \"%s\" or arguments (%d), line %d\n", ARG[0], (i32)argc, (i32)g_bms_line_number);
                fprintf(stderr, "The script may use other supported languages and C structs, continue? (y/N)\n  ");
                if(get_yesno(NULL) != 'y') {
                    myexit(QUICKBMS_ERROR_BMS);
                }
                g_c_structs_allowed = 1;
            }

            if(c_structs_do) {
                argc = c_structs(argument, argc, &cmd);
                if(argc >= 0) {
                    c_structs_do = 0;
                    goto redo;
                }
            }

            fprintf(stderr, "\nError: invalid command \"%s\" or arguments (%d), line %d\n", ARG[0], (i32)argc, (i32)g_bms_line_number);
            myexit(QUICKBMS_ERROR_BMS);
        }

        if(CMD.type == CMD_NONE) {
            fprintf(stderr, "\nError: there is an error in QuickBMS because there is no command type at line %d\n", (i32)g_bms_line_number);
            myexit(QUICKBMS_ERROR_BMS);
        }
        //CMD.debug_line = debug_line;

        // not much useful
        if(g_verbose > 0) {
            for(i = 0; i <= argc; i++) {
                printf("  ARG%-2d \"%s\"\n", (i32)i, ARG[i]);
            }
            printf("\n");
        }

        // free, introduced with multiline
        for(i = 0; i <= argc; i++) {
            FREE(ARG[i])
        }


        // note that the following method doesn't work if we have an Append inside a function!
        if((CMD.type == CMD_Log) || (CMD.type == CMD_CLog)) {
            if(is_append) {
                u8 *fname = VAR(0);
                u8 *varname = VARNAME(0);
                if(is_MEMORY_FILE(varname) && !stricmp(varname, fname)) {  // from file.c
                    if(g_reimport) is_append_hashing_necessary = 1;    // or memory file reimporting will not work
                } else {
                    is_append_hashing_necessary = 1;
                }
            }
        }


        cmd++;
        if(inputs && !inputs[0]) break;
    }
    if(!cmd) {
        fprintf(stderr, "\nError: the input BMS script is empty\n");
        myexit(QUICKBMS_ERROR_BMS);
    }

    if(g_script_uses_append && !is_append_hashing_necessary) {
        if(!include_workaround) g_script_uses_append = 0;
    }
    return(cmd);
}



void g_mex_default_init(int file_only) {
    if(!g_mex_default) return; // just in case...
    if(!file_only) EXTRCNT_idx   = add_var(0, "EXTRCNT", NULL, 0, sizeof(int)); // used by MultiEx as fixed variable
    BytesRead_idx = add_var(0, "BytesRead", NULL, 0, sizeof(int));              // used by MultiEx as fixed variable
    NotEOF_idx    = add_var(0, "NotEOF",    NULL, 1, sizeof(int));              // used by MultiEx as fixed variable
    EOF_idx       = add_var(0, "EOF",       NULL, myfilesize(0), sizeof(int));  // used by MultiEx as fixed variable
    SOF_idx       = add_var(0, "SOF",       NULL, 0, sizeof(int));              // used by MultiEx as fixed variable
}



void bms_init(int reinit) {
    int     i,
            j;

        g_codepage              = g_codepage_default;
        g_last_cmd              = 0;
        g_bms_line_number       = 0;
        g_extracted_files       = 0;
        g_extracted_files2      = 0;
        g_extracted_logs        = 0;
        g_reimported_files      = 0;
        g_reimported_files_skip = 0;
        g_reimported_files_404  = 0;
        g_reimported_logs       = 0;
        g_endian                = MYLITTLE_ENDIAN;
        //g_force_overwrite       = 0;
        //g_force_rename          = 0;
        g_compression_type      = COMP_ZLIB;
        g_comtype_dictionary_len= 0;
        g_comtype_scan          = 0;
        g_encrypt_mode          = 0;
        g_append_mode           = APPEND_MODE_NONE;
        g_temporary_file_used   = 0;
        g_mex_default           = 0;
        g_script_uses_append    = 0;
        memcpy(&g_filexor,   &g_filexor_reset, sizeof(filexor_t));  //memset(&g_filexor,   0, sizeof(g_filexor));
        memcpy(&g_filerot,   &g_filexor_reset, sizeof(filexor_t));  //memset(&g_filerot,   0, sizeof(g_filerot));
        memcpy(&g_filecrypt, &g_filexor_reset, sizeof(filexor_t));  //memset(&g_filecrypt, 0, sizeof(g_filecrypt));
        g_comtype_dictionary    = NULL;
        //EXTRCNT_idx             = 0;
        //BytesRead_idx           = 0;
        //NotEOF_idx              = 0;
        g_memfile_reimport_name     = -1;
        if(!g_ipc_web_api) g_replace_fdnum0 = 0;    // it's mandatory in web api, it may cause problems with some rare scripts
        if(g_debug_output) g_debug_output->level = 0;
        g_slog_id               = 0;

    if(g_mex_default) {
        g_mex_default_init(0);
    }
    CMD_Encryption_func(-1, 0);

    // input folder only: in case someone writes bad scripts
    //do NOT enable//
    /*for(i = 0; i < MAX_VARS; i++) {
        if(g_variable[i].name)  g_variable[i].name[0]  = 0;
        if(g_variable[i].value) g_variable[i].value[0] = 0;
        g_variable[i].value32 = 0;
    }*/
    // input folder only: enough useful

    // leave them allocated!
    for(i = 0; i < MAX_FILES; i++) {
        g_memory_file[i].pos  = 0;
        g_memory_file[i].size = 0;
    }

    // leave them allocated!
    for(i = 0; i < MAX_ARRAYS; i++) {
        g_array[i].elements = 0;
    }

    /* files are initialized when opened, do not enable the following at the moment
    for(i = 0; i < MAX_FILES; i++) {
        g_filenumber[i].bitchr = 0;
        g_filenumber[i].bitpos = 0;
        g_filenumber[i].bitoff = 0;
        g_filenumber[i].hexhtml_size = 0;
        g_filenumber[i].coverage = 0;
    }
    */

    CMD_CallDLL_func(-1, NULL, 0, NULL, 0);

    if(reinit) return;

    // not done in reinit because they contain allocated stuff
    memset(g_filenumber,  0, sizeof(g_filenumber));
    g_variable = g_variable_main;
    memset(g_variable,    0, sizeof(g_variable_main));
    memset(g_memory_file, 0, sizeof(g_memory_file));
    memset(g_array,       0, sizeof(g_array));

    check_g_command(0);  // automatically checks !g_command
    memset(g_command,     0, sizeof(command_t) * (MAX_CMDS + 1));
    for(i = 0; i < MAX_CMDS; i++) {
        for(j = 0; j < MAX_ARGS; j++) {
            g_command[i].var[j] = -0x7fffff;  // helps a bit to identify errors in QuickBMS, DO NOT MODIFY IT! NEVER! (it's used in places like check_condition)
            g_command[i].num[j] = -0x7fffff;  // helps a bit to identify errors in QuickBMS
            // do NOT touch g_command[i].str[j]
        }
    }

    xgetcwd(g_current_folder, PATHSZ);
    g_quickbms_version = quickbmsver(VER);
}



void bms_finish(void) { // totally useless function, except in write mode for closing the files
    int     i;

    for(i = 0; i < MAX_FILES; i++) {
        myfclose(i);
    }
    for(i = 0; i < MAX_FILES; i++) {
        myfclose(-i);
    }
    if(g_temporary_file_used) {
        if(!g_keep_temporary_file) {
            //fprintf(stderr, "\n- a temporary file was created, do you want to delete it (y/N)?\n  ");
            //if(get_yesno(NULL) == 'y')
            unlink(TEMPORARY_FILE);
        }
    }

    /*
    freeing memory is a problem if XDBG_ALLOC_ACTIVE has been disabled
    at runtime so... who cares, skip it!

    int     j;

    for(i = 0; i < MAX_FILES; i++) {
        FREE(g_filenumber[i].fullname)
        FREE(g_filenumber[i].filename)
        FREE(g_filenumber[i].basename)
        FREE(g_filenumber[i].fileext)
        FREE(g_filenumber[i].filepath)
    }
    memset(g_filenumber, 0, sizeof(g_filenumber));

    g_variable = g_variable_main;
    for(i = 0; i < MAX_VARS; i++) {
        FREE_VAR(&g_variable[i]);
    }
    memset(g_variable, 0, sizeof(g_variable_main));

    for(i = 0; i < MAX_CMDS; i++) {
        FREE(g_command[i].debug_line)
        for(j = 0; j < MAX_ARGS; j++) {
            FREE(g_command[i].str[j])
        }
    }
    memset(g_command,     0, sizeof(command_t) * (MAX_CMDS + 1));

    for(i = 0; i < MAX_FILES; i++) {
        FREE(g_memory_file[i].data)
    }
    memset(g_memory_file, 0, sizeof(g_memory_file));

    for(i = 0; i < MAX_ARRAYS; i++) {
        for(j = 0; j < g_array[i].elements; j++) {
            FREE_VAR(&g_array[i].var[j]);
        }
        FREE(g_array[i].var)
    }
    memset(g_array, 0, sizeof(g_array));
    dumpa(0, NULL, NULL, -1, -1, -1);
    //unzip(0, NULL, 0, NULL, 0);
    bms_line(NULL, NULL, NULL, -1, NULL, 0);
    */

    // xdbg_toggle() is not ready yet
    //xdbg_freeall();
}



void quickbms_statistics(time_t benchmark) {
    benchmark = time(NULL) - benchmark;
    if(g_reimport) {
        if(g_reimported_files == g_reimported_logs) fprintf(stderr, "\n- %"PRId" files "             "reimported in %d seconds\n", g_reimported_files,                    (i32)benchmark);
        else                                        fprintf(stderr, "\n- %"PRId" files (%"PRId" logs) reimported in %d seconds\n", g_reimported_files, g_reimported_logs, (i32)benchmark);
        if(g_reimported_files_skip)                 fprintf(stderr,   "- %"PRId" files or logs skipped due to size or compression problems\n", g_reimported_files_skip);
        if(g_reimported_files_404)                  fprintf(stderr,   "- %"PRId" files or logs left original, not available in the folder\n", g_reimported_files_404);
    } else {
        if(g_extracted_files == g_extracted_logs)   fprintf(stderr, "\n- %"PRId" files "             "found in %d seconds\n",      g_extracted_files,                     (i32)benchmark);
        else                                        fprintf(stderr, "\n- %"PRId" files (%"PRId" logs) found in %d seconds\n",      g_extracted_files,  g_extracted_logs,  (i32)benchmark);
    }
}



void quickbms_set_reimport_var(void) {
    int     value = 0;
    if(!g_reimport) value = 0;
    else if(g_reimport > 0) value = 1;
    else if(g_reimport < 0) {
        value = 2;
        if(g_reimport_shrink_enlarge) value = 3;
    }
    add_var(0, "QUICKBMS_REIMPORT", NULL, value, sizeof(int));
}

