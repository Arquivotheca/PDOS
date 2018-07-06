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

/* In C99 or higher we would just include <stdbool.h>. But, we need to
 * support older C compilers (C89/C90) which don't come with <stdbool.h>
 */
#define true 1
#define false 0
#define bool int

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
static void doExec(char *b,char *p);
static void changedisk(int drive);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);
static void readBat(char *fnm);
static int exists(char *fnm);
static int isBlankString(char *str);
static void showArgsUnsupportedMsg(char *cmdName);

/* Prototypes for command execution routines */
static void do_cd(char *to);
static void do_copy(char *b);
static void do_date(char *ignored);
static void do_del(char *fnm);
static void do_dir(char *pattern);
static void do_echo(char *msg);
static void do_exit(char *ignored);
static void do_help(char *cmd);
static void do_mcd(char *dnm);
static void do_md(char *dnm);
static void do_path(char *s);
static void do_pause(char *src);
static void do_prompt(char *s);
static void do_rd(char *dnm);
static void do_reboot(char *ignored);
static void do_rem(char *src);
static void do_ren(char *src);
static void do_time(char *ignored);
static void do_type(char *file);
static void do_ver(char *ignored);

/* Prototypes for detailed help routines */
static void help_cd(void);
static void help_copy(void);
static void help_date(void);
static void help_del(void);
static void help_dir(void);
static void help_echo(void);
static void help_exit(void);
static void help_help(void);
static void help_mcd(void);
static void help_md(void);
static void help_path(void);
static void help_pause(void);
static void help_prompt(void);
static void help_rd(void);
static void help_reboot(void);
static void help_rem(void);
static void help_ren(void);
static void help_time(void);
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

/* Define command macro.
 * Parameters are:
 * name - name of this command (lowercase, unquoted)
 * aliases - empty string "" for no aliases. Otherwise each alias in string
 *           prefixed with pipe character.
 * description - one-line description of command for help system.
 */
#define CMDDEF(name,aliases,description) { #name aliases , description , do_##name, help_##name }

/* Sentinel record for end of command registry */
#define CMD_REGISTRY_END { NULL, NULL, NULL, NULL }

/* The command registry stores definitions of all internal commands.
 */
static cmdBlock cmdRegistry[] =
{
    CMDDEF(cd,"|chdir","Changes the current directory"),
    CMDDEF(copy,"","Copies files and directories"),
    CMDDEF(date,"","Shows the date"),
    CMDDEF(del,"|erase","Deletes files"),
    CMDDEF(dir,"","Lists files and directories"),
    CMDDEF(echo,"","Displays a message"),
    CMDDEF(exit,"","Exits PCOMM"),
    CMDDEF(help,"","Provides information about PDOS commands"),
    CMDDEF(mcd,"","Make new directory and change to it"),
    CMDDEF(md,"|mkdir","Creates new directories"),
    CMDDEF(path,"","Displays or modifies PATH variable"),
    CMDDEF(pause,"","Wait for user to press any key"),
    CMDDEF(prompt,"","Displays or modifies PROMPT variable"),
    CMDDEF(rd,"|rmdir","Removes directories"),
    CMDDEF(reboot,"","Reboots the computer"),
    CMDDEF(rem,"","Comment in a batch file"),
    CMDDEF(ren,"|rename","Renames files and directories"),
    CMDDEF(time,"","Shows the time"),
    CMDDEF(type,"","Reads and displays a text file"),
    CMDDEF(ver,"","Displays the current version of PDOS"),
    CMD_REGISTRY_END
};

/* utility function - check if string is blank */
static bool isBlankString(char *str)
{
    while (*str != 0)
    {
        if (!isspace(*str))
        {
            return false;
        }
        str++;
    }
    return true;
}

/* utility function - show an error if command doesn't accept arguments */
static void showArgsUnsupportedMsg(char *cmdName)
{
    printf("ERROR: Command '%s' does not accept any arguments\n", cmdName);
}

/* utility function - show an error if command requires arguments */
static void showArgsRequiredMsg(char *cmdName)
{
    printf("ERROR: Command '%s' requires arguments, but none supplied\n", cmdName);
}

/* utility macro - fail command if non-blank arguments supplied */
#define CMD_HAS_NO_ARGS(cmdName,args) \
    do { \
        if (!isBlankString(args)) { \
            showArgsUnsupportedMsg(cmdName); \
            return; \
        } \
    }  while (0)

/* utility macro - fail command if non-blank arguments missing */
#define CMD_REQUIRES_ARGS(cmdName,args) \
    do { \
        if (isBlankString(args)) { \
            showArgsRequiredMsg(cmdName); \
            return; \
        } \
    }  while (0)

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

    /* Remove newline at end of buffer */
    len = strlen(buf);
    if ((len > 0) && (buf[len - 1] == '\n'))
    {
        len--;
        buf[len] = '\0';
    }

    /* If command line is blank, do nothing */
    if (isBlankString(buf))
    {
        return;
    }

    /* Split command and arguments */
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
        do_cd(buf + 2);
    }
    else if (ins_strncmp(buf, "cd\\", 3) == 0)
    {
        do_cd(buf + 2);
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
            /* No internal command found, presume external command */
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

static void do_exit(char *ignored)
{
    CMD_HAS_NO_ARGS("EXIT",ignored);
#ifdef CONTINUOUS_LOOP
    primary = 0;
#endif
    if (!primary)
    {
        term = 1;
    }
}

static void do_echo(char *msg)
{
    printf("%s\n", msg);
}

static void do_reboot(char *ignored)
{
    CMD_HAS_NO_ARGS("REBOOT",ignored);
    PosReboot();
    /* if we return from PosReboot(), we know it failed */
    printf("ERROR: Reboot failed\n");
}

static void do_type(char *file)
{
    FILE *fp;
    CMD_REQUIRES_ARGS("TYPE",file);

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

static void do_del(char *fnm)
{
    int ret;
    DTA *dta;
    CMD_REQUIRES_ARGS("DEL",fnm);

    dta = PosGetDTA();
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

static void do_dir(char *pattern)
{
    DTA *dta;
    int ret;
    char *p;
    time_t tt;
    struct tm *tms;

    dta = PosGetDTA();
    if (isBlankString(pattern))
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
        printf("%-13s %9ld %02x %04d-%02d-%02d %02d:%02d:%02d %s\n",
               dta->file_name,
               dta->file_size,
               dta->attrib,
               tms->tm_year + 1900,
               tms->tm_mon + 1,
               tms->tm_mday,
               tms->tm_hour,
               tms->tm_min,
               tms->tm_sec,
               dta->lfn);
        ret = PosFindNext();
    }
    return;
}

#ifdef __32BIT__
#define PDOS_FLAVOR "PDOS-32"
#else
#define PDOS_FLAVOR "PDOS-16"
#endif

static void do_ver(char *ignored)
{
    int ver, major, minor;
    CMD_HAS_NO_ARGS("VER",ignored);
    ver = PosGetDosVersion();
    minor = (ver & 0xFF);
    major = ((ver>>8)&0xFF);
    printf("PCOMM for " PDOS_FLAVOR "\n");
    printf("Reporting DOS version %d.%d\n", major, minor);
    printf("PCOMM built at %s %s\n", __DATE__, __TIME__);
    return;
}

static void do_path(char *s)
{
     if (isBlankString(s))
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

static void do_prompt(char *s)
{
     if (isBlankString(s))
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

static void do_copy(char *b)
{
    char bbb[512];
    char *src;
    char *dest;
    FILE *fsrc;
    FILE *fdest;
    int bytes_read = 0;
    CMD_REQUIRES_ARGS("COPY",b);

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

static void do_cd(char *to)
{
    int ret;
    int d;

    /* No arguments means to print current drive and directory */
    if (isBlankString(to))
    {
        d = PosGetDefaultDrive();
        drive[0] = d + 'A';
        PosGetCurDir(0, cwd);
        printf("%s:\\%s\n", drive, cwd);
        return;
    }

    ret = PosChangeDir(to);

    if (ret == POS_ERR_PATH_NOT_FOUND) printf("Invalid directory\n");

    return;
}

static void changedisk(int drive)
{
    int selected;
    PosSelectDisk(toupper(drive) - 'A');
    selected = PosGetDefaultDrive() + 'A';
    if (selected != toupper(drive))
    {
        printf("ERROR: Failed changing to drive %c:\n", toupper(drive));
    }
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

static void help_date(void)
{
    printf("DATE\n\n");
    printf("Limitations (may be removed in future versions):\n");
    printf("- No arguments supported at present\n");
    printf("- No facility to change the date\n");
}

static void help_time(void)
{
    printf("TIME\n\n");
    printf("Limitations (may be removed in future versions):\n");
    printf("- No arguments supported at present\n");
    printf("- No facility to change the time\n");
}

static void help_rem(void)
{
    printf("REM [any arguments ignored]\n");
}

static void help_pause(void)
{
    printf("PAUSE\n");
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

/* Assumption as to number of lines before we print
 * "Press any key to continue...". Actually we should call some API to find
 * out screen dimensions and calculate dynamically. But for now, we will just
 * assume a standard 25 line display. The paging offset then is a bit less
 * than 25 to take into account the 3 line header, etc.
 */
#define HELP_PAGE_AMOUNT 19

/* do_help function and condition for it originally written by Alica Okano.
 * Rewritten to use table-driven design by Simon Kissane.
 */
static void do_help(char *cmd)
{
    int i = 0;
    int pageAt = HELP_PAGE_AMOUNT;
    int maxCmdNameLen = 0, curCmdNameLen = 0;
    cmdBlock *block = NULL;
    char *pipe = NULL;
    char tmp[128];
    if (isBlankString(cmd))
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

            /* Page the screen if necessary */
            if (i >= pageAt && cmdRegistry[i+1].name != NULL)
            {
                do_pause("");
                pageAt += HELP_PAGE_AMOUNT;
            }
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

static void do_md(char *dnm)
{
    int ret;
    CMD_REQUIRES_ARGS("MD",dnm);

    ret = PosMakeDir(dnm);

    if (ret == POS_ERR_PATH_NOT_FOUND) printf("Unable to create directory\n");
    if (ret == POS_ERR_ACCESS_DENIED) printf("Access denied\n");

    return;
}

static void do_mcd(char *dnm)
{
    CMD_REQUIRES_ARGS("MCD",dnm);
    do_md(dnm);
    do_cd(dnm);
}

static void do_rd(char *dnm)
{
    int ret;
    CMD_REQUIRES_ARGS("RD",dnm);

    ret = PosRemoveDir(dnm);

    if (ret == POS_ERR_PATH_NOT_FOUND)
        printf("Invalid path, not directory,\nor directory not empty\n");
    else if (ret == POS_ERR_ACCESS_DENIED) printf("Access denied\n");
    else if (ret == POS_ERR_INVALID_HANDLE) printf("Invalid handle\n");
    else if (ret == POS_ERR_ATTEMPTED_TO_REMOVE_CURRENT_DIRECTORY)
        printf("Attempt to remove current directory\n");

    return;
}

static void do_ren(char *src)
{
    int ret;
    char *dest;
    CMD_REQUIRES_ARGS("REN",src);

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

static void do_date(char *ignored)
{
    int y, m, d, dw;
    CMD_HAS_NO_ARGS("DATE",ignored);
    PosGetSystemDate(&y,&m,&d,&dw);
    printf("%04d-%02d-%02d\n", y, m, d);
}

static void do_time(char *ignored)
{
    int hr, min, sec, hund;
    CMD_HAS_NO_ARGS("TIME",ignored);
    PosGetSystemTime(&hr,&min,&sec,&hund);
    printf("%02d:%02d:%02d\n", hr, min, sec);
}

static void do_rem(char *ignored)
{
    /* Nothing to do for ocmment */
}

static void do_pause(char *ignored)
{
    CMD_HAS_NO_ARGS("PAUSE",ignored);
    printf("Press any key to continue . . .\n");
    PosDirectCharInputNoEcho();
}
