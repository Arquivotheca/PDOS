/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

typedef struct vgatext {
    volatile unsigned char *video;
    unsigned int cursor_row;
    unsigned int cursor_column;
} VGATEXT;

void vgatext_init(VGATEXT *vgatext);
void vgatext_read_cursor_position(VGATEXT *vgatext,
                                  unsigned int *row,
                                  unsigned int *column);
void vgatext_set_cursor_position(VGATEXT *vgatext,
                                 unsigned int row,
                                 unsigned int column);
void vgatext_write_with_attr(VGATEXT *vgatext,
                             unsigned char c,
                             unsigned char attr);
void vgatext_write_new_line(VGATEXT *vgatext);
void vgatext_write_text(VGATEXT *vgatext,
                        unsigned char c,
                        unsigned char color);
void vgatext_clear_screen(VGATEXT *vgatext, unsigned char attr);
    
unsigned int vgatext_get_max_cols(VGATEXT *vgatext);
