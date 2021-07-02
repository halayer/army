@echo off

gcc main.c ARM.c bus.c isa.c instr.c -Iinclude -o main.exe

exit /B