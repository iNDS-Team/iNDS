# Makefile for exophase ARM on ARM JIT

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE 		:= libexophasejit
LOCAL_ARM_MODE 		:= arm
LOCAL_CFLAGS		:= -DHAVE_JIT -DUSE_EXOPHASEJIT
LOCAL_C_INCLUDES	:= 	$(LOCAL_PATH)/../ \
						$(LOCAL_PATH)/../../
LOCAL_SRC_FILES		:= 	arm_stub.S\
						cpu_threaded.cpp\
						cpu.cpp\
						dynarec_linker.cpp\
						warm.cpp\
						exophasejit.cpp
						
LOCAL_LDLIBS 		:= -llog

include $(BUILD_STATIC_LIBRARY)

