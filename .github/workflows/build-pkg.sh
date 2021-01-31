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
         USBLIBS="$(brew --prefix libusb-compat)/lib/libusb.a $(brew --prefix libusb)/lib/libusb-1.0.a -framework CoreFoundation -framework IOKit"
    ZIPLIB=$(find $(brew --prefix libusb-compat)/lib -name "*dylib" -type f -depth 1)
    ;;
  Linux)
    make
    ;;
esac

RELEASE_FILE=$PKG_NAME-$(uname -m)-$(uname -s).zip
mkdir $PKG_NAME
if [ -n "$ZIPLIB" ]; then eval "cp -p $ZIPLIB $PKG_NAME"; ZIPLIB=$(basename "$ZIPLIB"); fi
cp -p micronucleus${EXE_EXT} $PKG_NAME
echo "Built on $(date) from $GITHUB_REF:$GITHUB_SHA" > $PKG_NAME/BUILD_INFO
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
