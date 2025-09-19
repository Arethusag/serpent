@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul

cl /nologo /TC /Zi /Od /W4 /WX /MDd /I"extern\bluey" "serpent.c" "extern\bluey\bluey.c" "extern\bluey\bluey_utils.c" /Fe:"serpent.exe" /Fd:"serpent.pdb"
