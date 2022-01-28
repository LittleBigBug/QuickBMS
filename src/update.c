/*
    Copyright 2013 Luigi Auriemma

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



int quickbms_update(void) {
    static u8   *website[] = {
        "aluigi.org/quickbms.htm",
        "aluigi.altervista.org/quickbms.htm",
        NULL
    };

    mydown_options  opt;
    int     n,
            buffsz;
    u8      *buff = NULL,
            *p,
            *v,
            *s;

    for(n = 0; website[n]; n++) {
        memset(&opt, 0, sizeof(opt));
        opt.verbose = 1;
        opt.useragent = "Mozilla/4.0";
        opt.filedata = &buff;

        buffsz = mydown(
            website[n],
            NULL,
            &opt);
        if(buffsz != -1) break;
    }
    if(!website[n] || !buff || !buffsz) {
        fprintf(stderr, "\nError: the QuickBMS website isn't available\n");
        myexit(QUICKBMS_ERROR_UPDATE);
    }

    // the current version of mydownlib adds the final 0x00 automatically
    //buff[buffsz - 1] = 0;   // doesn't matter if the last byte is zeroed

    p = strstr(buff, "quickbms.zip");
    if(!p) {
        fprintf(stderr, "\nError: the QuickBMS page content (zip) isn't available\n");
        myexit(QUICKBMS_ERROR_UPDATE);
    }

    v = strstr(p, "0.");
    if(!v) {
        fprintf(stderr, "\nError: the QuickBMS page content (version) isn't available\n");
        myexit(QUICKBMS_ERROR_UPDATE);
    }

    strstr(p, ".zip")[4] = 0;
    for(; p >= buff; p--) {
        if(strchr("\"'= ", *p)) break;
    }
    p++;

    strchr(v, '<')[0] = 0;

    s = strchr(website[n], '/');
    if(!s) s = website[n] + strlen(website[n]);

    if(!stricmp(v, VER)) {
        printf("\n- No updates available, you already have the latest version\n");
    } else {
        printf("\n- Update %s available:\n\n  http://%.*s/%s\n\n",
            v,
            s - website[n], website[n], p);
    }

    FREE(buff);
    myexit(QUICKBMS_OK);
    return 0;
}

