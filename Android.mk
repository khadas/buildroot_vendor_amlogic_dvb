LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libam_adp
LOCAL_MODULE_TAGS := optional 
LOCAL_PREBUILT_LIBS := libam_adp.so 
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := libam_mw
LOCAL_MODULE_TAGS := optional 
LOCAL_PREBUILT_LIBS := libam_mw.so 
include $(BUILD_MULTI_PREBUILT)
