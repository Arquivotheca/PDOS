/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

typedef struct variable {
    char *name;
    size_t len;
    char *value;

    struct variable *next;
} variable;

variable *variable_add(char *name, char *value);
variable *variable_find(char *name);
void variable_change(char *name, char *value);

char *variable_expand_line(char *line);
void parse_var_line(char *line);
