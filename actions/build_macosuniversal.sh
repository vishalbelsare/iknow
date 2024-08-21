#!/usr/bin/env bash

# Build universal2 wheels for Python 3.8 through Python 3.12, compatible with
# Mac OS X 10.9+ x86_64 and macOS 11.0+ arm64. This script must be executed with
# the repository root as the working directory.
#
# Usage: actions/build_macosuniversal.sh
#
# Required Environment Variables:
# - ICU_URL is the URL to a .tgz source release of ICU
# - ICUDIR is the directory to install ICU
# - MACOSX_DEPLOYMENT_TARGET is the minimum supported Mac OS version for x86_64
# - JSON_INCLUDE is the directory containing JSON for Modern C++ headers

set -euxo pipefail


##### Build ICU if it's not cached #####
if ! [ -f "$ICUDIR/iknow_icu_url.txt" ] || [ $(cat "$ICUDIR/iknow_icu_url.txt") != "$ICU_URL" ]; then
  rm -rf "$ICUDIR"
  curl -L -o icu4c-src.tgz "$ICU_URL"
  tar xfz icu4c-src.tgz
  cd icu/source
  CFLAGS="-arch x86_64 -arch arm64" CXXFLAGS="-std=c++11 -arch x86_64 -arch arm64" LDFLAGS=-headerpad_max_install_names ./runConfigureICU MacOSX --prefix="$ICUDIR"
  make -j $(sysctl -n hw.logicalcpu)
  make install
  echo "$ICU_URL" > "$ICUDIR/iknow_icu_url.txt"
fi

##### Build iKnow engine and run C++ unit tests #####
export IKNOWPLAT=macosuniversal
cd "$GITHUB_WORKSPACE"
make -j $(sysctl -n hw.logicalcpu) test

##### Build iknowpy wheels #####
cd modules/iknowpy
for PYTHON in python3.{8..12}; do
  "$PYTHON" setup.py bdist_wheel --plat-name=macosx-$MACOSX_DEPLOYMENT_TARGET-universal2 --no-dependencies
done
"$PYTHON" setup.py merge --no-dependencies
DYLD_LIBRARY_PATH="$GITHUB_WORKSPACE/kit/$IKNOWPLAT/release/bin:$ICUDIR/lib" "$PYTHON" -m delocate.cmd.delocate_wheel -w wheelhouse dist/merged/iknowpy-*.whl


##### Report cache statistics #####
ccache -s
