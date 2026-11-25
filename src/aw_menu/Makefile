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
subdir-ccflags-y += -Wno-error=array-bounds
subdir-ccflags-y += -Wno-error=return-mismatch
subdir-ccflags-y += -Wno-error=comment
subdir-ccflags-y += -Wno-error=unused-label
subdir-ccflags-y += -Wno-error=dangling-pointer
EXTRA_CFLAGS += -DPLATFORM_ALLWIN_FREERTOS
EXTRA_CFLAGS += -DCONFIG_RWNX
EXTRA_CFLAGS += -DCONFIG_RWNX_FULLMAC
EXTRA_CFLAGS += -DCONFIG_LWIP
EXTRA_CFLAGS += -DDIVOOM_NO_LEGACY_WIFIMG
subdir-ccflags-y += $(EXTRA_CFLAGS)

obj-y += divoom_astrootoo_alarm.o
obj-y += divoom_astrootoo_config_wifi.o
obj-y += divoom_astrootoo_face_market.o
obj-y += divoom_astrootoo_games.o
obj-y += divoom_astrootoo_language_route.o
obj-y += divoom_astrootoo_main_menu.o
obj-y += divoom_astrootoo_menu_registry.o
obj-y += divoom_astrootoo_placeholder.o
obj-y += divoom_astrootoo_settings.o
obj-y += divoom_astrootoo_settings_ex.o
obj-y += divoom_astrootoo_settings_wifi.o
obj-y += divoom_astrootoo_settings_tws.o
obj-y += divoom_astrootoo_network_reset_confirm.o
obj-y += divoom_astrootoo_sleep.o
obj-y += divoom_astrootoo_source.o
obj-y += divoom_radio_menu.o
obj-y += divoom_lvgl_top_radio_wait.o
obj-y += divoom_astrootoo_tools.o
obj-y += divoom_astrootoo_ui_base.o
obj-y += divoom_astrootoo_watch.o
obj-y += divoom_boot_logo_data.o
obj-y += divoom_dial_preview_default.o
obj-y += divoom_face_preview_loader.o
obj-y += divoom_load_message.o
obj-y += divoom_lvgl_message_box.o
obj-y += divoom_lvgl_movie_play.o
obj-y += divoom_lvgl_normal_menu_list.o
obj-y += divoom_lvgl_poweron.o
obj-y += divoom_lvgl_set_language.o
obj-y += divoom_menu_control.o
obj-y += divoom_menu_direct_ctrl.o
obj-y += divoom_popup_menu.o

# --- added aw_menu (full port) ---
obj-y += divoom_alarm_active_menu.o
obj-y += divoom_alarm_tf_sound_pick.o
obj-y += divoom_astrootoo_oobe_config_wifi.o
obj-y += divoom_astrootoo_oobe_welcome.o
obj-y += divoom_astrootoo_settings_slideshow.o
obj-y += divoom_astrootoo_time_pick.o
obj-y += divoom_bt_set_secret_menu.o
obj-y += divoom_buddy.o
obj-y += divoom_buddy_config.o
obj-y += divoom_gl_music_name.o
obj-y += divoom_lvgl_accept_app_photo_view.o
obj-y += divoom_lvgl_alarm_add_menu.o
obj-y += divoom_lvgl_alarm_clock_wake_hold.o
obj-y += divoom_lvgl_alarm_edit_menu.o
obj-y += divoom_lvgl_alarm_menu.o
obj-y += divoom_lvgl_audio_source_menu.o
obj-y += divoom_lvgl_aux_music_menu.o
obj-y += divoom_lvgl_bt_music_menu.o
obj-y += divoom_lvgl_calendar.o
obj-y += divoom_lvgl_calendar_active.o
obj-y += divoom_lvgl_calendar_list_menu.o
obj-y += divoom_lvgl_config_wifi.o
obj-y += divoom_lvgl_count_down_menu.o
obj-y += divoom_lvgl_count_down_start_menu.o
obj-y += divoom_lvgl_device_update.o
obj-y += divoom_lvgl_file_transfer.o
obj-y += divoom_lvgl_gallery_transfer.o
# Musixmatch implementation is linked from libastrotoo_bundle.

obj-y += divoom_lvgl_main_menu.o
obj-y += divoom_lvgl_mcp_tool.o
obj-y += divoom_lvgl_mix_music2.o
obj-y += divoom_lvgl_mixer.o
obj-y += divoom_lvgl_more_function.o
obj-y += divoom_lvgl_music_effect.o
obj-y += divoom_lvgl_noise_meter_menu.o
obj-y += divoom_lvgl_photo_add_picture_to_album.o
obj-y += divoom_lvgl_photo_manage.o
obj-y += divoom_lvgl_photo_push.o
obj-y += divoom_lvgl_photo_setting_menu.o
obj-y += divoom_lvgl_photo_slide_effect.o
obj-y += divoom_lvgl_photo_sticker.o
obj-y += divoom_lvgl_photo_stream.o
obj-y += divoom_lvgl_photo_view.o
obj-y += divoom_lvgl_play_gif_file.o
obj-y += divoom_lvgl_power_off_disp_menu.o
obj-y += divoom_lvgl_power_shutdown_menu.o
obj-y += divoom_lvgl_reseting_device.o
obj-y += divoom_lvgl_score_board_menu.o
obj-y += divoom_lvgl_select_album_play.o
obj-y += divoom_lvgl_setting_about.o
obj-y += divoom_lvgl_setting_bluetooth.o
obj-y += divoom_lvgl_setting_color_temp.o
obj-y += divoom_lvgl_setting_date_time.o
obj-y += divoom_lvgl_setting_idle_poweroff.o
obj-y += divoom_lvgl_setting_night_view.o
obj-y += divoom_lvgl_setting_power_off.o
obj-y += divoom_lvgl_setting_restore.o
obj-y += divoom_lvgl_setting_screen_save.o
obj-y += divoom_lvgl_setting_temperature.o
obj-y += divoom_lvgl_setting_theme.o
obj-y += divoom_lvgl_setting_wifi.o
obj-y += divoom_lvgl_setting_wifi_properties.o
obj-y += divoom_lvgl_sleep_aid.o
obj-y += divoom_lvgl_sleep_aid_active.o
obj-y += divoom_lvgl_slide_setting.o
obj-y += divoom_lvgl_stop_watch_menu.o
obj-y += divoom_lvgl_stream_cont.o
obj-y += divoom_lvgl_system_menu.o
obj-y += divoom_lvgl_system_setting.o
obj-y += divoom_lvgl_test_3dgif.o
obj-y += divoom_lvgl_test_image.o
obj-y += divoom_lvgl_tf_music_folder_menu.o
obj-y += divoom_lvgl_tf_music_menu.o
obj-y += divoom_lvgl_tool_menu.o
obj-y += divoom_lvgl_top_ambient_menu.o
obj-y += divoom_lvgl_top_audio_menu.o
obj-y += divoom_lvgl_top_audio_tip_menu.o
obj-y += divoom_lvgl_top_battery_menu.o
obj-y += divoom_lvgl_top_phone_call_menu.o
obj-y += divoom_lvgl_top_toast.o
obj-y += divoom_lvgl_top_bright_volume_menu.o
obj-y += divoom_lvgl_top_bright_volume_video.o
obj-y += divoom_lvgl_top_danmaku_menu.o
obj-y += divoom_lvgl_top_leave_msg_notice_menu.o
obj-y += divoom_lvgl_top_main_menu.o
obj-y += divoom_lvgl_top_music_ctrl_hint.o
obj-y += divoom_lvgl_top_reached_home_menu.o
obj-y += divoom_lvgl_top_source_switch_menu.o
obj-y += divoom_lvgl_uac_music_menu.o
obj-y += divoom_lvgl_hot_update_prompt.o
# legacy: voice_record_tool 全页未挂 menu 105（AstroToo 用 astrootoo_tools MEMO）
# obj-y += divoom_lvgl_voice_record_tool_menu.o
obj-y += divoom_lvgl_white_noise_list_menu.o
obj-y += divoom_lvgl_white_noise_play_main.o
obj-y += divoom_lvgl_wifi_disconnect.o
obj-y += divoom_photo_play_ctrl.o
obj-y += divoom_lvgl_album_video.o
CFLAGS_divoom_lvgl_album_video.o += \
	-Werror=frame-larger-than=1024 \
	-I components/common/aw/multimedia/eyesee-mpp/middleware/include \
	-I components/common/aw/multimedia/eyesee-mpp/middleware/include/media \
	-I components/common/aw/multimedia/eyesee-mpp/middleware/include/utils \
	-I components/common/aw/multimedia/eyesee-mpp/middleware/media/include \
	-I components/common/aw/multimedia/eyesee-mpp/middleware/media/include/component \
	-I components/common/aw/multimedia/libcedarc/include \
	-I components/common/aw/multimedia/libcedarc/base/include \
	-I components/common/aw/multimedia/libcedarc/vdecoder
obj-y += divoom_tomato_time_edit_bell.o
obj-y += divoom_tomato_time_edit_menu.o
obj-y += divoom_tomato_time_edit_repeat_menu.o
obj-y += divoom_tomato_time_edit_time_menu.o
obj-y += divoom_tomato_time_menu.o
obj-y += divoom_white_noise_menu.o
obj-y += divoom_white_noise_time_menu.o
