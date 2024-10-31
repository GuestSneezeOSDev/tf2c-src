@echo off
devtools\bin\vpc.exe /tf2classic /2013 +game /mksln Tf2classic.sln
timeout /t 1 /nobreak > NUL
