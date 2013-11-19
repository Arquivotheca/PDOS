/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcomm - command processor for pdos                               */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "pos.h"
#include "dostime.h"

void __exec(char *cmd, void *env);



/* Written By NECDET COKYAZICI, Public Domain */
void putch(int a);
void bell(void);
void safegets(char *buffer, int size);



static char buf[200];
static unsigned char cmdt[140];
static struct {
    int env;
    unsigned char *cmdtail;
    char *fcb1;
    char *fcb2;
} parmblock = { 0, cmdt, NULL, NULL };
static size_t len;
static char drive[2] = "A";
static char cwd[65];
static char prompt[50] = ">";
static int singleCommand = 0;
static int primary = 0;
static int term = 0;

static void parseArgs(int argc, char **argv);
static void processInput(void);
static void putPrompt(void);
static void dotype(char *file);
static void dodir(char *pattern);
static void dover(void);
static void dodel(char *fnm);
static void changedir(char *to);
static void changedisk(int drive);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);
static void readAutoExec(void);

int main(int argc, char **argv)
{
#ifdef USING_EXE
    pdosRun();
#endif
#ifdef __32BIT__
    install_runtime();
#endif
    parseArgs(argc, argv);
    if (singleCommand)
    {
        processInput();
        return (0);
    }
    if (primary)
    {
        printf("welcome to pcomm\n");
        readAutoExec();
    }
    else
    {
        printf("welcome to pcomm - exit to return\n");
    }
    while (!term)
    {
        putPrompt();
        safegets(buf, sizeof buf);

        processInput();
    }
    printf("thankyou for using pcomm!\n");
    return (0);
}

static void parseArgs(int argc, char **argv)
{
    int x;
    
    if (argc > 1)
    {
        if ((argv[1][0] == '-') || (argv[1][0] == '/'))
        {
            if ((argv[1][1] == 'C') || (argv[1][1] == 'c'))
            {
                singleCommand = 1;
            }            
            if ((argv[1][1] == 'P') || (argv[1][1] == 'p'))
            {
                primary = 1;
            }            
        }
    }
    if (singleCommand)
    {
        strcpy(buf, "");
        for (x = 2; x < argc; x++)
        {
            strcat(buf, *(argv + x));
            strcat(buf, " ");
        }
        len = strlen(buf);
        if (len > 0)
        {
            buf[len - 1] = '\0';
        }
    }
    return;
}

static void processInput(void)
{
    char *p;

    len = strlen(buf);
    if ((len > 0) && (buf[len - 1] == '\n'))
    {
        len--;
        buf[len] = '\0';
    }
    p = strchr(buf, ' ');
    if (p != NULL)
    {
        *p++ = '\0';
    }
    else
    {
        p = buf + len;
    }
    len -= (size_t)(p - buf);
    if (ins_strcmp(buf, "exit") == 0)
    {
#ifdef CONTINUOUS_LOOP
        primary = 0;
#endif
        if (!primary)
        {
            term = 1;
        }
    }
    else if (ins_strcmp(buf, "type") == 0)
    {
        dotype(p);
    }
    else if (ins_strcmp(buf, "dir") == 0)
    {
        dodir(p);
    }
    else if (ins_strcmp(buf, "ver") == 0)
    {
        dover();
    }
    else if (ins_strcmp(buf, "echo") == 0)
    {
        printf("%s\n", p);
    }
    else if (ins_strcmp(buf, "cd") == 0)
    {
        changedir(p);
    }
    else if (ins_strncmp(buf, "cd.", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if (ins_strncmp(buf, "cd\\", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if (ins_strcmp(buf, "reboot") == 0)
    {
        PosReboot();
    }
    else if(ins_strcmp(buf, "del")==0)
    {
        dodel(p);
    }
    else if ((strlen(buf) == 2) && (buf[1] == ':'))
    {
        changedisk(buf[0]);
    }
    else
    {
        char *tt;
        
        cmdt[0] = (unsigned char)len;
        memcpy(cmdt + 1, p, len);
        memcpy(cmdt + len + 1, "\r", 2);
        tt = buf + strlen(buf);
        strcpy(tt, ".com");
        __exec(buf, &parmblock);        
        strcpy(tt, ".exe");
        __exec(buf, &parmblock);
    }
    return;
}

static void putPrompt(void)
{
    int d;
    
    d = PosGetDefaultDrive();
    drive[0] = d + 'A';
    PosGetCurDir(0, cwd);
    printf("\n%s:\\%s%s", drive, cwd, prompt);
    fflush(stdout);
    return;
}

static void dotype(char *file)
{
    FILE *fp;
    
    fp = fopen(file, "r");
    if (fp != NULL)
    {
        while (fgets(buf, sizeof buf, fp) != NULL)
        {
            fputs(buf, stdout);
        }
    }
    else
    {
       printf("file not found: %s\n", file);
    }
    return;
}

static void dodel(char *fnm)
{    
    int ret;
    DTA *dta;

    dta=PosGetDTA();
    if(*fnm=='\0')
    {
        printf("Please Specify the file name \n");
        return;
    }
    ret=PosFindFirst(fnm,0x10);
    
    if(ret==2)
    {
        printf("File not found \n");
        printf("No Files Deleted \n");
        return;
    }
    while(ret==0)
    {           
        PosDeleteFile(dta->file_name);  
        ret=PosFindNext();
    }
    return;
}
static void dodir(char *pattern)
{
    DTA *dta;
    int ret;
    char *p;
    time_t tt;
    struct tm *tms;
    
    dta = PosGetDTA();
    if (*pattern == '\0')
    {
        p = "*.*";
    }
    else
    {
        p = pattern;
    }
    ret = PosFindFirst(p, 0x10);
    while (ret == 0)
    {
        tt = dos_to_timet(dta->file_date,dta->file_time);
        tms = localtime(&tt);
        printf("%-13s %9ld %02x %04d-%02d-%02d %02d:%02d:%02d\n", 
               dta->file_name,
               dta->file_size,
               dta->attrib,
               tms->tm_year + 1900,
               tms->tm_mon + 1,
               tms->tm_mday,
               tms->tm_hour,
               tms->tm_min,
               tms->tm_sec);
        ret = PosFindNext();
    }
    return;
}

static void dover(void)
{
    printf("%s %s\n", __DATE__, __TIME__);
    return;
}

static void changedir(char *to)
{
    PosChangeDir(to);
    return;
}

static void changedisk(int drive)
{
    PosSelectDisk(toupper(drive) - 'A');
    return;
}

static int ins_strcmp(char *one, char *two)
{
    while (toupper(*one) == toupper(*two))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
    }
    if (toupper(*one) < toupper(*two))
    {
        return (-1);
    }
    return (1);
}

static int ins_strncmp(char *one, char *two, size_t len)
{
    size_t x = 0;
    
    if (len == 0) return (0);
    while ((x < len) && (toupper(*one) == toupper(*two)))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
        x++;
    }
    if (x == len) return (0);
    return (toupper(*one) - toupper(*two));
}

static void readAutoExec(void)
{
    FILE *fp;
    
    fp = fopen("AUTOEXEC.BAT", "r");
    if (fp != NULL)
    {        
        while (fgets(buf, sizeof buf, fp) != NULL)
        {
            processInput();
        }
        fclose(fp);
    }
    return;
}



/* Written By NECDET COKYAZICI, Public Domain */


void putch(int a)
{
    putchar(a);
    fflush(stdout);
}


void bell(void)
{
    putch('\a');
}


void safegets(char *buffer, int size)
{
    int a;
    int shift;
    int i = 0;

    while (1)
    {

        a = PosGetCharInputNoEcho();

/*
        shift = BosGetShiftStatus();

        if (shift == 1)
        {
            if ((a != '\n') && (a != '\r') && (a != '\b'))
            if (islower(a))
            a -= 32;
     
            else if ((a >= '0') && (a <= '9'))
            a -= 16;
        }

*/

        if (i == size)
        {
            buffer[size] = '\0';
      
            if ((a == '\n') || (a == '\r'))
            return;

            bell();
        }

        if ((i == 0) && (a == '\b'))
        continue;

        if (i < size)
        {

            if ((a == '\n') || (a == '\r'))
            {
                putch('\n');

                buffer[i] = '\0';

                return;
            }


            if (a == '\b')
            {
        
                if (i == 0)
                continue;
        
                else
                i--;

                putch('\b');
                putch(' ');
                putch('\b');

                continue;
            }
            else
            putch(a);


            if (isprint((unsigned char)a))
            {
                buffer[i] = a;
                i++;
            }
            else bell();

        }
        else bell();

    }

}
