@echo off
rem %1==projectDir %2==outputdir
set local

IF not exist %2\Data\ mkdir %2\Data
IF exist %1\data\ ( xcopy %1\Data\*.* %2\Data /s /y )

rem deploy shaders
IF exist %1\ShadingUtils\ ( xcopy %1\ShadingUtils\*.* %2\Data /s /y )

rem deploy ray tracing data
IF exist %1\Raytracing\Data\ ( xcopy %1\Raytracing\Data\*.* %2\Data /s /y )

rem deploy effects