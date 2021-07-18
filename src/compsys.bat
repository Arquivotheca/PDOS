rem Use Watcom's version of int86 routine which compensates
rem for flags not being popped in INT 25H and 26H

wcl -I. -ml -DWATNATIVE sys.c pos.c
