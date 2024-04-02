@echo off

setlocal

for %%I in ("%~dp0.") do for %%J in ("%%~dpI.") do set ParentFolderName=%%~dpnxJ

set UtilsDir=%ParentFolderName%\Utils
call %ParentFolderName%\Scripts\Cleanup.bat
cd "%UtilsDir%"
"%UtilsDir%\7-Zip\x64\7za.exe" x "%UtilsDir%\PakExport.7z" -o%UtilsDir%

::sync project settings
echo F|XCOPY /S /Q /Y /F "%ParentFolderName%\..\..\Config\DefaultEngine.ini" "%UtilsDir%\PakExport\Config\DefaultEngine.ini"

exit /b

:UnZipFile <ExtractTo> <newzipfile>
set vbs="%temp%\_.vbs"
if exist %vbs% del /f /q %vbs%
>%vbs%  echo Set fso = CreateObject("Scripting.FileSystemObject")
>>%vbs% echo destPath = fso.GetAbsolutePathName(%1)
>>%vbs% echo strZipPath = fso.GetAbsolutePathName(%2)
>>%vbs% echo If NOT fso.FolderExists(destPath) Then
>>%vbs% echo fso.CreateFolder(destPath)
>>%vbs% echo End If
>>%vbs% echo set objShell = CreateObject("Shell.Application")
>>%vbs% echo set FilesInZip=objShell.NameSpace(strZipPath).items
>>%vbs% echo objShell.NameSpace(destPath).CopyHere(FilesInZip)
>>%vbs% echo Set fso = Nothing
>>%vbs% echo Set objShell = Nothing
cscript //nologo %vbs%
if exist %vbs% del /f /q %vbs%
