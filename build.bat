@echo off

gcc -g main.c ARM.c cbs/bus.c isa.c instr.c -Iinclude -o main.exe

exit /B