#include <stddef.h>

#ifndef FAT_INCLUDED
#define FAT_INCLUDED

#define MAXSECTSZ 512

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
    unsigned char datetime[4];
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
    char new_file[12]; /*new filename for fatRename*/
} FAT;

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
int fatDeleteFile(FAT *fat,const char *fnm); /*To delete a file from a given directory*/
int fatRenameFile(FAT *fat,const char *old,const char *new);/*To rename a given file*/
#endif
