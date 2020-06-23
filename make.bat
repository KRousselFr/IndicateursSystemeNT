windres -i IndicSys.rc -o IndicSysRes.o
gcc -std=c99 -c IndicSys.c -o IndicSys.o -Wall -Wextra -Wconversion
gcc IndicSys.o IndicSysRes.o -o IndicSys.exe -Wall -Wextra -Wconversion -O -mwindows
strip IndicSys.exe
