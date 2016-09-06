@echo off
rem %1==config %2==platformname %3==outputdir
set local

rem set some local variables
if "%2" == "Win32" SET ATBDLL="AntTweakBar.dll"
if "%2" == "x64" SET ATBDLL="AntTweakBar64.dll"

if not exist %3 mkdir %3
SET FALCOR_PROJECT_DIR=%~dp0
copy /y %FALCOR_PROJECT_DIR%\Externals\glew\bin\Release\%2\glew32.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\AntTweakBar\lib\%ATBDLL% %3
copy /y %FALCOR_PROJECT_DIR%\Externals\GLFW\lib\%2\%1\glfw3.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\FreeImage\Lib\%2\%1\freeimage.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\assimp\bin\%2\*.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\FFMpeg\bin\%2\*.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\D3DCompiler\%2\D3Dcompiler_47.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\OptiX\bin64\*.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\openvr\bin\win64\openvr_api.dll %3
copy /y %FALCOR_PROJECT_DIR%\Externals\crosscompiler.exe %3

rem copy and overwrite internal files
for /r %FALCOR_PROJECT_DIR%\..\..\Internals\ %%f in (*.dll) do @copy /y "%%f" %3

copy /y %FALCOR_PROJECT_DIR%\CopyData.bat %3
call %FALCOR_PROJECT_DIR%\CopyData.bat %FALCOR_PROJECT_DIR%\Source %3