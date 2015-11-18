# Android makefile for tinycc

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE 		:= libtinyccx86
LOCAL_ARM_MODE 		:= arm
LOCAL_CFLAGS		:= -DANDROID -DONE_SOURCE -DTCC_TARGET_I386

LOCAL_SRC_FILES		:= libtcc.c

include $(BUILD_STATIC_LIBRARY)