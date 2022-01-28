// modified from my old quickrva.c tool
/*
    Copyright 2007-2021 Luigi Auriemma

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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "quickrva.h"

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;


#define SECNAMESZ   32
#define MYPAD(X)    ((X + (q->sec_align - 1)) & (~(q->sec_align - 1)))
#define MYALIGNMENT 0x1000  // default in case not available



#ifdef WIN32
#include <windows.h>

typedef struct {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} MYIMAGE_NT_HEADERS32;

typedef struct {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} MYIMAGE_NT_HEADERS64;
#endif

typedef struct {    // from http://hte.sf.net
    u32     vsize;
    u32     base_reloc_addr;
    u32     flags;
    u32     page_map_index;
    u32     page_map_count;
    u8      name[4];
} vxd_section_t;

typedef struct {
    u8      e_ident[16];
    u16     e_type;
    u16     e_machine;
    u32     e_version;
    u32     e_entry;
    u32     e_phoff;
    u32     e_shoff;
    u32     e_flags;
    u16     e_ehsize;
    u16     e_phentsize;
    u16     e_phnum;
    u16     e_shentsize;
    u16     e_shnum;
    u16     e_shstrndx;
} elf32_header_t;

typedef struct {
    u8      e_ident[16];
    u16     e_type;
    u16     e_machine;
    u32     e_version;
    u64     e_entry;
    u64     e_phoff;
    u64     e_shoff;
    u32     e_flags;
    u16     e_ehsize;
    u16     e_phentsize;
    u16     e_phnum;
    u16     e_shentsize;
    u16     e_shnum;
    u16     e_shstrndx;
} elf64_header_t;

typedef struct {
    u32     sh_name;
    u32     sh_type;
    u32     sh_flags;
    u32     sh_addr;     
    u32     sh_offset;
    u32     sh_size;
    u32     sh_link;
    u32     sh_info;
    u32     sh_addralign;
    u32     sh_entsize;
} elf32_section_t;

typedef struct {
    u32     sh_name;
    u32     sh_type;
    u64     sh_flags;
    u64     sh_addr;
    u64     sh_offset;
    u64     sh_size;
    u32     sh_link;
    u32     sh_info;
    u64     sh_addralign;
    u64     sh_entsize;
} elf64_section_t;

typedef struct {    // from http://hte.sf.net
    u32     magic_id;
    u8      signature[256];
    u32     base_address;
    u32     size_of_headers;
    u32     size_of_image;
    u32     size_of_imageheader;
    u32     timedate;
    u32     certificate_address;
    u32     number_of_sections;
    u32     section_header_address;
    u32     initialisation_flags;
    u32     entry_point;
    u32     tls_address;
    u32     pe_stack_commit;
    u32     pe_heap_reserve;
    u32     pe_heap_commit;
    u32     pe_base_address;
    u32     pe_size_of_image;
    u32     pe_checksum;
    u32     pe_timedate;
    u32     debug_pathname_address;
    u32     debug_filename_address;
    u32     debug_unicode_filename_address;
    u32     kernel_image_thunk_address;
    u32     non_kernel_import_directory_address;
    u32     number_of_library_versions;
    u32     library_versions_address;
    u32     kernel_library_version_address;
    u32     xapi_library_version_address;
    u32     logo_bitmap_address;
    u32     logo_bitmap_size;
} xbe_header_t;

typedef struct {    // from http://hte.sf.net
    u32     section_flags;
    u32     virtual_address;
    u32     virtual_size;
    u32     raw_address;
    u32     raw_size;
    u32     section_name_address;
    u32     section_name_ref_count;
    u32     head_shared_page_ref_count_address;
    u32     tail_shared_page_ref_count_address;
    u8      section_digest[20];
} xbe_section_t;

typedef struct {
    u8      Name[SECNAMESZ + 1];
    u64     VirtualAddress;
    u32     VirtualSize;
    u32     PointerToRawData;
    u32     SizeOfRawData;
    u32     Characteristics;
} section_t;



unsigned long long rva2file(quickrva_t *q, unsigned long long va) {
    u64     diff;
    int     i,
            ret;

    section_t   *section = q->section;

    va  -= q->imagebase;
    ret  = -1;
    diff = -1;
    for(i = 0; i < q->sections; i++) {
        if((q->sections > 1) && !section[i].VirtualAddress) continue;
        if((va >= section[i].VirtualAddress) && (va < (section[i].VirtualAddress + section[i].VirtualSize))) {
            if((va - section[i].VirtualAddress) < diff) {
                diff = va - section[i].VirtualAddress;
                ret  = i;
            }
        }
    }
    if(ret < 0) return(-1);
    return(section[ret].PointerToRawData + va - section[ret].VirtualAddress);
}



unsigned long long file2rva(quickrva_t *q, unsigned long long file) {
    u64     diff;
    int     i,
            ret;

    section_t   *section = q->section;

    ret  = -1;
    diff = -1;
    for(i = 0; i < q->sections; i++) {
        if((file >= section[i].PointerToRawData) && (file < (section[i].PointerToRawData + section[i].SizeOfRawData))) {
            if((file - section[i].PointerToRawData) < diff) {
                diff = file - section[i].PointerToRawData;
                ret  = i;
            }
        }
    }
    if(ret < 0) return(-1);
    return(section[ret].VirtualAddress + file - section[ret].PointerToRawData);
}



static int get_section(quickrva_t *q, unsigned long long file) {
    u64     diff;
    int     i,
            ret;

    section_t   *section = q->section;

    ret  = -1;
    diff = -1;
    for(i = 0; i < q->sections; i++) {
        if((file >= section[i].PointerToRawData) && (file < (section[i].PointerToRawData + section[i].SizeOfRawData))) {
            if((file - section[i].PointerToRawData) < diff) {
                diff = file - section[i].PointerToRawData;
                ret  = i;
            }
        }
    }
    return(ret);
}



static int parse_PE(quickrva_t *q) {
#ifdef WIN32
    IMAGE_DOS_HEADER        *doshdr;
    MYIMAGE_NT_HEADERS32    *nt32hdr;
    MYIMAGE_NT_HEADERS64    *nt64hdr;
    IMAGE_ROM_HEADERS       *romhdr;
    IMAGE_OS2_HEADER        *os2hdr;
    IMAGE_VXD_HEADER        *vxdhdr;
    IMAGE_SECTION_HEADER    *sechdr;
    vxd_section_t           *vxdsechdr;
    u32     tmp;
    int     i;
    u8      *p;

    p = q->filemem;
    doshdr  = (IMAGE_DOS_HEADER *)p;
    if(doshdr->e_magic != IMAGE_DOS_SIGNATURE) return(-1);

    if((doshdr->e_lfanew >= sizeof(IMAGE_DOS_HEADER)) && doshdr->e_cs) {  // note that the following instructions have been tested on various executables but I'm not sure if they are perfect
        tmp = doshdr->e_cparhdr * 16;
        if(doshdr->e_cs < 0x8000) tmp += doshdr->e_cs * 16;
        p += tmp;
    } else {
        if(doshdr->e_lfanew && ((q->filememsz < 0) || (doshdr->e_lfanew < q->filememsz))) {
            p += doshdr->e_lfanew;
        } else {
            p += sizeof(IMAGE_DOS_HEADER);
        }
    }

    nt32hdr = (MYIMAGE_NT_HEADERS32 *)p;
    nt64hdr = (MYIMAGE_NT_HEADERS64 *)p;
    romhdr  = (IMAGE_ROM_HEADERS *)p;
    os2hdr  = (IMAGE_OS2_HEADER *)p;
    vxdhdr  = (IMAGE_VXD_HEADER *)p;

    if(nt32hdr->Signature == IMAGE_NT_SIGNATURE) {
        if(nt32hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            q->asmsz        = 32;
            p += sizeof(MYIMAGE_NT_HEADERS32);
            q->imagebase    = nt32hdr->OptionalHeader.ImageBase;
            q->sec_align    = nt32hdr->OptionalHeader.SectionAlignment;
            q->entrypoint   = q->imagebase + nt32hdr->OptionalHeader.AddressOfEntryPoint;
            q->sections     = nt32hdr->FileHeader.NumberOfSections;
        } else if(nt64hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            q->asmsz        = 64;
            p += sizeof(MYIMAGE_NT_HEADERS64);
            q->imagebase    = nt64hdr->OptionalHeader.ImageBase;
            q->sec_align    = nt64hdr->OptionalHeader.SectionAlignment;
            q->entrypoint   = q->imagebase + nt64hdr->OptionalHeader.AddressOfEntryPoint;
            q->sections     = nt64hdr->FileHeader.NumberOfSections;
        } else if(romhdr->OptionalHeader.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
            q->asmsz        = 32;
            p += sizeof(IMAGE_ROM_HEADERS);
            q->imagebase    = 0;
            q->sec_align    = MYALIGNMENT;
            q->entrypoint   = q->imagebase + romhdr->OptionalHeader.AddressOfEntryPoint;
            q->sections     = 0;
            q->section      = NULL;
            return(0);
        } else {
            return(-1);
        }

        q->section = calloc(sizeof(section_t), q->sections);
        if(!q->section) return -1; //std_err();
        section_t   *section = q->section;

        sechdr = (IMAGE_SECTION_HEADER *)p;
        for(i = 0; i < q->sections; i++) {
            strncpy(section[i].Name, sechdr[i].Name, IMAGE_SIZEOF_SHORT_NAME);
            section[i].VirtualAddress   = sechdr[i].VirtualAddress;
            section[i].VirtualSize      = sechdr[i].Misc.VirtualSize;
            section[i].PointerToRawData = sechdr[i].PointerToRawData;
            section[i].SizeOfRawData    = sechdr[i].SizeOfRawData;
            section[i].Characteristics  = sechdr[i].Characteristics;
            if(!section[i].VirtualSize) section[i].VirtualSize = section[i].SizeOfRawData;  // Watcom
        }

    } else if(os2hdr->ne_magic == IMAGE_OS2_SIGNATURE) {
        q->asmsz        = 16;
        p += sizeof(IMAGE_OS2_HEADER);
        q->imagebase    = 0;
        q->sec_align    = os2hdr->ne_align;
        q->entrypoint   = q->imagebase + os2hdr->ne_csip;
        q->sections     = 0;

    } else if(
      (vxdhdr->e32_magic == IMAGE_OS2_SIGNATURE_LE) ||  // IMAGE_VXD_SIGNATURE is the same signature
      (vxdhdr->e32_magic == 0x3357) ||                  // LX, W3 and W4: I guess they are the same... I hope
      (vxdhdr->e32_magic == 0x3457) ||
      (vxdhdr->e32_magic == 0x584C)) {
        q->asmsz        = 16;
        p += sizeof(IMAGE_VXD_HEADER);
        q->imagebase    = 0;
        q->sec_align    = vxdhdr->e32_pagesize;
        q->entrypoint   = 0;    // handled later
        q->sections     = vxdhdr->e32_objcnt;

        q->section = calloc(sizeof(section_t), q->sections);
        if(!q->section) return -1; //std_err();
        section_t   *section = q->section;

        tmp = vxdhdr->e32_datapage;
        vxdsechdr = (vxd_section_t *)p;
        for(i = 0; i < q->sections; i++) {
            strncpy(section[i].Name, vxdsechdr[i].name, 4);
            section[i].VirtualAddress   = vxdsechdr[i].base_reloc_addr;
            section[i].VirtualSize      = vxdsechdr[i].vsize;
            section[i].PointerToRawData = tmp;
            section[i].SizeOfRawData    = vxdsechdr[i].vsize;
            section[i].Characteristics  = vxdsechdr[i].flags;
            tmp += MYPAD(section[i].SizeOfRawData);
            if(!q->entrypoint && (tmp > vxdhdr->e32_eip)) {    // I'm not totally sure if this is correct but it's not an important field
                q->entrypoint = section[i].VirtualAddress + vxdhdr->e32_eip;
            }
        }
    } else {
        q->asmsz        = 16;
        q->imagebase    = 0;
        q->sec_align    = 0;
        q->entrypoint   = q->imagebase + (doshdr->e_cs < 0x8000) ? doshdr->e_ip : 0;
        q->sections     = 0;
    }
    return(p - q->filemem);
#else
    return -1;
#endif
}



static int parse_ELF32(quickrva_t *q) {
    elf32_header_t  *elfhdr;
    elf32_section_t *elfsec;
    int     i;
    u8      *p;

    p = q->filemem;
    elfhdr = (elf32_header_t *)p;     p += sizeof(elf32_header_t);
    if(memcmp(elfhdr->e_ident, "\x7f""ELF", 4)) return(-1);
    if(elfhdr->e_ident[4] != 1) return(-1); // only 32 bit supported
    if(elfhdr->e_ident[5] != 1) return(-1); // only little endian

    q->asmsz        = 32;
    q->imagebase    = 0;
    q->sec_align    = 0;
    q->entrypoint   = elfhdr->e_entry;

    q->sections = elfhdr->e_shnum;
    q->section = calloc(sizeof(section_t), q->sections);
    if(!q->section) return -1; //std_err();
    section_t   *section = q->section;

    elfsec = (elf32_section_t *)(q->filemem + elfhdr->e_shoff);
    for(i = 0; i < q->sections; i++) {
        strncpy(section[i].Name, q->filemem + elfsec[elfhdr->e_shstrndx].sh_offset + elfsec[i].sh_name, SECNAMESZ);
        section[i].Name[SECNAMESZ]  = 0;
        section[i].VirtualAddress   = elfsec[i].sh_addr;
        section[i].VirtualSize      = elfsec[i].sh_size;
        section[i].PointerToRawData = elfsec[i].sh_offset;
        section[i].SizeOfRawData    = elfsec[i].sh_size;
        section[i].Characteristics  = elfsec[i].sh_flags;
        if(!section[i].VirtualSize) section[i].VirtualSize = section[i].SizeOfRawData;  // Watcom
    }
    return(p - q->filemem);
}



static int parse_ELF64(quickrva_t *q) {
    elf64_header_t  *elfhdr;
    elf64_section_t *elfsec;
    int     i;
    u8      *p;

    p = q->filemem;
    elfhdr = (elf64_header_t *)p;     p += sizeof(elf64_header_t);
    if(memcmp(elfhdr->e_ident, "\x7f""ELF", 4)) return(-1);
    if(elfhdr->e_ident[4] != 2) return(-1); // only 64 bit supported
    if(elfhdr->e_ident[5] != 1) return(-1); // only little endian

    q->asmsz        = 64;
    q->imagebase    = 0;
    q->sec_align    = 0;
    q->entrypoint   = elfhdr->e_entry;

    q->sections = elfhdr->e_shnum;
    q->section = calloc(sizeof(section_t), q->sections);
    if(!q->section) return -1; //std_err();
    section_t   *section = q->section;

    elfsec = (elf64_section_t *)(q->filemem + elfhdr->e_shoff);
    for(i = 0; i < q->sections; i++) {
        strncpy(section[i].Name, q->filemem + elfsec[elfhdr->e_shstrndx].sh_offset + elfsec[i].sh_name, SECNAMESZ);
        section[i].Name[SECNAMESZ]  = 0;
        section[i].VirtualAddress   = elfsec[i].sh_addr;
        section[i].VirtualSize      = elfsec[i].sh_size;
        section[i].PointerToRawData = elfsec[i].sh_offset;
        section[i].SizeOfRawData    = elfsec[i].sh_size;
        section[i].Characteristics  = elfsec[i].sh_flags;
        if(!section[i].VirtualSize) section[i].VirtualSize = section[i].SizeOfRawData;  // Watcom
    }
    return(p - q->filemem);
}



static int parse_XBE(quickrva_t *q) {
    xbe_header_t    *xbe_header;
    xbe_section_t   *xbe_section;
    int     i;
    u8      *p;

    p = q->filemem;
    xbe_header = (xbe_header_t *)p;     p += sizeof(xbe_header_t);
    if(xbe_header->magic_id != 0x48454258) return(-1);  // XBEH

    q->asmsz        = 32;
    q->imagebase    = xbe_header->base_address;
    q->sec_align    = MYALIGNMENT;  // ???
    q->entrypoint   = q->imagebase + xbe_header->entry_point;
    q->sections     = xbe_header->number_of_sections;

    q->section = calloc(sizeof(section_t), q->sections);
    if(!q->section) return -1; //std_err();
    section_t   *section = q->section;

    xbe_section = (xbe_section_t *)(q->filemem + xbe_header->section_header_address - q->imagebase);
    for(i = 0; i < q->sections; i++) {
        strncpy(section[i].Name, q->filemem + xbe_section[i].section_name_address - q->imagebase, SECNAMESZ);
        section[i].Name[SECNAMESZ]  = 0;
        section[i].VirtualAddress   = xbe_section[i].virtual_address;
        section[i].VirtualSize      = xbe_section[i].virtual_size;
        section[i].PointerToRawData = xbe_section[i].raw_address;
        section[i].SizeOfRawData    = xbe_section[i].raw_size;
        section[i].Characteristics  = xbe_section[i].section_flags;
    }
    return(p - q->filemem);
}



static int parse_file(quickrva_t *q) {
    int     offset;

    if(!q) return -1;

                   offset = parse_PE(q);
    if(offset < 0) offset = parse_ELF32(q);
    if(offset < 0) offset = parse_ELF64(q);
    if(offset < 0) offset = parse_XBE(q);
    if(offset < 0) {
        //printf("Error: this is not a valid DOS/PE/ELF/XBE file or isn't supported yet\n");
        return(-1);
    }
    if(!q->sections || !q->section) { // possible work-around in case of errors
        q->sections = 1;
        if(q->section) free(q->section);
        q->section = calloc(sizeof(section_t), q->sections);
        if(!q->section) return -1; //std_err();
        section_t   *section = q->section;
        section[0].VirtualAddress   = 0;
        section[0].VirtualSize      = (q->filememsz < 0) ? 0x7fffffff : q->filememsz - offset;
        section[0].PointerToRawData = offset;
        section[0].SizeOfRawData    = section[0].VirtualSize;
        section[0].Characteristics  = 0;
    }
    return(0);
}



int quickrva(quickrva_t *q, unsigned char *data, int size) {
    if(!q) return -1;
    memset(q, 0, sizeof(quickrva_t));
    q->filemem      = data;
    q->filememsz    = size;
    return parse_file(q);
}



