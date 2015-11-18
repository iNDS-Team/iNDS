include ../../../../makefile.machine

PROG = ppmd.exe

LIB = $(LOCAL_LIBS)
RM = rm -f
CFLAGS = -c -I. -I../../../  -I../../../myWindows  -I../../../include_windows -UENV_UNIX
CXXFLAGS=$(CFLAGS)

OBJS = \
  PpmdAlone.o \
  PpmdDecoder.o \
  PpmdEncoder.o \
  CommandLineParser.o \
  MyString.o \
  StringConvert.o \
  MyVector.o \
  InBuffer.o \
  OutBuffer.o \
  FileStreams.o \
  C_FileIO.o \
  StreamUtils.o \
  StringToInt.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CXX) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIB)

PpmdAlone.o: PpmdAlone.cpp
	$(CXX) $(CXXFLAGS) PpmdAlone.cpp

PpmdDecoder.o: ../PpmdDecoder.cpp
	$(CXX) $(CXXFLAGS) ../PpmdDecoder.cpp

PpmdEncoder.o: ../PpmdEncoder.cpp
	$(CXX) $(CXXFLAGS) ../PpmdEncoder.cpp

InBuffer.o: ../../Common/InBuffer.cpp
	$(CXX) $(CXXFLAGS) ../../Common/InBuffer.cpp

OutBuffer.o: ../../Common/OutBuffer.cpp
	$(CXX) $(CXXFLAGS) ../../Common/OutBuffer.cpp

FileStreams.o: ../../Common/FileStreams.cpp
	$(CXX) $(CXXFLAGS) ../../Common/FileStreams.cpp

StreamUtils.o: ../../Common/StreamUtils.cpp
	$(CXX) $(CXXFLAGS) ../../Common/StreamUtils.cpp

C_FileIO.o: ../../../Common/C_FileIO.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/C_FileIO.cpp

CommandLineParser.o: ../../../Common/CommandLineParser.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/CommandLineParser.cpp

MyWindows.o: ../../../Common/MyWindows.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/MyWindows.cpp

MyString.o: ../../../Common/MyString.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/MyString.cpp

StringConvert.o: ../../../Common/StringConvert.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/StringConvert.cpp

StringToInt.o: ../../../Common/StringToInt.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/StringToInt.cpp

MyVector.o: ../../../Common/MyVector.cpp
	$(CXX) $(CXXFLAGS) ../../../Common/MyVector.cpp

clean:
	-$(RM) $(PROG) $(OBJS) testfile* *.exe ir.out
	-$(RM) -fr SunWS_cache


test: $(PROG)
	cp PpmdAlone.cpp testfile
	./$(PROG) e testfile testfile.ppmd
	./$(PROG) d testfile.ppmd testfile.ppmd.d 
	diff -s testfile testfile.ppmd.d
	./$(PROG) e -so testfile > testfile.ppmd.2
	./$(PROG) d -si -so < testfile.ppmd.2 > testfile.ppmd.2.d
	diff -s testfile testfile.ppmd.2.d
	./$(PROG) e -o32 -m256 testfile testfile.ppmd.3
	./$(PROG) d testfile.ppmd.3 testfile.ppmd.3.d 
	diff -s testfile testfile.ppmd.3.d
	@echo =========
	@echo All Done 
	@echo =========


