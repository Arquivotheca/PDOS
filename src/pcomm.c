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

/* Written By NECDET COKYAZICI, Public Domain */
void putch(int a);
void bell(void);
void safegets(char *buffer, int size);



static char buf[200];
static unsigned char cmdt[140];
static char path[500] = ";" ; /* Used to store path */
static struct {
    int env;
    unsigned char *cmdtail;
    char *fcb1;
    char *fcb2;
} parmblock = { 0, cmdt, NULL, NULL };
static size_t len;
static char drive[2] = "A";
static char cwd[65];
/* Sometimes I test PCOMM under MS-DOS.
 * And then I get confused about whether I am in PCOMM or COMMAND.COM.
 * Making the default prompt for PCOMM different from that of COMMAND.COM
 * Helps avoid that confusion.
 * - Simon Kissane
 */
static char prompt[50] = "]";
static int singleCommand = 0;
static int primary = 0;
static int term = 0;

static void parseArgs(int argc, char **argv);
static void processInput(void);
static void putPrompt(void);
static void doCopy(char *b);
static void doExec(char *b,char *p);
static void dotype(char *file);
static void doEcho(char *msg);
static void doExit(char *ignored);
static void dodir(char *pattern);
static void doReboot(char *ignored);
static void dover(char *ignored);
static void dodel(char *fnm);
static void doPrompt(char *s);
static void dopath(char *s);
static void changedir(char *to);
static void changedisk(int drive);
static void dohelp(char *cmd);
static void domkdir(char *dnm);
static void doMCD(char *dnm);
static void dormdir(char *dnm);
static void dorename(char *src);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);
static void readBat(char *fnm);
static int exists(char *fnm);

/* Prototypes for detailed help routines */
static void help_cd(void);
static void help_copy(void);
static void help_del(void);
static void help_dir(void);
static void help_echo(void);
static void help_exit(void);
static void help_help(void);
static void help_mcd(void);
static void help_md(void);
static void help_path(void);
static void help_prompt(void);
static void help_rd(void);
static void help_reboot(void);
static void help_ren(void);
static void help_type(void);
static void help_ver(void);

/* Function signature for command implementation functions */
typedef void (*cmdProc)(char *arg);

/* Function signature for command help display functions */
typedef void (*cmdHelp)(void);

/* Defines an internal command */
typedef struct cmdBlock
{
    char *name;
    char *description;
    cmdProc proc;
    cmdHelp help;
}
cmdBlock;

/* The command registry stores definitions of all internal commands.
 *
 * Fields:
 * { name, description, proc, help }
 *
 * where:
 * - name is name of command. Prefix aliases with pipe symbol.
 * - description is one-line description of command for help system
 * - proc is procedure to execute for command
 * - help is detailed help procedure, displays detailed help
 *
 */
static cmdBlock cmdRegistry[] =
{
    { "cd|chdir", "Changes the current directory", changedir, help_cd },
    { "copy", "Copies files and directories", doCopy, help_copy },
    { "del", "Deletes files", dodel, help_del },
    { "dir", "Lists files and directories", dodir, help_dir },
    { "echo", "Displays a message", doEcho, help_echo },
    { "exit", "Exits PCOMM", doExit, help_exit },
    { "help", "Provides information about PDOS commands", dohelp, help_help },
    { "mcd", "Make new directory and change to it", doMCD, help_mcd },
    { "md|mkdir", "Creates new directories", domkdir, help_md },
    { "path", "Displays or modifies PATH variable", dopath, help_path },
    { "prompt", "Displays or modifies PROMPT variable", doPrompt, help_prompt },
    { "rd|rmdir", "Removes directories", dormdir, help_rd },
    { "reboot", "Reboots the computer", doReboot, help_reboot },
    { "ren|rename", "Renames files and directories", dorename, help_ren },
    { "type", "Reads and displays a text file", dotype, help_type },
    { "ver", "Displays the current version of PDOS", dover, help_ver },
    { NULL, NULL, NULL, NULL } /* Sentinel record for end of command registry */
};

/* Searches command registry for command with given name */
cmdBlock *findCommand(char *cmdName)
{
    char tmp[128];
    char *p1, *p2;
    int i;
    cmdBlock *block;

    /* Ignore initial whitespace */
    while (isspace(*cmdName))
    {
        cmdName++;
    }

    /* Ignore final whitespace */
    for (i = 0; cmdName[i] != 0; i++)
    {
        if (isspace(cmdName[i]))
        {
            cmdName[i] = 0;
            break;
        }
    }

    /* Search the registry for the command */
    for (i = 0; cmdRegistry[i].name != NULL; i++)
    {
        block = &(cmdRegistry[i]);
        if (strchr(block->name,'|') == NULL)
        {
            if (ins_strcmp(block->name,cmdName) == 0)
            {
                return block;
            }
        }
        else
        {
            strcpy(tmp,block->name);
            p1 = tmp;
            while (1)
            {
                p2 = strchr(p1,'|');
                if (p2 == NULL)
                {
                    if (ins_strcmp(p1,cmdName) == 0)
                    {
                        return block;
                    }
                    break;
                }
                *p2 = 0;
                p2++;
                if (ins_strcmp(p1,cmdName) == 0)
                {
                    return block;
                }
                p1 = p2;
            }
        }
    }
    return NULL;
}

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
        readBat("AUTOEXEC.BAT");
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
            len--;
            buf[len] = '\0';
        }
    }
    return;
}

static void processInput(void)
{
    char *p;
    cmdBlock *block;

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

    /* Check for special case syntax first */
    if (ins_strncmp(buf, "cd.", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if (ins_strncmp(buf, "cd\\", 3) == 0)
    {
        changedir(buf + 2);
    }
    else if ((strlen(buf) == 2) && (buf[1] == ':'))
    {
        changedisk(buf[0]);
    }
    else
    {
        /* Look for internal command in registry */
        block = findCommand(buf);
        if (block != NULL)
        {
            /* Command was found, call implementation if present */
            if (block->proc == NULL)
            {
                printf("ERROR: No implementation found for command '%s'\n", buf);
            }
            else
            {
                block->proc(p);
            }
        }
        else
        {
            doExec(buf,p);
        }
    }
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

static void doExit(char *ignored)
{
#ifdef CONTINUOUS_LOOP
    primary = 0;
#endif
    if (!primary)
    {
        term = 1;
    }
}

static void doEcho(char *msg)
{
    printf("%s\n", msg);
}

static void doReboot(char *ignored)
{
    PosReboot();
}

static void dotype(char *file)
{
    FILE *fp;

    if (strcmp(file,"") == 0)
    {
        printf("Required Parameter Missing\n");
        return;
    }

    fp = fopen(file, "r");

    if (fp != NULL)
    {
        while (fgets(buf, sizeof buf, fp) != NULL)
        {
            fputs(buf, stdout);
        }
        fclose(fp);
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

    dta = PosGetDTA();
    if(*fnm == '\0')
    {
        printf("Please Specify the file name \n");
        return;
    }
    ret = PosFindFirst(fnm,0x10);

    if(ret == 2)
    {
        printf("File not found \n");
        printf("No Files Deleted \n");
        return;
    }
    while(ret == 0)
    {
        PosDeleteFile(dta->file_name);
        ret = PosFindNext();
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

static void dover(char *ignored)
{
    printf("%s %s\n", __DATE__, __TIME__);
    return;
}

static void dopath(char *s)
{
     if (strcmp(s, "") == 0)
     {
        char *t;

        t = path;

        if (*t == ';')
        {
            t++;
        }

        if (*t == '\0')
        {
            printf("No Path defined\n");
        }

        else
        {
            printf("%s\n", t);
        }
     }

     else if (*s == ';')
     {
        strcpy(path, s);
     }

     else
     {
        strcpy(path, ";");
        strcat(path, s);
     }
     return;
}

static void doPrompt(char *s)
{
     if (strcmp(s, "") == 0)
     {
        printf("Current PROMPT is %s\n", prompt);
     }

     else
     {
        strcpy(prompt, s);
     }
}

static void doExec(char *b,char *p)
{
    char *r;
    char *s;
    char *t;
    size_t ln;
    size_t ln2;
    char tempbuf[FILENAME_MAX];

    s = path;
    ln = strlen(p);
    cmdt[0] = ln;
    memcpy(cmdt + 1, p, ln);
    memcpy(cmdt + ln + 1, "\r", 2);

    while(*s != '\0')
    {
        t = strchr(s, ';');

        if (t == NULL)
        {
            t = s + strlen(s);
        }

        ln2 = t - s;
        memcpy(tempbuf, s ,ln2);

        if (ln2 != 0)
        {
            tempbuf[ln2++] = '\\';
        }

        strcpy(tempbuf + ln2, b);
        ln2 += strlen(b);
        strcpy(tempbuf + ln2, ".com");

        if (exists(tempbuf))
        {
            PosExec(tempbuf, &parmblock);
            break;
        }

        strcpy(tempbuf + ln2 ,".exe");

        if (exists(tempbuf))
        {
            PosExec(tempbuf, &parmblock);
            break;
        }

        strcpy(tempbuf + ln2 ,".bat");

        if (exists(tempbuf))
        {
            readBat(tempbuf);
            break;
        }

        s = t;

        if (*s == ';')
        {
            s++;
        }
    }

    if (*s == '\0')
    {
        printf("Command '%s' not found\n", b);
    }

    return;
}

static void doCopy(char *b)
{
    char bbb[512];
    char *src;
    char *dest;
    FILE *fsrc;
    FILE *fdest;
    int bytes_read = 0;

    src = b;
    dest = strchr(b,' ');

    if (dest == NULL)
    {
        printf("\n Two Filenames required");
        return;
    }

    *dest++ ='\0';

    printf("Source is %s \n",src);
    printf("Dest is %s \n",dest);

    fsrc = fopen(src,"rb");

    if (fsrc != NULL)
    {
        fdest = fopen(dest,"wb");

        if (fdest != NULL)
        {
            while((bytes_read = fread(bbb,1,sizeof(bbb),fsrc)) != 0)
            {
                fwrite(bbb,1,bytes_read,fdest);
            }

            fclose(fdest);
        }

        else
        {
            printf("Destination %s file not found \n",dest);
        }

        fclose(fsrc);
    }

    else
    {
        printf("Source file %s not found \n",src);
    }

    return;

}

static void changedir(char *to)
{
    int ret;

    ret = PosChangeDir(to);

    if (ret == POS_ERR_PATH_NOT_FOUND) printf("Invalid directory\n");

    return;
}

static void changedisk(int drive)
{
    PosSelectDisk(toupper(drive) - 'A');
    return;
}

/*
 * **** HELP ROUTINES ***
 */
static void help_exit(void)
{
    printf("EXIT\n");
}

static void help_path(void)
{
    printf("PATH variable provides a list of directories\n");
    printf("which are searched when looking for executables.\n\n");
    printf("PATH\n");
    printf("PATH [path]\n");
    printf("PATH [;]\n");
}

static void help_prompt(void)
{
    printf("PROMPT variable stores the PCOMM prompt.\n");
    printf("PROMPT\n");
    printf("PROMPT [prompt]\n");
}

static void help_type(void)
{
    printf("TYPE [path]\n");
}

static void help_dir(void)
{
    printf("DIR\n");
}

static void help_ver(void)
{
    printf("VER\n");
}

static void help_echo(void)
{
    printf("ECHO [message]\n");
}

static void help_cd(void)
{
    printf("CD [path]\n");
}

static void help_reboot(void)
{
    printf("REBOOT\n");
}

static void help_del(void)
{
    printf("DEL [path]\n");
}

static void help_copy(void)
{
    printf("COPY [path1] [path2]\n");
}

static void help_help(void)
{
    printf("HELP\n");
    printf("HELP [command]\n");
}

static void help_md(void)
{
    printf("MD [path]\n");
}

static void help_mcd(void)
{
    printf("MCD [path]\n");
}

static void help_rd(void)
{
    printf("RD [path]\n");
}

static void help_ren(void)
{
    printf("REN [path] [name]\n");
}

/* Modify string in-place to be all upper case */
static void strtoupper(char *s)
{
    while (*s != 0)
    {
        *s = toupper(*s);
        s++;
    }
}

/* dohelp function and condition for it originally written by Alica Okano.
 * Rewritten to use table-driven design by Simon Kissane.
 */
static void dohelp(char *cmd)
{
    int i = 0;
    int maxCmdNameLen = 0, curCmdNameLen = 0;
    cmdBlock *block = NULL;
    char *pipe = NULL;
    char tmp[128];
    if(*cmd == '\0')
    {
        printf("Use HELP [command] to obtain more information about\n");
        printf("specific command.\n\n");

        /* Find max command name length. Used to construct columns. */
        for (i = 0; cmdRegistry[i].name != NULL; i++)
        {
            block = &(cmdRegistry[i]);
            curCmdNameLen = strlen(block->name);
            if (curCmdNameLen > maxCmdNameLen)
                maxCmdNameLen = curCmdNameLen;
        }

        /* Print each line */
        for (i = 0; cmdRegistry[i].name != NULL; i++)
        {
            block = &(cmdRegistry[i]);

            /* Allocate temporary buffer to modify name */
            memset(tmp, 0, 128);
            strcpy(tmp,block->name);

            /* Strip aliases */
            pipe = strchr(tmp,'|');
            if (pipe != NULL)
            {
                while (*pipe != 0)
                {
                    *pipe = 0;
                    pipe++;
                }
            }

            /* Make upper case */
            strtoupper(tmp);

            /* Left pad with spaces */
            while (strlen(tmp) < maxCmdNameLen)
                tmp[strlen(tmp)] = ' ';
            tmp[strlen(tmp)] = ' ';

            /* Print help line */
            printf("%s %s.\n", tmp, block->description);
        }
    }
    else
    {
        /* Lookup command, report error if not found */
        block = findCommand(cmd);
        if (block == NULL)
        {
            printf("ERROR: No such command \"%s\"\n",cmd);
            return;
        }

        /* Allocate temporary buffer to modify name */
        memset(tmp, 0, 128);
        strcpy(tmp,block->name);

        /* Strip aliases */
        pipe = strchr(tmp,'|');
        if (pipe != NULL)
        {
            while (*pipe != 0)
            {
                *pipe = 0;
                pipe++;
            }
        }

        /* Make upper case */
        strtoupper(tmp);

        /* Print header line */
        printf("%s - %s\n\n", tmp, (block->description != NULL ? block->description : "(no description)"));

        /* Print aliases */
        if (strchr(block->name,'|') != NULL)
        {
            memset(tmp, 0, 128);
            strcpy(tmp,block->name);
            pipe = strchr(tmp,'|');
            pipe++;
            for (i = 0; pipe[i] != 0; i++)
            {
                pipe[i] = toupper(pipe[i]);
                if (pipe[i] == '|')
                {
                    pipe[i] = ' ';
                }
            }
            printf("Aliases: %s\n\n", pipe);
        }

        /* Print detailed help */
        if (block->help != NULL)
        {
            (block->help)();
        }
    }

    return;
}

static void domkdir(char *dnm)
{
    int ret;

    if(*dnm == '\0')
    {
        printf("Required Parameter Missing\n");
        return;
    }

    ret = PosMakeDir(dnm);

    if (ret == POS_ERR_PATH_NOT_FOUND) printf("Unable to create directory\n");
    if (ret == POS_ERR_ACCESS_DENIED) printf("Access denied\n");

    return;
}

static void doMCD(char *dnm) {
    domkdir(dnm);
    changedir(dnm);
}

static void dormdir(char *dnm)
{
    int ret;

    if(*dnm == '\0')
    {
        printf("Required Parameter Missing\n");
        return;
    }

    ret = PosRemoveDir(dnm);

    if (ret == POS_ERR_PATH_NOT_FOUND)
        printf("Invalid path, not directory,\nor directory not empty\n");
    else if (ret == POS_ERR_ACCESS_DENIED) printf("Access denied\n");
    else if (ret == POS_ERR_INVALID_HANDLE) printf("Invalid handle\n");
    else if (ret == POS_ERR_ATTEMPTED_TO_REMOVE_CURRENT_DIRECTORY)
        printf("Attempt to remove current directory\n");

    return;
}

static void dorename(char *src)
{
    int ret;
    char *dest;

    dest = strchr(src, ' ');
    if (*src == '\0' || dest == NULL)
    {
        printf("Required Parameter Missing\n");
        return;
    }

    *dest = '\0';
    dest++;

    ret = PosRenameFile(src, dest);

    if (ret == POS_ERR_FILE_NOT_FOUND) printf("File not found\n");
    else if (ret == POS_ERR_PATH_NOT_FOUND) printf("Invalid path\n");
    else if (ret == POS_ERR_ACCESS_DENIED) printf("Access denied\n");
    else if (ret == POS_ERR_FORMAT_INVALID) printf("Invalid format\n");

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

static void readBat(char *fnm)
{
    FILE *fp;

    fp = fopen(fnm, "r");
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

static int exists(char *fnm)
{
    FILE *fp;

    fp = fopen(fnm,"rb");

    if (fp != NULL)
    {
        fclose(fp);
        return 1;
    }

    return 0;
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
