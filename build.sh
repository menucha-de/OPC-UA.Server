#!/bin/bash
 
field() {
  grep -i "^$1:" debian/control | sed -e 's/[^:]*: *//' | head -n 1
}
 
SOURCE=$(field Source)
VERSION=$(head -1 debian/changelog | sed 's/.*(\(.*\)).*/\1/')
TMP=$(mktemp -d)
ID=${SOURCE}_${VERSION}
DSC=$ID.dsc
FILE=$ID.tar.xz
TARGET=target
 
mkdir $TARGET
 
tar cfJ $TARGET/$FILE --exclude-vcs --exclude ./$TARGET .
  
cat > $TARGET/$DSC <<EOF
Format: 3.0 (native)
Source: $SOURCE
Architecture: $ARCH
Version: $VERSION
Build-Depends: $(field Build-Depends)
Files:
 $(md5sum $TARGET/$FILE | sed -e "s/ $TARGET\//$(stat -c %s $TARGET/$FILE) /")
EOF
 
cd $TARGET
 
export SBUILD_CONFIG=../sbuild.conf
sbuild -v -d $DIST --host $ARCH $DSC
