/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#include "support.h"

const char *filename(const char *name)
{
    const char *c;

    /* Skips disk name. */
    if (ISALPHA(name[0]) && (name[1] == ':'))
        name += 2;

    for (c = name; *name; name++)
    {
        if ((*name == '/') || (*name == '\\'))
            c = name + 1;
    }

    return (c);
}
