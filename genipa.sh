#!/bin/bash
if ! which ldid &> /dev/null; then
    echo "ldid is not installed. Install with brew install ldid."
    exit 1
fi

OUTDIR=./dist/out.xcarchive
ORIG=$(pwd)

# Cleanup
if [ -d "$OUTDIR" ]; then
    echo "Cleaning previous build..."
    rm -r $OUTDIR
fi

xcodebuild -workspace iNDS.xcworkspace -scheme iNDS archive CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" -archivePath "$OUTDIR" | xcpretty
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    cd "$OUTDIR/Products"
    ldid -S "Applications/iNDS.app/iNDS"
    mv "Applications" "Payload"
    zip -r "iNDS.ipa" "Payload"
    cp "iNDS.ipa" ../../
    cd $ORIG
    echo -e "\nThe unsigned IPA can be found at dist/iNDS.ipa"
else
    echo -e "\nBuild failed!"
fi