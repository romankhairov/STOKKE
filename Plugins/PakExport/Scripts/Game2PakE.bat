@echo off

setlocal
set ThisDir=%~dp0
set UtilsDir=%ThisDir%..\Utils
set PakExportDir=%UtilsDir%\PakExport

set Dir=%1
set Dir=%Dir:"=%

echo Dir=%Dir%

::fix root path
"%UtilsDir%\fnr.exe" --cl --dir "%PakExportDir%\Plugins\PakE\Content" --fileMask "*.*" --includeSubDirectories --caseSensitive --KeepModifiedDate --skipBinaryFileDetection --alwaysUseEncoding "windows-1251" --find "Game" --replace "PakE"

exit /b
