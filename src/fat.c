/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  fat - file allocation table routines                             */
/*                                                                   */
/*  layout is boot sector, then FAT (clusters) then root directory   */
/*  then file storage area                                           */
/*                                                                   */
/*********************************************************************/

#include <string.h>
#include <stdio.h>
#include "fat.h"
#include "bos.h"
#include "unused.h"
#include "pos.h"

static int fatEndCluster(FAT *fat, unsigned int cluster);
static void fatPosition(FAT *fat, const char *fnm);
static void fatNextSearch(FAT *fat, char *search, const char **upto);
static void fatRootSearch(FAT *fat, char *search);
static void fatDirSearch(FAT *fat, char *search);
static void fatClusterAnalyse(FAT *fat,
                              unsigned int cluster,
                              unsigned long *startSector,
                              unsigned int *nextCluster);
static void fatDirSectorSearch(FAT *fat,
                               char *search,
                               unsigned long startSector,
                               int numsectors);
static void fatReadLogical(FAT *fat, long sector, void *buf);
static void fatWriteLogical(FAT *fat, long sector, void *buf);
static void fatMarkCluster(FAT *fat, unsigned int cluster);
static unsigned int fatFindFreeCluster(FAT *fat);
static void fatChain(FAT *fat, FATFILE *fatfile);
static void fatNuke(FAT *fat, unsigned int cluster);

static unsigned char dir_buf[MAXSECTSZ];


/*
 * fatDefaults - initial call which cannot fail
 */

void fatDefaults(FAT *fat)
{
    fat->sectors_per_cylinder = 1;
    fat->num_heads = 1;
    fat->sectors_per_track = 1;
    fat->bootstart = 0;
    fat->fatstart = 1;
    fat->dbuf = dir_buf;
    return;
}


/*
 * fatInit - initialize the FAT handler.
 */

void fatInit(FAT *fat,
             unsigned char *bpb,
             void (*readLogical)(void *diskptr, long sector, void *buf),
             void (*writeLogical)(void *diskptr, long sector, void *buf),
             void *parm)
{
    fat->readLogical = readLogical;
    fat->writeLogical = writeLogical;
    fat->parm = parm;
    fat->drive = bpb[25];
    fat->sector_size = bpb[0] | ((unsigned int)bpb[1] << 8);
    fat->sectors_per_cluster = bpb[2];
    fat->bytes_per_cluster = fat->sector_size * fat->sectors_per_cluster;
#if 0
    printf("sector_size is %x\n", (int)fat->sector_size);
    printf("sectors per cluster is %x\n", (int)fat->sectors_per_cluster);
#endif
    fat->numfats = bpb[5];
    fat->fatsize = bpb[11] | ((unsigned int)bpb[12] << 8);
    fat->sectors_per_disk = bpb[8] | ((unsigned int)bpb[9] << 8);
    fat->num_heads = bpb[15];
    if (fat->sectors_per_disk == 0)
    {
        fat->sectors_per_disk =
            bpb[21]
            | ((unsigned int)bpb[22] << 8)
            | ((unsigned long)bpb[23] << 16)
            | ((unsigned long)bpb[24] << 24);
    }
    fat->hidden = bpb[17]
                  | ((unsigned long)bpb[18] << 8)
                  | ((unsigned long)bpb[19] << 16)
                  | ((unsigned long)bpb[20] << 24);
    fat->rootstart = fat->fatsize * fat->numfats + fat->fatstart;
    fat->rootentries = bpb[6] | ((unsigned int)bpb[7] << 8);
    fat->rootsize = fat->rootentries / (fat->sector_size / 32);
    fat->sectors_per_track = (bpb[13] | ((unsigned int)bpb[14] << 8));
    fat->filestart = fat->rootstart + fat->rootsize;
#if 0
    printf("filestart is %d\n", fat->filestart);
#endif
    fat->num_tracks = fat->sectors_per_disk / fat->sectors_per_track;
    fat->num_cylinders = (unsigned int)(fat->num_tracks / fat->num_heads);
    fat->sectors_per_cylinder = fat->sectors_per_track * fat->num_heads;
    if ((fat->sectors_per_disk / fat->sectors_per_cluster) < 4087)
    {
        fat->fat16 = 0;
    }
    else
    {
        fat->fat16 = 1;
    }
    return;
}


/*
 * fatTerm - terminate the FAT handler.
 */

void fatTerm(FAT *fat)
{
    unused(fat);
    return;
}


/*
 * fatEndCluster - determine whether the cluster number provided
 * is an indicator of "no more clusters, it's time to go to bed",
 * courtesy Neil, Young Ones.
 */

static int fatEndCluster(FAT *fat, unsigned int cluster)
{
    if (fat->fat16)
    {
        /* if(cluster==0) return (1); */
        if (cluster >= 0xfff8U)
        {
            return (1);
        }
    }
    else
    {
        if ((cluster >= 0xff8U) || (cluster == 0xff0U))
        {
            return (1);
        }
    }
    return (0);
}


/* Return codes should be Positive */

unsigned int fatCreatFile(FAT *fat, const char *fnm, FATFILE *fatfile,
                          int attrib)
{
    DIRENT *p;
    int found = 0;
    unsigned char lbuf[MAXSECTSZ];
    FATFILE tempfatfile;

    fat->notfound = 0;

    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    fatPosition(fat,fnm);
    p = fat->de;

    if (!fat->notfound)
    {
        fat->currcluster = p->start_cluster[1] << 8
                           | p->start_cluster[0];
        fatNuke(fat, fat->currcluster);
        found = 1;
    }
    if (found || (p->file_name[0] == '\0'))
    {
        fatfile->startcluster=0;
        memset(p, '\0', sizeof(DIRENT));
        memcpy(p->file_name, fat->search,
              (sizeof(p->file_name)+sizeof(p->file_ext)));
        p->file_attr = (unsigned char)attrib;
        fatfile->totbytes = 0;
        p->file_size[0] = fatfile->totbytes;
        p->file_size[1] = (fatfile->totbytes >> 8) & 0xff ;
        p->file_size[2] = (fatfile->totbytes >> 16) & 0xff ;
        p->file_size[3] = (fatfile->totbytes >> 24) & 0xff ;

        fatfile->dirSect = fat->dirSect;
        fatfile->dirOffset = ((unsigned char*)p - fat->dbuf);
        fatfile->lastBytes = 0;

        /* if file was found, don't mark next entry as null */
        if (found)
        {
            fatWriteLogical(fat, fat->dirSect, fat->dbuf);
        }
        else
        {
            p++;
            if ((unsigned char *) p != (fat->dbuf + fat->sector_size))
            {
                p->file_name[0] = '\0';
                fatWriteLogical(fat, fat->dirSect, fat->dbuf);
            }
            else
            {
                fatWriteLogical(fat, fat->dirSect, fat->dbuf);

                if ((fat->dirSect - fat->startSector
                    == fat->sectors_per_cluster - 1))
                {
                    if (fat->processing_root) return (POS_ERR_PATH_NOT_FOUND);
                    fatChain(fat,&tempfatfile);
                    fat->dirSect = tempfatfile.sectorStart;
                }
                else
                {
                    fat->dirSect++;
                }

                fatReadLogical(fat, fat->dirSect, lbuf);
                lbuf[0] = '\0';
                fatWriteLogical(fat, fat->dirSect, lbuf);
            }
        }
    }
    return (0);
}

unsigned int fatCreatDir(FAT *fat, const char *dnm, const char *parentname,
                         int attrib)
{
    DIRENT *p;
    DIRENT dot;
    unsigned char lbuf[MAXSECTSZ];
    int startcluster = 0;
    int parentstartcluster = 0;
    long startsector;
    FATFILE tempfatfile;

    if ((dnm[0] == '\\') || (dnm[0] == '/'))
    {
        dnm++;
    }
    if ((parentname[0] == '\\') || (parentname[0] == '/'))
    {
        parentname++;
    }

    if(parentname[0] != '\0')
    {
        fatPosition(fat,parentname);
        if (!fat->currcluster) return (POS_ERR_PATH_NOT_FOUND);
        parentstartcluster = fat->currcluster;
    }
    else
    {
        parentstartcluster = 0;
    }

    fatPosition(fat,dnm);
    p = fat->de;

    if (!fat->notfound)
    {
        return (POS_ERR_PATH_NOT_FOUND);
    }
    if (p->file_name[0] == '\0')
    {
        startcluster = fatFindFreeCluster(fat);
        if(!(startcluster)) return (POS_ERR_PATH_NOT_FOUND);

        fat->currcluster = startcluster;

        memset(p, '\0', sizeof(DIRENT));
        memcpy(p->file_name, fat->search,
              (sizeof(p->file_name)+sizeof(p->file_ext)));
        p->file_attr = (unsigned char)attrib;
        p->start_cluster[1] = (fat->currcluster >> 8) & 0xff;
        p->start_cluster[0] = fat->currcluster & 0xff;

        p->file_size[0] = p->file_size[1] = p->file_size[2]
            = p->file_size[3] = 0;

        p++;
        if ((unsigned char *) p != (fat->dbuf + fat->sector_size))
        {
            p->file_name[0] = '\0';
            fatWriteLogical(fat, fat->dirSect, fat->dbuf);
        }
        else
        {
            fatWriteLogical(fat, fat->dirSect, fat->dbuf);

            if ((fat->dirSect - fat->startSector
                == fat->sectors_per_cluster - 1))
            {
                if (fat->processing_root) fat->processing_root = 2;
                else
                {
                    fatChain(fat,&tempfatfile);
                    fat->dirSect = tempfatfile.sectorStart;
                }
            }
            else
            {
                fat->dirSect++;
            }

            if (fat->processing_root != 2)
            {
                fatReadLogical(fat, fat->dirSect, lbuf);
                lbuf[0] = '\0';
                fatWriteLogical(fat, fat->dirSect, lbuf);
            }
        }

        fat->currcluster = startcluster;
        fatMarkCluster(fat, fat->currcluster);

        startsector = (fat->currcluster - 2)
                * (long)fat->sectors_per_cluster
                + fat->filestart;

        memset(lbuf,'\0',sizeof(lbuf));

        memset(&dot,'\0',sizeof(dot));
        memset(dot.file_name, ' ', sizeof(dot.file_name)
               + sizeof(dot.file_ext));
        dot.file_name[0] = DIRENT_DOT;
        dot.file_attr = DIRENT_SUBDIR;

        dot.start_cluster[1] = (startcluster >> 8) & 0xff;
        dot.start_cluster[0] = startcluster & 0xff;

        memcpy(lbuf,&dot,sizeof(dot));

        memset(&dot,'\0',sizeof(dot));
        memset(dot.file_name, ' ', sizeof(dot.file_name)
               + sizeof(dot.file_ext));
        dot.file_name[0] = DIRENT_DOT;
        dot.file_name[1] = DIRENT_DOT;
        dot.file_attr = DIRENT_SUBDIR;

        dot.start_cluster[1] = (parentstartcluster >> 8) & 0xff;
        dot.start_cluster[0] = parentstartcluster & 0xff;

        memcpy(lbuf+sizeof(dot),&dot,sizeof(dot));

        fatWriteLogical(fat, startsector, lbuf);
    }
    return (0);
}

unsigned int fatCreatNewFile(FAT *fat, const char *fnm, FATFILE *fatfile,
                          int attrib)
{
    DIRENT *p;
    int found = 0;
    unsigned char lbuf[MAXSECTSZ];
    FATFILE tempfatfile;

    fat->notfound = 0;

    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    fatPosition(fat,fnm);
    p = fat->de;

    if (!fat->notfound)
    {
        return (POS_ERR_FILE_EXISTS);
    }
    if (found || (p->file_name[0] == '\0'))
    {
        fatfile->startcluster=0;
        memset(p, '\0', sizeof(DIRENT));
        memcpy(p->file_name, fat->search,
              (sizeof(p->file_name)+sizeof(p->file_ext)));
        p->file_attr = (unsigned char)attrib;
        fatfile->totbytes = 0;
        p->file_size[0] = fatfile->totbytes;
        p->file_size[1] = (fatfile->totbytes >> 8) & 0xff ;
        p->file_size[2] = (fatfile->totbytes >> 16) & 0xff ;
        p->file_size[3] = (fatfile->totbytes >> 24) & 0xff ;

        fatfile->dirSect = fat->dirSect;
        fatfile->dirOffset = ((unsigned char*)p - fat->dbuf);
        fatfile->lastBytes = 0;

        /* if file was found, don't mark next entry as null */
        if (found)
        {
            fatWriteLogical(fat, fat->dirSect, fat->dbuf);
        }
        else
        {
            p++;
            if ((unsigned char *) p != (fat->dbuf + fat->sector_size))
            {
                p->file_name[0] = '\0';
                fatWriteLogical(fat, fat->dirSect, fat->dbuf);
            }
            else
            {
                fatWriteLogical(fat, fat->dirSect, fat->dbuf);

                if ((fat->dirSect - fat->startSector
                    == fat->sectors_per_cluster - 1))
                {
                    if (fat->processing_root) return (POS_ERR_PATH_NOT_FOUND);
                    fatChain(fat,&tempfatfile);
                    fat->dirSect = tempfatfile.sectorStart;
                }
                else
                {
                    fat->dirSect++;
                }

                fatReadLogical(fat, fat->dirSect, lbuf);
                lbuf[0] = '\0';
                fatWriteLogical(fat, fat->dirSect, lbuf);
            }
        }
    }
    return (0);
}


/* If opening root directory, abide by rootentries.  If opening
   subdirectory or file, follow clusters */

unsigned int fatOpenFile(FAT *fat, const char *fnm, FATFILE *fatfile)
{
    DIRENT *p;

    fat->notfound = 0;
    fat->currfatfile=fatfile;
    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    if (strcmp(fnm, "") == 0)
    {
        fatfile->root = 1;
        fatfile->nextCluster = 0xffff;
        fatfile->sectorCount = fat->rootsize;
        fatfile->sectorStart = fat->rootstart;
        fatfile->lastBytes = 0;
        fatfile->lastSectors = fat->rootsize;
        fat->currcluster = 0;
        fatfile->dir = 1;
        fatfile->attr=DIRENT_SUBDIR;
    }
    else
    {
        fatfile->root = 0;
        fatPosition(fat, fnm);
        if (fat->notfound) return (POS_ERR_FILE_NOT_FOUND);
        p = fat->de;
        fatfile->dirSect = fat->dirSect;
        fatfile->dirOffset = ((unsigned char*)p - fat->dbuf);
        fatfile->startcluster = fat->currcluster;
        fatfile->fileSize = p->file_size[0]
                            | ((unsigned long)p->file_size[1] << 8)
                            | ((unsigned long)p->file_size[2] << 16)
                            | ((unsigned long)p->file_size[3] << 24);
        /* empty files have a start cluster of 0 */
        if ((fatfile->fileSize == 0)
            && (fatfile->startcluster == 0))
        {
            if (fat->fat16)
            {
                fat->currcluster = fatfile->startcluster = 0xfff8;
            }
            else
            {
                fat->currcluster = fatfile->startcluster = 0xff8;
            }
        }

        fatfile->attr = p->file_attr;

        fatfile->ftime= p->last_modtime[0]
                    | ((unsigned int) p->last_modtime[1] << 8);

        fatfile->fdate= p->last_moddate[0]
                    | ((unsigned int) p->last_moddate[1] << 8);

        fatfile->lastBytes = (unsigned int)
                             (fatfile->fileSize
                              % (fat->sectors_per_cluster
                                 * fat->sector_size));
        fatfile->lastSectors = fatfile->lastBytes / fat->sector_size;
        fatfile->lastBytes = fatfile->lastBytes % fat->sector_size;
        fatfile->dir = (fatfile->fileSize == 0);
        fatClusterAnalyse(fat,
                          fat->currcluster,
                          &fatfile->sectorStart,
                          &fatfile->nextCluster);
        fatfile->sectorCount = fat->sectors_per_cluster;
    }
    fatfile->currentCluster = fat->currcluster;
    fatfile->sectorUpto = 0;
    fatfile->byteUpto = 0;
    if (fat->notfound)
    {
        return (POS_ERR_FILE_NOT_FOUND);
    }
    return (0);
}


/*
 * fatReadFile - read from an already-open file.
 */

size_t fatReadFile(FAT *fat, FATFILE *fatfile, void *buf, size_t szbuf)
{
    size_t bytesRead = 0;
    static unsigned char bbuf[MAXSECTSZ];
    int bytesAvail;
    int sectorsAvail;

    /* until we reach the end of the chain */
    while (!fatEndCluster(fat, fatfile->currentCluster))
    {
        /* assume all sectors in cluster are available */
        sectorsAvail = fatfile->sectorCount;

        if (fatEndCluster(fat, fatfile->nextCluster) && !fatfile->dir)
        {
            /* exception - a full cluster has 0 last sectors */
            if ((fatfile->lastSectors == 0) && (fatfile->lastBytes == 0))
            {
                /* do nothing */
            }
            else
            {
                /* reduce sectors available */
                sectorsAvail = fatfile->lastSectors + 1;
            }
        }

        /* cycle through the sectors */
        while (fatfile->sectorUpto != sectorsAvail)
        {
            /* assume whole sector is available */
            bytesAvail = fat->sector_size;

            /* but if this is the last cluster, we need special processing */
            if (fatEndCluster(fat, fatfile->nextCluster) && !fatfile->dir)
            {
                /* if we have reached the last sector, abide by lastBytes */
                if (fatfile->sectorUpto == fatfile->lastSectors)
                {
                    /* exception - a full cluster has 0 last sectors */
                    if ((fatfile->lastSectors == 0)
                        && (fatfile->lastBytes == 0))
                    {
                        /* do nothing */
                    }
                    else
                    {
                        bytesAvail = fatfile->lastBytes;
                    }
                }
            }
            /* while we haven't used up the bytesAvail */
            while (fatfile->byteUpto != bytesAvail)
            {
                /* read data */
                fatReadLogical(fat,
                               fatfile->sectorStart + fatfile->sectorUpto,
                               bbuf);
                /* copy data to supplied buffer */
                if ((bytesAvail - fatfile->byteUpto)
                    > (szbuf - bytesRead))
                {
                    memcpy((char *)buf + bytesRead,
                           bbuf + fatfile->byteUpto,
                           szbuf - bytesRead);
                    fatfile->byteUpto += (szbuf - bytesRead);
                    bytesRead = szbuf;
                    return (bytesRead);
                }
                else
                {
                    memcpy((char *)buf + bytesRead,
                           bbuf + fatfile->byteUpto,
                           bytesAvail - fatfile->byteUpto);
                    bytesRead += (bytesAvail - fatfile->byteUpto);
                    fatfile->byteUpto += (bytesAvail - fatfile->byteUpto);
                }
            }
            fatfile->sectorUpto++;
            fatfile->byteUpto = 0;
        }
        fatfile->currentCluster = fatfile->nextCluster;
        fatClusterAnalyse(fat,
                          fatfile->currentCluster,
                          &fatfile->sectorStart,
                          &fatfile->nextCluster);
        fatfile->sectorUpto = 0;
    }
    return (bytesRead);
}


/*
 * fatWriteFile - write to an already-open file.
 */

size_t fatWriteFile(FAT *fat, FATFILE *fatfile, const void *buf, size_t szbuf)
{
    static unsigned char bbuf[MAXSECTSZ];
    size_t rem; /* remaining bytes in sector */
    size_t tsz; /* temporary size */
    size_t done; /* written bytes */
    DIRENT *d;


    /* Regardless of whether szbuf is 0 or remaining bytes is 0,
       we will not break into a new sector or cluster */
    if (szbuf == 0) return (0);

    if ((fatfile->startcluster==0)
        || (fat->fat16 && (fatfile->startcluster == 0xfff8))
        || (!fat->fat16 && (fatfile->startcluster == 0xff8))
       )
    {
        fat->currcluster = fatFindFreeCluster(fat);
        fatfile->startcluster = fat->currcluster;
        fatMarkCluster(fat, fat->currcluster);
        fatfile->sectorStart = (fat->currcluster - 2)
                * (long)fat->sectors_per_cluster
                + fat->filestart;
        fatfile->sectorUpto = 0;

        fatReadLogical(fat,
                       fatfile->dirSect,
                       bbuf);

        d = (DIRENT *) (bbuf + fatfile->dirOffset);

        d->start_cluster[1] = (fat->currcluster >> 8) & 0xff;
        d->start_cluster[0] = fat->currcluster & 0xff;

        fatWriteLogical(fat,
                        fatfile->dirSect,
                        bbuf);
    }
    /* get remainder of bytes in current sector */
    rem = fat->sector_size - fatfile->lastBytes;

    /* If the last written bytes were 0, or the remainder 0, we
       don't need to update (and thus read) this sector */
    if ((fatfile->lastBytes != 0) && (rem != 0))
    {
        fatReadLogical(fat,
                       fatfile->sectorStart + fatfile->sectorUpto,
                       bbuf);
    }

    /* if the size of data to be written fits in to the remaining
       space, then update the just-read buffer */
    if (szbuf <= rem)
    {
        memcpy(bbuf + fatfile->lastBytes,
               buf,
               szbuf);
        fatWriteLogical(fat,
                        fatfile->sectorStart + fatfile->sectorUpto,
                        bbuf);
        fatfile->lastBytes += szbuf;
        fatfile->totbytes += szbuf;
    }
    else
    {
        /* we have more data than can fit in this sector.
           so use up remaining space */
        if (rem != 0)
        {
            memcpy(bbuf + fatfile->lastBytes,
                   buf,
                   rem);
            fatWriteLogical(fat,
                            fatfile->sectorStart + fatfile->sectorUpto,
                            bbuf);
        }
        done = rem; /* advance position in user-provided buffer */
        tsz = szbuf - rem; /* size of user-provided data remaining */
        fatfile->sectorUpto++;
        /* as soon as we get up to a full sector it is time to break
           into a new cluster */
        if (fatfile->sectorUpto == fat->sectors_per_cluster)
        {
            fatChain(fat, fatfile);
        }
        /* go through data, each full sector at a time */
        while (tsz > fat->sector_size)
        {
            memcpy(bbuf, (char *)buf + done, fat->sector_size);
            fatWriteLogical(fat,
                            fatfile->sectorStart + fatfile->sectorUpto,
                            bbuf);
            fatfile->sectorUpto++;

            /* break into a new cluster when full */
            if (fatfile->sectorUpto == fat->sectors_per_cluster)
            {
                fatChain(fat, fatfile);
            }
            tsz -= fat->sector_size;
            done += fat->sector_size;
        }
        memcpy(bbuf, (char *)buf + done, tsz);
        fatWriteLogical(fat,
                        fatfile->sectorStart + fatfile->sectorUpto,
                        bbuf);
        /* lastBytes records the amount of data written to the
           last sector */
        fatfile->lastBytes = tsz;

        /* totbytes contains the size of the entire file */
        fatfile->totbytes += szbuf;
    }

    /* now update the directory sector with the new file size */
    fatReadLogical(fat,
                   fatfile->dirSect,
                   bbuf);

    d = (DIRENT *) (bbuf + fatfile->dirOffset);
    d->file_size[0]=fatfile->totbytes & 0xff;
    d->file_size[1]=(fatfile->totbytes >> 8) & 0xff;
    d->file_size[2]=(fatfile->totbytes >> 16) & 0xff;
    d->file_size[3]=(fatfile->totbytes >> 24)  & 0xff;

    fatWriteLogical(fat,
                    fatfile->dirSect,
                    bbuf);
    return (szbuf);
}


/*
 * fatPosition - given a filename, retrieve the starting
 * cluster, or set not found flag if not found.
 */

static void fatPosition(FAT *fat, const char *fnm)
{
    fat->notfound = 0;
    fat->upto = fnm;
    fat->currcluster = 0;
    fat->processing_root = 0;
    fatNextSearch(fat, fat->search, &fat->upto);
    if (fat->notfound) return;
    fatRootSearch(fat, fat->search);
    if (fat->notfound)
    {
        fat->processing_root = 1;
        return;
    }
    while (!fat->last)
    {
        fatNextSearch(fat, fat->search, &fat->upto);
        if (fat->notfound) return;
        fatDirSearch(fat, fat->search);
    }
    return;
}


/*
 * fatNextSearch - get the next part of the filename in order
 * to do a search.
 *
 * Inputs:
 * fat - pointer to the current FAT object, unused.
 * *upto - the bit of the filename we are ready to search
 *
 * Outputs:
 * *upto - the bit of the filename to search next time
 * search - set to the portion of the filename we should look
 *          for in the current directory
 * fat->last - set to 1 or 0 depending on whether this was the last
 *         portion of the filename.(int last defined in struct FAT)
 *
 * e.g. if the original filename was \FRED\MARY\JOHN.TXT and
 * *upto was pointing to MARY... on input, search will get
 * "MARY            " in it, and *upto will point to "JOHN...".
 */

static void fatNextSearch(FAT *fat, char *search, const char **upto)
{
    const char *p;
    const char *q;

    unused(fat);
    p = strchr(*upto, '\\');
    q = strchr(*upto, '/');
    if (p == NULL)
    {
        p = q;
    }
    else if (q != NULL)
    {
        if (q < p)
        {
            p = q;
        }
    }
    if (p == NULL)
    {
        p = *upto + strlen(*upto);
        fat->last = 1;
    }
    else
    {
        fat->last = 0;
    }
    if ((p - *upto) > 12)
    {
        fat->notfound = 1;
        return;
    }
    q = memchr(*upto, '.', p - *upto);
    if (q != NULL)
    {
        if ((q - *upto) > 8)
        {
            fat->notfound = 1;
            return;
        }
        if ((p - q) > 4)
        {
            fat->notfound = 1;
            return;
        }
        memcpy(search, *upto, q - *upto);
        memset(search + (q - *upto), ' ', 8 - (q - *upto));
        memcpy(search + 8, q + 1, p - q - 1);
        memset(search + 8 + (p - q) - 1, ' ', 3 - ((p - q) - 1));
    }
    else
    {
        memcpy(search, *upto, p - *upto);
        memset(search + (p - *upto), ' ', 11 - (p - *upto));
    }
    if (fat->last)
    {
        *upto = p;
    }
    else
    {
        *upto = p + 1;
    }
    return;
}


/*
 * fatRootSearch - search the root directory for an entry
 * (given by "search").  The root directory is defined by a
 * starting sector number and the previously determined size
 * (number of sectors).
 */

static void fatRootSearch(FAT *fat, char *search)
{
    fatDirSectorSearch(fat, search,fat->rootstart, fat->rootsize);
    return;
}


/*
 * fatDirSearch - search a directory chain looking for the
 * search string.
 */

static void fatDirSearch(FAT *fat, char *search)
{
    unsigned int nextCluster;
    fat->fnd=0;

    fatClusterAnalyse(fat, fat->currcluster, &fat->startSector, &nextCluster);
    fatDirSectorSearch(fat,
                       search,
                       fat->startSector,
                       fat->sectors_per_cluster);

    while (fat->fnd == 0)    /* not found but not end */
    {
        if (fatEndCluster(fat, nextCluster))
        {
            fat->notfound = 1;
            return;
        }
        fat->currcluster = nextCluster;
        fatClusterAnalyse(fat, fat->currcluster, &fat->startSector, &nextCluster);
        fatDirSectorSearch(fat,
                           search,
                           fat->startSector,
                           fat->sectors_per_cluster);
    }
    return;
}


/*
 * fatClusterAnalyse - given a cluster number, this function
 * determines the sector that the cluster starts and also the
 * next cluster number in the chain.
 */

static void fatClusterAnalyse(FAT *fat,
                              unsigned int cluster,
                              unsigned long *startSector,
                              unsigned int *nextCluster)
{
    unsigned int fatSector;
    static unsigned char buf[MAXSECTSZ];
    int offset;

    *startSector = (cluster - 2) * (long)fat->sectors_per_cluster
                   + fat->filestart;
    if (fat->fat16)
    {
        fatSector = fat->fatstart + (cluster * 2) / fat->sector_size;
        fatReadLogical(fat, fatSector, buf);
        offset = (cluster * 2) % fat->sector_size;
        *nextCluster = buf[offset] | ((unsigned int)buf[offset + 1] << 8);
    }
    else
    {
        fatSector = fat->fatstart + (cluster * 3 / 2) / fat->sector_size;
        fatReadLogical(fat, fatSector, buf);
        offset = (cluster * 3 / 2) % fat->sector_size;
        *nextCluster = buf[offset];
        if (offset == (fat->sector_size - 1))
        {
            fatReadLogical(fat, fatSector + 1, buf);
            offset = -1;
        }
        *nextCluster = *nextCluster | ((unsigned int)buf[offset + 1] << 8);
        if ((cluster * 3 % 2) == 0)
        {
            *nextCluster = *nextCluster & 0xfffU;
        }
        else
        {
            *nextCluster = *nextCluster >> 4;
        }
    }
    return;
}


/*
 * fatDirSectorSearch - go through a block of sectors (as specified
 * by startSector and numsectors) which consist entirely of directory
 * entries, looking for the "search" string.  When found, retrieve some
 * information about the file, including the starting cluster and the
 * file size.  If we get to the end of the directory (NUL in first
 * character of directory entry), set the notfound flag.  Otherwise,
 * if we reach the end of this block of sectors without reaching the
 * end of directory marker, we set the cluster to 0.
 */

static void fatDirSectorSearch(FAT *fat,
                               char *search,
                               unsigned long startSector,
                               int numsectors)
{
    int x;
    unsigned char *buf;
    DIRENT *p;

    buf = fat->dbuf;
    for (x = 0; x < numsectors; x++)
    {
        fatReadLogical(fat, startSector + x, buf);
        for (p = (DIRENT *) buf; (unsigned char *) p < buf + fat->sector_size; p++)
        {
            if (memcmp(p->file_name, search, 11) == 0)
            {
                fat->fnd=1;
                fat->currcluster = p->start_cluster[0]
                    | ((unsigned int) p->start_cluster[1] << 8);
                fat->de = p;
                fat->dirSect = startSector + x;
                return;
            }
            else if (p->file_name[0] == '\0')
            {
                fat->fnd=1;
                fat->de = p;
                fat->dirSect = startSector + x;
                fat->notfound = 1;
                return;
            }
        }
    }
    fat->currcluster = 0;
    return;
}


/*
 * fatReadLogical - read a logical disk sector by calling the
 * function provided at initialization.
 */

static void fatReadLogical(FAT *fat, long sector, void *buf)
{
    fat->readLogical(fat->parm, sector, buf);
    return;
}


/*
 * fatWriteLogical - write a logical disk sector by calling the
 * function provided at initialization.
 */

static void fatWriteLogical(FAT *fat, long sector, void *buf)
{
    fat->writeLogical(fat->parm, sector, buf);
    return;
}


/* fatMarkCluster - mark a cluster as end of chain */

static void fatMarkCluster(FAT *fat, unsigned int cluster)
{
    unsigned long fatSector;
    static unsigned char buf[MAXSECTSZ];
    int offset;

    /* +++ need fat12 logic too */
    if (fat->fat16)
    {
        fatSector = fat->fatstart + (cluster * 2) / fat->sector_size;
        fatReadLogical(fat, fatSector, buf);
        offset = (cluster * 2) % fat->sector_size;
        buf[offset] = 0xff;
        buf[offset + 1] = 0xff;
        fatWriteLogical(fat, fatSector, buf);
    }

    return;
}


/* fatFindFreeCluster - get next available free cluster */

static unsigned int fatFindFreeCluster(FAT *fat)
{
    static unsigned char buf[MAXSECTSZ];
    unsigned long fatSector;
    int found = 0;
    int x;
    unsigned int ret;

    /* +++ need fat12 logic too */
    if (fat->fat16)
    {
        for (fatSector = fat->fatstart;
             fatSector < fat->rootstart;
             fatSector++)
        {
            fatReadLogical(fat, fatSector, buf);
            for (x = 0; x < MAXSECTSZ; x += 2)
            {
                if ((buf[x] == 0) && (buf[x + 1] == 0))
                {
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        if (found)
        {
            ret = (fatSector-fat->fatstart)*MAXSECTSZ/2 + x/2;
            return (ret);
        }
    }
    return (0);
}


/* fatChain - break into a new cluster */

static void fatChain(FAT *fat, FATFILE *fatfile)
{
    unsigned long fatSector;
    static unsigned char buf[MAXSECTSZ];
    int offset;
    unsigned int newcluster;
    unsigned int oldcluster;

    oldcluster = fat->currcluster;
    newcluster = fatFindFreeCluster(fat);
    /* +++ need fat12 logic too */
    if (fat->fat16)
    {
        /* for fat-16, each cluster in the FAT takes up 2 bytes */
        fatSector = fat->fatstart + (oldcluster * 2) / fat->sector_size;
        /* read the FAT sector that contains this cluster entry */
        fatReadLogical(fat, fatSector, buf);
        offset = (oldcluster * 2) % fat->sector_size;
        /* point the old cluster towards the new one */
        buf[offset] = newcluster & 0xff;
        buf[offset + 1] = newcluster >> 8;
        fatWriteLogical(fat, fatSector, buf);

        /* mark the new cluster as last */
        fatMarkCluster(fat, newcluster);

        /* update to new cluster */
        fat->currcluster = newcluster;

        /* update to new set of sectors */
        fatfile->sectorStart = (fat->currcluster - 2)
                    * (long)fat->sectors_per_cluster
                    + fat->filestart;
        fatfile->sectorUpto = 0;
    }

    return;
}


/*
  Delete a file by searching for its start sector and setting all the
  clusters to point to zero,by calling the fatnuke function
*/
unsigned int fatDeleteFile(FAT *fat,const char *fnm)
{
    fat->notfound = 0;

    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    fatPosition(fat,fnm);

    if(fat->notfound)
    {
        return(POS_ERR_FILE_NOT_FOUND);
    }
    else
    {
        fatNuke(fat,fat->currcluster);
        fat->de->file_name[0]=DIRENT_DEL;
        fatWriteLogical(fat, fat->dirSect, fat->dbuf);
        return(0);
    }
}
/**/

/*
  To rename a given file from old to new
*/
unsigned int fatRenameFile(FAT *fat,const char *old,const char *new)
{
    FATFILE fatfile;
    char fnm[FILENAME_MAX];
    char *p;
    int len;
    int lenext;

    if ((old[0] == '\\') || (old[0] == '/'))
    {
        old++;
    }
    fat->currfatfile = &fatfile;
    fat->notfound=0;
    strcpy(fnm,old);
    p=strrchr(fnm,'\\');
    if (p == NULL)
    {
        strcpy(fnm, new);
    }
    else
    {
        strcpy(p+1,new);
    }

    fatPosition(fat,fnm);
    if(!fat->notfound)
    {
        return(POS_ERR_FILE_NOT_FOUND);
    }
    else
    {
        memset(fat->new_file, ' ', 11);
        p = strchr(new, '.');
        if (p != NULL)
        {
            len=(p-(char *) new);
            lenext=strlen(p+1);
            if (len > 8)
            {
                len=8;
            }
            memcpy(fat->new_file, new, len);

            if (lenext > 3)
            {
                lenext=3;
            }
            memcpy(fat->new_file + 8, p+1, lenext);

        }
        else
        {
            len=strlen(new);
            if (len > 8)
            {
                len=8;
            }
            memcpy(fat->new_file,new,len);
        }

        fat->notfound=0;
        fatPosition(fat, old);

        if(fat->notfound)
        {
            return (POS_ERR_FILE_NOT_FOUND);
        }
        else
        {
            memcpy(fat->de->file_name, fat->new_file, 11);
            fatWriteLogical(fat, fat->dirSect, fat->dbuf);
            return (0);
        }
    }
}
/**/

/*To get the attributes of the file given by filename fnm*/
unsigned int fatGetFileAttributes(FAT *fat,const char *fnm,int *attr)
{
    unsigned int ret;
    FATFILE fatfile;

    ret=fatOpenFile(fat,fnm,&fatfile);
    if(ret==0)
    {
       *attr=fatfile.attr;
    }
    else
    {
       *attr=0;
    }
    return(ret);
}
/**/


/*To set the attributes of the file given by file name fnm*/
unsigned int fatSetFileAttributes(FAT *fat,const char *fnm,int attr)
{
    fat->notfound = 0;

    if ((fnm[0] == '\\') || (fnm[0] == '/'))
    {
        fnm++;
    }
    fatPosition(fat,fnm);

    if(fat->notfound)
    {
        return(POS_ERR_FILE_NOT_FOUND);
    }
    else
    {
        fat->de->file_attr = attr;
        fatWriteLogical(fat, fat->dirSect, fat->dbuf);
        return(0);
    }
}
/**/

/**/
unsigned int fatUpdateDateAndTime(FAT *fat,FATFILE *fatfile)
{
    static unsigned char bbuf[MAXSECTSZ];
    DIRENT *d;

    fatReadLogical(fat,
                   fatfile->dirSect,
                   bbuf);

    d = (DIRENT *) (bbuf + fatfile->dirOffset);
    d->last_moddate[0]= fatfile->fdate & 0xff;
    d->last_moddate[1]= (fatfile->fdate >> 8) & 0xff;
    d->last_modtime[0]= fatfile->ftime & 0xff;
    d->last_modtime[1]= (fatfile->ftime >> 8) & 0xff;

    fatWriteLogical(fat,
                    fatfile->dirSect,
                    bbuf);
    return 0;
}
/**/

/* Delete a file by setting cluster chain to zeros */
static void fatNuke(FAT *fat, unsigned int cluster)
{
    unsigned long fatSector;
    static unsigned char buf[MAXSECTSZ];
    int offset;
    int buffered = 0;

    if (cluster == 0) return;
    while (!fatEndCluster(fat, cluster))
    {
        /* +++ need fat12 logic too */
        if (fat->fat16)
        {
            fatSector = fat->fatstart + (cluster * 2) / fat->sector_size;
            if (buffered != fatSector)
            {
                if (buffered != 0)
                {
                    fatWriteLogical(fat, buffered, buf);
                }
                fatReadLogical(fat, fatSector, buf);
                buffered = fatSector;
            }
            offset = (cluster * 2) % fat->sector_size;
            cluster = buf[offset + 1] << 8 | buf[offset];
            buf[offset] = 0x00;
            buf[offset + 1] = 0x00;
        }
    }
    if (buffered != 0)
    {
        fatWriteLogical(fat, buffered, buf);
    }
    return;
}
/**/
