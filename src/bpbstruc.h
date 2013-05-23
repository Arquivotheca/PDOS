typedef struct {
    unsigned char BytesPerSector[2];
    unsigned char SectorsPerClustor;
    unsigned char ReservedSectors[2];
    unsigned char FatCount;
    unsigned char RootEntries[2];
    unsigned char TotalSectors16[2];
    unsigned char MediaType;
    unsigned char FatSize16[2];
    unsigned char SectorsPerTrack[2];
    unsigned char Heads[2];
    unsigned char HiddenSectors_Low[2];
    unsigned char HiddenSectors_High[2];
    unsigned char TotalSectors32[4];
} BPB;

