/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#include "cpplib.h"
#include <stdio.h>

typedef struct pp_output pp_output;

pp_output *init_pp_output(cpp_reader *reader, FILE *out);
void preprocess_file(pp_output *pp_o, cpp_reader *reader);
