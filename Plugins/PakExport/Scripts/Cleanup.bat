@echo off

setlocal
set ThisDir=%~dp0
set UtilsDir=%ThisDir%..\Utils
set PakExportDir=%UtilsDir%\PakExport
RMDIR /s /q "%PakExportDir%"
