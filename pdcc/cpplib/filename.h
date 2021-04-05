/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#define IS_ABSOLUTE_PATH(path) IS_ABSOLUTE_PATH_2(1, path)
#define IS_ABSOLUTE_PATH_2(i, path) \
 (IS_DIR_SEPARATOR((i), (path)[0]) \
  || HAS_DISK_SPECIFICATION((i), (path)))

#define IS_DIR_SEPARATOR(i, c) \
 ((c == '/') || ((c == '\\') && (i)))

#define HAS_DISK_SPECIFICATION(i, path) \
 ((path)[0] && ((path)[1] == ':') && (i))
