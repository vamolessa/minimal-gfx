@echo off
setlocal

where /Q cl || (
    echo error: run this on a msvc enabled shell
    exit /b 1
)
set CL=-nologo -std:c17 -utf-8 -external:W0 -external:anglebrackets -external:I . -Z7 -Od -MTd -RTCcsu
set _CL_=-link -incremental:no

cl opengl45.c

echo.
echo finished
