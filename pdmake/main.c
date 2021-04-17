/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "read.h"
#include "dep.h"
#include "commands.h"
#include "rule.h"
#include "xmalloc.h"

static int dry_run = 0; /* Run no commands. */

variable *default_goal_var;

static rule *rules = NULL;
static suffix_rule *suffix_rules = NULL;

void rule_add(char *name, struct dep *deps, struct commands *cmds)
{
    rule *r = xmalloc(sizeof(*r));

    r->name = name;
    r->deps = deps;
    r->cmds = cmds;

    r->next = rules;
    rules = r;
}

void rule_add_suffix(char *name, struct commands *cmds)
{
    suffix_rule *s = xmalloc(sizeof(*s));
    char *p = strchr(name + 1, '.');

    *p = '\0';
    s->first = xstrdup(name);
    *p = '.';
    s->second = xstrdup(p);

    s->cmds = cmds;

    s->next = suffix_rules;
    suffix_rules = s;
}

void rule_use(rule *r, char *name)
{
    struct dep *dep;
    char *new_cmds;
    char *p, *q;
    char *star_name;
    
    for (dep = r->deps; dep; dep = dep->next)
    {
        rule_search_and_build(dep->name);
    }

    if (r->cmds == NULL) return;

    star_name = xstrdup(name);
    p = strrchr(star_name, '.');
    if (p) *p = '\0';

    variable_change("@", xstrdup(name));
    variable_change("*", star_name);

    p = r->cmds->text;
    q = strchr(p, '\n');
    while (1)
    {
        *q = '\0';
        new_cmds = xstrdup(p);
        *q = '\n';
        new_cmds = variable_expand_line(new_cmds);
        printf("%s\n", new_cmds);

        if (!dry_run) system(new_cmds);
        free(new_cmds);

        p = q + 1;
        q = strchr(p, '\n');
        if (q == NULL) break;
    }
}

void suffix_rule_use(suffix_rule *s, char *name)
{
    char *lesser_name, *star_name;
    char *p;
    char *new_cmds;
    char *q;
    
    if (s->cmds == NULL) return;

    lesser_name = xmalloc(strlen(name) + strlen(s->first) + 1);
    memcpy(lesser_name, name, strlen(name) + 1);
    p = strrchr(lesser_name, '.');
    if (p) *p = '\0';
    strcat(lesser_name, s->first);

    star_name = xstrdup(name);
    p = strrchr(star_name, '.');
    if (p) *p = '\0';

    variable_change("@", xstrdup(name));
    variable_change("<", lesser_name);
    variable_change("*", star_name);

    p = s->cmds->text;
    q = strchr(p, '\n');
    while (1)
    {
        *q = '\0';
        new_cmds = xstrdup(p);
        *q = '\n';
        new_cmds = variable_expand_line(new_cmds);
        printf("%s\n", new_cmds);

        if (!dry_run) system(new_cmds);
        free(new_cmds);

        p = q + 1;
        q = strchr(p, '\n');
        if (q == NULL) break;
    }   
}

void rule_search_and_build(char *name)
{
    rule *r;
    suffix_rule *s;
    char *suffix;

    for (r = rules; r; r = r->next)
    {
        if (strcmp(name, r->name) == 0) break;
    }

    if (r)
    {
        rule_use(r, name);
        return;
    }

    suffix = strrchr(name, '.');
    for (s = suffix_rules; s; s = s->next)
    {
        if (strcmp(suffix, s->second) == 0)
        {
            FILE *f;
            char *orig, *p;
            
            orig = xmalloc(strlen(name) + strlen(s->first) + 1);
            memcpy(orig, name, strlen(name) + 1);
            p = strrchr(orig, '.');
            *p = '\0';
            strcat(orig, s->first);

            f = fopen(orig, "r");
            free(orig);

            if (f)
            {
                fclose(f);
                break;
            }
        }
    }

    if (s)
    {
        suffix_rule_use(s, name);
        return;
    }

}

void help(void)
{
    printf("Usage: pdmake [options] [target]...\n");
    printf("Options:\n");
    printf("  -B                  "
           "Make everything regardless of timestamps.\n");
    printf("  -f FILE             "
           "Read FILE as a makefile.\n");
    printf("  -h, --help          "
           "Print this message and exit.\n");
    printf("  -n, --dry_run       "
           "Run no commands, only print them.\n");
}

int main(int argc, char **argv)
{
    int i;
    char *name = "Makefile";
    char *goal = NULL;
    
    default_goal_var = variable_add(xstrdup(".DEFAULT_GOAL"), xstrdup(""));

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'B':
                    printf("Rebuilding everything, regardless of timestamps.\n");
                    break;

                case 'f':
                    if (argv[i][2] == '\0')
                    {
                        name = argv[++i];
                    }
                    else
                    {
                        name = argv[i] + 2;
                    }
                    break;

                case 'h':
                    help();
                    return (0);

                case 'n':
                    dry_run = 1;
                    break;

                case '-':
                    if (strcmp("help", argv[i] + 2) == 0)
                    {
                        help();
                        return (0);
                    }
                    else if (strcmp("dry_run", argv[i] + 2) == 0)
                    {
                        dry_run = 1;
                    }
                    else printf("Unknown switch! Use -h for help.\n");
                    break;

                default: printf("Unknown switch! Use -h for help.\n"); break;
            }
        }
        else
        {
            goal = argv[i];
        }
    }

    read_makefile(name);

    if (goal == NULL) goal = default_goal_var->value;
    
    rule_search_and_build(goal);
    
    return (0);
}
