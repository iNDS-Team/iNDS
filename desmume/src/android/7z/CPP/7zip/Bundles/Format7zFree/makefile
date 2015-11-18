PROG=../../../../bin/7z.so

LOCAL_FLAGS=\
  $(CC_SHARED) \
  -DEXTERNAL_CODECS \
  -DCOMPRESS_MT \
  -DCOMPRESS_BZIP2_MT \
  -DCOMPRESS_MF_MT

include ../../../../makefile.crc32
include ../../../../makefile.machine

LOCAL_SHARED=$(LINK_SHARED)
LIBS=$(LOCAL_LIBS_DLL)

# NewHandler.o
COMMON_OBJS = \
  CRC.o \
  IntToString.o \
  MyMap.o \
  MyString.o \
  MyWindows.o \
  MyXml.o \
  StringConvert.o \
  StringToInt.o \
  MyVector.o \
  UTFConvert.o \
  Wildcard.o \

WIN_OBJS = \
  FileDir.o \
  FileFind.o \
  FileIO.o \
  PropVariant.o \
  PropVariantUtils.o \
  Synchronization.o \
  System.o \
  Time.o \

7ZIP_COMMON_OBJS = \
  InBuffer.o \
  InOutTempBuffer.o \
  CreateCoder.o \
  FilterCoder.o \
  LimitedStreams.o \
  LockedStream.o \
  MethodId.o \
  MethodProps.o \
  MemBlocks.o \
  OffsetStream.o \
  OutBuffer.o \
  OutMemStream.o \
  ProgressMt.o \
  ProgressUtils.o \
  StreamBinder.o \
  StreamObjects.o \
  StreamUtils.o \
  VirtThread.o \

AR_OBJS = \
  ArchiveExports.o \
  DllExports2.o \
  ArjHandler.o \
  DebHandler.o \
  DmgHandler.o \
  ElfHandler.o \
  MachoHandler.o \
  MubHandler.o \
  PeHandler.o \
  RpmHandler.o \
  XarHandler.o \
  ZHandler.o \

AR_COMMON_OBJS = \
  CoderMixer2.o \
  CoderMixer2MT.o \
  CrossThreadProgress.o \
  DummyOutStream.o \
  FindSignature.o \
  InStreamWithCRC.o \
  ItemNameUtils.o \
  MultiStream.o \
  OutStreamWithCRC.o \
  OutStreamWithSha1.o \
  HandlerOut.o \
  ParseProperties.o \


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
  7zRegister.o \

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

CHM_OBJS = \
  ChmHandler.o \
  ChmHeader.o \
  ChmIn.o \
  ChmRegister.o \

COM_OBJS = \
  ComHandler.o \
  ComIn.o \
  ComRegister.o \

CPIO_OBJS = \
  CpioHandler.o \
  CpioHeader.o \
  CpioIn.o \
  CpioRegister.o \

GZ_OBJS = \
  GZipHandler.o \
  GZipHandlerOut.o \
  GZipHeader.o \
  GZipIn.o \
  GZipOut.o \
  GZipUpdate.o \
  GZipRegister.o \

HFS_OBJS = \
  HfsHandler.o \
  HfsIn.o \
  HfsRegister.o \

ISO_OBJS = \
  IsoHandler.o \
  IsoHeader.o \
  IsoIn.o \
  IsoRegister.o \

LZH_OBJS = \
  LzhCRC.o \
  LzhHandler.o \
  LzhIn.o \
  LzhOutStreamWithCRC.o \
  LzhRegister.o \

LZM_OBJS = \
  LzmaArcRegister.o \
  LzmaFiltersDecode.o \
  LzmaHandler.o \
  LzmaIn.o \

NSIS_OBJS = \
  NsisDecode.o \
  NsisHandler.o \
  NsisIn.o \
  NsisRegister.o \

RAR_OBJS = \
  RarHandler.o \
  RarHeader.o \
  RarIn.o \
  RarItem.o \
  RarVolumeInStream.o \
  RarRegister.o \

# SplitHandlerOut.o
SPLIT_OBJS = \
  SplitHandler.o \
  SplitRegister.o \

TAR_OBJS = \
  TarHandler.o \
  TarHandlerOut.o \
  TarHeader.o \
  TarIn.o \
  TarOut.o \
  TarUpdate.o \
  TarRegister.o \

UDF_OBJS = \
  UdfHandler.o \
  UdfIn.o \
  UdfRegister.o \

WIM_OBJS = \
  WimHandler.o \
  WimIn.o \
  WimRegister.o \

ZIP_OBJS = \
  ZipAddCommon.o \
  ZipHandler.o \
  ZipHandlerOut.o \
  ZipHeader.o \
  ZipIn.o \
  ZipItem.o \
  ZipOut.o \
  ZipUpdate.o \
  ZipRegister.o \

COMPRESS_OBJS = \
  CodecExports.o \
  ArjDecoder1.o \
  ArjDecoder2.o \
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
  DeflateNsisRegister.o \
  DeflateRegister.o \
  ImplodeDecoder.o \
  ImplodeHuffmanDecoder.o \
  LzhDecoder.o \
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
  ZlibDecoder.o \
  ZDecoder.o \

COMPRESS_OBJS_NON_FREE = \
  Rar1Decoder.o \
  Rar2Decoder.o \
  Rar3Decoder.o \
  Rar3Vm.o \
  RarCodecsRegister.o \

CRYPTO_OBJS = \
  7zAes.o \
  7zAesRegister.o \
  HmacSha1.o \
  MyAes.o \
  Pbkdf2HmacSha1.o \
  RandGen.o \
  Rar20Crypto.o \
  RarAes.o \
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
  Sha256.o \

OBJS = \
  wine_date_and_time.o \
  myGetTickCount.o \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BZ2_OBJS) \
  $(CAB_OBJS) \
  $(CHM_OBJS) \
  $(COM_OBJS) \
  $(CPIO_OBJS) \
  $(GZ_OBJS) \
  $(HFS_OBJS) \
  $(ISO_OBJS) \
  $(LZH_OBJS) \
  $(LZM_OBJS) \
  $(NSIS_OBJS) \
  $(RAR_OBJS) \
  $(SPLIT_OBJS) \
  $(TAR_OBJS) \
  $(UDF_OBJS) \
  $(WIM_OBJS) \
  $(ZIP_OBJS) \
  $(COMPRESS_OBJS) \
  $(CRYPTO_OBJS) \
  $(C_OBJS) \
  $(OBJ_CRC32)  \

include ../../../../makefile.glb

