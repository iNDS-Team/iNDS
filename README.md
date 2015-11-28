iNDS
=======
###### Supports iOS 7 - 9.1.

iNDS is a port of the multi-platform Nintendo DS emulator to iOS.

Currently, emulation is powered by the DeSmuME threaded ARM interpreter and runs at nearly full speed on the iPhone 5 and above.

Due to the need to mmap the entire ROM into memory, older devices with only 256MB of RAM are not supported. These devices include the iPod touch 4, iPad 1, iPhone 3GS, and anything below those devices.

[iNDS](http://www.williamlcobb.com/inds)

[DeSmuME](http://desmume.org/) 

Installing iNDS
------------------------
<!-- ##### Please Do not redistribute iNDS on other sites. We already provide official ways to download iNDS below.
#### Option 1: Cydia

If you're jailbroken, please add the following source: http://www.williamlcobb.com/repo

Download iNDS from the repository.

#### Option 2: Compile and install iNDS from the Xcode IDE
-->
##### Please Do not redistribute self-compiled versions of iNDS on other sites. 
#### Compile and install iNDS from the Xcode IDE (your only option for now)
##### IMPORTANT: Make sure your working directory is devoid of spaces. Otherwise, bad things will happen.

1. Clone the iNDS Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/iNDSEmulator/iNDS.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

3. Open "iNDS.xcodeproj" located within the cloned "iNDS" folder.

4. Connect your iOS device and let Xcode Organizer associate itself with your device.

5. In the Xcode IDE, select "iOS Device" (or "$YOUR_NAME's iPhone") and make sure you are using the Release configuration.

6. Click the "Run" button or press Command + R.

7. Please note that if you are using a jailbroken device, this installation of iNDS will be treated as a non-jailbroken installation. If you want to compile a jailbroken version of iNDS yourself, please see Option 2a below.

#### Option 2a: Compile and install iNDS using the Makefile (Jailbroken users only!)

1. Clone the iNDS Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/williamlcobb/iNDS.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

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

##### Please do not open issues about the following topics:
* Slow performance
* Crashing on older devices with 256MB of RAM (iPod touch 4, iPhone 3GS, iPad 1, and anything released prior to those devices.)

##### Your issue ticket will be closed if you fail to follow the above instructions.

To-do
------------------------
###### Planned improvments
* GNU Lightning JIT
* OpenGL ES rendering
* Sharing roms between devices
* Sharable game hacks
* Add more localizations
* Much more
