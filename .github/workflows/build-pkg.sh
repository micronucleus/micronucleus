#! /bin/sh

PKG_NAME=micronucleus-cli
WORKSPACE_DIR=$(pwd)
cd commandline

case $BUILD_OS in
  Windows)
    mingw32-make USBLIBS=-lusb0
    ZIPLIB=/mingw64/bin/libusb*.dll
    EXE_EXT=.exe
    ;;
  macOS)
    make USBFLAGS=$(PKG_CONFIG_PATH=$(brew --prefix libusb-compat)/lib/pkgconfig pkg-config --cflags libusb) \
         USBLIBS="$(PKG_CONFIG_PATH=$(brew --prefix libusb-compat)/lib/pkgconfig pkg-config --libs libusb) -lusb"
    ZIPLIB=$(find $(brew --prefix libusb-compat)/lib -name "*dylib" -type f -depth 1)
    ;;
  Linux)
    make
    ;;
esac

g++ -o launcher${EXE_EXT} avr-dummy/avrdude-dummy.cpp

RELEASE_FILE=$PKG_NAME-$(uname -m)-$(uname -s).zip
mkdir $PKG_NAME
if [ -n "$ZIPLIB" ]; then eval "cp -p $ZIPLIB $PKG_NAME"; ZIPLIB=$(basename "$ZIPLIB"); fi
cp -p micronucleus${EXE_EXT} launcher${EXE_EXT} $PKG_NAME
eval zip -r $WORKSPACE_DIR/$RELEASE_FILE $PKG_NAME

## Prepare release and artifact upload

case $GITHUB_REF in
    refs/tags/*)
        RELEASE_TAG=$(basename $GITHUB_REF)
        ;;
    refs/heads/*)
        RELEASE_BRANCH=$(basename $GITHUB_REF)
        ;;
esac
for release_item in TAG BRANCH FILE
do
  eval echo "::set-output name=RELEASE_$release_item::\$RELEASE_$release_item"
done
