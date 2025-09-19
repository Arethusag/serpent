@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul

cl /nologo /TC /Zi /Od /W4 /WX /Za /MDd /I"extern\bluey" "source\snake.c" "extern\bluey\bluey.c" "extern\bluey\bluey_utils.c" /Fe:"build\snake.exe" /Fd:"build\snake.pdb"
