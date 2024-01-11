@echo off

setlocal

set DestName=%1
set DestName=%DestName:"=%
set SourceDir=%2
set SourceDir=%SourceDir:"=%

for %%I in ("%~dp0.") do for %%J in ("%%~dpI.") do set ParentFolderName=%%~dpnxJ

set UtilsDir=%ParentFolderName%\Utils

cd "%SourceDir%"

"%UtilsDir%\7-Zip\x64\7zr.exe" a -r "%DestName%.7z" *
