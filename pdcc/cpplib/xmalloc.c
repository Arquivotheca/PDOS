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

static void *xmalloc(size_t size)
{
    void *p = malloc(size);

    if (p == NULL)
    {
        fprintf(stderr, "failed to allocate %u bytes\n", size);
        abort();
    }

    return (p);
}
