#! /bin/sh

PKG_NAME=micronucleus-cli
case $GITHUB_REF in
    refs/tags/*)
        RELEASE_TAG=$(basename $GITHUB_REF)
        PKG_NAME="$PKG_NAME-$RELEASE_TAG"
        ;;
    refs/heads/*)
        RELEASE_BRANCH=$(basename $GITHUB_REF)
        PKG_NAME="$PKG_NAME-$RELEASE_BRANCH-$(echo $GITHUB_SHA | head -c8)"
        ;;
esac

WORKSPACE_DIR=$(pwd)
cd commandline

CROSS_TARGET=$1
case "$CROSS_TARGET" in
   i386) triplet=i686-linux-gnu ;;
   armel) triplet=arm-linux-gnueabi ;;
   armhf) triplet=arm-linux-gnueabihf ;;
esac

if [ -n "$triplet" ]; then
    CCARG="CC=${triplet}-gcc"
    RELEASE_FILE=$PKG_NAME-$CROSS_TARGET-$(uname -s).zip
else
    RELEASE_FILE=$PKG_NAME-$(uname -m)-$(uname -s).zip
fi


case $BUILD_OS in
  Windows)
    mingw32-make USBLIBS=-lusb0
    ZIPLIB=/mingw64/bin/libusb*.dll
    EXE_EXT=.exe
    ;;
  macOS)
    make USBFLAGS=$(PKG_CONFIG_PATH=$(brew --prefix libusb-compat)/lib/pkgconfig pkg-config --cflags libusb) \
         USBLIBS="$(brew --prefix libusb-compat)/lib/libusb.a $(brew --prefix libusb)/lib/libusb-1.0.a -framework CoreFoundation -framework IOKit"
    ;;
  Linux)
    make $CCARG
    ;;
esac

mkdir $PKG_NAME
if [ -n "$ZIPLIB" ]; then eval "cp -p $ZIPLIB $PKG_NAME"; ZIPLIB=$(basename "$ZIPLIB"); fi
cp -p micronucleus${EXE_EXT} $PKG_NAME
echo "Built on $(date) from $GITHUB_REF:$GITHUB_SHA" > $PKG_NAME/BUILD_INFO
eval zip -r $WORKSPACE_DIR/$RELEASE_FILE $PKG_NAME

## Prepare release and artifact upload

for release_item in TAG BRANCH FILE
do
  eval echo "::set-output name=RELEASE_$release_item::\$RELEASE_$release_item"
done
