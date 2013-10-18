#include <stddef.h>

#ifndef FAT_INCLUDED
#define FAT_INCLUDED

#define MAXSECTSZ 512

/*File name special values*/
#define DIRENT_AVA 0x00         
#define DIRENT_IC 0x05           
#define DIRENT_DOT 0x2E         
#define DIRENT_DEL 0xE5         
/**/

/*File attributes*/
#define DIRENT_READONLY 0x01    
#define DIRENT_HIDDEN 0x02       
#define DIRENT_SYSTEM 0x04      
#define DIRENT_SUBDIR 0x10       
#define DIRENT_ARCHIVE 0x20      
#define DIRENT_DEVICE 0X40      
#define DIRENT_RESERVED 0x80    
/**/

/*Last access attributes*/
#define DIRENT_EXTRAB7  0x80    
#define DIRENT_EXTRAB6  0x40    
#define DIRENT_EXTRAB5  0x20    
#define DIRENT_EXTRAB4  0x10    
#define DIRENT_EXTRAB3  0x08    
#define DIRENT_EXTRAB2  0x04    
#define DIRENT_EXTRAB1  0x02    
#define DIRENT_EXTRAB0  0x01    
/**/

/*File access rights bitmap*/
#define DIRENT_ACCESSB0 0x0001  
#define DIRENT_ACCESSB1 0x0002  
#define DIRENT_ACCESSB2 0x0004  
#define DIRENT_ACCESSB3 0x0008  
#define DIRENT_ACCESSB4 0x0010  
#define DIRENT_ACCESSB5 0x0020  
#define DIRENT_ACCESSB6 0x0040  
#define DIRENT_ACCESSB7 0x0080  
#define DIRENT_ACCESSB8 0x0100  
#define DIRENT_ACCESSB9 0x0200  
#define DIRENT_ACCESSB10 0x0400 
#define DIRENT_ACCESSB11 0x0800 
/**/

typedef struct {
    int root;
    unsigned int cluster; /* start cluster for this file (for reading)
        or current cluster (for writing) */
    unsigned long fileSize;
    unsigned int lastSectors; /* when opening a file for read, this
        contains the number of sectors expected in the last cluster,
        based on file size and cluster size. */
    unsigned int lastBytes; /* when opening a file for read, this
        contains the number of bytes expected in the last sector.
        when opening for write, it contains how many bytes were
        written in the last sector. */
    unsigned int currentCluster; /* cluster currently up to for
        reading. */
    unsigned int ftime;
    unsigned int fdate;
    unsigned int attr;
    unsigned int sectorCount; /* how many sectors in the current cluster */
    unsigned long sectorStart; /* starting sector for this cluster */
    unsigned int nextCluster; /* next cluster in chain */
    unsigned int byteUpto;
    unsigned int sectorUpto; /* which sector currently being processed,
    zero-based */
    unsigned long dirSect; /* sector which contains directory entry */
    unsigned int dirOffset; /* offset in sector for this directory entry */
    unsigned long totbytes; /* total bytes written to this file */
    int dir; /* whether this is a directory or a file */
} FATFILE;

typedef struct {
    unsigned long num_tracks;
    unsigned int num_cylinders;
    unsigned int num_heads;
    unsigned int sectors_per_track;
    unsigned int sector_size;
    unsigned long sectors_per_cylinder;
    unsigned long sectors_per_disk;
    unsigned int sectors_per_cluster;
    unsigned int bytes_per_cluster;
    unsigned int numfats;
    unsigned int bootstart;
    unsigned int fatstart;
    unsigned int rootstart;
    unsigned int filestart;
    unsigned int drive;
    unsigned char pbuf[MAXSECTSZ];
    unsigned char *buf;
    unsigned int rootentries;
    unsigned int rootsize;
    unsigned int fatsize;
    int fat16;
    unsigned long hidden;
    int notfound;
    int operation; /*Defines the fat operation to be performed*/
    int currcluster;
    FATFILE *currfatfile;
    void (*readLogical)(void *diskptr, long sector, void *buf);
    void (*writeLogical)(void *diskptr, long sector, void *buf);
    void *parm;
    char new_file[12]; /*new filename for rename*/
    int last;
	int new_attr;
} FAT;

/*Structure for Directory Entry */
typedef struct {
    unsigned char file_name[8]; /*Short file name (0x00)*/       
    unsigned char file_ext[3];  /*Short file extension (0x08)*/ 
    unsigned char file_attr;    /*file attributes (0x0B)*/
    unsigned char extra_attributes;  /*extra attributes (0x0C)*/
    unsigned char first_char;        /*first character of deleted file
                                       (0x0D)*/
    unsigned char create_time[2];    /*create time(0x0E)*/
    unsigned char create_date[2];    /*create date(0x10)*/
    unsigned char last_access[2];    /*last access (0x12)*/
    unsigned char access_rights[2];  /*file access rights (0x14)*/
    unsigned char last_modtime[2];   /*last modified time (0x16)*/
    unsigned char last_moddate[2];   /*last modified date (0x18)*/
    unsigned char start_cluster[2];  /*Size of file in clusters (0x1A)*/
    unsigned char file_size[4];      /*Size of file in bytes (0x1C) */
}DIRENT;
/**/


void fatDefaults(FAT *fat);
void fatInit(FAT *fat,
             unsigned char *bpb,
             void (*readLogical)(void *diskptr, long sector, void *buf),
             void (*writeLogical)(void *diskptr, long sector, void *buf),
             void *parm);
void fatTerm(FAT *fat);
int fatCreatFile(FAT *fat, const char *fnm, FATFILE *fatfile, int attrib);
int fatOpenFile(FAT *fat, const char *fnm, FATFILE *fatfile);
size_t fatReadFile(FAT *fat, FATFILE *fatfile, void *buf, size_t szbuf);
size_t fatWriteFile(FAT *fat, FATFILE *fatfile, void *buf, size_t szbuf);
int fatDeleteFile(FAT *fat,const char *fnm); 
/*To delete a file from a given directory*/
int fatRenameFile(FAT *fat,const char *old,const char *new);
/*To rename a given file*/
int fatGetFileAttributes(FAT *fat,const char *fnm,int *attr);
/*To Get the file attributes from the file name specified by fnm*/
int fatSetFileAttributes(FAT *fat,const char *fnm,int attr);
/*To Set the attributes of the given file*/
int fatUpdateDateAndTime(FAT *fat,FATFILE *fatfile);
/*To update the date and time of the given file*/
#endif
