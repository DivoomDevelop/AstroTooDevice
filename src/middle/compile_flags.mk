# Curated AstroToo Kbuild
# AstroToo common include paths
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/app/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/net/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/net_manage/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/divoom_light/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/divoom_light/interface
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/aw_menu/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/product_test/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/driver/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lua-5.4/src
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/webp/webp
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/zstd
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/sqlite
subdir-ccflags-y += -I include/FreeRTOS_POSIX
subdir-ccflags-y += -I components/common/aw/wireless/wifimanager/core/include
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl/src
subdir-ccflags-y += -I components/common/thirdparty/network/mbedtls-2.16.0/include
subdir-ccflags-y += -include projects/f101s3/astrotoo/src/middle/include/divoom_lvgl9_compat.h
subdir-ccflags-y += -Dl_signalT=int
subdir-ccflags-y += -Wno-error=pointer-sign
subdir-ccflags-y += -Wno-error=unused-variable
subdir-ccflags-y += -Wno-error=unused-but-set-variable
subdir-ccflags-y += -Wno-error=implicit-function-declaration
subdir-ccflags-y += -Wno-error=incompatible-pointer-types
subdir-ccflags-y += -Wno-error=int-conversion
subdir-ccflags-y += -Wno-error=return-type
subdir-ccflags-y += -Wno-error=unused-function
subdir-ccflags-y += -Wno-error=discarded-qualifiers
subdir-ccflags-y += -Wno-error=format-overflow
subdir-ccflags-y += -Wno-error=uninitialized
subdir-ccflags-y += -Wno-error=parentheses
subdir-ccflags-y += -Wno-error=misleading-indentation
subdir-ccflags-y += -Wno-error=unused-const-variable=
subdir-ccflags-y += -Wno-error=maybe-uninitialized
subdir-ccflags-y += -Wno-error=format
subdir-ccflags-y += -Wno-error=format-truncation
subdir-ccflags-y += -Wno-error=address
subdir-ccflags-y += -Wno-error=array-bounds
subdir-ccflags-y += -Wno-error=return-mismatch
subdir-ccflags-y += -Wno-error=comment
subdir-ccflags-y += -Wno-error=unused-label
subdir-ccflags-y += -Wno-error=dangling-pointer

MPP_MIDDLEWARE_PATH := components/common/aw/multimedia/eyesee-mpp/middleware
MPP_DEMUXER_BASE := $(MPP_MIDDLEWARE_PATH)/media/LIBRARY/libdemuxer
subdir-ccflags-y += -I $(MPP_MIDDLEWARE_PATH)/include \
	-I $(MPP_MIDDLEWARE_PATH)/include/rtos \
	-I $(MPP_MIDDLEWARE_PATH)/include/utils \
	-I $(MPP_MIDDLEWARE_PATH)/include/media \
	-I $(MPP_MIDDLEWARE_PATH)/include/media/utils \
	-I $(MPP_MIDDLEWARE_PATH)/media/include \
	-I $(MPP_MIDDLEWARE_PATH)/media/include/utils \
	-I $(MPP_MIDDLEWARE_PATH)/media/include/component \
	-I $(MPP_MIDDLEWARE_PATH)/media/include/include_render \
	-I $(MPP_MIDDLEWARE_PATH)/../system/public/include \
	-I $(MPP_MIDDLEWARE_PATH)/../system/public/include/vo \
	-I $(MPP_MIDDLEWARE_PATH)/../system/public/include/utils \
	-I components/common/aw/multimedia/libcedarc/include \
	-I components/common/aw/multimedia/libcedarc/base/include \
	-I components/common/aw/multimedia/libcedarc/vdecoder \
	-I $(MPP_DEMUXER_BASE)/libcore/base/include \
	-I $(MPP_DEMUXER_BASE)/libcore/stream/include \
	-I $(MPP_DEMUXER_BASE)/libcore/parser/include \
	-I $(MPP_DEMUXER_BASE)/libcore/parser/mov
subdir-ccflags-y += -I components/common/aw/multimedia/jpegdecode \
	-I components/common/aw/multimedia/pngdecode
subdir-ccflags-y += -I components/common/thirdparty/speexdsp
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/tone_lite
ifdef CONFIG_DIVOOM_TONE_AAC_FAAD2
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/thirdparty/faad2/include
else
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/thirdparty/helix_aac
endif
subdir-ccflags-y += -I drivers/rtos-hal/include/hal
subdir-ccflags-y += -I drivers/rtos-hal/include/hal/aw-tiny-sound-lib
subdir-ccflags-y += -I drivers/rtos-hal/include/hal/sound_v2

