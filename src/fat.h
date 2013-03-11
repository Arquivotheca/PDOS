#include <stddef.h>

#ifndef FAT_INCLUDED
#define FAT_INCLUDED

#define MAXSECTSZ 512

typedef struct {
    unsigned long num_tracks;
    unsigned int num_cylinders;
    unsigned int num_heads;
    unsigned int sectors_per_track;
    unsigned int sector_size;
    unsigned long sectors_per_cylinder;
    unsigned long sectors_per_disk;
    unsigned int sectors_per_cluster;
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
    void (*readLogical)(void *diskptr, long sector, void *buf);
    void (*writeLogical)(void *diskptr, long sector, void *buf);
    void *parm;
} FAT;

typedef struct {
    int root;
    unsigned int cluster;
    unsigned long fileSize;
    unsigned lastSectors;
    unsigned lastBytes;
    unsigned int currentCluster;
    unsigned char datetime[4];
    unsigned int attr;
    unsigned int sectorCount;
    unsigned long sectorStart;
    unsigned int nextCluster;
    unsigned int byteUpto;
    unsigned int sectorUpto;
    unsigned long dirSect;
    unsigned int dirOffset;
    unsigned long totbytes;
    int dir;
} FATFILE;

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

#endif
