

#define CM_FLOPPY	0x1
#define CM_HARDDISK	0x2
#define CM_NFS		0x3


#define CM_READ 	0x1
#define CM_WRITE 	0x2


struct medium {
    int media;
    int flags;
    char *path;
    char *regex;
};

