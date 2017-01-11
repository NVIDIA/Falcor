@echo off

if "%1"=="release" (
    SET config="released3d12"
    goto findVS
    ) 
if "%1"=="debug" (
    SET config="debugd3d12"
    goto findVS
    )
echo Usage: BuildFalcorTest.bat ^<release^|debug^>

exit /B 1

:findVS
if defined VS140COMNTOOLS goto build
echo Can't find environment variable "VS140COMNTOOLS". Make sure Visual Studio 2015 is installed correctly.

:build
setlocal
echo Starting (%config%) build of %2
call "%VS140COMNTOOLS%\..\IDE\devenv.exe" FalcorTest.sln /rebuild %config% /project %2
if not %errorlevel%==0 goto buildFailed 
echo Build Succeeded
exit /B 0

:buildFailed
echo Build Failed
exit /B 1
