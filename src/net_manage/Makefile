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
subdir-ccflags-y += -I include/FreeRTOS_POSIX
subdir-ccflags-y += -I components/common/aw/wireless/wifimanager/core/include
subdir-ccflags-y += -I components/common/thirdparty/wireless/aic8800/src
subdir-ccflags-y += -I components/common/aw/xradio/include
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
EXTRA_CFLAGS += -DPLATFORM_ALLWIN_FREERTOS
EXTRA_CFLAGS += -DCONFIG_RWNX
EXTRA_CFLAGS += -DCONFIG_RWNX_FULLMAC
EXTRA_CFLAGS += -DCONFIG_LWIP
EXTRA_CFLAGS += -DDIVOOM_NO_LEGACY_WIFIMG
subdir-ccflags-y += $(EXTRA_CFLAGS)

obj-y += divoom_wifi_con.o
obj-y += divoom_wifi_legacy_compat.o
obj-y += divoom_wifi_sta_rssi.o
obj-y += divoom_ble_accept_data.o

CFLAGS_divoom_wifi_sta_rssi.o += -DCONFIG_TX_NOCOPY
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/inc
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/drv/fhost
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/drv/wlan
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/drv/macif
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/drv/fw
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/drv/sdio
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/rtos_al/aw_freertos
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/net_al/aw_freertos
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/common/api
CFLAGS_divoom_wifi_sta_rssi.o += -I components/common/thirdparty/wireless/aic8800/host/inc/priv
