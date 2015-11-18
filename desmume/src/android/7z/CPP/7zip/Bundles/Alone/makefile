LOCAL_FLAGS= \
  -DCOMPRESS_MT \
  -DCOMPRESS_BZIP2_MT \
  -DCOMPRESS_MF_MT \
  -DBREAK_HANDLER \
  -DBENCH_MT

include ../../../../makefile.crc32
include ../../../../makefile.machine

PROG=../../../../bin/7za$(BINSUFFIX)

LIBS=$(LOCAL_LIBS)

CONSOLE_OBJS = \
  ConsoleClose.o \
  ExtractCallbackConsole.o \
  List.o \
  Main.o \
  MainAr.o \
  OpenCallbackConsole.o \
  PercentPrinter.o \
  UpdateCallbackConsole.o \
  UserInputUtils.o

COMMON_OBJS = \
  CommandLineParser.o \
  CRC.o \
  IntToString.o \
  ListFileUtils.o \
  StdInStream.o \
  StdOutStream.o \
  MyString.o \
  StringConvert.o \
  StringToInt.o \
  UTFConvert.o \
  MyWindows.o \
  MyVector.o \
  Wildcard.o

# DLL.o MemoryLock.o 
WIN_OBJS = \
  Error.o \
  FileDir.o \
  FileFind.o \
  FileIO.o \
  FileName.o \
  PropVariant.o \
  PropVariantConversions.o \
  Synchronization.o \
  System.o \
  Time.o

7ZIP_COMMON_OBJS = \
  CreateCoder.o \
  FilePathAutoRename.o \
  FileStreams.o \
  FilterCoder.o \
  InBuffer.o \
  InOutTempBuffer.o \
  LimitedStreams.o \
  LockedStream.o \
  MemBlocks.o \
  MethodId.o \
  MethodProps.o \
  OffsetStream.o \
  OutBuffer.o \
  OutMemStream.o \
  ProgressMt.o \
  ProgressUtils.o \
  StreamBinder.o \
  StreamObjects.o \
  StreamUtils.o \
  VirtThread.o

# WorkDir.o
UI_COMMON_OBJS = \
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

AR_OBJS = \
  ZHandler.o \

AR_COMMON_OBJS = \
  CoderMixer2.o \
  CoderMixer2MT.o \
  CrossThreadProgress.o \
  DummyOutStream.o \
  FindSignature.o \
  HandlerOut.o \
  InStreamWithCRC.o \
  ItemNameUtils.o \
  MultiStream.o \
  OutStreamWithCRC.o \
  ParseProperties.o


7Z_OBJS = \
  7zCompressionMode.o \
  7zDecode.o \
  7zEncode.o \
  7zExtract.o \
  7zFolderInStream.o \
  7zFolderOutStream.o \
  7zHandler.o \
  7zHandlerOut.o \
  7zHeader.o \
  7zIn.o \
  7zOut.o \
  7zProperties.o \
  7zSpecStream.o \
  7zUpdate.o \
  7zRegister.o

BZ2_OBJS = \
  BZip2Handler.o \
  BZip2HandlerOut.o \
  BZip2Update.o \
  bz2Register.o \

CAB_OBJS = \
  CabBlockInStream.o \
  CabHandler.o \
  CabHeader.o \
  CabIn.o \
  CabRegister.o \

GZ_OBJS = \
  GZipHandler.o \
  GZipHandlerOut.o \
  GZipHeader.o \
  GZipIn.o \
  GZipOut.o \
  GZipUpdate.o \
  GZipRegister.o \

LZM_OBJS = \
  LzmaArcRegister.o \
  LzmaFiltersDecode.o \
  LzmaHandler.o \
  LzmaIn.o \

# SplitHandlerOut.o
SPLIT_OBJS = \
  SplitHandler.o \
  SplitRegister.o

TAR_OBJS = \
  TarHandler.o \
  TarHandlerOut.o \
  TarHeader.o \
  TarIn.o \
  TarOut.o \
  TarUpdate.o \
  TarRegister.o

ZIP_OBJS = \
  ZipAddCommon.o \
  ZipHandler.o \
  ZipHandlerOut.o \
  ZipHeader.o \
  ZipIn.o \
  ZipItem.o \
  ZipOut.o \
  ZipUpdate.o \
  ZipRegister.o

COMPRESS_OBJS = \
  Bcj2Coder.o \
  Bcj2Register.o \
  BcjCoder.o \
  BcjRegister.o \
  BitlDecoder.o \
  BranchCoder.o \
  BranchMisc.o \
  BranchRegister.o \
  ByteSwap.o \
  ByteSwapRegister.o \
  BZip2Crc.o \
  BZip2Decoder.o \
  BZip2Encoder.o \
  BZip2Register.o \
  CopyCoder.o \
  CopyRegister.o \
  Deflate64Register.o \
  DeflateDecoder.o \
  DeflateEncoder.o \
  DeflateRegister.o \
  ImplodeDecoder.o \
  ImplodeHuffmanDecoder.o \
  LzmaDecoder.o \
  LzmaEncoder.o \
  LzmaRegister.o \
  LzOutWindow.o \
  Lzx86Converter.o \
  LzxDecoder.o \
  PpmdDecoder.o \
  PpmdEncoder.o \
  PpmdRegister.o \
  QuantumDecoder.o \
  ShrinkDecoder.o \
  ZDecoder.o \

LZMA_BENCH_OBJS = \
  LzmaBench.o \
  LzmaBenchCon.o \

CRYPTO_OBJS = \
  7zAes.o \
  7zAesRegister.o \
  HmacSha1.o \
  MyAes.o \
  Pbkdf2HmacSha1.o \
  RandGen.o \
  Sha1.o \
  WzAes.o \
  ZipCrypto.o \
  ZipStrong.o \

# Alloc.o
C_OBJS = \
  Bra.o \
  Bra86.o \
  BraIA64.o \
  BwtSort.o \
  HuffEnc.o \
  LzFind.o \
  LzFindMt.o \
  LzmaDec.o \
  LzmaEnc.o \
  Sort.o \
  Threads.o \
  Aes.o \
  Sha256.o

OBJS=\
myGetTickCount.o \
wine_date_and_time.o \
myAddExeFlag.o \
mySplitCommandLine.o \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BZ2_OBJS) \
  $(CAB_OBJS) \
  $(GZ_OBJS) \
  $(LZM_OBJS) \
  $(SPLIT_OBJS) \
  $(TAR_OBJS) \
  $(ZIP_OBJS) \
  $(COMPRESS_OBJS) \
  $(LZMA_BENCH_OBJS) \
  $(CRYPTO_OBJS) \
  $(C_OBJS) \
  $(OBJ_CRC32)

include ../../../../makefile.glb

