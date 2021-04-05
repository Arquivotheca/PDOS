/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#include <ctype.h>

#define ISDIGIT(a) (isdigit(a)) /* 0-9 */
#define ISIDNUM(a) (isdigit(a) || islower(a) \
                    || isupper(a) || (a == '_')) /* 0-9a-zA-Z_ */
#define ISNVSPACE(a) (((a) == ' ') || ((a) == '\t') || ((a) == '\f') \
                      || ((a) == '\v') || ((a) == '\0'))
#define ISALPHA(a) (isalpha(a))
#define ISXDIGIT(a) (isxdigit(a))

const char *filename(const char *name);
