@echo off
setlocal ENABLEDELAYEDEXPANSION 

set "action="
if "%1"=="clean" set action=clean
if "%1"=="build" set action=build
if "%1"=="rebuild" set action=rebuild
if not defined action goto usage

set "config="
if "%2"=="release" set config=released3d12
if "%2"=="debug" set config=debugd3d12
if not defined config goto usage

goto findVS

:findVS
if defined VS140COMNTOOLS goto act
echo Can't find environment variable "VS140COMNTOOLS". Make sure Visual Studio 2015 is installed correctly.
exit /B 1

:act
set project=%3
set errFileSuffix=_%2%_BuildFailLog.txt
if defined project (
    set errFile="%project%%errFileSuffix%"
    echo Starting %action% of %config% config of project %project%. If fail, log will be written to !errFile!
    call "%VS140COMNTOOLS%\..\IDE\devenv.com" FalcorTest.sln /%action% %config% /project %project% > !errFile!
) else (
    set errFile="FalcorTestSolution%errFileSuffix%"
    echo Starting %action% of %config% config of entire solution. If fail, log will be written to !errFile!
    call "%VS140COMNTOOLS%\..\IDE\devenv.com" FalcorTest.sln /%action% %config% > !errFile!
)
if not %errorlevel%==0 (
    goto buildFailed 
)else (
    del !errFile!
)

exit /B 0

:buildFailed
echo Build Failed
exit /B 1

:usage
echo Usage: BuildFalcorTest.bat ^<clean^|build^|rebuild^> ^<release^|debug^> [projectName(entire sln if omitted)]
exit /B 1
