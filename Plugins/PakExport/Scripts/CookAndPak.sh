#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

mkdir -p $SCRIPTPATH/Logs

exec 3>&1 4>&2
trap 'exec 2>&4 1>&3' 0 1 2 3
exec 1>$SCRIPTPATH/Logs/CookAndPak.out 2>&1

UtilsDir="$SCRIPTPATH/../Utils"

PakExportDir="$UtilsDir/PakExport"
PakESavedDir="$PakExportDir/Plugins/PakE/Saved"
SourcePakPath="$PakESavedDir/StagedBuilds/Mac/PakExport/Plugins/PakE/Content/Paks/Mac/PakEPakExport-Mac.pak"

UEDir=$1
DestDir=$2

echo UEDir=$UEDir
echo DestDir=$DestDir

#fix root path
$SCRIPTPATH/Game2PakE.sh $PakExportDir/Plugins/PakE/Content

#cook game
$SCRIPTPATH/CookGame.sh $UEDir

#cook PakE help plugin
$SCRIPTPATH/CookPakE.sh $UEDir

#copy pak to destination dir
#mkdir -p "$d"
cp "$SourcePakPath" "$DestDir"
