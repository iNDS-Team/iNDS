#!/bin/bash
if ! which unipkg &> /dev/null; then
    echo "unipkg is not installed. Install with npm install -g unipkg."
    exit 1
fi

if ! which ldid &> /dev/null; then
    echo "ldid is not installed. Install with brew install ldid."
    exit 1
fi

INDS_VER=$(sed -n '/MARKETING_VERSION/{s/MARKETING_VERSION = //;s/;//;s/^[[:space:]]*//;p;q;}' ./iNDS.xcodeproj/project.pbxproj)
OUTDIR=./dist/out.xcarchive
OUTFILE="dist/net.nerd.iNDS_${INDS_VER}_iphoneos-arm.deb"
ORIG=$(pwd)

# Cleanup
if [ -d "$OUTDIR" ]; then
    echo "Cleaning previous build..."
    rm -r $OUTDIR
fi

if [ -f "$OUTFILE" ]; then
    echo "Cleaning previous output..."
    rm $OUTFILE
fi


xcodebuild -workspace iNDS.xcworkspace -scheme iNDS archive GCC_PREPROCESSOR_DEFINITIONS="JAILBROKEN=1 $GCC_PREPROCESSOR_DEFINITIONS" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" -archivePath "$OUTDIR" | xcpretty
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    cd "$OUTDIR/Products"
    cp -R $ORIG/DEBIAN .
    ldid -S../../../Tools/ent.xml "Applications/iNDS.app/iNDS"
    cd ..
    INDS_VER=$INDS_VER unipkg build "Products"
    cp "net.nerd.iNDS_${INDS_VER}_iphoneos-arm.deb" ../
    echo -e "\nThe deb can be found at dist/net.nerd.iNDS_${INDS_VER}_iphoneos-arm.deb"
else
    echo -e "\nBuild failed!"
fi