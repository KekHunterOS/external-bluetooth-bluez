LOCAL_PATH := external/bluetooth

# Retrieve BlueZ version from configure.ac file
BLUEZ_VERSION := `grep "^AC_INIT" $(LOCAL_PATH)/bluez/configure.ac | sed -e "s/.*,.\(.*\))/\1/"`

ANDROID_VERSION := $(shell echo $(PLATFORM_VERSION) | awk -F. '{ printf "0x%02d%02d%02d",$$1,$$2,$$3 }')

CM_ANDROID_VERSION := $(shell echo $(CM_VERSION) | awk -F. '{ printf "0x%02d%02d00",$$1,$$2 }')

ANDROID_GE_5_0_0 := $(shell test `echo $$(($(ANDROID_VERSION)))` -lt `echo $$((0x050000))`; echo $$?)

# Specify pathmap for glib and sbc
pathmap_INCL += glib:external/bluetooth/glib \
		sbc:external/bluetooth/sbc \

# Specify common compiler flags
BLUEZ_COMMON_CFLAGS := -DVERSION=\"$(BLUEZ_VERSION)\" \
			-DANDROID_VERSION=$(ANDROID_VERSION) \
			-DCM_ANDROID_VERSION=$(CM_ANDROID_VERSION) \
			-DANDROID_STORAGEDIR=\"/data/misc/bluetooth\" \
			-DHAVE_LINUX_IF_ALG_H \
			-DHAVE_LINUX_TYPES_H \

# Enable warnings enabled in autotools build
BLUEZ_COMMON_CFLAGS += -Wall -Wextra \
			-Wdeclaration-after-statement \
			-Wmissing-declarations \
			-Wredundant-decls \
			-Wcast-align \
			-Werror \

# Disable warnings enabled by Android but not enabled in autotools build
BLUEZ_COMMON_CFLAGS += -Wno-pointer-arith \
			-Wno-missing-field-initializers \
			-Wno-maybe-uninitialized \
			-Wno-unused-parameter \

ifeq ($(BOARD_HAVE_BLUETOOTH_BLUEZ),true)

#
# bluetooth.default.so HAL
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/android/hal-ipc.c \
	bluez/android/hal-bluetooth.c \
	bluez/android/hal-socket.c \
	bluez/android/hal-hidhost.c \
	bluez/android/hal-pan.c \
	bluez/android/hal-a2dp.c \
	bluez/android/hal-avrcp.c \
	bluez/android/hal-handsfree.c \
	bluez/android/hal-gatt.c \
	bluez/android/hal-utils.c \
	bluez/android/hal-health.c \

ifeq ($(ANDROID_GE_5_0_0), 1)
LOCAL_SRC_FILES += \
	bluez/android/hal-handsfree-client.c \
	bluez/android/hal-map-client.c \
	bluez/android/hal-a2dp-sink.c \
	bluez/android/hal-avrcp-ctrl.c
endif

LOCAL_C_INCLUDES += \
	$(call include-path-for, system-core) \
	$(call include-path-for, libhardware) \

LOCAL_SHARED_LIBRARIES := \
	libcutils \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE := bluetooth.default
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_REQUIRED_MODULES := bluetoothd bluetoothd-snoop init.bluetooth.rc
LOCAL_REQUIRED_MODULES += brcm_patchram_plus

ifeq ($(ANDROID_GE_5_0_0), 1)
LOCAL_MODULE_RELATIVE_PATH := hw
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
endif

include $(BUILD_SHARED_LIBRARY)

#
# btmon
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/monitor/main.c \
	bluez/monitor/display.c \
	bluez/monitor/hcidump.c \
	bluez/monitor/control.c \
	bluez/monitor/packet.c \
	bluez/monitor/l2cap.c \
	bluez/monitor/avctp.c \
	bluez/monitor/avdtp.c \
	bluez/monitor/a2dp.c \
	bluez/monitor/rfcomm.c \
	bluez/monitor/bnep.c \
	bluez/monitor/uuid.c \
	bluez/monitor/sdp.c \
	bluez/monitor/vendor.c \
	bluez/monitor/lmp.c \
	bluez/monitor/crc.c \
	bluez/monitor/ll.c \
	bluez/monitor/hwdb.c \
	bluez/monitor/keys.c \
	bluez/monitor/ellisys.c \
	bluez/monitor/analyze.c \
	bluez/monitor/intel.c \
	bluez/monitor/broadcom.c \
	bluez/src/shared/util.c \
	bluez/src/shared/queue.c \
	bluez/src/shared/crypto.c \
	bluez/src/shared/btsnoop.c \
	bluez/src/shared/mainloop.c \
	bluez/lib/hci.c \
	bluez/lib/bluetooth.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := btmon

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# btproxy
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/tools/btproxy.c \
	bluez/src/shared/mainloop.c \
	bluez/src/shared/util.c \
	bluez/src/shared/ecc.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := btproxy

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# bluetoothd-snoop
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/android/bluetoothd-snoop.c \
	bluez/src/shared/mainloop.c \
	bluez/src/shared/btsnoop.c \
	bluez/android/log.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := bluetoothd-snoop

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# init.bluetooth.rc
#

include $(CLEAR_VARS)

LOCAL_MODULE := init.bluetooth.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := bluez/android/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_PREBUILT)

endif # BOARD_HAVE_BLUETOOTH_BLUEZ

#
# btmgmt
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/tools/btmgmt.c \
	bluez/lib/bluetooth.c \
	bluez/lib/hci.c \
	bluez/lib/sdp.c \
	bluez/src/shared/mainloop.c \
	bluez/src/shared/io-mainloop.c \
	bluez/src/shared/mgmt.c \
	bluez/src/shared/queue.c \
	bluez/src/shared/util.c \
	bluez/src/shared/gap.c \
	bluez/src/uuid-helper.c \
	bluez/client/display.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \
	$(LOCAL_PATH)/bluez/android/compat \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := btmgmt

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# hcitool
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/tools/hcitool.c \
	bluez/src/oui.c \
	bluez/lib/bluetooth.c \
	bluez/lib/hci.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := hcitool

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# hciconfig
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bluez/tools/hciconfig.c \
	bluez/tools/csr.c \
	bluez/lib/bluetooth.c \
	bluez/lib/hci.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := hciconfig

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)

#
# hciattach
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bluez/tools/hciattach.c \
	bluez/tools/hciattach_st.c \
	bluez/tools/hciattach_ti.c \
	bluez/tools/hciattach_tialt.c \
	bluez/tools/hciattach_ath3k.c \
	bluez/tools/hciattach_qualcomm.c \
	bluez/tools/hciattach_intel.c \
	bluez/tools/hciattach_bcm43xx.c \
	bluez/lib/bluetooth.c \
	bluez/lib/hci.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/bluez \

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := hciattach

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/bluez/configure.ac

include $(BUILD_EXECUTABLE)
