// PpmdAlone.cpp

#include "StdAfx.h"

#include "../../../Common/MyWindows.h"
#include "../../../Common/MyInitGuid.h"

#include <stdio.h>

#if defined(_WIN32) || defined(OS2) || defined(MSDOS)
#include <fcntl.h>
#include <io.h>
#define MY_SET_BINARY_MODE(file) _setmode(_fileno(file), O_BINARY)
#else
#define MY_SET_BINARY_MODE(file)
#endif

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

#include "../../Common/FileStreams.h"
#include "../../Common/StreamUtils.h"

#include "../PpmdDecoder.h"
#include "../PpmdEncoder.h"

using namespace NCommandLineParser;

#ifdef _WIN32
bool g_IsNT = false;
static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

static const char *kCantAllocate = "Can not allocate memory";
static const char *kReadError = "Read error";
static const char *kWriteError = "Write error";

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kOrder,
  kUsedMemorySize,
  kStdIn,
  kStdOut
};
}

static const CSwitchForm kSwitchForms[] = 
{
  { L"?",  NSwitchType::kSimple, false },
  { L"H",  NSwitchType::kSimple, false },
  { L"O", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"M", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"SI",  NSwitchType::kSimple, false },
  { L"SO",  NSwitchType::kSimple, false }
};

static const int kNumSwitches = sizeof(kSwitchForms) / sizeof(kSwitchForms[0]);

static void PrintHelp()
{
  fprintf(stderr, "\nUsage:  PPMD <e|d> inputFile outputFile [<switches>...]\n"
             "  e: encode file\n"
             "  d: decode file\n"
/*
             "  b: Benchmark\n"
*/
    "<Switches>\n"
    "  -o{N}:  set order - [4, 32], default: 4\n"
    "  -m{N}:  set memory size - [4,512], default: 4 (4 MB)\n"
    "  -si:    read data from stdin (only with d)\n"
    "  -so:    write data to stdout\n"
    );
}

static void PrintHelpAndExit(const char *s)
{
  fprintf(stderr, "\nError: %s\n\n", s);
  PrintHelp();
  throw -1;
}

static void IncorrectCommand()
{
  PrintHelpAndExit("Incorrect command");
}

static void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    UStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
    strings.Add(MultiByteToUnicodeString(arguments[i]));
}

static bool GetNumber(const wchar_t *s, UInt32 &value)
{
  value = 0;
  if (MyStringLen(s) == 0)
    return false;
  const wchar_t *end;
  UInt64 res = ConvertStringToUInt64(s, &end);
  if (*end != L'\0')
    return false;
  if (res > 0xFFFFFFFF)
    return false;
  value = UInt32(res);
  return true;
}

int main2(int n, const char *args[])
{
  #ifdef _WIN32
  g_IsNT = IsItWindowsNT();
  #endif

  fprintf(stderr, "\nPPMD 4.49 Copyright (c) 1999-2007 Igor Pavlov  2007-07-05\n");

  if (n == 1)
  {
    PrintHelp();
    return 0;
  }

  bool unsupportedTypes = (sizeof(Byte) != 1 || sizeof(UInt32) < 4 || sizeof(UInt64) < 4);
  if (unsupportedTypes)
  {
    fprintf(stderr, "Unsupported base types. Edit Common/Types.h and recompile");
    return 1;
  }   

  UStringVector commandStrings;
  WriteArgumentsToStringList(n, args, commandStrings);
  CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...) 
  {
    IncorrectCommand();
  }

  if(parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  int paramIndex = 0;
  if (paramIndex >= nonSwitchStrings.Size())
    IncorrectCommand();
  const UString &command = nonSwitchStrings[paramIndex++]; 

/* FIXME
  if (command.CompareNoCase(L"b") == 0)
  {
    const UInt32 kNumDefaultItereations = 1;
    UInt32 numIterations = kNumDefaultItereations;
    {
      if (paramIndex < nonSwitchStrings.Size())
        if (!GetNumber(nonSwitchStrings[paramIndex++], numIterations))
          numIterations = kNumDefaultItereations;
    }
    return LzmaBenchCon(stderr, numIterations, numThreads, dictionary);
  }
*/

  bool encodeMode = false;
  if (command.CompareNoCase(L"e") == 0)
    encodeMode = true;
  else if (command.CompareNoCase(L"d") == 0)
    encodeMode = false;
  else
    IncorrectCommand();

  bool stdInMode = parser[NKey::kStdIn].ThereIs;
  bool stdOutMode = parser[NKey::kStdOut].ThereIs;

  CMyComPtr<ISequentialInStream> inStream;
  CInFileStream *inStreamSpec = 0;
  if (stdInMode)
  {
    inStream = new CStdInFileStream;
    MY_SET_BINARY_MODE(stdin);
  }
  else
  {
    if (paramIndex >= nonSwitchStrings.Size())
      IncorrectCommand();
    const UString &inputName = nonSwitchStrings[paramIndex++]; 
    inStreamSpec = new CInFileStream;
    inStream = inStreamSpec;
    if (!inStreamSpec->Open(GetSystemString(inputName)))
    {
      fprintf(stderr, "\nError: can not open input file %s\n", 
          (const char *)GetOemString(inputName));
      return 1;
    }
  }

  CMyComPtr<ISequentialOutStream> outStream;
  if (stdOutMode)
  {
    outStream = new CStdOutFileStream;
    MY_SET_BINARY_MODE(stdout);
  }
  else
  {
    if (paramIndex >= nonSwitchStrings.Size())
      IncorrectCommand();
    const UString &outputName = nonSwitchStrings[paramIndex++]; 
    COutFileStream *outStreamSpec = new COutFileStream;
    outStream = outStreamSpec;
    if (!outStreamSpec->Create(GetSystemString(outputName), true))
    {
      fprintf(stderr, "\nError: can not open output file %s\n", 
        (const char *)GetOemString(outputName));
      return 1;
    }
  }

  UInt64 fileSize;
  if (encodeMode)
  {
    // NCompress::NLZMA::CEncoder *encoderSpec = new NCompress::NLZMA::CEncoder;
    NCompress::NPpmd::CEncoder *encoderSpec = new NCompress::NPpmd::CEncoder;
    CMyComPtr<ICompressCoder> encoder = encoderSpec;

    if (stdInMode)
      IncorrectCommand();

    UInt32 order = 4;
    if(parser[NKey::kOrder].ThereIs)
      if (!GetNumber(parser[NKey::kOrder].PostStrings[0], order))
        IncorrectCommand();
   if (order < 4) order = 4;
    
    UInt32 memSize = 4;
    if(parser[NKey::kUsedMemorySize].ThereIs)
      if (!GetNumber(parser[NKey::kUsedMemorySize].PostStrings[0], memSize))
        IncorrectCommand();
   if (memSize < 4 ) memSize = 4; 
    
 
    PROPID propIDs[] = 

    {
	NCoderPropID::kUsedMemorySize,
	NCoderPropID::kOrder
    };
    const int kNumPropsMax = sizeof(propIDs) / sizeof(propIDs[0]);

    PROPVARIANT properties[kNumPropsMax];
    for (int p = 0; p < kNumPropsMax; p++)
      properties[p].vt = VT_UI4;

    properties[0].ulVal = memSize * 1024 * 1024; // memory
    properties[1].ulVal = order;

    int numProps = kNumPropsMax;

    if (encoderSpec->SetCoderProperties(propIDs, properties, numProps) != S_OK)
      IncorrectCommand();
    encoderSpec->WriteCoderProperties(outStream);

/* 
    if (eos || stdInMode)
      fileSize = (UInt64)(Int64)-1;
    else
*/
      inStreamSpec->File.GetLength(fileSize);

    for (int i = 0; i < 8; i++)
    {
      Byte b = Byte(fileSize >> (8 * i));
      if (outStream->Write(&b, 1, 0) != S_OK)
      {
        fprintf(stderr, kWriteError);
        return 1;
      }
    }
    HRESULT result = encoder->Code(inStream, outStream, 0, 0, 0);
    if (result == E_OUTOFMEMORY)
    {
      fprintf(stderr, "\nError: Can not allocate memory\n");
      return 1;
    }   
    else if (result != S_OK)
    {
      fprintf(stderr, "\nEncoder error = %X\n", (unsigned int)result);
      return 1;
    }   
  }
  else
  {
    // NCompress::NLZMA::CDecoder *decoderSpec = new NCompress::NLZMA::CDecoder;
    NCompress::NPpmd::CDecoder *decoderSpec = new NCompress::NPpmd::CDecoder;
    CMyComPtr<ICompressCoder> decoder = decoderSpec;
    const UInt32 kPropertiesSize = 5;
    Byte header[kPropertiesSize + 8];
    if (ReadStream_FALSE(inStream, header, kPropertiesSize + 8) != S_OK)
    {
      fprintf(stderr, kReadError);
      return 1;
    }
    if (decoderSpec->SetDecoderProperties2(header, kPropertiesSize) != S_OK)
    {
      fprintf(stderr, "SetDecoderProperties error");
      return 1;
    }
    fileSize = 0;
    for (int i = 0; i < 8; i++)
      fileSize |= ((UInt64)header[kPropertiesSize + i]) << (8 * i);

    if (decoder->Code(inStream, outStream, 0, &fileSize, 0) != S_OK)
    {
      fprintf(stderr, "Decoder error");
      return 1;
    }   
  }
  return 0;
}

int main(int n, const char *args[])
{
  try { return main2(n, args); }
  catch(const char *s) 
  { 
    fprintf(stderr, "\nError: %s\n", s);
    return 1; 
  }
  catch(...) 
  { 
    fprintf(stderr, "\nError\n");
    return 1; 
  }
}
