ifeq ($(CONFIG_COMPONENTS_LVGL9),y)
obj-y += divoom_lvgl_init.o
obj-y += divoom_storage_test_ui.o
obj-y += divoom_lvgl_mem.o
obj-$(CONFIG_DIVOOM_FINSH_AUTOTEST) += divoom_lvgl_display_test.o

subdir-ccflags-y += -DUSE_SUNXIFB=1
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lvgl/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/app/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/product_test/include
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/platform
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/platform/disp
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl/src
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/webp/webp
MPP_MIDDLEWARE_PATH := components/common/aw/multimedia/eyesee-mpp/middleware
subdir-ccflags-y += -I $(MPP_MIDDLEWARE_PATH)/include \
	-I $(MPP_MIDDLEWARE_PATH)/include/rtos \
	-I $(MPP_MIDDLEWARE_PATH)/include/utils \
	-I $(MPP_MIDDLEWARE_PATH)/include/media \
	-I $(MPP_MIDDLEWARE_PATH)/include/media/utils \
	-I $(MPP_MIDDLEWARE_PATH)/media/include \
	-I $(MPP_MIDDLEWARE_PATH)/media/include/utils \
	-I $(MPP_MIDDLEWARE_PATH)/media/include/component \
	-I $(MPP_MIDDLEWARE_PATH)/../system/public/include \
	-I $(MPP_MIDDLEWARE_PATH)/../system/public/include/vo \
	-I components/common/aw/multimedia/libcedarc/include \
	-I components/common/aw/multimedia/libcedarc/base/include \
	-I components/common/aw/multimedia/libcedarc/vdecoder \
	-I components/common/aw/multimedia/pngdecode
endif
