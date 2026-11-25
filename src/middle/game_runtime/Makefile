# Curated AstroToo Kbuild
# AstroToo common include paths
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/app/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/net/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/net_manage/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/divoom_light/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/divoom_light/interface
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/aw_menu/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/driver/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lua-5.4/src
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/webp/webp
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/lib_headers/zstd
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

obj-y += divoom_game_runtime_loader.o
obj-y += divoom_game_native_lua.o
obj-y += divoom_game_pack_install.o
# miniz 统一由 app/ota_miniz 提供（OTA ZIP）；此处不再编入，避免与 eyesee-mpp MOV 自带 miniz 重复/混版本
