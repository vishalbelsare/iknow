#!/usr/bin/env bash

# Build Windows x86_64 wheels for Python 3.6 through Python 3.10. This script
# must be executed with the repository root as the working directory.
#
# Usage: actions/build_windows.sh
#
# Required Environment Variables:
# - ICU_URL is the URL to a .zip pre-built release of ICU for Windows x86_64
# - ICUDIR is the directory to install ICU
# - JSON_URL is the URL of the C++ JSON project on Github
# - JSONDIR is the directory to install the JSON header
# - BUILDCACHE_DIR is the directory where buildcache stores its cache
# - PYINSTALL_DIR is the directory where Python instances are installed
# - JSON_INCLUDE is the directory containing JSON for Modern C++ headers
# - PYVERSIONS is a space-delimited string of Python versions to install with
#   NuGet

set -euxo pipefail


##### Install ICU if it's not cached #####
if ! [ -f "$ICUDIR/iknow_icu_url.txt" ] || [ $(cat "$ICUDIR/iknow_icu_url.txt") != "$ICU_URL" ]; then
  rm -rf "$ICUDIR"
  curl -L -o icu4c.zip "$ICU_URL"
  mkdir -p "$ICUDIR"
  unzip -q icu4c.zip -d "$ICUDIR"
  echo "$ICU_URL" > "$ICUDIR/iknow_icu_url.txt"
fi

##### Build iKnow engine and run C++ unit tests #####
cd "$GITHUB_WORKSPACE/modules"
MSBUILD_PATH="/c/Program Files/Microsoft Visual Studio/2022/Enterprise/MSBuild/Current/Bin"
BUILDCACHE_IMPERSONATE=cl.exe PATH="$MSBUILD_PATH:$PATH" \
  MSBuild.exe iKnowEngine.sln -p:Configuration=Release -p:Platform=x64 \
    -maxcpucount \
    -p:ForceImportBeforeCppTargets="$(pwd)/EnableBuildCache.props" \
    -p:TrackFileAccess=false \
    -p:CLToolExe=buildcache.exe \
    -p:CLToolPath="$BUILDCACHE_EXE_DIR"
PATH="$ICUDIR/bin64:$PATH" ../kit/x64/Release/bin/iKnowEngineTest.exe


##### Build iknowpy wheels #####
cd iknowpy
for PYVERSION in $PYVERSIONS; do
  PYTHON="$PYINSTALL_DIR/python.$PYVERSION/tools/python.exe"
  "$PYTHON" setup.py bdist_wheel --no-dependencies
done
"$PYTHON" setup.py merge --no-dependencies
"$PYTHON" -m delvewheel repair dist/merged/iknowpy-*.whl --add-path "$ICUDIR/bin64;../../kit/x64/Release/bin"


##### Report cache statistics #####
"$BUILDCACHE_EXE_DIR/buildcache.exe" -s
