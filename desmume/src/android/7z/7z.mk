# Android ndk makefile for p7zip https://sourceforge.net/projects/p7zip/

LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)


LOCAL_MODULE    		:= 	libsevenzip
LOCAL_C_INCLUDES		:= 	$(LOCAL_PATH)/CPP \
							$(LOCAL_PATH)/CPP/include_windows \
							$(LOCAL_PATH)/CPP/myWindows
LOCAL_SRC_FILES			:= 	C/Threads.c \
							C/LzmaDec.c \
							C/LzmaEnc.c \
							C/LzFind.c \
							C/7zCrc.c \
							C/Aes.c \
							C/HuffEnc.c \
							C/Sort.c \
							C/BwtSort.c \
							CPP/7zip/Archive/ArchiveExports.cpp \
							CPP/7zip/Archive/DllExports2.cpp \
							CPP/7zip/Archive/Common/ItemNameUtils.cpp \
							CPP/7zip/Archive/Common/OutStreamWithCRC.cpp \
							CPP/7zip/Archive/Common/InStreamWithCRC.cpp \
							CPP/7zip/Archive/Common/ParseProperties.cpp \
							CPP/7zip/Archive/Common/HandlerOut.cpp \
							CPP/7zip/Archive/Common/CoderMixer2MT.cpp \
							CPP/7zip/Archive/Common/CoderMixer2.cpp \
							CPP/7zip/Archive/Common/DummyOutStream.cpp \
							CPP/7zip/Archive/Common/FindSignature.cpp \
							CPP/7zip/Archive/7z/7zCompressionMode.cpp \
							CPP/7zip/Archive/7z/7zDecode.cpp \
							CPP/7zip/Archive/7z/7zEncode.cpp \
							CPP/7zip/Archive/7z/7zExtract.cpp \
							CPP/7zip/Archive/7z/7zFolderInStream.cpp \
							CPP/7zip/Archive/7z/7zFolderOutStream.cpp \
							CPP/7zip/Archive/7z/7zHandler.cpp \
							CPP/7zip/Archive/7z/7zHandlerOut.cpp \
							CPP/7zip/Archive/7z/7zHeader.cpp \
							CPP/7zip/Archive/7z/7zIn.cpp \
							CPP/7zip/Archive/7z/7zOut.cpp \
							CPP/7zip/Archive/7z/7zProperties.cpp \
							CPP/7zip/Archive/7z/7zRegister.cpp \
							CPP/7zip/Archive/7z/7zSpecStream.cpp \
							CPP/7zip/Archive/7z/7zUpdate.cpp \
							CPP/7zip/Archive/Rar/RarHandler.cpp \
							CPP/7zip/Archive/Rar/RarHeader.cpp \
							CPP/7zip/Archive/Rar/RarIn.cpp \
							CPP/7zip/Archive/Rar/RarItem.cpp \
							CPP/7zip/Archive/Rar/RarRegister.cpp \
							CPP/7zip/Archive/Rar/RarVolumeInStream.cpp \
							CPP/7zip/Archive/Zip/ZipAddCommon.cpp \
							CPP/7zip/Archive/Zip/ZipHandler.cpp \
							CPP/7zip/Archive/Zip/ZipHandlerOut.cpp \
							CPP/7zip/Archive/Zip/ZipHeader.cpp \
							CPP/7zip/Archive/Zip/ZipIn.cpp \
							CPP/7zip/Archive/Zip/ZipItem.cpp \
							CPP/7zip/Archive/Zip/ZipOut.cpp \
							CPP/7zip/Archive/Zip/ZipRegister.cpp \
							CPP/7zip/Archive/Zip/ZipUpdate.cpp \
							CPP/7zip/Archive/BZip2/bz2Register.cpp \
							CPP/7zip/Archive/BZip2/BZip2Handler.cpp \
							CPP/7zip/Archive/BZip2/BZip2HandlerOut.cpp \
							CPP/7zip/Archive/BZip2/BZip2Update.cpp \
							CPP/Common/MyWindows.cpp \
							CPP/Common/MyString.cpp \
							CPP/Common/StringConvert.cpp \
							CPP/Common/MyVector.cpp \
							CPP/Common/UTFConvert.cpp \
							CPP/Common/IntToString.cpp \
							CPP/Common/StringToInt.cpp \
							CPP/Windows/PropVariant.cpp \
							CPP/Windows/PropVariantConversions.cpp \
							CPP/Windows/System.cpp \
							CPP/Windows/Time.cpp \
							CPP/Windows/Synchronization.cpp \
							CPP/Windows/FileIO.cpp \
							CPP/Windows/FileFind.cpp \
							CPP/Windows/FileDir.cpp \
							CPP/myWindows/wine_date_and_time.cpp \
							CPP/myWindows/myGetTickCount.cpp \
							CPP/7zip/Common/CreateCoder.cpp \
							CPP/7zip/Common/FilterCoder.cpp \
							CPP/7zip/Common/StreamUtils.cpp \
							CPP/7zip/Common/ProgressUtils.cpp \
							CPP/7zip/Common/LimitedStreams.cpp \
							CPP/7zip/Common/MemBlocks.cpp \
							CPP/7zip/Common/OutMemStream.cpp \
							CPP/7zip/Common/ProgressMt.cpp \
							CPP/7zip/Common/OutBuffer.cpp \
							CPP/7zip/Common/InBuffer.cpp \
							CPP/7zip/Common/StreamObjects.cpp \
							CPP/7zip/Common/LockedStream.cpp \
							CPP/7zip/Common/VirtThread.cpp \
							CPP/7zip/Common/InOutTempBuffer.cpp \
							CPP/7zip/Common/MethodProps.cpp \
							CPP/7zip/Common/OffsetStream.cpp \
							CPP/7zip/Common/MethodId.cpp \
							CPP/7zip/Common/StreamBinder.cpp \
							CPP/7zip/Compress/CodecExports.cpp \
							CPP/7zip/Compress/CopyCoder.cpp \
							CPP/7zip/Compress/LzmaDecoder.cpp \
							CPP/7zip/Compress/LzmaEncoder.cpp \
							CPP/7zip/Compress/ImplodeDecoder.cpp \
							CPP/7zip/Compress/ShrinkDecoder.cpp \
							CPP/7zip/Compress/ImplodeHuffmanDecoder.cpp \
							CPP/7zip/Compress/LzOutWindow.cpp \
							CPP/7zip/Compress/BitlDecoder.cpp \
							CPP/7zip/Compress/DeflateDecoder.cpp \
							CPP/7zip/Compress/DeflateEncoder.cpp \
							CPP/7zip/Compress/DeflateRegister.cpp \
							CPP/7zip/Compress/LzmaRegister.cpp \
							CPP/7zip/Compress/RarCodecsRegister.cpp \
							CPP/7zip/Compress/Rar1Decoder.cpp \
							CPP/7zip/Compress/Rar2Decoder.cpp \
							CPP/7zip/Compress/Rar3Decoder.cpp \
							CPP/7zip/Compress/Rar3Vm.cpp \
							CPP/7zip/Compress/BZip2Crc.cpp \
							CPP/7zip/Compress/BZip2Decoder.cpp \
							CPP/7zip/Compress/BZip2Encoder.cpp \
							CPP/7zip/Compress/BZip2Register.cpp \
							CPP/7zip/Crypto/ZipStrong.cpp \
							CPP/7zip/Crypto/ZipCrypto.cpp \
							CPP/7zip/Crypto/Sha1.cpp \
							CPP/7zip/Crypto/RandGen.cpp \
							CPP/7zip/Crypto/MyAes.cpp \
							CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp \
							CPP/7zip/Crypto/HmacSha1.cpp \
							CPP/7zip/Crypto/RarAes.cpp \
							CPP/7zip/Crypto/Rar20Crypto.cpp \
							CPP/7zip/Crypto/WzAes.cpp

LOCAL_ARM_MODE 			:= 	thumb
LOCAL_ARM_NEON 			:= 	false
LOCAL_CFLAGS			:= -DCOMPRESS_MT

include $(BUILD_STATIC_LIBRARY)