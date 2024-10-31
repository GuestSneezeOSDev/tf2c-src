@echo off

set "source=F:\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin"
set "target=%~dp0bin"

mklink /d "%target%" "%source%"

echo Symbolic link created from "%source%" to "%target%". Check for error at the top!
pause