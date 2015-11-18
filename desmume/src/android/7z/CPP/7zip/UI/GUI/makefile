PROG=../../../../bin/7zG

LOCAL_FLAGS=\
	-DUNICODE \
	-D_UNICODE \
	-DLANG \
	-DCOMPRESS_MT \
	-DEXTERNAL_LZMA \
	-DEXTERNAL_CODECS \
	-DBENCH_MT \
	-I. \
	`wx-config --unicode=yes --cxxflags`

include ../../../../makefile.crc32
include ../../../../makefile.machine

LIBS=`wx-config --unicode=yes --libs` $(LOCAL_LIBS_DLL)

OBJS=\
CompressDialog.o \
CompressDialog_rc.o \
ExtractDialog.o \
ExtractDialog_rc.o \
BenchmarkDialog.o \
BenchmarkDialog_rc.o \
ExtractGUI.o \
GUI.o \
wxGUI.o \
UpdateCallbackGUI.o \
UpdateGUI.o \
ArchiveCommandLine.o \
ArchiveExtractCallback.o \
ArchiveOpenCallback.o \
DefaultName.o \
EnumDirItems.o \
Extract.o \
ExtractingFilePath.o \
LoadCodecs.o \
OpenArchive.o \
PropIDUtils.o \
SetProperties.o \
SortUtils.o \
TempFiles.o \
Update.o \
UpdateAction.o \
UpdateCallback.o \
UpdatePair.o \
UpdateProduce.o \
WorkDir.o \
MyMessages.o \
ZipRegistry.o \
FormatUtils.o \
ProgramLocation.o \
ProgressDialog2.o \
ProgressDialog2_rc.o \
MessagesDialog.o \
MessagesDialog_rc.o \
OverwriteDialog.o \
OverwriteDialog_rc.o \
PasswordDialog.o \
PasswordDialog_rc.o \
ExtractCallback.o \
SplitUtils.o \
StringUtils.o \
LangUtils.o \
FilePathAutoRename.o \
FileStreams.o \
ProgressUtils.o \
StreamUtils.o \
CopyCoder.o \
LzmaBench.o \
CommandLineParser.o \
CRC.o \
IntToString.o \
Lang.o \
ListFileUtils.o \
MyString.o \
MyVector.o \
StringConvert.o \
StringToInt.o \
TextConfig.o \
UTFConvert.o \
Wildcard.o \
DLL.o \
Error.o \
FileDir.o \
FileFind.o \
FileIO.o \
FileName.o \
PropVariant.o \
PropVariantConversions.o \
Registry.o \
Synchronization.o \
Time.o \
System.o \
Window.o \
Dialog.o \
Controls.o \
MyWindows.o \
wine_date_and_time.o \
wine_GetXXXDefaultLangID.o \
Threads.o \
$(OBJ_CRC32)

include ../../../../makefile.glb

