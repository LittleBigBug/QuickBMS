/*
    Copyright 2018 Luigi Auriemma

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



typedef struct extracted_file_tree_f_t {
    u8      *name;
    extracted_file_t    *ef;
    struct extracted_file_tree_f_t *next;
    struct extracted_file_tree_f_t *prev;
} extracted_file_tree_f_t;

typedef struct extracted_file_tree_t {
    int     id;     // must be negative for not colliding with file id
    int     parent_id;
    u8      *name;
    u8      *path;
    extracted_file_tree_f_t      *files;    // list of files
    struct extracted_file_tree_t *folders;  // list of sub-folders
    struct extracted_file_tree_t *next;
    struct extracted_file_tree_t *prev;
} extracted_file_tree_t;
extracted_file_tree_t *g_extracted_file_tree = NULL;



int extracted_file_tree_cmp(extracted_file_tree_t *a, extracted_file_tree_t *b) {
    return mystricmp(a->name, b->name);
}



extracted_file_tree_t *extracted_file_tree_node(int id, u8 *name, u8 *path) {
    extracted_file_tree_t   *eft;
    eft = real_calloc(1, sizeof(extracted_file_tree_t));
    if(!eft) STD_ERR(QUICKBMS_ERROR_MEMORY);
    eft->id = id;
    if(!name) name = "";    // avoid possible NULL pointers
    eft->name = real_malloc(strlen(name) + 1);
    strcpy(eft->name, name);
    if(!path) path = "";    // avoid possible NULL pointers
    eft->path = real_malloc(strlen(path) + 1);
    strcpy(eft->path, path);
    return eft;
}



void extracted_file_tree_free(extracted_file_tree_t **head) {
    extracted_file_tree_t   *eft,
                            *eft_tmp1,
                            *eft_tmp2;
    extracted_file_tree_f_t *eftf,
                            *eftf_tmp1,
                            *eftf_tmp2;

    if(!head) return;
    CDL_FOREACH_SAFE((*head), eft, eft_tmp1, eft_tmp2) {
        CDL_FOREACH_SAFE(eft->files, eftf, eftf_tmp1, eftf_tmp2) {
            CDL_DELETE(eft->files, eftf);
            real_free(eftf);
        }
        CDL_DELETE((*head), eft);
        real_free(eft->name);
        real_free(eft->path);
        real_free(eft);
    }
    (*head) = NULL;
}



// works as appender too
void extracted_file_tree_build(extracted_file_tree_t **head) {
    extracted_file_tree_f_t *eftf;
    extracted_file_tree_t   *eft,
                            *current_eft,
                            eft_cmp;
    extracted_file_t        *ef;

    int     id  = 0;    // remember that -0 doesn't exist :)
    u8      c,
            *p,
            *l;

    if(!head) return;

    memset(&eft_cmp, 0, sizeof(eft_cmp));   // necessary if check a->ef

    eft = extracted_file_tree_node(--id, "", "");   // we need a root
    CDL_PREPEND((*head), eft);

    CDL_FOREACH(g_extracted_file, ef) {
        current_eft = *head;
        for(p = ef->name; p && *p; p = l) {
            while(*p && ((*p == '/') || (*p == '\\'))) p++;
            for(l = p; *l && (*l != '/') && (*l != '\\'); l++);
            c = *l;
            *l = 0;

            if(c) {     // folder, c is the path delimiter

                eft_cmp.name = p;
                CDL_SEARCH(current_eft->folders, eft, &eft_cmp, extracted_file_tree_cmp);
                if(!eft) {
                    eft = extracted_file_tree_node(--id, p, ef->name);
                    CDL_PREPEND(current_eft->folders, eft);
                }
                eft->parent_id = current_eft->id;
                current_eft = eft;

            } else {    // file

                eftf = real_calloc(1, sizeof(extracted_file_tree_f_t));
                if(!eftf) STD_ERR(QUICKBMS_ERROR_MEMORY);
                eftf->name = real_malloc(strlen(p) + 1);
                strcpy(eftf->name, p);
                eftf->ef = ef;
                CDL_PREPEND(current_eft->files, eftf);
                break;

            }

            *l++ = c;
            while(*l && ((*l == '/') || (*l == '\\'))) l++;
        }
    }
}



enum {
    extracted_file_tree_view_text1,
    extracted_file_tree_view_text2,
    extracted_file_tree_view_text3,
    extracted_file_tree_view_json1,
    extracted_file_tree_view_json2,
    extracted_file_tree_view_web,
    extracted_file_tree_view_dos,
    extracted_file_tree_view_ls,
};
void extracted_file_tree_view_print(int mode, int level, u8 *name, extracted_file_tree_t *eft, extracted_file_t *ef) {
    int     x;
    int     is_file = ef ? 1 : 0;
    u8      *p;

    switch(mode) {
        case extracted_file_tree_view_text1: {
            printf("%*s|- %s\n", (level + (is_file ? 1 : 0)) * 4, "", name);
            break;
        }
        case extracted_file_tree_view_text2: {
            printf("|");
            for(x=0;x<level + (is_file ? 1 : 0);x++) printf("__");
            printf(" %s\n", name);
            break;
        }
        case extracted_file_tree_view_text3: {  // tree /f /a (an approximation because we don't have all the fields)
            if(!is_file) {
                for(x=0;x<level;x++) printf("|   ");
                if(level) printf("\n");
                for(x=0;x<(level-1);x++) printf("|   ");
                if(level) printf("+---");
            } else {
                for(x=0;x<level;x++) printf("|   ");
                printf("%c   ", eft->folders ? '|' : ' ');
            }
            printf("%s\n", name);
            break;
        }
        case extracted_file_tree_view_json1: {
            if(level) {
                printf("%*s\"%s\":\n", (level + (is_file ? 1 : 0)) * 4, "", name);
            }
            if(!is_file) {
                printf("%*s[{\n", (level + (is_file ? 1 : 0)) * 4, "");
            } else {
                printf("%*s{\n", (level + (is_file ? 1 : 0)) * 4, "");
                printf("%*s\"offset\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->offset);
                if(ef->zsize) printf("%*s\"zsize\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->zsize);
                printf("%*s\"size\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->size);
                if(ef->compression) printf("%*s\"compression\":%d\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->compression);
                if(ef->encryption) printf("%*s\"encryption\":%d\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->encryption);
                printf("%*s},\n", (level + (is_file ? 1 : 0)) * 4, "");
            }
            break;
        }
        case extracted_file_tree_view_json2: {
            // https://www.npmjs.com/package/directory-structure-json
            printf("%*s{\n", (level + (is_file ? 1 : 0)) * 4, "");
            printf("%*s\"name\":\"%s\",\n", (level + (is_file ? 1 : 0)) * 4, "", name);
            printf("%*s\"type\":\"%s\",\n", (level + (is_file ? 1 : 0)) * 4, "", is_file ? "file" : "folder");
            if(!is_file) {
                printf("%*s\"children\": [\n", (level + (is_file ? 1 : 0)) * 4, "");
            } else {
                printf("%*s\"offset\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->offset);
                if(ef->zsize) printf("%*s\"zsize\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->zsize);
                printf("%*s\"size\":%"PRIu"\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->size);
                if(ef->compression) printf("%*s\"compression\":%d\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->compression);
                if(ef->encryption) printf("%*s\"encryption\":%d\",\n", (level + (is_file ? 1 : 0)) * 4, "", ef->encryption);
                printf("%*s},\n", (level + (is_file ? 1 : 0)) * 4, "");
            }
            break;
        }
        case extracted_file_tree_view_web: {
            if(!is_file) {
                printf(
                    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
                    "<html>\n"
                    " <head>\n"
                    "  <title>Index of /%s</title>\n"
                    " </head>\n"
                    " <body>\n"
                    "<h1>Index of /%s</h1>\n"
                    "\n"
                    "<pre><img src=\"/icons/blank.gif\" alt=\"Icon \"> <a "/*href=\"?C=N;O=D\"*/">Name</a>                                                             <a "/*href=\"?C=M;O=A\"*/">Last modified</a>              <a "/*href=\"?C=S;O=A\"*/">Size</a>  <a "/*href=\"?C=D;O=A\"*/">Description: Offset, ZSize, Path</a><hr><img src=\"/icons/back.gif\" alt=\"[PARENTDIR]\"> <a href=\"%s%"PRId"\">Parent Directory</a>\n",
                    eft->path, eft->path, (g_ipc_web_api > 0) ? "tree?id=" : "", eft->parent_id);
                extracted_file_tree_t   *current_eft = eft->folders;
                CDL_FOREACH(current_eft, eft) {
                    x = 64 - strlen(eft->name);
                    if(x < 0) x = 0;
                    printf(
                    "<img src=\"/icons/folder.gif\" alt=\"[DIR]\"> <a href=\"%s%"PRId"\">%s/</a>%*s%s %14s  %"PRIx" %14s %s\n",
                    (g_ipc_web_api > 0) ? "tree?id=" : "", eft->id, eft->name, x, "", get_dos_time(), "-", 0, myitoa(0), eft->path);
                }
            } else {
                p = NULL;
                if(name) p = strrchr(name, '.');
                if(p) p++;
                else  p = "";
                x = 64 - strlen(name);
                printf(
                    "<img src=\"/icons/binary.gif\" alt=\"[%3.3s]\"> <a "
                    //"href=\"%s%"PRId"\""
                    ">%s</a>%*s %s %14s  %"PRIx" %14s %s\n",
                    p,
                    //(g_ipc_web_api > 0) ? "tree?id=" : "", ef->id,
                    name, x, "", get_dos_time(), myitoa(ef->size), ef->offset, myitoa(ef->zsize), ef->name);
            }
            break;
        }
        case extracted_file_tree_view_dos: {
            if(!is_file) {
                printf("\n Directory of %s\n\n", eft->path);
                extracted_file_tree_t   *current_eft = eft->folders;
                CDL_FOREACH(current_eft, eft) {
                    printf("%s    %-14s %s\n", get_dos_time(), "<DIR>", eft->name);
                }
            } else {
                printf("%s    %-14s %s\n", get_dos_time(), myitoa(ef->size), name);
            }
            break;
        }
        case extracted_file_tree_view_ls: {
            if(!is_file) {
                printf("\n/%s\n", eft->path);
                printf("dr--r--r-- 1 root root %9"PRIu" %s  .\n",  0, get_ls_time());
                printf("dr--r--r-- 1 root root %9"PRIu" %s  ..\n", 0, get_ls_time());
                extracted_file_tree_t   *current_eft = eft->folders;
                CDL_FOREACH(current_eft, eft) {
                    printf("dr--r--r-- 1 root root         0 %s  %s\n", get_ls_time(), eft->name);
                }
            } else {
                printf("-r--r--r-- 1 root root %9s %s  %s\n", myitoa(ef->size), get_ls_time(), name);
            }
            break;
        }
        default: break;
    }
}



void extracted_file_tree_view(extracted_file_tree_t *current_eft, int level, int mode, int folder_id) {
    extracted_file_tree_f_t *eftf;
    extracted_file_tree_t   *eft;

    if(!current_eft) return;
    CDL_FOREACH(current_eft, eft) {
        int     do_visualize = (folder_id >= 0) || ((folder_id < 0) && (eft->id == folder_id)); // this is for the future
        if(do_visualize) {
            extracted_file_tree_view_print(mode, level, eft->name, eft, NULL);
            CDL_FOREACH(eft->files, eftf) {
                extracted_file_tree_view_print(mode, level, eftf->name, eft, eftf->ef);
            }
            switch(mode) {
                case extracted_file_tree_view_web:
                    printf("<hr></pre>\n</body></html>\n\n");
                    break;
                default: break;
            }
        }
        if(eft->folders) {
            extracted_file_tree_view(eft->folders, level + 1, mode, folder_id);
        }
        if(do_visualize) {
            switch(mode) {
                case extracted_file_tree_view_json1: printf("%*s}],\n", level * 4, ""); break;
                case extracted_file_tree_view_json2: printf("%*s]},\n", level * 4, ""); if(!level) printf("]\n"); break;
                default: break;
            }
        }
        if((folder_id < 0) && (eft->id == folder_id)) break;
    }
}


