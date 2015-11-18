# Android ndk for math-neon
# http://code.google.com/p/math-neon/

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE 		:= 	libmathneon
LOCAL_SRC_FILES		:=  math_acosf.c \
						math_ceilf.c \
						math_expf.c \
						math_frexpf.c \
						math_logf.c \
						math_modf.c \
						math_sinf.c \
						math_sqrtfv.c \
						math_vec3.c \
						math_asinf.c \
						math_cosf.c \
						math_fabsf.c \
						math_invsqrtf.c \
						math_mat2.c \
						math_powf.c \
						math_sinfv.c \
						math_tanf.c \
						math_vec4.c \
						math_atan2f.c \
						math_coshf.c \
						math_floorf.c \
						math_ldexpf.c \
						math_mat3.c \
						math_runfast.c \
						math_sinhf.c \
						math_tanhf.c \
						math_atanf.c \
						math_debug.c \
						math_fmodf.c \
						math_log10f.c \
						math_mat4.c \
						math_sincosf.c \
						math_sqrtf.c \
						math_vec2.c
LOCAL_ARM_NEON 			:= true
LOCAL_ARM_MODE 			:= arm
LOCAL_CFLAGS			:= -std=gnu99 -DHAVE_NEON=1 -march=armv7-a -marm -mfloat-abi=softfp -mfpu=neon
include $(BUILD_STATIC_LIBRARY)