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
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl/src
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/platform
subdir-ccflags-y += -DUSE_SUNXIFB=1
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

obj-y += divoom_disp_com.o
obj-y += divoom_disp_ctrl.o
obj-y += divoom_screen_driver.o
obj-y += divoom_mirror_rtos_stub.o
obj-y += divoom_disp_power_on.o
obj-y += divoom_disp_light.o
obj-y += divoom_disp_led.o

# --- added divoom_light (full port) ---
obj-y += divoom_disp_alarm.o
obj-y += divoom_disp_ambient_ctrl.o
obj-y += divoom_disp_common_use.o
obj-y += divoom_disp_count_time_down.o
obj-y += divoom_disp_download_file.o
obj-y += divoom_disp_draw.o
obj-y += divoom_disp_event.o
obj-y += divoom_disp_fill_ctrl.o
obj-y += divoom_disp_game.o
obj-y += divoom_disp_gif.o
obj-y += divoom_disp_high_pri_event_dis_ctrl.o
obj-y += divoom_disp_iffft_text.o
obj-y += divoom_disp_ifft.o
obj-y += divoom_disp_local_gif.o
obj-y += divoom_disp_math.o
obj-y += divoom_disp_no_disp.o
obj-y += divoom_disp_noise.o
obj-y += divoom_disp_phone_ctrl.o
# AstroToo: 不链入 ~2.5MB 内嵌 Divoom 小图（LVGL 主 UI，旧灯阵显示已 stub）
obj-y += divoom_disp_picture_astrotoo_stub.o
obj-y += divoom_disp_play_net_image.o
obj-y += divoom_disp_score.o
obj-y += divoom_disp_sleep.o
obj-y += divoom_disp_subscribe.o
# 产测：AstroToo 不编完整 divoom_disp_test.c（旧灯阵显示已无效）
obj-y += divoom_disp_test_astrotoo_stub.o
obj-y += divoom_disp_time_manage.o
obj-y += divoom_disp_update.o
obj-y += divoom_disp_watch.o
obj-y += divoom_gif_ctrl_com.o
obj-y += divoom_light_breathing_light.o
obj-y += divoom_light_env.o
obj-y += divoom_light_gfx.o
obj-y += divoom_light_math.o
obj-y += divoom_light_music.o
obj-y += divoom_light_pic_scan.o
obj-y += divoom_light_sand_paint.o
obj-y += divoom_light_special.o
obj-y += divoom_light_start_blink.o
obj-y += divoom_light_word.o
obj-y += divoom_load_movie.o
obj-y += divoom_rgb_hsv.o
obj-y += divoom_user_define.o
