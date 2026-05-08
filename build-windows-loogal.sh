#!/usr/bin/env bash
set -euo pipefail

mkdir -p releases/loogal-windows

SRC=$(
find src -name '*.c' \
! -path 'src/gui/*' \
! -name 'cmd_action.c'
)

x86_64-w64-mingw32-gcc \
-O3 \
-Wall \
-Wextra \
-std=gnu11 \
-static \
-static-libgcc \
-static-libstdc++ \
-DWIN32 \
-D_WINDOWS \
-Iinclude \
$SRC \
-o releases/loogal-windows/loogal.exe \
-lm \
-lshlwapi

dotnet publish windows/LoogalWindow/LoogalWindow.csproj \
-c Release \
-r win-x64 \
--self-contained true \
-p:PublishSingleFile=true \
-o releases/loogal-windows

echo
echo "DONE"
echo "releases/loogal-windows/LoogalWindow.exe"
echo "releases/loogal-windows/loogal.exe"
