//
//  elf.h
//  DisARM
//
//

#ifndef DisARM_elf_h
#define DisARM_elf_h

typedef struct _elfHeader
{
    char magic[4];
    char class;
    char byteorder;
    char hversion;
    char pad[9];
    
    short filetype;
    short archtype;
    
    int fversion;
    int entry;
    int phdrpos;
    int shdrpos;
    int flags;
    
    short hdrsize;
    short phdrent;
    short phdrcnt;
    short shdrent;
    short shdrcnt;
    short strsec;
    
} ELFHEADER;

typedef struct _elfProgHeader
{
    int type;
    int offset;
    int virtaddr;
    int physaddr;
    int filesize;
    int memsize;
    int flags;
    int align;
    
} ELFPROGHDR;

#endif
