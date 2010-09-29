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

static char buf[200];
static size_t len;
static char drive[7] = "PDOS00";
static char cwd[65];
static char prompt[50] = ">";
static int singleCommand = 0;
static int primary = 0;
static int term = 0;

static void parseArgs(int argc, char **argv);
static void readAutoExec(void);
static void processInput(void);
static void putPrompt(void);
static void dotype(char *file);
static void dodir(char *pattern);
static void changedir(char *to);
static void changedisk(int drive);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);

int main(int argc, char **argv)
{
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
    term = 1;
    while (!term)
    {
        putPrompt();
        fgets(buf, sizeof buf, stdin);

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

static void readAutoExec(void)
{
    FILE *fp;

    /* fp = fopen("AUTOEXEC.BAT", "r"); */
    fp = stdin;
    if (fp != NULL)
    {
        while (fgets(buf, sizeof buf, fp) != NULL)
        {
            processInput();
        }
        /* fclose(fp); */
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
        /* PosReboot(); */
    }
#if 0
    else if ((strlen(buf) == 2) && (buf[1] == ':'))
    {
        changedisk(buf[0]);
    }
#endif
    else
    {
        if (*p != '\0')
        {
            p--;
            *p = ' ';
        }
        printf("got %s\n", buf);
        /* system(buf); */
    }
    return;
}

static void putPrompt(void)
{
#if 0
    printf("\n%s:\\%s%s", drive, cwd, prompt);
#endif
    printf("\n");
    printf("%s:\\%s%s", drive, cwd, prompt);
    printf("\n");
#if 0
    fflush(stdout);
#endif
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

static void dodir(char *pattern)
{
    return;
}

static void changedir(char *to)
{
    /* PosChangeDir(to); */
    return;
}

static void changedisk(int drive)
{
    /* PosSelectDisk(toupper(drive) - 'A'); */
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
