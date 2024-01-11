#!/bin/bash

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

mkdir -p $SCRIPTPATH/Logs

exec 3>&1 4>&2
trap 'exec 2>&4 1>&3' 0 1 2 3
exec 1>$SCRIPTPATH/Logs/CookGame.out 2>&1

PakExportProjectFile="$SCRIPTPATH/../Utils/PakExport/PakExport.uproject"

UEDir=$1
UECmdDir=$UEDir/Binaries/Mac/UnrealEditor-Cmd
UEUATDir=$UEDir/Build/BatchFiles/RunUAT.sh

#cook game
"$UEUATDir" -ScriptsForProject="$PakExportProjectFile" BuildCookRun -project="$PakExportProjectFile" -noP4 -clientconfig=Shipping -serverconfig=Shipping -unrealexe="$UECmdDir" -utf8output -platform=Mac -targetplatform=Mac -build -cook -map=Minimal_Default -unversionedcookedcontent -pak -createreleaseversion=1.0 -compressed -stage -package -cmdline="Minimal_Default -Messaging" -installed
