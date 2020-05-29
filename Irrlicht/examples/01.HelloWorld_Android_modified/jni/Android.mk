LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)
LOCAL_MODULE := Irrlicht
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../source/Irrlicht/Android/obj/local/$(TARGET_ARCH_ABI)/libIrrlicht.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := HelloWorldMobile

LOCAL_CFLAGS := -D_IRR_ANDROID_PLATFORM_ -pipe -fno-exceptions -fno-rtti -fstrict-aliasing

ifndef NDEBUG
LOCAL_CFLAGS += -g -D_DEBUG
else
LOCAL_CFLAGS += -fexpensive-optimizations -O3
endif

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS += -fno-stack-protector
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include

LOCAL_SRC_FILES := main.cpp android_tools.cpp

LOCAL_LDLIBS := -lEGL -llog -lGLESv1_CM -lGLESv2 -lz -landroid

LOCAL_STATIC_LIBRARIES := Irrlicht android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

