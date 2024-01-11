#!/bin/bash

DestName=$1
SourceDir=$2

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
UtilsDir="$SCRIPTPATH/../Utils"

cd "$SourceDir"

"$UtilsDir/7-Zip-Mac/7zz" a -r "$DestName.7z" *
