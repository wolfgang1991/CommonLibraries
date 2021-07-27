#!/bin/bash

echo "This are windows only examples for named pipes. mingw64 is used."

COMMON="../../Common"
DIR="$PWD"
cd "$COMMON"
set -v
mingw32-make.exe -j8 PLATFORM=win32 DEBUG=1 CC=gcc
set +v
cd "$DIR"

set -v
g++ -g serverMain.cpp -I$COMMON -L$COMMON -lCommon -o serverMain.exe
g++ -g clientMain.cpp -I$COMMON -L$COMMON -lCommon -o clientMain.exe
g++ -g nonBlockingServerMain.cpp -I$COMMON -L$COMMON -lCommon -o nonBlockingServerMain.exe

