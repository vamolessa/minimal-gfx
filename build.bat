@echo off
setlocal

rem run this on a msvc enabled shell
set CL=-external:W0 -external:anglebrackets -external:I .

cl opengl45.c
