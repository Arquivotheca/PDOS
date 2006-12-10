tcc -c -ml -I..\pdpclib world.c
tlink -x ..\pdpclib\dosstart.obj+world.obj,world.exe,,..\pdpclib\borland.lib,
