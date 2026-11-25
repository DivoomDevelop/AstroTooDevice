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
subdir-ccflags-y += -I components/common/thirdparty/network/mbedtls-2.16.0/include
subdir-ccflags-y += -I components/common/aw/wireless/wifimanager/core/include
subdir-ccflags-y += -I components/common/thirdparty/wireless/aic8800/src
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl
subdir-ccflags-y += -I components/common/thirdparty/littlevgl-9/lvgl/src
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/game_runtime
subdir-ccflags-y += -I components/common/aw/update/
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
# RTOS SHTTPD (match components/.../shttpd/Makefile)
subdir-ccflags-y += -I components/common/thirdparty/network/shttpd/include
subdir-ccflags-y += -I components/common/thirdparty/network/lwip-2.1.2/src/include
subdir-ccflags-y += -I components/common/thirdparty/network/lwip-2.1.2/src/include/lwip
subdir-ccflags-y += -I components/common/aw/xradio/include
subdir-ccflags-y += -I drivers/drv/wireless/xradio/os
subdir-ccflags-y += -I drivers/drv/wireless/xradio/os/include

obj-y += divoom_main.o
obj-y += divoom_boot_ab.o
obj-y += divoom_uboot_check.o
obj-y += divoom_device_error.o
ifeq ($(DIVOOM_BOOTFAULT_TEST),1)
obj-y += bootfault_test.o
endif
ifeq ($(DIVOOM_BLACKBOX_TEST),1)
# Explicit diagnostic build only; release builds expose no bbtest/key injection.
obj-y += divoom_blackbox_cli.o
subdir-ccflags-y += -DDIVOOM_BLACKBOX_TEST=1 -DDIVOOM_BTCORE_UART_CLI
endif
obj-y += divoom_onoff.o
obj-y += divoom_idle_poweroff.o
obj-y += divoom_show_mode_ctrl.o
obj-y += divoom_power_on_manage.o
obj-y += divoom_net_app.o
obj-y += divoom_light_app.o
obj-y += divoom_app_config_wifi.o
obj-y += divoom_user_key.o
obj-y += divoom_sleep.o
obj-y += divoom_alarm.o
obj-y += divoom_sys_model_ctrl.o
obj-y += divoom_app_schedule.o
obj-y += divoom_sleep_aid_ctrl.o
obj-y += divoom_power_control.o
obj-y += divoom_tomato_time.o
obj-y += divoom_server_msg_manage.o
obj-y += divoom_http_server.o
obj-y += divoom_http_server_shttpd.o
obj-y += divoom_lan_api.o
obj-y += divoom_lan_upload.o

# --- added app (full port) ---
obj-y += divoom_app_count_time_down.o
obj-y += divoom_app_file_manage.o
obj-y += divoom_app_onoff.o
obj-y += divoom_app_tool.o
obj-y += divoom_bt_music.o
obj-y += divoom_bt_bip.o
obj-y += divoom_btcore_uart_cmd.o
obj-y += divoom_tws_host.o
obj-y += divoom_btcore_key.o
obj-y += divoom_btcore_key_light.o
obj-y += divoom_btcore_uart_cli.o
obj-y += divoom_btcore_rx_stat_cli.o
obj-y += divoom_buzzer_music.o
obj-y += divoom_disp_ctrl_app.o
obj-y += divoom_disp_file_manage.o
obj-y += divoom_eq_noise_mic_fft.o
obj-y += divoom_leave_msg.o
obj-y += divoom_leave_msg_ctrl.o
obj-y += divoom_mic_ctrl.o
obj-y += divoom_music_ctrl.o
obj-y += divoom_net_radio.o
obj-y += divoom_radio.o
obj-y += divoom_ota_jl.o
obj-y += divoom_ota_server.o
obj-y += divoom_ota_rtos.o
obj-y += divoom_ota_zip.o
obj-y += ota_miniz/
obj-y += divoom_save_to_sd.o
obj-y += divoom_simu_flash.o
obj-y += divoom_spiffs_manage.o
obj-y += divoom_time_plan.o
obj-y += divoom_voice_app.o
obj-y += divoom_white_noise.o
obj-y += divoom_white_noise_player.o
obj-y += divoom_lyric_demand.o

# Home Assistant: client core, token HTTP and WebSocket transport.
obj-y += divoom_ha.o
obj-y += divoom_ha_wire.o
obj-y += divoom_ha_http.o

