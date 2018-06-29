#!/bin/bash
if ! which unipkg &> /dev/null; then
    echo "unipkg is not installed. Install with npm install -g unipkg."
    exit 1
fi

OUTDIR=./dist/out.xcarchive
ORIG=$(pwd)

# Cleanup
if [ -d "$OUTDIR" ]; then
    echo "Cleaning previous build..."
    rm -r $OUTDIR
fi


xcodebuild -workspace iNDS.xcworkspace -scheme iNDS archive CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" -archivePath "$OUTDIR" | xcpretty
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    cd "$OUTDIR/Products"
    INDS_VER=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" Applications/iNDS.app/Info.plist)
    cp -R $ORIG/DEBIAN .
    ldid -S "Applications/iNDS.app/iNDS"
    cd ..
    INDS_VER=$INDS_VER unipkg build "Products"
    cp "net.nerd.iNDS_${INDS_VER}_iphoneos-arm.deb" ../
    echo -e "\nThe deb can be found at dist/net.nerd.iNDS_${INDS_VER}_iphoneos-arm.deb"
else
    echo -e "\nBuild failed!"
fi