@echo off

REM Compiler flags:
REM - /nologo : Suppress version banner
REM - /Zi     : Generate debug information
REM - /Od     : Disable optimization (debug build)
REM - /W4     : Maximum warning level
REM - /Za     : Disable Microsoft extensions (ANSI compliance)
REM - /MDd    : Use debug multithreaded DLL runtime
REM - /Fe     : Output executable name
REM - /Fd     : Debug information file

cl /nologo /TC /Zi /Od /W4 /MDd main.c /Fe:"build/main.exe" /Fo:"build/main.obj" /Fd:"build/main.pdb"
