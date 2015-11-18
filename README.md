Nitrogen
=======
###### Supports iOS 5, 6, 7, and 8.

Nitrogen is a port of the multi-platform Nintendo DS emulator to iOS and Android (soon).

Currently, emulation is powered by a threaded ARM interpreter. As a result, emulation is rather slow on older devices, like the iPhone 4s and below.

Nitrogen runs at nearly full speed on the iPhone 5 and above, and achieves full speed on devices using the A7-S5L8960X SoC (iPhone 5s, iPad Air, iPad mini Retina, and anything newer than these devices).

Due to the need to mmap the entire ROM into memory, older devices with only 256MB of RAM are not supported by Nitrogen. These devices include the iPod touch 4, iPad 1, iPhone 3GS, and anything below those devices.

[Nitrogen](http://nitrogen.reimuhakurei.net/)

[DeSmuME](http://desmume.org/) 

Installing Nitrogen
------------------------
<!-- ##### Do not redistribute Nitrogen on other sites. We already provide official ways to download Nitrogen below. THIS IS A FAIR WARNING.
#### Option 1: Download Nitrogen from the Nitrogen site

If you're jailbroken, please follow the instructions here: http://nitrogen.reimuhakurei.net/i/?page/downloads#jailbroken

If you're NOT jailbroken, please follow the instructions here: http://nitrogen.reimuhakurei.net/i/?page/downloads#notjailbroken

#### Option 2: Compile and install Nitrogen from the Xcode IDE
-->
##### Do not redistribute self-compiled versions of Nitrogen on other sites. We will be adding an official method to download Nitrogen very soon. Any unofficial copies of Nitrogen must change the name and icon. THIS IS A FAIR WARNING.
#### Compile and install Nitrogen from the Xcode IDE (your only option for now)
##### IMPORTANT: Make sure your working directory is devoid of spaces. Otherwise, bad things will happen.

1. Clone the Nitrogen Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/NitrogenEmulator/Nitrogen.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

3. Open "Nitrogen.xcodeproj" located within the cloned "Nitrogen" folder.

4. Connect your iOS device and let Xcode Organizer associate itself with your device.

5. In the Xcode IDE, select "iOS Device" (or "$YOUR_NAME's iPhone") and make sure you are using the Release configuration.

6. Click the "Run" button or press Command + R.

7. Please note that if you are using a jailbroken device, this installation of Nitrogen will be treated as a non-jailbroken installation. If you want to compile a jailbroken version of Nitrogen yourself, please see Option 2a below.

#### Option 2a: Compile and install Nitrogen using the Makefile (Jailbroken users only!)

1. Clone the Nitrogen Git repository. You may do this from a Terminal instance with the command `git clone https://github.com/NitrogenEmulator/Nitrogen.git`, or from a Git frontend like [SourceTree](http://sourcetreeapp.com/).

2. If you have not yet opened a Terminal instance, do so now.

3. Make sure `THEOS_DEVICE_IP` is defined. You can do so by running `export THEOS_DEVICE_IP=<your iOS device's IP address>`.

4. `cd` to the cloned "Nitrogen" folder, and run `make install`.

5. Nitrogen will compile and `scp` itself to your iOS device.


Adding ROMs to Nitrogen
------------------------

#### Option 1 - On-device via Safari (All devices)
1. Open Safari.
2. Download a ROM image of a Nintendo DS game that you **legally own the actual game cartridge for.**
3. Tap the "Open in..." button in the top left hand corner, and select Nitrogen.
4. Nitrogen will automatically unzip the file, delete the readme, find the \*.nds file, and refresh itself. Your ROM should show up in the list.

#### Option 2 - On-device via browser with download capabilities (All devices)
1. Download one of the many web browsers available on the App Store with download managers built in, such as [Dolphin Browser](https://itunes.apple.com/us/app/dolphin-browser/id452204407?mt=8).
2. Download a ROM image of a Nintendo DS game that you **legally own the actual game cartridge for.**
3. From the app's list of downloaded files, tap on the downloaded file, and select Nitrogen in the "Open in..." menu.
4. Nitrogen will automatically unzip the file, delete the readme, find the \*.nds file, and refresh itself. Your ROM should show up in the list.

#### Option 3 - Using a computer via iFunBox Classic (Non-jailbroken devices ONLY)
1. Plug your device into your computer and launch [iFunBox Classic](http://dl.i-funbox.com/).
2. Go to [device name] -> iTunes File Sharing -> Nitrogen
3. Drag and drop \*.nds ROM images of Nintendo DS games that you **legally own the actual game cartridge for** into iFunBox.
4. Restart Nitrogen to see the changes.

#### Option 4 - On-device via Safari Downloader+/Chrome Downloader+ (Jailbroken devices)
1. Download a download manager tweak from Cydia like Safari Downloader+ or Chrome Downloader+.
2. Download a ROM image of a Nintendo DS game that you **legally own the actual game cartridge for.**
3. Using iFile or another filesystem explorer, move the downloaded .nds file to /User/Documents/. Alternatively, you can also tap on the downloaded file in the download manager's list of files, and select Nitrogen in the "Open in..." menu.
4. Nitrogen will automatically unzip the file, delete the readme, find the *.nds file, and refresh itself. Your ROM should show up in the list.

#### Option 5 - Via OpenSSH / iFunBox Classic (Jailbroken devices)
1. Install OpenSSH from Cydia if you plan to utilise SCP (SSH) to transfer ROMs.
2. If you do not wish to utilise SCP, then download [iFunBox Classic](http://dl.i-funbox.com/) and install it on your computer.
3. ROMs (\*.nds) go in the directory: /User/Documents/
4. Saves (in DeSmuME's \*.dsv format) go in: /User/Documents/Battery/

Reporting Bugs
------------------------
#### When something in Nitrogen isn't working correctly for you, please [open a GitHub issue ticket here](https://github.com/NitrogenEmulator/Nitrogen/issues/new).
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
###### We'll get to these, really!
* GNU Lightning JIT
* OpenGL ES rendering
* Automatically fix permissions of crucial folders on the jailbroken distribution
* Ability to change the folder the ROM chooser reads from
* ROM streaming
* ROM auto-trimming
* Add more localizations
* Much more
