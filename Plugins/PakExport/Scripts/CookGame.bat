@echo off

setlocal
set ThisDir=%~dp0
set UtilsDir=%ThisDir%..\Utils
set PakExportDir=%UtilsDir%\PakExport
set PakExportProjectFile=%PakExportDir%\PakExport.uproject

set UEDir=%1
set UEDir=%UEDir:"=%
set UECmdDir=%UEDir%/Binaries/Win64/UnrealEditor-Cmd.exe
set UEUATDir=%UEDir%/Build/BatchFiles/RunUAT.bat

::cook game
"%UEUATDir%" -ScriptsForProject="%PakExportProjectFile%" BuildCookRun -project="%PakExportProjectFile%" -noP4 -clientconfig=Development -serverconfig=Development -nocompile -nocompileeditor -installed -unrealexe="%UECmdDir%" -utf8output -platform=Win64 -targetplatform=Win64 -build -cook -map=Minimal_Default -unversionedcookedcontent -pak -createreleaseversion=1.0 -compressed -stage -package -cmdline="Minimal_Default -Messaging"
