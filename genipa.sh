OUTDIR=./dist/out.xcarchive
ORIG=$(pwd)
xcodebuild -workspace iNDS.xcworkspace -scheme iNDS archive CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" -archivePath "$OUTDIR" | xcpretty
cd "$OUTDIR/Products"
mv "Applications" "Payload"
zip -r "iNDS.ipa" "Payload"
cp "iNDS.ipa" ../../
cd $ORIG
echo "\nThe unsigned IPA can be found at dist/iNDS.ipa"