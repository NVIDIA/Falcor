@echo off
setlocal
pushd %~dp0

:: TODO: ensure that everything is built?

.\Source\Debug\SpireTestTool.exe %*
