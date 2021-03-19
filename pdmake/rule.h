/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

typedef struct rule {
    struct rule *next;

    char *name;
    struct dep *deps;
    struct commands *cmds;
} rule;

typedef struct suffix_rule {
    struct suffix_rule *next;

    char *first;
    char *second;

    struct commands *cmds;
} suffix_rule;

void rule_add(char *name, struct dep *deps, struct commands *cmds);

void rule_search_and_build(char *name);

void rule_add_suffix(char *name, struct commands *cmds);
