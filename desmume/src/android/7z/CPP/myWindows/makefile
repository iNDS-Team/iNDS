PROG=test_emul

LOCAL_FLAGS=-I. -I.. -I../include_windows

include ../../makefile.crc32
include ../../makefile.machine

LIBS=$(LOCAL_LIBS)

OBJS=myGetTickCount.o \
	wine_date_and_time.o \
	mySplitCommandLine.o \
	myAddExeFlag.o \
	test_emul.o \
	MyVector.o \
	MyString.o \
	MyWindows.o

include ../../makefile.glb

