:: v0.1 ~ brysondev
@ECHO OFF
cls
title build-Binaries for OFD
echo (%TIME%) Script started.
whoami
dir
FOR /F "tokens=*" %%g IN ('echo %~dp0..\src\') do (SET solutionsFile=%%g)
:: cd ..
rmdir /S /Q game
cd src
call "createtf2classic.bat"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsMSBuildCmd.bat"
cd %solutionsFile%
devenv.com Tf2Classic.sln /Clean
devenv.com Tf2Classic.sln /Rebuild Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
    echo Build Successful
    exit 0
)
if not %BUILD_STATUS%==0 (
    echo Build failed
    exit 1
)
:: Catch all just end success...
exit 0
