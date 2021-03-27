/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#include "vgatext.h"
#include "support.h"

#include <string.h>

#define MAX_COL 80
#define MAX_ROW 25

void vgatext_init(VGATEXT *vgatext)
{
    /* Checks BIOS data area to see if the monitor is monochrome. */
    if ((*((unsigned short *)0x410) & 0x30) == 0x30)
    {
        vgatext->video = (volatile unsigned char *)0xb0000;
    }
    else vgatext->video = (volatile unsigned char *)0xb8000;
    vgatext->cursor_row = 0;
    vgatext->cursor_column = 0;
}

void vgatext_read_cursor_position(VGATEXT *vgatext,
                                  unsigned int *row,
                                  unsigned int *column)
{
    unsigned int pos = 0;

    outp(0x3D4, 0x0F);
    pos |= inp(0x3D5);
    outp(0x3D4, 0x0E);
    pos |= ((unsigned int)inp(0x3D5)) << 8;

    vgatext->cursor_row = *row = pos / MAX_COL;
    vgatext->cursor_column = *column = pos % MAX_COL;
}

void vgatext_set_cursor_position(VGATEXT *vgatext,
                                 unsigned int row,
                                 unsigned int column)
{
    unsigned int pos = row * MAX_COL + column;
    
    vgatext->cursor_row = row;
    vgatext->cursor_column = column;

    outp(0x3D4, 0x0F);
    outp(0x3D5, (unsigned char)(pos & 0xFF));
    outp(0x3D4, 0x0E);
    outp(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void vgatext_write_with_attr(VGATEXT *vgatext,
                             unsigned char c,
                             unsigned char attr)
{
    vgatext->video[(vgatext->cursor_row
                    * MAX_COL + vgatext->cursor_column) * 2] = c;
    vgatext->video[(vgatext->cursor_row
                    * MAX_COL + vgatext->cursor_column) * 2 + 1] = attr;
}

void vgatext_write_new_line(VGATEXT *vgatext)
{
    vgatext->cursor_column = 0;
    vgatext->cursor_row++;
    if (vgatext->cursor_row == MAX_ROW)
    {
        int i;

        for (i = 0; i < MAX_COL * (MAX_ROW - 1) * 2; i++)
        {
            vgatext->video[i] = vgatext->video[i + MAX_COL * 2];
        }
        for (i = 0; i < MAX_COL * 2; i++)
        {
            vgatext->video[MAX_COL * (MAX_ROW - 1) * 2 + i] = 0;
        }
        vgatext->cursor_row--;
    }
    
    vgatext_set_cursor_position(vgatext,
                                vgatext->cursor_row,
                                vgatext->cursor_column);
}

void vgatext_write_text(VGATEXT *vgatext,
                        unsigned char c,
                        unsigned char color)
{
    switch (c)
    {
        case '\r': vgatext->cursor_column = 0; break;
        case '\n': vgatext_write_new_line(vgatext); break;
        case 7: /* +++Do nothing? */ break;
        default:
            vgatext->video[(vgatext->cursor_row
                            * MAX_COL + vgatext->cursor_column) * 2] = c;
            break;
    }
}

void vgatext_clear_screen(VGATEXT *vgatext, unsigned char attr)
{
    int i;

    for (i = 0; i < MAX_COL * MAX_ROW; i++)
    {
        vgatext->video[i * 2] = 0;
        vgatext->video[i * 2 + 1] = attr;
    }

    vgatext_set_cursor_position(vgatext, 0, 0);
}

unsigned int vgatext_get_max_cols(VGATEXT *vgatext)
{
    return (MAX_COL);
}

