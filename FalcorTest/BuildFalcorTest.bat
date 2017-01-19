@echo off
setlocal ENABLEDELAYEDEXPANSION 
set CL=/D_AUTOTESTING

set "action="
if "%1"=="clean" set action=clean
if "%1"=="build" set action=build
if "%1"=="rebuild" set action=rebuild
if not defined action goto usage

set "config="
if "%2"=="released3d12" set config=released3d12
if "%2"=="released3d11" set config=released3d11
if "%2"=="releasegl" set config=releasegl
if "%2"=="debugd3d12" set config=debugd3d12
if "%2"=="debugd3d11" set config=debugd3d11
if "%2"=="debuggl" set config=debuggl
if not defined config goto usage

goto findVS

:findVS
if defined VS140COMNTOOLS goto act
echo Can't find environment variable "VS140COMNTOOLS".  

:act
set project=%3
set errFileSuffix=_BuildFailLog.txt
if defined project (
    set errFile="%project%%errFileSuffix%"
    echo Starting %action% of %config% config of project %project%
    call "%VS140COMNTOOLS%\..\IDE\devenv.com" FalcorTest.sln /%action% %config% /project %project% > !errFile!
) else (
    set errFile="FalcorTestSolution%errFileSuffix%"
    echo Starting %action% of %config% config of entire solution
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
echo Usage: BuildFalcorTest.bat ^<clean^|build^|rebuild^> config [projectName(entire sln if omitted)]
exit /B 1
