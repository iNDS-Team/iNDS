iNDS
=======
###### Supports iOS 8.1 - 9.2.

iNDS is a derivation of the previous Nintendo DS apps for iOS, nds4ios and Nitrogen. 

[Nitrogen](https://github.com/NitrogenEmulator) 

Currently, emulation is powered by the DeSmuME threaded ARM interpreter and runs at nearly full speed on the iPhone 5 and above.

Due to the need to mmap the entire ROM into memory, older devices with only 256MB of RAM are not supported. These devices include the iPod touch 4, iPad 1, iPhone 3GS, and anything below those devices.

[iNDS](http://www.williamlcobb.com/iNDS.html)

[DeSmuME](http://desmume.org/) 

Installing iNDS
------------------------
#### Option 1a: Cydia

If you're jailbroken, please add the following source: http://www.williamlcobb.com/repo

Download iNDS from the repository.

#### Option 1b: iEmulators
##### This method requires that the uploaded version does not have a revoked certificate.
1. Visit (http://iEmulators.com/) 
2. Go to the "Apps Section" and scroll until you see "iNDS", tap it
3. Tap "Download Page"
4. Tap "iNDS" again.
5. Hit install followed by "Install iNDS"
6. Wait for the app to download fully, do not open it.
7. Go to your Settings app, general, device management (formally profiles) find the profile for iNDS, and accept the profile.
8. Open the iNDS app on your Homescreen and enjoy.
 
NOTE: If the certificate does get revoked and you do not have Dropbox Sync enabled but wish to keep your save files you will have to leave the app UNDELETED until it gets a new certificate. Check weekly or follow iEmulators' twitter.

#### Option 2a: Compile and install iNDS from the Xcode IDE
##### Please Do not redistribute self-compiled versions of iNDS on other sites. 
#### Compile and install iNDS from the Xcode IDE
##### IMPORTANT: Make sure your working directory is devoid of spaces. Otherwise, bad things will happen.

1. Clone the iNDS Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/WilliamLCobb/iNDS.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

3. Open "iNDS.xcodeproj" located within the cloned "iNDS" folder.

4. Connect your iOS device and let Xcode Organizer associate itself with your device.

5. In the Xcode IDE, select "iOS Device" (or "$YOUR_NAME's iPhone") and make sure you are using the Release configuration.

6. Click the "Run" button or press Command + R.

7. Please note that if you are using a jailbroken device, this installation of iNDS will be treated as a non-jailbroken installation. If you want to compile a jailbroken version of iNDS yourself, please see Option 2a below.

#### Option 2b: Compile and install iNDS using the Makefile (Jailbroken users only!)

1. Clone the iNDS Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/WilliamLCobb/iNDS.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

2. If you have not yet opened a Terminal instance, do so now.

3. Make sure `THEOS_DEVICE_IP` is defined. You can do so by running `export THEOS_DEVICE_IP=<your iOS device's IP address>`.

4. `cd` to the cloned "iNDS" folder, and run `make install`.

5. iNDS will compile and `scp` itself to your iOS device.

Reporting Bugs
------------------------
#### When something in iNDS isn't working correctly for you, please [open a GitHub issue ticket here](https://github.com/williamlcobb/iNDS/issues/new).
##### Please include the following information:
* iOS device
* iOS version
* Jailbreak status
* Download location
* Current iNDS Version

##### Please do not open issues about the following topics:
* Slow performance
* Crashing on older devices with 256MB of RAM (Anything prior to iPhone 4 devices.)

##### Your issue ticket will be closed if you fail to follow the above instructions.

To-do
------------------------
###### Planned improvments
* Cheat Codes
* GNU Lightning JIT (JAILBROKEN DEVICES ONLY)
* Sharing roms between devices
* Sharable game hacks
* Add more localizations
* Much more
